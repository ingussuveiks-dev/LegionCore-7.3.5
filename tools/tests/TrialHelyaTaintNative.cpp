// Actual Taint AuraScript/dispel/expiry/missile dispatch after earned raid access.
#define TRIAL_HELYA_PHASE3_SUPPORT_ONLY
#include "TrialHelyaPhase3Native.cpp"
static void LoadTaintConditions()
{
    static bool loaded=false;if(loaded)return;loaded=true;
    auto row=WorldDatabase.Query("SELECT SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=228053");
    Require(row&&row->GetRowCount()==1,"Native Taint condition missing/ambiguous");auto f=row->Fetch();
    auto c=new Condition();c->SourceType=CONDITION_SOURCE_TYPE_SPELL_IMPLICIT_TARGET;c->SourceGroup=f[0].GetUInt32();c->SourceEntry=228053;c->ConditionType=ConditionTypes(f[1].GetInt32());c->ConditionTarget=f[2].GetUInt8();c->ConditionValue1=f[3].GetUInt32();c->ConditionValue2=f[4].GetUInt32();c->NegativeCondition=f[5].GetBool();
    Require(sConditionMgr->isConditionTypeValid(c)&&(sConditionMgr->*Access(ValidateSourceTag{}))(c)&&(sConditionMgr->*Access(PhaseThreeConditionTag{}))(c),"Native Taint condition admission failed");
}
struct TaintWatch
{
    std::vector<Unit*> casters;
    std::map<uint32,std::set<ObjectGuid>> casts,targets,owners,damaged;
    std::map<uint32,std::vector<Position>> destinations;
    std::map<uint32,uint64> delay;
    uint32 elapsed=0;
    explicit TaintWatch(Creature& helya,InstanceMap& map)
    {casters.push_back(&helya);for(auto const& ref:map.GetPlayers())casters.push_back(ref.getSource());}
    void Observe()
    {
        for(auto caster:casters)for(auto id:{228053u,228052u,228051u})for(auto spell:NativeSpellEvents(*caster,id)){
            if(casts[id].insert(spell->m_castGuid[0]).second){owners[id].insert(caster->GetGUID());if(spell->m_targets.HasDst())destinations[id].push_back(*spell->m_targets.GetDstPos());std::cout<<"TAINT_CAST time="<<elapsed<<" spell="<<id<<" caster="<<caster->GetGUID().GetCounter()<<" delay="<<spell->GetDelayMoment()<<std::endl;}
            delay[id]=std::max(delay[id],spell->GetDelayMoment());
            for(auto const& target:*spell->GetUniqueTargetInfo())if(target->effectMask&1){targets[id].insert(target->targetGUID);if(target->processed&&target->damage>0)damaged[id].insert(target->targetGUID);}
        }
    }
    void Advance(InstanceMap& map,uint32 ms)
    {
        Observe();for(unsigned t=0;t<ms;t+=10){PumpHelya(map,10);Observe();for(auto const& ref:map.GetPlayers()){ref.getSource()->Unit::Update(10);Observe();}elapsed+=10;}
    }
};
static void TaintRestart(GarmPlayer& p,InstanceMap& map,Creature& helya)
{
    ClearCombat(p,helya);PumpHelya(map,2000);for(unsigned t=0;t<2000;t+=10)for(auto const& ref:map.GetPlayers())ref.getSource()->Unit::Update(10);
    if(helya.isInCombat()){ClearCombat(p,helya);PumpHelya(map,2000);}
    Require(map.GetInstanceScript()->GetBossState(2)==NOT_STARTED&&!helya.HasUnitState(UNIT_STATE_EVADE),"Native Taint reset did not finish");
    map.PlayerRelocation(&p,510,670,2,0);helya.SetInCombatWith(&p);Require(helya.AI()->GetData(2)==1,"Native Taint retry failed");helya.RemoveAura(232229);
}
static void TaintSelection(GarmPlayer& p,InstanceMap& map,Creature& helya,GarmPlayer& closePlayer,GarmPlayer& distantPlayer)
{
    GarmPlayer extra1(map,p.getRace()),extra2(map,p.getRace()),extra3(map,p.getRace()),extra4(map,p.getRace());
    LootPet pet(p,map);pet.setFaction(p.getFaction());pet.SetMaxHealth(p.GetMaxHealth());pet.SetHealth(p.GetMaxHealth());
    std::vector<Player*> players{&p,&closePlayer,&distantPlayer,&extra1,&extra2,&extra3,&extra4};
    unsigned const cap=map.GetDifficultyID()==DIFFICULTY_MYTHIC_RAID?5:3;
    for(auto full:{false,true}){
        TaintRestart(p,map,helya);helya.getThreatManager().clearReferences();
        unsigned const eligible=full?unsigned(players.size()):cap-1;
        for(unsigned i=0;i<players.size();++i){players[i]->SetGameMaster(i>=eligible);map.PlayerRelocation(players[i],510+float(i),670,2,0);players[i]->RemoveAura(228054);if(i<eligible)helya.AddThreat(players[i],i?1.0f:100000.0f);}
        pet.RemoveAura(228054);helya.AddThreat(&pet,1.0f);
        Require(helya.getThreatManager().getThreatList().size()==eligible+1,"Taint target probe did not admit controlled native threat inputs");
        unsigned elapsed=0;for(;elapsed<25000;elapsed+=10){PumpHelya(map,10);bool applied=pet.HasAura(228054);for(auto player:players)applied|=player->HasAura(228054);if(applied)break;}
        unsigned selected=0;for(auto player:players)if(player->HasAura(228054))++selected;
        std::cout<<"TAINT_SELECTION full="<<full<<" elapsed="<<elapsed<<" eligible="<<eligible<<" selected="<<selected<<" pet="<<pet.HasAura(228054)<<" tank="<<p.HasAura(228054)<<std::endl;
        Require(elapsed<25000&&helya.getVictim()==&p,"Native first Taint/tank victim missing");
#ifdef TRIAL_HELYA_TAINT_BASELINE
        if(!full)Require(pet.HasAura(228054)&&selected==eligible,"Pinned raw-threat pet selection no longer reproduced");
        Require(selected+unsigned(pet.HasAura(228054))==cap,"Pinned Taint cap changed");
#else
        Require(!pet.HasAura(228054)&&p.HasAura(228054)&&selected==std::min(cap,eligible),"Native Taint omitted tank, selected pet, or lost player slots");
        for(unsigned i=eligible;i<players.size();++i)Require(!players[i]->HasAura(228054),"Taint selected a Player outside its threat inputs");
#endif
    }
    ClearCombat(p,helya);PumpHelya(map,2000);
    for(auto player:players)player->SetGameMaster(false);
}
static void TaintCase(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    p.GetOrInitPetStable();LoadTaintConditions();GarmPlayer closePlayer(map,p.getRace()),distantPlayer(map,p.getRace());
    if(p.getRace()==RACE_HUMAN)for(auto id:{228054u,228053u,228052u,228051u}){
        auto s=sSpellMgr->GetSpellInfo(id);Require(s,"Native Taint spell missing");auto misc=s->GetMisc(map.GetDifficultyID());
        std::cout<<"CATALOG spell="<<id<<" duration="<<s->GetDuration(map.GetDifficultyID())<<" speed="<<misc->MiscData.Speed<<" launch="<<misc->MiscData.LaunchDelay<<" description="<<sSpellStore.LookupEntry(id)->Description->Str[0]<<std::endl;
        for(unsigned i=0;i<3;++i){auto e=s->GetEffect(SpellEffIndex(i),map.GetDifficultyID());if(e->Effect)std::cout<<" EFFECT index="<<i<<" type="<<e->Effect<<" aura="<<e->ApplyAuraName<<" trigger="<<e->TriggerSpell<<" targets="<<e->TargetA.GetTarget()<<','<<e->TargetB.GetTarget()<<" radius="<<e->CalcRadius(&helya)<<std::endl;}
    }
    TaintSelection(p,map,helya,closePlayer,distantPlayer);
    for(auto mode:{AURA_REMOVE_BY_ENEMY_SPELL,AURA_REMOVE_BY_EXPIRE,AURA_REMOVE_BY_DEFAULT,AURA_REMOVE_BY_CANCEL,AURA_REMOVE_BY_DEATH}){
        TaintRestart(p,map,helya);map.PlayerRelocation(&closePlayer,512,670,2,0);map.PlayerRelocation(&distantPlayer,530,670,2,0);
        TaintWatch watch(helya,map);helya.AddAura(228054,&p);Require(p.HasAura(228054),"Native Taint did not apply");Position const original=p.GetPosition();
        if(mode==AURA_REMOVE_BY_EXPIRE){auto duration=p.GetAura(228054)->GetMaxDuration();if(duration<0){p.GetAura(228054)->SetMaxDuration(1);p.GetAura(228054)->SetDuration(1);duration=1;}watch.Advance(map,uint32(duration)+40);}
        else if(mode==AURA_REMOVE_BY_ENEMY_SPELL)p.RemoveAurasDueToSpellByDispel(228054,527,helya.GetGUID(),&closePlayer);
        else p.GetAura(228054)->Remove(mode);
        watch.Observe();Require(!p.HasAura(228054),"Native Taint removal did not finish");map.PlayerRelocation(&p,530,670,2,0);watch.Advance(map,8000);
        unsigned burst=0,essence=0;
#ifdef TRIAL_HELYA_TAINT_BASELINE
        burst=mode==AURA_REMOVE_BY_ENEMY_SPELL?0:1;essence=mode==AURA_REMOVE_BY_ENEMY_SPELL?1:0;
#else
        if(mode==AURA_REMOVE_BY_ENEMY_SPELL||mode==AURA_REMOVE_BY_EXPIRE)burst=essence=1;
#endif
        Require(watch.casts[228053].size()==burst&&watch.casts[228052].size()==essence&&watch.casts[228051].size()==essence,"Native Taint removal lost/duplicated burst or Essence");
        if(essence){Require(watch.delay[228052]==4000&&watch.destinations[228052].size()==1&&watch.destinations[228052].front().GetExactDist(&original)<0.01f,"Essence lost native four-second delay/removal location");Require(watch.targets[228051]==std::set<ObjectGuid>{closePlayer.GetGUID()},"Native eight-yard Essence hit moving carrier/distant player or missed nearby player");Require(watch.owners[228052]==std::set<ObjectGuid>{helya.GetGUID()}&&watch.owners[228051]==watch.owners[228052],"Essence lost Helya ownership");}
#ifndef TRIAL_HELYA_TAINT_BASELINE
        if(burst){Require(watch.owners[228053]==std::set<ObjectGuid>{helya.GetGUID()}&&watch.targets[228053]==std::set<ObjectGuid>{p.GetGUID(),closePlayer.GetGUID(),distantPlayer.GetGUID()},"Taint raid burst has wrong owner/actual player targets");}
#endif
        std::cout<<"TAINT_REMOVE mode="<<uint32(mode)<<" burst="<<watch.casts[228053].size()<<" essence="<<watch.casts[228052].size()<<" impact="<<watch.casts[228051].size()<<" raidTargets="<<watch.targets[228053].size()<<" essenceTargets="<<watch.targets[228051].size()<<std::endl;
    }
    TaintRestart(p,map,helya);helya.AddAura(228054,&p);TaintWatch pending(helya,map);p.RemoveAurasDueToSpellByDispel(228054,527,helya.GetGUID(),&closePlayer);pending.Advance(map,500);Require(pending.casts[228052].size()==1,"Pending-Essence wipe probe did not start a real missile");ClearCombat(p,helya);pending.Advance(map,8000);Require(pending.casts[228051].empty(),"A pre-wipe Essence missile impacted after reset");
    TaintRestart(p,map,helya);helya.AddAura(228054,&p);TaintWatch wipe(helya,map);ClearCombat(p,helya);wipe.Advance(map,8000);
#ifdef TRIAL_HELYA_TAINT_BASELINE
    Require(wipe.casts[228053].size()==1,"Pinned reset burst changed");
#else
    Require(wipe.casts[228053].empty()&&wipe.casts[228052].empty()&&wipe.casts[228051].empty(),"Native reset created new Taint combat effects");
#endif
    std::cout<<"TAINT_WIPE burst="<<wipe.casts[228053].size()<<" essence="<<wipe.casts[228052].size()<<std::endl;
    helya.AddAura(228054,&p);TaintWatch stale(helya,map);p.RemoveAurasDueToSpellByDispel(228054,527,helya.GetGUID(),&closePlayer);stale.Advance(map,8000);
#ifdef TRIAL_HELYA_TAINT_BASELINE
    Require(stale.casts[228052].size()==1,"Pinned inactive-caster Essence changed");
#else
    Require(stale.casts[228053].empty()&&stale.casts[228052].empty()&&stale.casts[228051].empty()&&map.GetInstanceScript()->GetBossState(2)==NOT_STARTED,"Stale Taint removal restarted encounter/effects");
    TaintRestart(p,map,helya);
    {GarmPlayer carrier(map,p.getRace());map.PlayerRelocation(&carrier,510,670,2,0);map.PlayerRelocation(&p,530,670,2,0);map.PlayerRelocation(&closePlayer,512,670,2,0);map.PlayerRelocation(&distantPlayer,530,670,2,0);helya.AddAura(228054,&carrier);carrier.SetHealth(1);TaintWatch lethal(helya,map);carrier.RemoveAurasDueToSpellByDispel(228054,527,helya.GetGUID(),&closePlayer);lethal.Observe();for(unsigned t=0;t<50&&carrier.IsAlive();t+=10){helya.Update(10);lethal.Observe();}lethal.Advance(map,8000);Require(!carrier.IsAlive()&&!carrier.GetHealth()&&lethal.casts[228053].size()==1&&lethal.casts[228052].size()==1&&lethal.casts[228051].size()==1,"Lethal raid burst suppressed/duplicated the independent Essence");std::cout<<"TAINT_LETHAL carrierDead=1 essence=1"<<std::endl;}
    TaintRestart(p,map,helya);auto ghost=helya.SummonCreature(WORLD_TRIGGER,p.GetPosition(),TEMPSUMMON_MANUAL_DESPAWN);Require(ghost,"Native missing-caster fixture failed to summon");auto ghostGuid=ghost->GetGUID();ghost->AddAura(228054,&p);ghost->DespawnOrUnsummon();PumpHelya(map,20);Require(p.HasAura(228054)&&!p.GetAura(228054)->GetCaster(),"Native orphan Taint fixture did not lose its caster");TaintWatch orphan(helya,map);p.RemoveAurasDueToSpellByDispel(228054,527,ghostGuid,&closePlayer);orphan.Advance(map,100);Require(!p.HasAura(228054)&&orphan.casts[228053].empty()&&orphan.casts[228052].empty(),"Missing-caster Taint did not cleanly stop");std::cout<<"TAINT_ORPHAN safe=1"<<std::endl;
#endif
    std::cout<<"TAINT_STALE essence="<<stale.casts[228052].size()<<std::endl;
    TaintRestart(p,map,helya);for(unsigned i=0;i<16&&helya.AI()->GetData(2)==1;++i)p.DealDamage(&helya,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(helya.AI()->GetData(2)==2,"Native Taint victory bootstrap missed phase two");for(auto tentacle:Actors(map,114900))p.Kill(tentacle,false);PumpHelya(map,7000);Require(helya.AI()->GetData(2)==3,"Native Taint victory bootstrap missed phase three");helya.AddAura(228054,&p);TaintWatch victory(helya,map);helya.ResetPlayerDamageReq();helya.LowerPlayerDamageReq(1);helya.SetLootRecipient(&p);for(unsigned i=0;i<16&&helya.IsAlive();++i)p.DealDamage(&helya,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);victory.Advance(map,8000);
    Require(helya.isDead(false)&&!p.HasAura(228054)&&map.GetInstanceScript()->GetBossState(2)==DONE&&p.GetQuestStatus(44868)==QUEST_STATUS_COMPLETE&&p.GetQuestObjectiveData(44868,55482)==1,"Native Taint victory cleanup lost quest credit");Checkpoint(map,group,7);
#ifdef TRIAL_HELYA_TAINT_BASELINE
    Require(victory.casts[228053].size()==1,"Pinned victory self-cast burst changed");
#else
    Require(victory.casts[228053].empty()&&victory.casts[228052].empty()&&victory.casts[228051].empty(),"Victory cleanup cast Taint combat effects");
#endif
    std::cout<<"TAINT_VICTORY burst="<<victory.casts[228053].size()<<" essence="<<victory.casts[228052].size()<<std::endl;
    std::cout<<"PASS: native Taint removal/expiry/dispel/Essence location/targets/reset; diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunGarm(argc,argv,TaintCase);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
