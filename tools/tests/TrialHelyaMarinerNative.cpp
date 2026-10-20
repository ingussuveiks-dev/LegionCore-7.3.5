#define TRIAL_HELYA_GRIMELORD_SUPPORT_ONLY
#include "TrialHelyaGrimelordNative.cpp"
struct MarinerWatch
{
    InstanceMap& map;ObjectGuid guid;uint32 elapsed=0;
    std::map<uint32,std::set<ObjectGuid>> casts,targets,damaged,damagedTargets;
    std::map<uint32,std::vector<uint32>> times;
    std::map<uint32,std::map<ObjectGuid,std::set<ObjectGuid>>> perCast;
    std::map<uint32,std::vector<Position>> destinations;
    std::vector<int32> pulseDamage;
    explicit MarinerWatch(InstanceMap& m,Creature& c):map(m),guid(c.GetGUID()){}
    void Observe(){if(auto c=map.GetCreature(guid))for(auto id:{228633u,228632u,228631u,228611u,228619u,228621u})for(auto s:NativeSpellEvents(*c,id)){
        if(casts[id].insert(s->m_castGuid[0]).second){times[id].push_back(elapsed);if(s->m_targets.HasDst())destinations[id].push_back(*s->m_targets.GetDstPos());}
        for(auto const& t:*s->GetUniqueTargetInfo())if(t->effectMask&1){targets[id].insert(t->targetGUID);perCast[id][s->m_castGuid[0]].insert(t->targetGUID);if(t->processed&&t->damage>0){damagedTargets[id].insert(t->targetGUID);if(damaged[id].insert(s->m_castGuid[0]).second&&id==228621)pulseDamage.push_back(t->damage);}}
    }}
    void Advance(uint32 ms,bool ai=true){Observe();for(unsigned t=0;t<ms;t+=10){if(ai)PumpHelya(map,10);else if(auto c=map.GetCreature(guid))c->Unit::Update(10);Observe();for(auto const& ref:map.GetPlayers()){auto p=ref.getSource();if(p->IsAlive())p->SetHealth(p->GetMaxHealth());p->Unit::Update(10);}Observe();elapsed+=10;}}
};
static Creature* MarinerBegin(GarmPlayer& p,InstanceMap& map,Creature& helya)
{
    auto grim=GrimBegin(p,map,helya);grim->DespawnOrUnsummon();PumpHelya(map,20);
    auto c=helya.SummonCreature(114809,458,639,7.5f,0,TEMPSUMMON_MANUAL_DESPAWN);Require(c&&c->AI(),"Native Mariner summon missing");PumpHelya(map,20);return c;
}
static void MarinerCase(GarmPlayer& p,InstanceMap& map,Creature& helya,Group& group)
{
    p.GetOrInitPetStable();GarmPlayer closePlayer(map,p.getRace()),distantPlayer(map,p.getRace());LootPet pet(p,map);pet.setFaction(p.getFaction());pet.SetMaxHealth(p.GetMaxHealth());pet.SetHealth(p.GetMaxHealth());
    auto c=MarinerBegin(p,map,helya);
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
    ClearCombat(p,helya);PumpHelya(map,3000);std::cout<<"PASS: native Mariner observation diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(p.getRace())<<std::endl;
}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);try{return RunGarm(argc,argv,MarinerCase);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
