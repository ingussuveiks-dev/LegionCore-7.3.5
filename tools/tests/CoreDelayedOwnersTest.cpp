// Full production helpers/action block and actual FunctionProcessor engine.
// Actor, movement, summon and instance endpoints are controlled ASAN adapters.
#include <atomic>
#include <mutex>
#include <memory>
#include <vector>
#include <cstdint>
#include <iostream>
#include <string>
#include <stdexcept>
#include "FunctionProcessorActual.inc"
enum TempSummonType { TEMPSUMMON_TIMED_DESPAWN=1 };
enum { SPELL_AURA_CONTROL_VEHICLE=1,FAIL=2,IN_PROGRESS=3,ENCOUNTER_FRAME_ENGAGE=4,ENCOUNTER_FRAME_INSTANCE_START=5,ENCOUNTER_FRAME_SET_ALLOWING_RELEASE=6,CREATURE_SUMMON_GROUP_COMBAT=7,UNIT_FIELD_FLAGS_2=8,UNIT_FLAG2_FEIGN_DEATH=16,MAX_DB_MOTION_TYPE=3,WAYPOINT_MOTION_TYPE=2,SMART_ACTION_SET_REACT_STATE=99 };
enum ReactStates { REACT_PASSIVE,REACT_DEFENSIVE,REACT_AGGRESSIVE };
#define TC_LOG_DEBUG(...) ((void)0)
struct Position{float x=1,y=2,z=3,o=4;};
static uint32 chosenRadius=30;
uint32 urand(uint32 a,uint32 b){if(chosenRadius<a||chosenRadius>b)throw std::runtime_error("radius bounds changed");return chosenRadius;}
struct Creature{
 FunctionProcessor event,combat;
 ReactStates react=REACT_DEFENSIVE;uint32 restores=0,summons=0,groups=0,stops=0,moves=0,radius=0,movement=0,flags=0;bool fighting=true,active=false,failSummon=false;
 uint32 entry=299,lastEntry=0,despawn=0;Position pose;TempSummonType kind=TEMPSUMMON_TIMED_DESPAWN;
 struct Motion{Creature*actor;void MoveRandom(uint32 r){++actor->moves;actor->radius=r;}}motion{this};
 void AddDelayedEvent(uint64 ms,std::function<void()>&&f){event.AddDelayedEvent(ms,std::move(f));}
 void AddDelayedCombat(uint64 ms,std::function<void()>&&f){combat.AddDelayedEvent(ms,std::move(f));}
 Creature* SummonCreature(uint32 id,Position const&p,TempSummonType k,uint32 d){++summons;lastEntry=id;pose=p;kind=k;despawn=d;return failSummon?nullptr:this;}
 ReactStates GetReactState(){return react;}void SetReactState(ReactStates s){react=s;++restores;}
 bool isInCombat(){return fighting;}void AttackStop(){++stops;}
 uint32 GetGUIDLow(){return 1;}uint32 GetEntry(){return entry;}
 void setActive(bool b){active=b;}void SummonCreatureGroup(uint32){++groups;}
 bool HasFlag(uint32,uint32 flag){return(flags&flag)!=0;}
 uint32 GetDefaultMovementType(){return movement;}Motion*GetMotionMaster(){return &motion;}
};
struct ScriptedAI{
 Creature*me;explicit ScriptedAI(Creature*c):me(c){}virtual~ScriptedAI()=default;
 void SummonCreatureDelay(uint32,uint32,Position const&,TempSummonType,uint32);
};
struct Instance{
 bool allowed=false;uint32 checks=0,repop=0,states=0,frames=0;
 bool CheckRequiredBosses(uint32,uint32){++checks;return allowed;}
 void RepopPlayersAtGraveyard(){++repop;}
 void SetBossState(uint32,uint32){++states;}
 template<class...T>void SendEncounterUnit(T...){++frames;}
};
struct BossAI:ScriptedAI{
 Instance*instance;uint32 _bossId=1,evades=0,zones=0,tasks=0;
 BossAI(Creature*c,Instance*i):ScriptedAI(c),instance(i){}
 void EnterEvadeMode(){++evades;}
 void AddDelayedEvent(uint64 ms,std::function<void()>&&f){me->AddDelayedEvent(ms,std::move(f));}
 void DoZoneInCombat(){++zones;}void ScheduleTasks(){++tasks;}void _EnterCombat();
};
struct CritterAI:ScriptedAI{using ScriptedAI::ScriptedAI;void InitializeAI();};
struct SmartScript{
 Creature*me;explicit SmartScript(Creature*c):me(c){}
 struct Event{struct Action{struct React{uint32 state=0,returnStateTimer=123,stopAttack=1;}react;}action;}e;
 void Apply(){switch(SMART_ACTION_SET_REACT_STATE){
#include "React.inc"
 }}
};
#include "Summon.inc"
#include "Critter.inc"
#include "Boss.inc"
static void Require(bool ok,char const* text){if(!ok)throw std::runtime_error(text);}
static void Run(std::string const& key,bool retire,bool cancel)
{
 Creature actor;Instance instance,other;Position pos;uint32 wait=key=="critter"?2000:key=="boss"?200:123;
 if(key=="summon"){
  auto ai=std::make_unique<ScriptedAI>(&actor);ai->SummonCreatureDelay(wait,1234,pos,TEMPSUMMON_TIMED_DESPAWN,5000);pos.x=999;
  if(retire)ai.reset();if(cancel)actor.combat.KillAllFunctions();actor.combat.Update(wait-1);Require(!actor.summons,"summon deadline changed");actor.combat.Update(1);
  Require(actor.summons==(cancel?0u:1u),"summon request lost or repeated");if(!cancel)Require(actor.lastEntry==1234&&actor.pose.x==1&&actor.pose.o==4&&actor.despawn==5000&&actor.kind==TEMPSUMMON_TIMED_DESPAWN,"summon arguments changed");actor.combat.Update(60000);Require(actor.summons==(cancel?0u:1u),"summon repeated");
 }else if(key=="react"){
  auto script=std::make_unique<SmartScript>(&actor);script->Apply();Require(actor.react==REACT_PASSIVE&&actor.stops==1,"initial SmartAI action changed");
  if(retire)script.reset();if(cancel)actor.event.KillAllFunctions();actor.event.Update(wait-1);Require(actor.react==REACT_PASSIVE,"react deadline changed");actor.event.Update(1);Require(actor.react==(cancel?REACT_PASSIVE:REACT_DEFENSIVE),"react restoration lost");actor.event.Update(60000);Require(actor.restores==(cancel?1u:2u),"react restoration repeated");
 }else if(key=="critter"){
  auto ai=std::make_unique<CritterAI>(&actor);ai->InitializeAI();if(retire)ai.reset();if(cancel)actor.event.KillAllFunctions();actor.event.Update(wait-1);Require(!actor.moves,"critter deadline changed");actor.event.Update(1);Require(actor.moves==(cancel?0u:1u),"critter movement lost");if(!cancel)Require(actor.radius==chosenRadius,"random bounds changed");actor.event.Update(60000);Require(actor.moves==(cancel?0u:1u),"critter movement repeated");
 }else if(key=="boss"){
  auto ai=std::make_unique<BossAI>(&actor,&instance);ai->_EnterCombat();Require(instance.checks==1&&ai->evades==1&&!actor.active&&!actor.groups,"rejected combat prerequisites changed");if(retire)ai.reset();if(cancel)actor.event.KillAllFunctions();actor.event.Update(wait-1);Require(!instance.repop,"repop deadline changed");actor.event.Update(1);Require(instance.repop==(cancel?0u:1u)&&!other.repop,"original instance request lost");actor.event.Update(60000);Require(instance.repop==(cancel?0u:1u),"repop repeated");
 }else throw std::runtime_error("unknown helper");
}
static void Alternatives()
{
 for(uint32 mode:{0u,1u,2u,3u,99u})for(uint32 radius:{30u,50u}){Creature actor;actor.movement=mode;chosenRadius=radius;CritterAI ai(&actor);ai.InitializeAI();actor.event.Update(2000);Require(actor.moves==(mode<MAX_DB_MOTION_TYPE&&mode!=WAYPOINT_MOTION_TYPE?1u:0u),"native movement admission changed");}
 {Creature actor;actor.flags=UNIT_FLAG2_FEIGN_DEATH;CritterAI ai(&actor);ai.InitializeAI();actor.event.Update(60000);Require(!actor.moves,"feign-death admission changed");}
 {Creature actor;Instance instance;instance.allowed=true;BossAI ai(&actor,&instance);ai._EnterCombat();actor.event.Update(1000);Require(actor.active&&actor.groups==1&&instance.states==1&&instance.frames==3&&ai.zones==1&&ai.tasks==1&&!instance.repop,"allowed boss branch changed");}
 {Creature actor;BossAI ai(&actor,nullptr);ai._EnterCombat();actor.event.Update(1000);Require(actor.groups==1&&!actor.active,"missing instance branch changed");}
 {Creature actor;actor.failSummon=true;ScriptedAI ai(&actor);ai.SummonCreatureDelay(1,1234,Position{},TEMPSUMMON_TIMED_DESPAWN,5000);actor.combat.Update(1);actor.combat.Update(1000);Require(actor.summons==1,"failed summon introduced duplicate/retry");}
 {Creature actor;SmartScript script(&actor);script.e.action.react.returnStateTimer=0;script.Apply();actor.event.Update(1000);Require(actor.react==REACT_PASSIVE&&actor.restores==1,"timer0 restoration introduced");}
}
int main(int argc,char**argv){try{Require(argc==2,"case required");std::string key=argv[1];Run(key,true,false);
#ifndef COUNTERFACTUAL
 Run(key,false,false);Run(key,true,true);Alternatives();
#endif
 std::cout<<"PASS complete core delayed owner "<<key<<"; actual queue,retired/live/cancelled and authored deadlines/arguments; controlled actor endpoints\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
