#define TRIAL_HELYA_PHASE3_SUPPORT_ONLY
#include "TrialHelyaPhase3Native.cpp"
struct CharmedAIUpdateTag{using Type=void(Player::*)();friend Type Access(CharmedAIUpdateTag);};
template struct FixtureAccess<CharmedAIUpdateTag,&Player::UpdateCharmedAI>;
struct ControlWatch
{
    InstanceMap& map;GarmPlayer& player;Creature& boss;uint32 elapsed=0;
    std::map<uint32,std::set<ObjectGuid>> casts,targets,damaged;
    ControlWatch(InstanceMap& m,GarmPlayer& p,Creature& h):map(m),player(p),boss(h){}
    void Observe(){for(Unit* caster:{static_cast<Unit*>(&player),static_cast<Unit*>(&boss)})for(auto id:{228854u,231863u,232350u})for(auto s:NativeSpellEvents(*caster,id)){
        casts[id].insert(s->m_castGuid[0]);for(auto const& t:*s->GetUniqueTargetInfo())if(t->effectMask&1){targets[id].insert(t->targetGUID);if(t->processed&&t->damage>0)damaged[id].insert(t->targetGUID);}
    }}
    // Freeze boss scheduling; tick the actual Player aura/spell dispatchers.
    // Do not buffer this Player's health: threshold tests depend on it.
    void Advance(uint32 ms,bool charmedAI=false){Observe();for(unsigned t=0;t<ms;t+=10){player.Unit::Update(10);if(charmedAI&&player.isCharmed())if(auto c=player.GetCharmer())if(c->IsCreature()&&c->IsAlive())(player.*Access(CharmedAIUpdateTag{}))();Observe();elapsed+=10;}}
};
static void ControlBegin(GarmPlayer& p,GarmPlayer& subject,InstanceMap& map,Creature& helya)
{
    subject.RemoveAura(232350);subject.RemoveAura(232450);Restart(p,map,helya);EnterPhaseThree(p,map,helya);
    map.PlayerRelocation(&subject,helya.GetPositionX()+8,helya.GetPositionY(),helya.GetPositionZ(),0);subject.SetHealth(subject.GetMaxHealth());
    helya.CastSpell(&subject,232450,true);Require(subject.HasAura(232450),"Native Axion absorb missing");subject.GetAura(232450)->SetDuration(1);
    Tick(subject,30);Require(subject.HasAura(232350)==map.IsMythicRaid(),"Actual absorb expiry crossed the Mythic gate");
}
#ifndef TRIAL_HELYA_CONTROL_BASELINE
struct ControlTravelSession
{
    GarmPlayer& player;
    ControlTravelSession(GarmPlayer& p,InstanceMap& raid):player(p){
        // Private fixture-owned workers only: actual transfer/ACK is driven
        // synchronously, without an authenticated socket or map-session loop.
        auto base=sMapMgr->CreateBaseMap(1220);base->SetMapStop();auto& thread=sMapMgr->_mapThreads[1220];if(thread&&thread->joinable())thread->join();
        auto outdoor=static_cast<MapInstanced*>(base);auto target=sMapMgr->CreateMap(1220,&p);target->SetMapStop();for(auto& zone:outdoor->_zoneThreads)if(zone.second->joinable())zone.second->join();
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());Require(!sessions.count(p.GetSession()->GetAccountId()),"Travel fixture would replace another session");
        sessions[p.GetSession()->GetAccountId()]=WorldSessionPtr(p.GetSession(),[](WorldSession*){});p.GetSession()->SetMap(&raid);
    }
    ~ControlTravelSession(){const_cast<SessionMap&>(sWorld->GetAllSessions()).erase(player.GetSession()->GetAccountId());player.GetSession()->SetMap(nullptr);}
};
static void ControlRoundTrip(GarmPlayer& p,GarmPlayer& subject,InstanceMap& map,Creature& helya,Group& group)
{
    sWorld->AddCharacterInfo(subject.GetGUID(),0,subject.GetName(),GENDER_MALE,subject.getRace(),subject.getClass(),subject.getLevel());
    subject.SaveToDB(true);FlushWrites();group.ConvertToRaid();Require(group.AddMember(&subject),"Native control-travel group admission failed");subject.SetRaidDifficultyID(map.GetDifficultyID());
    ControlTravelSession session(subject,map);auto faction=subject.getFaction();auto display=subject.GetDisplayId();auto destination=sWorldSafeLocsStore.LookupEntry(5779);Require(destination&&destination->MapID==1220,"Native outdoor destination missing");
    for(bool controlled:{true,false}){
        ControlBegin(p,subject,map,helya);subject.RemoveAura(228054);
        if(!controlled){subject.RemoveAura(232350);helya.CastSpell(&subject,232450,true);Require(subject.HasAura(232450),"Travel pending-Axion input missing");}
        helya.SetPower(POWER_ALTERNATE,10);ControlWatch travel(map,subject,helya);travel.Advance(1500);
        Require(subject.HasUnitState(UNIT_STATE_CASTING)==controlled,"Travel missed native controlled cast/pending absorb input");
        auto instance=map.GetInstanceId();auto saved=map.GetInstanceScript()->GetSaveData();auto mask=map.GetInstanceScript()->GetCompletedEncounterMask();auto bind=group.GetBoundInstance(map.GetDifficultyID(),1648);Require(bind&&bind->save->GetInstanceId()==instance,"Travel lost earned raid bind");
        Require(subject.SafeTeleport(1220,destination->Loc.X,destination->Loc.Y,destination->Loc.Z,destination->Loc.O*float(M_PI)/180.0f)&&subject.IsBeingTeleportedFar()&&!subject.IsInWorld(),"Native controlled far transfer did not leave raid");
        std::cout<<"CONTROL_DEPART controlled="<<controlled<<" control="<<subject.HasAura(232350)<<" transform="<<subject.HasAura(232358)<<" bleak="<<subject.HasAura(231862)<<" axion="<<subject.HasAura(232450)<<" casting="<<subject.HasUnitState(UNIT_STATE_CASTING)<<std::endl;
        Require(!subject.HasAura(232350)&&!subject.HasAura(232358)&&!subject.HasAura(231862)&&subject.GetCharmerGUID().IsEmpty()&&!subject.HasUnitState(UNIT_STATE_CASTING)&&subject.getFaction()==faction&&subject.GetDisplayId()==display&&subject.IsControlledByPlayer(),"Native map departure retained encounter control state");
        subject.GetSession()->HandleWorldPortAck();Require(subject.IsInWorld()&&subject.GetMapId()==1220&&!subject.IsBeingTeleportedFar(),"Outdoor worldport ACK failed");
        // Axion has no native change-map interruption flag. Do not invent
        // immediate removal: its real expiry outside Helya's map must be safe.
        if(!controlled){auto info=sSpellMgr->GetSpellInfo(232450);std::cout<<"CONTROL_OUTDOOR_AXION flags="<<info->AuraInterruptFlags[0]<<','<<info->AuraInterruptFlags[1]<<" duration="<<(subject.GetAura(232450)?subject.GetAura(232450)->GetDuration():0)<<" caster="<<(subject.GetAura(232450)&&subject.GetAura(232450)->GetCaster()!=nullptr)<<std::endl;}
        travel.Advance(15000,true);Require(travel.casts[231863].empty()&&helya.GetPower(POWER_ALTERNATE)==10&&!subject.HasAura(232350)&&!subject.HasAura(232450),"Outdoor Player completed/recreated encounter effects or retained expired absorb");
        Require(map.GetInstanceScript()->GetSaveData()==saved&&map.GetInstanceScript()->GetCompletedEncounterMask()==mask,"Departure changed active raid progress");
        Position entrance=helya.GetPosition();Require(!subject.SafeTeleport(1648,&entrance)&&subject.GetMapId()==1220&&!subject.IsBeingTeleportedFar(),"Combat re-entry bypassed native raid admission");
        ClearCombat(p,helya);Require(map.GetInstanceScript()->GetBossState(0)==DONE&&map.GetInstanceScript()->GetBossState(1)==DONE&&map.GetInstanceScript()->GetBossState(2)==NOT_STARTED&&map.GetInstanceScript()->GetCompletedEncounterMask()==mask,"Native wipe lost earned raid progress");
        Require(sMapMgr->CreateMap(1648,&subject)==&map,"Native map selection lost original instance/difficulty");
        Require(subject.SafeTeleport(1648,&entrance)&&subject.IsBeingTeleportedFar(),"Native return transfer failed");subject.GetSession()->HandleWorldPortAck();
        Require(subject.IsInWorld()&&subject.GetMap()==&map&&!subject.IsBeingTeleportedFar()&&subject.GetMap()->GetDifficultyID()==DIFFICULTY_MYTHIC_RAID,"Native return ACK selected wrong map/difficulty");
        travel.Advance(8000,true);Require(!subject.HasAura(232350)&&!subject.HasAura(232358)&&!subject.HasAura(231862)&&!subject.HasAura(232450)&&subject.GetCharmerGUID().IsEmpty()&&!subject.HasUnitState(UNIT_STATE_CASTING)&&travel.casts[231863].empty(),"Raid re-entry restored old control or a residual cast");
        bind=group.GetBoundInstance(map.GetDifficultyID(),1648);Require(bind&&bind->save->GetInstanceId()==instance&&map.GetInstanceScript()->GetCompletedEncounterMask()==mask,"Round trip changed raid bind/progress");
        std::cout<<"CONTROL_RETURN controlled="<<controlled<<" instance="<<instance<<" difficulty="<<uint32(map.GetDifficultyID())<<" mask="<<mask<<" bursts="<<travel.casts[231863].size()<<std::endl;
    }
}
#include "TrialHelyaControlLoginNative.inc"
#endif
static void ControlCase(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    LoadPhaseThreeConditions();sSpellMgr->LoadSpellPrcoCheck();GarmPlayer subject(map,p.getRace());auto faction=subject.getFaction();auto display=subject.GetDisplayId();
    p.GetOrInitPetStable();LootPet pet(p,map);pet.setFaction(p.getFaction());pet.SetMaxHealth(p.GetMaxHealth());pet.SetHealth(p.GetMaxHealth());
    ControlBegin(p,subject,map,helya);
    if(!map.IsMythicRaid()){Require(subject.GetCharmerGUID().IsEmpty()&&!subject.HasAura(232358),"Non-Mythic absorb charmed/transformed Player");ClearCombat(p,helya);std::cout<<"PASS: native Corrupted difficulty gate diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;return;}
    Require(subject.GetCharmerGUID()==helya.GetGUID()&&subject.HasAura(232358)&&subject.HasAura(231862)&&subject.getFaction()==helya.getFaction(),"Native Corrupted charm/transform/eruption/faction missing");
    std::cout<<"CONTROL_DATA threshold="<<subject.GetAuraEffect(232350,EFFECT_0)->GetAmount()<<" duration="<<subject.GetAura(232350)->GetMaxDuration()<<std::endl;
    Require(!subject.IsControlledByPlayer(),"Native NPC charm did not exercise the old loot-admission branch");
#ifndef TRIAL_HELYA_CONTROL_BASELINE
    // Interrupt the real aura-started cast, including EffectInterruptCast's
    // school prohibition and interrupt-only proc. This spell is not learned.
    subject.RemoveAura(228054);helya.SetPower(POWER_ALTERNATE,10);
    ControlWatch interrupted(map,subject,helya);interrupted.Advance(1500);
    Require(!subject.HasSpell(228854)&&subject.HasUnitState(UNIT_STATE_CASTING),"Interrupt fixture requires an unlearned, actual Infusion cast");
    subject.learnSpell(116,false,0,false);subject.learnSpell(133,false,0,false);
    auto interruptHealth=p.GetHealth();p.CastSpell(&subject,2139,true);auto interruptDamage=interruptHealth-p.GetHealth();interrupted.Advance(1000);
    std::cout<<"CONTROL_INTERRUPT cooldown="<<subject.GetSpellCooldownDelay(228854)<<" casting="<<subject.HasUnitState(UNIT_STATE_CASTING)<<" bursts="<<interrupted.casts[231863].size()<<" immediate="<<interruptDamage<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<std::endl;
    Require(interrupted.casts[231863].size()==1&&(interruptDamage||interrupted.damaged[231863].count(p.GetGUID()))&&helya.GetPower(POWER_ALTERNATE)==10,"Controlled Player interruption lost its single damaging eruption or energized Helya");
    Require(!subject.HasUnitState(UNIT_STATE_CASTING)&&subject.HasSpellCooldown(228854),"Unlearned controlled-Player Infusion bypassed Counterspell lockout");
    Require(subject.HasSpellCooldown(116)&&!subject.HasSpellCooldown(133),"Learned same-school spell or unrelated-school cooldown changed");
    Require(subject.CastSpell(&helya,228854,false)==SPELL_FAILED_NOT_READY&&subject.CastSpell(&helya,228854,TRIGGERED_IGNORE_CAST_IN_PROGRESS)==SPELL_FAILED_NOT_READY,"Ordinary/partial-trigger Infusion bypassed lockout");
    p.CastSpell(&subject,2139,true);interrupted.Advance(1000);Require(interrupted.casts[231863].size()==1&&!subject.HasUnitState(UNIT_STATE_CASTING),"Idle Counterspell erupted again or aura retry bypassed lockout");
    auto lockoutMs=sSpellMgr->GetSpellInfo(2139)->GetDuration(map.GetDifficultyID());
    Require(lockoutMs>0&&lockoutMs<=10000,"Review native Counterspell lockout duration");
    // Cooldowns use wall-clock time, unlike the deterministic Unit update
    // clock. Let the real lockout expire; never alter its stored endpoint.
    Sleep(lockoutMs+100);Require(!subject.HasSpellCooldown(228854)&&!subject.HasSpellCooldown(116),"Real-time school lockout failed to expire");
    interrupted.Advance(8500);Require(interrupted.casts[231863].size()==2&&helya.GetPower(POWER_ALTERNATE)==40,"Aura did not retry successfully after real Counterspell expiry");
    Require(!subject.HasSpell(228854),"Interrupt repair permanently taught scripted Infusion");
    std::cout<<"CONTROL_INTERRUPT_RECOVERY duration="<<lockoutMs<<" bursts="<<interrupted.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<std::endl;
    ControlBegin(p,subject,map,helya);
#endif
#ifdef TRIAL_HELYA_CONTROL_BASELINE
    // Original run without this isolation crashes in actual Taint damage at
    // Creature::hasLootRecipient. Keep that trace; isolate only to inspect the
    // independent aura bugs using the same pinned original libraries.
    subject.RemoveAura(228054);
#else
    for(Unit* attacker:{static_cast<Unit*>(&helya),static_cast<Unit*>(&p),static_cast<Unit*>(&pet)}){
        auto before=subject.GetHealth();attacker->DealDamage(&subject,100,nullptr,SPELL_DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        Require(subject.IsAlive()&&subject.GetHealth()<before&&subject.HasAura(232350),"Actual NPC/Player damage to charmed Player failed");
    }
#endif
    helya.SetPower(POWER_ALTERNATE,10);ControlWatch casting(map,subject,helya);casting.Advance(8500);
    std::cout<<"CONTROL_CAST infusion="<<casting.casts[228854].size()<<" burst="<<casting.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<" casting="<<subject.HasUnitState(UNIT_STATE_CASTING)<<std::endl;
    Require(casting.casts[231863].size()==1&&helya.GetPower(POWER_ALTERNATE)==40,"Controlled Player did not complete one native Infusion");
    for(unsigned pct:{25u,24u,10u}){
        ControlBegin(p,subject,map,helya);subject.RemoveAura(228054);subject.SetHealth(subject.CountPctFromMaxHealth(pct));helya.SetPower(POWER_ALTERNATE,10);ControlWatch threshold(map,subject,helya);threshold.Advance(1500);
        std::cout<<"CONTROL_THRESHOLD pct="<<pct<<" aura="<<subject.HasAura(232350)<<" transform="<<subject.HasAura(232358)<<" eruption="<<subject.HasAura(231862)<<" charmer="<<subject.GetCharmerGUID().GetCounter()<<" casting="<<subject.HasUnitState(UNIT_STATE_CASTING)<<std::endl;
#ifdef TRIAL_HELYA_CONTROL_BASELINE
        Require(subject.HasAura(232350)==(pct>=11)&&subject.HasUnitState(UNIT_STATE_CASTING),"Pinned11-percent release/post-release cast changed");
#else
        bool controlled=pct>=25;
        Require(subject.HasAura(232350)==controlled&&subject.HasAura(232358)==controlled&&subject.HasAura(231862)==controlled&&subject.HasUnitState(UNIT_STATE_CASTING)==controlled,"Native25-percent boundary or release cleanup failed");
        Require(subject.GetCharmerGUID()==(controlled?helya.GetGUID():ObjectGuid::Empty)&&subject.IsControlledByPlayer()!=controlled,"Native charmer/control flag did not restore at release");
#endif
        threshold.Advance(8000);std::cout<<"CONTROL_AFTER pct="<<pct<<" infusions="<<threshold.casts[228854].size()<<" bursts="<<threshold.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<std::endl;
#ifndef TRIAL_HELYA_CONTROL_BASELINE
        Require(threshold.casts[231863].size()==(pct>=25?1:0)&&helya.GetPower(POWER_ALTERNATE)==(pct>=25?40:10),"Released Player completed a residual Infusion/eruption");
#endif
        subject.RemoveAura(232350);Require(subject.getFaction()==faction&&subject.GetCharmerGUID().IsEmpty(),"Native removal did not restore Player faction/charmer");
    }
    // Cross the threshold while the native seven-second cast is in progress,
    // using real Player and owned-Pet damage rather than invoking the aura hook.
#ifndef TRIAL_HELYA_CONTROL_BASELINE
    for(unsigned repetition=0;repetition<16;++repetition)for(Unit* attacker:{static_cast<Unit*>(&p),static_cast<Unit*>(&pet)}){
        ControlBegin(p,subject,map,helya);subject.RemoveAura(228054);helya.SetPower(POWER_ALTERNATE,10);ControlWatch released(map,subject,helya);std::cout<<"CONTROL_DAMAGE_RELEASE pet="<<(attacker==&pet)<<" start"<<std::endl;released.Advance(1500);
        Require(subject.HasUnitState(UNIT_STATE_CASTING),"Release test missed actual Infusion cast");
        uint32 damage=uint32((subject.GetHealth()-subject.CountPctFromMaxHealth(24))*subject.GetHealthMultiplierForTarget(attacker));attacker->DealDamage(&subject,damage,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        Require(subject.GetHealthPct()<25&&subject.HasAura(232350),"Native damage did not cross25 before the periodic release check");std::cout<<"CONTROL_DAMAGE_RELEASE pet="<<(attacker==&pet)<<" damaged"<<std::endl;released.Advance(1000);
        Require(!subject.HasAura(232350)&&!subject.HasAura(232358)&&!subject.HasAura(231862)&&subject.GetCharmerGUID().IsEmpty()&&!subject.HasUnitState(UNIT_STATE_CASTING)&&subject.getFaction()==faction&&subject.IsControlledByPlayer(),"Damage release retained charm/transform/cast/faction");
        released.Advance(8000);Require(released.casts[231863].empty()&&helya.GetPower(POWER_ALTERNATE)==10,"Damaged release completed a stale cast");
    }
    for(bool expiry:{false,true}){
        ControlBegin(p,subject,map,helya);subject.RemoveAura(228054);helya.SetPower(POWER_ALTERNATE,10);ControlWatch removed(map,subject,helya);removed.Advance(1500);Require(subject.HasUnitState(UNIT_STATE_CASTING),"Removal test missed actual cast");
        if(expiry){subject.GetAura(232350)->SetDuration(1);removed.Advance(30);}else subject.RemoveAura(232350);
        removed.Advance(8000);Require(subject.GetCharmerGUID().IsEmpty()&&subject.getFaction()==faction&&!subject.HasAura(232358)&&!subject.HasAura(231862)&&!subject.HasUnitState(UNIT_STATE_CASTING)&&removed.casts[231863].empty()&&helya.GetPower(POWER_ALTERNATE)==10,"Explicit removal/expiry retained control or completed an old cast");
    }
    ControlBegin(p,subject,map,helya);ControlWatch wiped(map,subject,helya);wiped.Advance(1500);ClearCombat(p,helya);wiped.Advance(8000);
    Require(!subject.HasAura(232350)&&!subject.HasAura(232358)&&!subject.HasAura(231862)&&subject.GetCharmerGUID().IsEmpty()&&!subject.HasUnitState(UNIT_STATE_CASTING)&&subject.getFaction()==faction,"Wipe retained controlled Player state");
    ControlRoundTrip(p,subject,map,helya,group);
    ControlLoginCases(p,map,helya,group);
    // Use final scaled damage and native Player death/corpse/resurrection,
    // rather than removing an aura with a simulated death removal mode.
    for(Unit* killer:{static_cast<Unit*>(&helya),static_cast<Unit*>(&p),static_cast<Unit*>(&pet)}){
        ControlBegin(p,subject,map,helya);subject.RemoveAura(228054);subject.SetHealth(subject.CountPctFromMaxHealth(30));helya.SetPower(POWER_ALTERNATE,10);
        ControlWatch death(map,subject,helya);death.Advance(1500);Require(subject.HasUnitState(UNIT_STATE_CASTING),"Lethal hit missed native Infusion");
        auto raw=uint32(std::ceil(1.5*subject.GetHealth(killer)*subject.GetHealthMultiplierForTarget(killer)));
        std::cout<<"CONTROL_LETHAL begin killer="<<(killer==&helya?"boss":killer==&p?"player":"pet")<<" raw="<<raw<<std::endl;
        killer->DealDamage(&subject,raw,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
        Require(!subject.IsAlive()&&subject.getDeathState()==JUST_DIED,"Scaled killing blow did not enter real Player death");
        Require(!subject.HasAura(232350)&&!subject.HasAura(232358)&&!subject.HasAura(231862)&&subject.GetCharmerGUID().IsEmpty()&&!subject.HasUnitState(UNIT_STATE_CASTING)&&subject.getFaction()==faction&&subject.GetDisplayId()==display&&subject.IsControlledByPlayer(),"Actual Player death retained Corrupted control state");
        subject.KillPlayer();Require(subject.getDeathState()==CORPSE,"Native Player corpse transition failed");death.Advance(8000);
        Require(death.casts[231863].empty()&&helya.GetPower(POWER_ALTERNATE)==10,"Dead Player completed residual Infusion/Eruption");
        subject.ResurrectPlayer(1.0f,false);death.Advance(8000,true);
        Require(subject.IsAlive()&&subject.GetHealth()>0&&!subject.HasAura(232350)&&!subject.HasAura(232358)&&!subject.HasAura(231862)&&subject.GetCharmerGUID().IsEmpty()&&!subject.HasUnitState(UNIT_STATE_CASTING)&&subject.getFaction()==faction&&subject.GetDisplayId()==display&&subject.IsControlledByPlayer()&&death.casts[231863].empty()&&helya.GetPower(POWER_ALTERNATE)==10,"Native resurrection restored stale control/cast/effects");
        std::cout<<"CONTROL_LETHAL recovered killer="<<(killer==&helya?"boss":killer==&p?"player":"pet")<<" bursts="<<death.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<std::endl;
    }
#endif
    ControlBegin(p,subject,map,helya);subject.RemoveAura(228054);helya.SetPower(POWER_ALTERNATE,10);ControlWatch nativeAI(map,subject,helya);nativeAI.Advance(18000,true);
    std::cout<<"CONTROL_NATIVE_AI class="<<uint32(subject.getClass())<<" infusions="<<nativeAI.casts[228854].size()<<" bursts="<<nativeAI.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<" movement="<<uint32(subject.GetMotionMaster()->GetCurrentMovementGeneratorType())<<std::endl;
    Require(nativeAI.casts[231863].size()==2&&helya.GetPower(POWER_ALTERNATE)==70,"Native charmed-Player AI prevented repeated Infusion completion");
    subject.RemoveAura(232350);
#ifndef TRIAL_HELYA_CONTROL_BASELINE
    ControlBegin(p,subject,map,helya);subject.RemoveAura(228054);helya.SetPower(POWER_ALTERNATE,10);ControlWatch victory(map,subject,helya);victory.Advance(1500);
    Require(subject.HasUnitState(UNIT_STATE_CASTING),"Victory missed native Infusion");
    helya.CastSpell(&p,232450,true);Require(p.HasAura(232450),"Victory pending-absorb input missing");
    helya.ResetPlayerDamageReq();helya.LowerPlayerDamageReq(1);helya.SetLootRecipient(&p);
    Unit* winner=p.getRace()==RACE_ORC?static_cast<Unit*>(&pet):static_cast<Unit*>(&p);winner->Kill(&helya,false);
    Require(helya.isDead(false)&&map.GetInstanceScript()->GetBossState(2)==DONE,"Actual controlled-Player victory failed");
    // Assert immediately: waiting beyond Axion's ten-second duration would
    // conceal a missing victory removal behind ordinary aura expiry.
    Require(!subject.HasAura(232350)&&!subject.HasAura(232358)&&!subject.HasAura(231862)&&subject.GetCharmerGUID().IsEmpty()&&!subject.HasUnitState(UNIT_STATE_CASTING)&&subject.getFaction()==faction&&subject.GetDisplayId()==display&&subject.IsControlledByPlayer()&&!p.HasAura(232450)&&!p.HasAura(232350),"Victory did not immediately clear current control and pending Axion");
    auto victoryEnergy=helya.GetPower(POWER_ALTERNATE);victory.Advance(15000,true);Tick(p,15000);
    Require(!subject.HasAura(232350)&&!subject.HasAura(232358)&&!subject.HasAura(231862)&&subject.GetCharmerGUID().IsEmpty()&&!subject.HasUnitState(UNIT_STATE_CASTING)&&subject.getFaction()==faction&&subject.IsControlledByPlayer()&&!p.HasAura(232450)&&!p.HasAura(232350)&&victory.casts[231863].empty()&&helya.GetPower(POWER_ALTERNATE)==victoryEnergy,"Native victory retained/recreated control or completed residual Infusion");
    std::cout<<"CONTROL_VICTORY pet="<<(winner==&pet)<<" bursts="<<victory.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<" pendingAbsorb="<<p.HasAura(232450)<<std::endl;
#endif
    if(helya.IsAlive())ClearCombat(p,helya);std::cout<<"PASS: native Corrupted observation diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);try{return RunGarm(argc,argv,ControlCase);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
