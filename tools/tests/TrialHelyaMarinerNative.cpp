#define TRIAL_HELYA_GRIMELORD_SUPPORT_ONLY
#include "TrialHelyaGrimelordNative.cpp"
struct MarinerWatch
{
    InstanceMap& map;ObjectGuid guid,damageProbe;uint32 elapsed=0;
    std::map<uint32,std::set<ObjectGuid>> casts,targets,damaged,damagedTargets;
    std::map<uint32,std::vector<uint32>> times;
    std::map<uint32,std::map<ObjectGuid,std::set<ObjectGuid>>> perCast;
    std::map<uint32,std::vector<Position>> destinations;
    std::vector<int32> pulseDamage;
    explicit MarinerWatch(InstanceMap& m,Creature& c,Unit* probe=nullptr):map(m),guid(c.GetGUID()),damageProbe(probe?probe->GetGUID():ObjectGuid::Empty){}
    void Observe(){if(auto c=map.GetCreature(guid))for(auto id:{228633u,228632u,228631u,228611u,228619u,228621u})for(auto s:NativeSpellEvents(*c,id)){
        if(casts[id].insert(s->m_castGuid[0]).second){times[id].push_back(elapsed);if(s->m_targets.HasDst())destinations[id].push_back(*s->m_targets.GetDstPos());}
        for(auto const& t:*s->GetUniqueTargetInfo())if(t->effectMask&1){targets[id].insert(t->targetGUID);perCast[id][s->m_castGuid[0]].insert(t->targetGUID);if(t->processed&&t->damage>0){damagedTargets[id].insert(t->targetGUID);if((damageProbe.IsEmpty()||t->targetGUID==damageProbe)&&damaged[id].insert(s->m_castGuid[0]).second&&id==228621)pulseDamage.push_back(t->damage);}}
    }}
    void Advance(uint32 ms,bool ai=true){Observe();for(unsigned t=0;t<ms;t+=10){if(ai)PumpHelya(map,10);else if(auto c=map.GetCreature(guid))c->Unit::Update(10);Observe();for(auto const& ref:map.GetPlayers()){auto p=ref.getSource();if(p->IsAlive())p->SetHealth(p->GetMaxHealth());p->Unit::Update(10);}Observe();elapsed+=10;}}
};
static Creature* MarinerBegin(GarmPlayer& p,InstanceMap& map,Creature& helya)
{
    auto grim=GrimBegin(p,map,helya);grim->DespawnOrUnsummon();PumpHelya(map,20);
    auto c=helya.SummonCreature(114809,458,639,7.5f,0,TEMPSUMMON_MANUAL_DESPAWN);Require(c&&c->AI(),"Native Mariner summon missing");PumpHelya(map,20);return c;
}
static void MarinerLantern(GarmPlayer& p,InstanceMap& map,Creature& helya)
{
    auto c=MarinerBegin(p,map,helya);Require(c->GetPowerType()==POWER_ENERGY&&c->GetMaxPower(POWER_ENERGY)==100&&c->HasAura(228383),"Mariner primary energy/Darkness did not initialize");
    MarinerWatch profile(map,*c);unsigned const easy[]={1,3,6},hard[]={3,6,10};
    for(unsigned i=0;i<3;++i){profile.Advance(1000);Require(c->GetPower(POWER_ENERGY)==(map.IsLfrRaid()?easy[i]:hard[i]),"Native energizer failed advancing difficulty profile or leaked automatic regen");}
    // A full bar waits for an existing cast. Interrupted Lantern must keep its
    // energy and retry through the real Darkness periodic/AI event path.
    c->CastSpell(c,228633,false);Require(c->HasUnitState(UNIT_STATE_CASTING),"Blocking cast fixture did not start");c->SetPower(POWER_ENERGY,100);c->SetPower(POWER_ENERGY,100);
    MarinerWatch blocked(map,*c,&p);blocked.Advance(500);Require(blocked.casts[228619].empty()&&c->GetPower(POWER_ENERGY)==100,"Busy cast lost full bar or started duplicate Lantern");
    c->InterruptNonMeleeSpells(true);blocked.Advance(100);Require(c->HasUnitState(UNIT_STATE_CASTING),"Pending Lantern did not start after other cast ended");
    c->InterruptNonMeleeSpells(true);Require(c->GetPower(POWER_ENERGY)==100,"Interrupted Lantern consumed full energy");blocked.Advance(2500);Require(c->HasAura(228619)&&c->GetPower(POWER_ENERGY)==0,"Interrupted full-energy Lantern did not retry/consume on success");
    auto casts=blocked.casts[228619].size();Require(casts==2,"Full energy scheduled duplicate successful Lanterns");
    auto energy=c->GetPower(POWER_ENERGY);blocked.Advance(1000);Require(c->GetPower(POWER_ENERGY)==energy,"Energy refilled during native Lantern aura");blocked.Advance(6000);
    std::cout<<"LANTERN_OBSERVED casts="<<blocked.casts[228619].size()<<" expectedCasts="<<casts<<" pulses="<<blocked.casts[228621].size()<<" power="<<c->GetPower(POWER_ENERGY)<<" values=";for(auto d:blocked.pulseDamage)std::cout<<d<<',';std::cout<<std::endl;
    auto lantern=sSpellMgr->GetSpellInfo(228619);unsigned pulses=lantern->GetDuration(map.GetDifficultyID())/lantern->GetEffect(EFFECT_0,map.GetDifficultyID())->ApplyAuraPeriod+(lantern->HasAttribute(SPELL_ATTR5_START_PERIODIC_AT_APPLY)?1:0);
    Require(pulses==13&&blocked.casts[228619].size()==casts&&blocked.casts[228621].size()==pulses,"Lantern lost native immediate/half-second pulses or recast while channeling");
    Require(blocked.pulseDamage.size()==pulses&&blocked.pulseDamage[1]>blocked.pulseDamage[0]&&blocked.pulseDamage[8]>blocked.pulseDamage[1],"Native Lantern pulses did not ramp damage on the same Player");
    std::cout<<"MARINER_LANTERN pulses="<<blocked.casts[228621].size()<<" castsWithInterruptedAttempt="<<casts<<" firstDamage="<<blocked.pulseDamage[0]<<" peakDamage="<<blocked.pulseDamage.back()<<std::endl;
    c->SetPower(POWER_ENERGY,100);c->AI()->Reset();MarinerWatch reset(map,*c);reset.Advance(3000);Require(reset.casts[228619].empty()&&reset.casts[228621].empty()&&!c->HasAura(228383)&&!c->HasAura(228619)&&!c->HasAura(228621)&&c->GetPower(POWER_ENERGY)==0,"Reset retained pending Lantern/energy/aura state");
    c->AI()->EnterCombat(&p);reset.Advance(1000);Require(c->HasAura(228383)&&c->GetPower(POWER_ENERGY)==(map.IsLfrRaid()?1:3),"Same Mariner retry did not restart energy profile");
}
static void MarinerCase(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    p.GetOrInitPetStable();GarmPlayer closePlayer(map,p.getRace()),distantPlayer(map,p.getRace());LootPet pet(p,map);pet.setFaction(p.getFaction());pet.SetMaxHealth(p.GetMaxHealth());pet.SetHealth(p.GetMaxHealth());
    auto c=MarinerBegin(p,map,helya);
#ifndef TRIAL_HELYA_MARINER_BASELINE
    Require(c->GetPowerType()==POWER_ENERGY&&c->GetMaxPower(POWER_ENERGY)==100,"Native Mariner resource type is not energy100");
#endif
    std::cout<<"MARINER_INITIAL powerType="<<c->GetPowerType()<<" maxEnergy="<<c->GetMaxPower(POWER_ENERGY)<<" darkness="<<c->HasAura(228383)<<std::endl;
    if(p.getRace()==RACE_HUMAN)for(auto id:{228383u,228619u,228621u,228611u,228632u,228633u,228631u}){auto s=sSpellMgr->GetSpellInfo(id);std::cout<<"MARINER_CATALOG diff="<<uint32(map.GetDifficultyID())<<" spell="<<id<<" cap="<<s->GetMaxAffectedTargets(map.GetDifficultyID())<<" duration="<<s->GetDuration(map.GetDifficultyID())<<" cast="<<s->CalcCastTime(c)<<std::endl;}
    map.PlayerRelocation(&p,470,640,7.5f,0);map.PlayerRelocation(&closePlayer,495,640,7.5f,0);map.PlayerRelocation(&distantPlayer,520,640,7.5f,0);pet.Relocate(460,640,7.5f,0);
    c->AddThreat(&p,100000);c->AddThreat(&closePlayer,1);c->AddThreat(&distantPlayer,1);c->AddThreat(&pet,1);
    MarinerWatch w(map,*c);
    for(unsigned sec=0;sec<110;++sec){w.Advance(1000);if(sec<36||sec%10==0)std::cout<<"MARINER_ENERGY t="<<sec+1<<" power="<<c->GetPower(POWER_ENERGY)<<" aura="<<c->HasAura(228619)<<std::endl;}
    for(auto id:{228633u,228632u,228631u,228611u,228619u,228621u}){std::cout<<"MARINER_CASTS spell="<<id<<" count="<<w.casts[id].size()<<" targets="<<w.targets[id].size()<<" times=";for(auto t:w.times[id])std::cout<<t<<',';std::cout<<std::endl;}
    std::cout<<"MARINER_PULSES ";for(auto damage:w.pulseDamage)std::cout<<damage<<',';std::cout<<std::endl;
#ifdef TRIAL_HELYA_MARINER_BASELINE
    Require(w.casts[228633].empty()&&!w.casts[228632].empty(),"Pinned Mariner bypassed-selector defect changed");
#else
    Require(!w.casts[228619].empty()&&!w.casts[228621].empty(),"Natural energy cycle never produced Lantern damage");
    unsigned cap=map.IsMythicRaid()?3:1;Require(w.casts[228633].size()>=3,"Native Give No Quarter selector did not repeat");
    for(auto const& cast:w.perCast[228633]){Require(cast.second.size()==cap,"Native Mariner lost one/three Player cap");for(auto guid:cast.second)Require(guid.IsPlayer(),"Native Mariner selector admitted Pet/NPC");}
    Require(w.times[228633].front()+40>=6000&&w.times[228633].front()<=6040,"Native selector initial cast timer wrong");
    auto const& times=w.times[228633];unsigned repeat=map.IsLfrRaid()||map.IsNormalRaid()?9300:6100;for(unsigned i=1;i<times.size();++i)Require(times[i]-times[i-1]>=repeat&&times[i]-times[i-1]<=repeat+8000,"Native Give No Quarter cadence wrong");
#endif
    c=MarinerBegin(p,map,helya);map.PlayerRelocation(&p,500,640,7.5f,0);map.PlayerRelocation(&closePlayer,505,640,7.5f,0);map.PlayerRelocation(&distantPlayer,520,640,7.5f,0);closePlayer.SetGameMaster(true);distantPlayer.SetGameMaster(true);
    MarinerWatch marker(map,*c);c->CastSpell(c,228633,false);marker.Advance(1200,false);
#ifndef TRIAL_HELYA_MARINER_BASELINE
    Require(marker.targets[228633]==std::set<ObjectGuid>{p.GetGUID()}&&marker.destinations[228632].size()==1,"Native marker failed one-player admission or missile dispatch");
    Require(marker.destinations[228632][0].GetExactDist(500,640,7.5f)<0.1f,"Missile destination collapsed to caster instead of marked Player");
#endif
    map.PlayerRelocation(&p,530,640,7.5f,0);closePlayer.SetGameMaster(false);marker.Advance(5000,false);
#ifndef TRIAL_HELYA_MARINER_BASELINE
    Require(marker.damagedTargets[228631]==std::set<ObjectGuid>{closePlayer.GetGUID()},"Native fixed marker did not hit nearby Player or followed moved target/exceeded radius");
#endif
    std::cout<<"MARINER_MARKER destinations="<<marker.destinations[228632].size()<<" damageTargets="<<marker.damagedTargets[228631].size()<<std::endl;
#ifndef TRIAL_HELYA_MARINER_BASELINE
    MarinerLantern(p,map,helya);
#endif
    ClearCombat(p,helya);PumpHelya(map,3000);std::cout<<"PASS: native Mariner observation diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);try{return RunGarm(argc,argv,MarinerCase);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
