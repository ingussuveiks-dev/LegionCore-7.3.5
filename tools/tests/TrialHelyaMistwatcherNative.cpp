#define TRIAL_HELYA_GRIMELORD_SUPPORT_ONLY
#include "TrialHelyaGrimelordNative.cpp"
struct MistWatch
{
    InstanceMap& map;ObjectGuid guid,boss;uint32 elapsed=0;bool allMistwatchers=false;
    std::map<uint32,std::set<ObjectGuid>> casts,targets,damaged;
    std::map<uint32,std::vector<uint32>> times;
    MistWatch(InstanceMap& m,Creature& c,Creature& h):map(m),guid(c.GetGUID()),boss(h.GetGUID()){}
    void Observe(){std::vector<Creature*> casters{map.GetCreature(boss)};if(allMistwatchers){for(auto c:Actors(map,116335))casters.push_back(c);}else casters.push_back(map.GetCreature(guid));for(auto caster:casters)if(caster)for(auto id:{228854u,228021u,231862u,231863u,228300u,228032u,230356u})for(auto s:NativeSpellEvents(*caster,id)){
        if(casts[id].insert(s->m_castGuid[0]).second)times[id].push_back(elapsed);
        for(auto const& t:*s->GetUniqueTargetInfo())if(t->effectMask&1){targets[id].insert(t->targetGUID);if(t->processed&&t->damage>0)damaged[id].insert(t->targetGUID);}
    }}
    // Spell/AI isolation freezes boss scheduling, retaining its real phase and
    // native energize handler. Full-map cases separately exercise Fury timing.
    void Advance(uint32 ms,bool full=false){Observe();for(unsigned t=0;t<ms;t+=10){if(full)PumpHelya(map,10);else if(auto c=map.GetCreature(guid))c->Update(10);Observe();for(auto const& ref:map.GetPlayers()){auto p=ref.getSource();if(p->IsAlive())p->SetHealth(p->GetMaxHealth());p->Unit::Update(10);}Observe();elapsed+=10;}}
};
static Creature* MistBegin(GarmPlayer& p,InstanceMap& map,Creature& helya)
{
    auto g=GrimBegin(p,map,helya);g->DespawnOrUnsummon();PumpHelya(map,20);
    auto pos=helya.GetPosition();auto c=helya.SummonCreature(116335,pos.GetPositionX()+5,pos.GetPositionY(),pos.GetPositionZ(),0,TEMPSUMMON_MANUAL_DESPAWN);Require(c&&c->AI(),"Native Mistwatcher missing");map.PlayerRelocation(&p,pos.GetPositionX()+8,pos.GetPositionY(),pos.GetPositionZ(),0);return c;
}
static uint32 MistPhaseTwo(GarmPlayer& p,InstanceMap& map,Creature& helya,bool keep)
{
    auto c=MistBegin(p,map,helya);auto manual=c->GetGUID();p.Kill(c,false);
    MistWatch watch(map,*c,helya);watch.allMistwatchers=true;std::set<ObjectGuid> natural;
    // Actual boats summon the Mythic pair. Compare two earned encounters,
    // killing the pair before its first cast in the control. No energy input,
    // aura removal or boss-state/callback injection is used in this comparison.
    while(watch.elapsed<120000&&watch.times[228300].size()<2){
        for(auto actor:Actors(map,116335))if(actor->GetGUID()!=manual){natural.insert(actor->GetGUID());if(!keep&&actor->IsAlive())p.Kill(actor,false);}
        watch.Advance(10,true);
    }
    Require(!natural.empty()&&watch.times[228300].size()>=2&&helya.AI()->GetData(2)==2,"Natural Mythic comparison lost boats/casters or repeated long Fury");
    Require(keep?!watch.casts[231863].empty():watch.casts[231863].empty(),"Natural Mythic Infusion success/kill control did not execute");
    auto second=watch.times[228300][1];std::cout<<"MIST_PHASE2 keep="<<keep<<" natural="<<natural.size()<<" secondFury="<<second<<" infusions="<<watch.casts[228854].size()<<" bursts="<<watch.casts[231863].size()<<std::endl;return second;
}
static void MistCase(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    // EffectInterruptCast executes the ordinary proc-check service as well as
    // spell_proc_event. World loads both; omitting the former is a fixture bug.
    static bool procLoaded=false;if(!procLoaded){sSpellMgr->LoadSpellPrcoCheck();procLoaded=true;}
    LoadPhaseThreeConditions();
    auto proc=sSpellMgr->GetSpellProcEvent(231862);Require(proc&&!proc->empty()&&proc->front().procEx==PROC_EX_INTERRUPT,"Native Bleak Eruption lost its interrupt-only DB filter");
    p.GetOrInitPetStable();LootPet pet(p,map);pet.setFaction(p.getFaction());pet.SetMaxHealth(p.GetMaxHealth());pet.SetHealth(p.GetMaxHealth());
    auto c=MistBegin(p,map,helya);std::cout<<"MIST_INITIAL diff="<<uint32(map.GetDifficultyID())<<" owner="<<(c->GetAnyOwner()==&helya)<<" combat="<<c->isInCombat()<<" procAura="<<c->HasAura(231862)<<" cast="<<sSpellMgr->GetSpellInfo(228854)->CalcCastTime(c)<<" alt="<<helya.GetPower(POWER_ALTERNATE)<<std::endl;
    helya.SetPower(POWER_ALTERNATE,10);MistWatch success(map,*c,helya);success.Advance(10500);
    std::cout<<"MIST_SUCCESS infusion="<<success.casts[228854].size()<<" burst="<<success.casts[231863].size()<<" burstTargets="<<success.targets[231863].size()<<" damageTargets="<<success.damaged[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<" aura="<<c->HasAura(231862)<<std::endl;
    Require(success.casts[231863].size()==1&&success.damaged[231863].count(p.GetGUID())&&helya.GetPower(POWER_ALTERNATE)==40,"Native success must energize30 and damage Players exactly once");
    success.Advance(7000);Require(success.casts[231863].size()==2&&helya.GetPower(POWER_ALTERNATE)==70,"Native repeated Infusion duplicated/lost its energize or burst");
    c=MistBegin(p,map,helya);helya.SetPower(POWER_ALTERNATE,10);MistWatch interrupt(map,*c,helya);interrupt.Advance(3500);Require(c->HasUnitState(UNIT_STATE_CASTING),"Natural Mist Infusion never began");auto hpBeforeInterrupt=p.GetHealth();p.CastSpell(c,2139,true);auto immediateDamage=hpBeforeInterrupt-p.GetHealth();interrupt.Advance(1000);
    std::cout<<"MIST_INTERRUPT burst="<<interrupt.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<" casting="<<c->HasUnitState(UNIT_STATE_CASTING)<<std::endl;
    std::cout<<"MIST_INTERRUPT_DETAIL immediate="<<immediateDamage<<" targets="<<interrupt.targets[231863].size()<<" damaged="<<interrupt.damaged[231863].size()<<std::endl;
    Require(interrupt.casts[231863].size()==1&&helya.GetPower(POWER_ALTERNATE)==10&&!c->HasUnitState(UNIT_STATE_CASTING),"Actual Counterspell must interrupt once without energizing");
#ifdef TRIAL_HELYA_MISTWATCHER_BASELINE
    Require(!immediateDamage&&interrupt.targets[231863].empty(),"Pinned interrupt-school lockout rejection changed");
#else
    Require(immediateDamage||interrupt.damaged[231863].count(p.GetGUID()),"Actual interrupt eruption did not damage Players");
#endif
    Require(c->CastSpell(&helya,228854,false)==SPELL_FAILED_NOT_READY,"Ordinary Frost cast bypassed Counterspell school lockout");
    Require(c->CastSpell(&helya,228854,TRIGGERED_IGNORE_CAST_IN_PROGRESS)==SPELL_FAILED_NOT_READY,"Triggered cast lacking cooldown exemption bypassed lockout");
    p.CastSpell(c,2139,true);interrupt.Advance(1000);Require(interrupt.casts[231863].size()==1,"Counterspell on idle target incorrectly erupted");
    c=MistBegin(p,map,helya);MistWatch hit(map,*c,helya);auto hp=c->GetHealth();p.CastSpell(c,116,true);hit.Advance(1000);std::cout<<"MIST_ORDINARY burst="<<hit.casts[231863].size()<<" aura="<<c->HasAura(231862)<<" hit="<<(c->GetHealth()<hp)<<std::endl;
    Require(c->GetHealth()<hp&&hit.casts[231863].empty(),"Ordinary damaging Frostbolt must not trigger interrupt eruption");
    for(bool death:{false,true}){
        c=MistBegin(p,map,helya);helya.SetPower(POWER_ALTERNATE,10);MistWatch stopped(map,*c,helya);stopped.Advance(3500);Require(c->HasUnitState(UNIT_STATE_CASTING),"Lifecycle case missed actual cast");
        if(death){Unit* killer=p.getRace()==RACE_ORC?static_cast<Unit*>(&pet):static_cast<Unit*>(&p);killer->Kill(c,false);}else c->AI()->EnterEvadeMode();
        stopped.Advance(9000);Require(stopped.casts[231863].empty()&&helya.GetPower(POWER_ALTERNATE)==10,"Death/evade allowed pending Infusion to energize or erupt");
        std::cout<<"MIST_STOP death="<<death<<" burst="<<stopped.casts[231863].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<std::endl;
    }
    c=MistBegin(p,map,helya);MistWatch wiped(map,*c,helya);wiped.Advance(3500);auto guid=c->GetGUID();ClearCombat(p,helya);wiped.Advance(12000,true);
    Require(!map.GetCreature(guid)&&Actors(map,116335).empty()&&helya.AI()->GetData(2)==0&&!helya.HasAura(228300)&&!helya.HasAura(228546),"Actual wipe retained Mistwatcher or restarted Fury");
    // Retain a real phase-two summon through all nine actual tentacle deaths.
    // Phase-three initialization sets42. Keep its actual regeneration active:
    // native energize changes the field via ModifyPower, and the next ordinary
    // regeneration SetPower tick notifies the AI that it has reached100.
    c=MistBegin(p,map,helya);for(auto t:Actors(map,114900))p.Kill(t,false);MistWatch fury(map,*c,helya);fury.Advance(7000,true);
    Require(helya.AI()->GetData(2)==3&&c->IsAlive()&&c->HasUnitState(UNIT_STATE_CASTING),"Native phase-three transition lost the retained caster");
    Require(helya.HasAura(228546),"Native phase-three alternate regeneration missing");
    helya.SetPower(POWER_ALTERNATE,70);fury.Advance(2000,true);
    Require(helya.GetPower(POWER_ALTERNATE)<85&&fury.casts[228032].empty(),"Ordinary regeneration reached Fury before Infusion completed");
    fury.Advance(2000,true);
    Require(fury.casts[231863].size()==1&&fury.casts[228032].size()==1,"Infusion reaching100 did not start native phase-three Fury");
    fury.Advance(3000,true);
    std::cout<<"MIST_FURY phase3 bursts="<<fury.casts[231863].size()<<" fury="<<fury.casts[228032].size()<<" waves="<<fury.casts[230356].size()<<" energy="<<helya.GetPower(POWER_ALTERNATE)<<" bossCast="<<helya.HasUnitState(UNIT_STATE_CASTING)<<std::endl;
    Require(fury.casts[231863].size()==1&&fury.casts[228032].size()==1&&fury.casts[230356].size()>=1,"Native Fury did not complete its wave after Infusion");
    if(map.IsMythicRaid()){
        auto control=MistPhaseTwo(p,map,helya,false),infused=MistPhaseTwo(p,map,helya,true);
        Require(infused+5000<control,"Natural Mistwatchers did not materially advance the next phase-two Fury");
    }
    ClearCombat(p,helya);PumpHelya(map,3000);std::cout<<"PASS: native Mistwatcher observation diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);try{return RunGarm(argc,argv,MistCase);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
