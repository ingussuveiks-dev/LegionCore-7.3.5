#include <cassert>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
using ObjectGuid=uint64;
#include "Functions.inc"
#include "DeathState.inc"
enum{TYPEID_PLAYER=1,TYPEID_UNIT=2,POINT_MOTION_TYPE=8,WAYPOINT_MOTION_TYPE=2,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct Player;struct Creature;struct CreatureAI;
struct Unit {virtual~Unit()=default;virtual uint32 GetTypeId(){return TYPEID_UNIT;} };
struct Player:Unit {std::vector<uint32>credits;uint32 GetTypeId()override{return TYPEID_PLAYER;}void UpdateAchievementCriteria(uint32 type,uint32 id){assert(type==92);credits.push_back(id);} };
struct GameObject {ObjectGuid GetGUID(){return 234;} };
struct Instance {uint32 opens=0;void HandleGameObject(ObjectGuid guid,bool open){assert(guid==234&&open);++opens;} };
using InstanceScript=Instance;
struct Motion {std::vector<uint32>points,paths;void MovePoint(uint32 id,float,float,float){points.push_back(id);}void MovePath(uint32 id,bool repeat){assert(id==9859004&&!repeat);paths.push_back(id);} };
struct Creature:Unit {
 CreatureAI*i_AI=nullptr,*i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;DeathState state=ALIVE;bool inRange=true;
 FunctionProcessor functions;Motion motion;std::vector<uint8>talks;std::vector<uint32>casts;std::vector<Creature*>adds;Creature*target=nullptr;Player*nearest=nullptr;GameObject*door=nullptr;Instance*instance=nullptr;
 uint32 attacks=0,kills=0,despawns=0,homes=0;std::function<void()>onTalk;
 ~Creature();CreatureAI*AI(){return i_AI;}bool AIM_Destroy();void Lifecycle(uint32);DeathState getDeathState(){return state;}
 void SetHomePosition(float,float,float,float){++homes;}float GetPositionX(){return 1;}float GetPositionY(){return 2;}float GetPositionZ(){return 3;}float GetOrientation(){return 4;}
 Creature*FindNearestCreature(uint32 id,float radius,bool alive){assert(alive);assert((id==94714&&radius==85)||(id==96443&&radius==70));return target;}
 Player*FindNearestPlayer(float radius){assert(radius==100);return nearest;}GameObject*FindNearestGameObject(uint32 id,float r){assert(id==243522&&r==30);return door;}Instance*GetInstanceScript(){return instance;}
 bool InSamePhase(Creature*) const{return true;}
 bool IsWithinDistInMap(Unit*,float radius){assert(radius==20);return inRange;}Motion*GetMotionMaster(){return &motion;}
 void Kill(Creature*p){assert(p==this);++kills;}void DespawnOrUnsummon(uint32 delay){assert(delay==1000);++despawns;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&&f){functions.AddDelayedEvent(delay,std::move(f));}
};
struct CreatureAI {
 Creature*me;explicit CreatureAI(Creature*p):me(p){}virtual~CreatureAI()=default;virtual void UpdateLifecycle(uint32){}virtual void EnterEvadeMode(){}virtual void MovementInform(uint32,uint32){}virtual void MoveInLineOfSight(Unit*){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
 void Talk(uint8,ObjectGuid={});void AttackStart(Creature*p){assert(p);++me->attacks;}void DoCast(uint32 id){assert(id==193571);me->casts.push_back(id);}
};
Creature::~Creature(){if(i_disabledAI!=i_AI)delete i_disabledAI;delete i_AI;}
struct TextEndpoint {void SendChat(Creature*p,uint8 id,ObjectGuid={}){p->talks.push_back(id);if(p->onTalk)p->onTalk();} } textEndpoint;auto*sCreatureTextMgr=&textEndpoint;
#include "Talk.inc"
#define ASSERT(condition,...) assert(condition)
#define TC_LOG_DEBUG(...) ((void)0)
uint32 diagnostics=0;
#define TC_LOG_ERROR(...) (++diagnostics)
#include "AIDestroy.inc"
void Creature::Lifecycle(uint32 diff){
#include "Lifecycle.inc"
}
void GetCreatureListWithEntryInGrid(std::list<Creature*>&out,Creature*p,uint32 id,float r){assert(id==96571&&r==50);out.assign(p->adds.begin(),p->adds.end());}
struct Hruthnir:CreatureAI {
 using CreatureAI::CreatureAI;bool firstfighting=false,secondfighting=false,thirdfighting=false,fourfighting=false,_introDone=false;
// The independent real Pillik coordinator is exercised by the actual-method
// ASAN and native Kill suites. This fixture observes the six native clocks.
#include "PillikFields.inc"
 void QueuePillikHandoff(){} bool ArmPillikHandoff(){return true;} void TryPillikHandoff(){}
#include "Methods.inc"
};
struct Run {
 Creature actor,target,add;Player player;GameObject door;Instance instance;
 Run(){actor.i_AI=new Hruthnir(&actor);target.i_AI=new CreatureAI(&target);actor.target=&target;actor.nearest=&player;actor.door=&door;actor.instance=&instance;target.adds.push_back(&add);}
 Hruthnir*Content(){return static_cast<Hruthnir*>(actor.i_disabledAI?actor.i_disabledAI:actor.i_AI);}
 void Begin(uint32 path){if(path==0){Content()->firstfighting=true;Content()->EnterEvadeMode();}if(path==1)Content()->MovementInform(2,6);if(path==2)Content()->MovementInform(8,2);if(path==3)Content()->MovementInform(8,4);if(path==4)Content()->MovementInform(8,5);if(path==5)Content()->MoveInLineOfSight(&player);}
 void Tick(uint32 diff){actor.Lifecycle(diff);if(actor.state==ALIVE||actor.state==CORPSE)actor.functions.Update(diff);}
 void Done(uint32 path){if(path==0)assert(actor.talks==std::vector<uint8>({3,4})&&actor.motion.paths==std::vector<uint32>{9859004});if(path==1)assert(actor.talks==std::vector<uint8>{18}&&target.talks==std::vector<uint8>{5}&&actor.attacks==1&&Content()->secondfighting&&add.kills==1&&add.despawns==1&&player.credits==std::vector<uint32>{46288});if(path==2)assert(actor.talks==std::vector<uint8>({19,6})&&actor.attacks==1&&Content()->thirdfighting&&instance.opens==1);if(path==3)assert(actor.talks==std::vector<uint8>({7,8,9}));if(path==4)assert(actor.talks==std::vector<uint8>({12,13}));if(path==5)assert(actor.talks==std::vector<uint8>({0,1})&&actor.motion.points==std::vector<uint32>{1});}
};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);std::cout<<std::unitbuf;
#ifdef BASELINE
 for(uint32 path=0;path<6;++path){Run r;r.Begin(path);assert(r.actor.functions.SizeQueue()==1);assert(r.actor.AIM_Destroy());assert(r.actor.functions.SizeQueue()==1);}
 std::cout<<"PASS EXPECTED BROKEN e4a7e933a: six raw AI callbacks survive actual AIM_Destroy in Unit queue. Dangling callbacks deliberately not executed; no fabricated crash.\n";
#else
 uint32 cases=0;uint32 delays[]={5000,5000,4000,15000,2000,5000};
 for(uint32 path=0;path<6;++path){
  for(uint32 mode=0;mode<5;++mode){Run r;r.Begin(path);assert(!r.actor.functions.SizeQueue()&&r.Content()->_presentationFunctions.SizeQueue()==1);if(mode){r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=mode==1?new CreatureAI(&r.actor):mode==2?nullptr:r.actor.i_disabledAI;}if(mode==4)r.Tick(delays[path]);else {r.Tick(delays[path]-1);assert(r.actor.talks.size()==(path==2?0u:path==3?2u:1u));r.Tick(1);}r.Done(path);r.Tick(30000);r.Done(path);++cases;}
  for(uint32 elapsed:{0u,1u,999u}){Run r;r.Begin(path);r.Tick(elapsed);auto count=r.actor.talks.size();assert(r.actor.AIM_Destroy());r.Tick(30000);assert(r.actor.talks.size()==count&&r.target.talks.empty()&&r.actor.motion.paths.empty());r.actor.i_AI=new Hruthnir(&r.actor);r.Tick(30000);assert(r.actor.talks.size()==count);++cases;}
  for(auto state:{ALIVE,CORPSE,DEAD,JUST_DIED,JUST_RESPAWNED}){Run r;r.Begin(path);auto count=r.actor.talks.size();r.actor.state=state;r.Tick(delays[path]);if(state==ALIVE||state==CORPSE)r.Done(path);else {assert(r.actor.talks.size()==count);r.actor.state=ALIVE;r.Tick(delays[path]);r.Done(path);}++cases;}
  {Run r;r.Begin(path);r.actor.onTalk=[&]{assert(r.actor.m_AI_locked&&!r.actor.AIM_Destroy());};r.Tick(delays[path]);r.actor.onTalk={};r.Done(path);++cases;}
  {Run r;r.Begin(path);uint32 unrelated=0;r.actor.AddDelayedEvent(1,[&]{++unrelated;});assert(r.actor.AIM_Destroy());r.Tick(30000);assert(unrelated==1);++cases;}
 }
 for(bool ai:{false,true}){Run r;if(!ai){delete r.target.i_AI;r.target.i_AI=nullptr;}r.Begin(1);r.Tick(5000);r.Done(1);++cases;}
 for(uint32 path:{1u,2u}){Run r;r.actor.target=nullptr;r.Begin(path);r.Tick(15000);assert(!r.actor.attacks&&r.target.talks.empty()&&!r.add.kills);assert((path==1&&!r.Content()->secondfighting)||(path==2&&r.Content()->thirdfighting));++cases;}
 {Run r;r.actor.nearest=nullptr;r.Begin(1);r.Tick(5000);assert(r.actor.attacks==1&&r.player.credits.empty());++cases;}
 {Run r;r.actor.door=nullptr;r.Begin(2);r.Tick(4000);assert(!r.instance.opens&&r.actor.attacks==1);++cases;}
 {Run r;r.actor.instance=nullptr;r.Begin(2);r.Tick(4000);assert(!r.instance.opens&&r.actor.attacks==1);++cases;}
 {Run r;r.Content()->secondfighting=true;r.Content()->EnterEvadeMode();assert(r.actor.talks.empty()&&r.actor.motion.points.empty()&&r.player.credits.empty());r.Content()->secondfighting=false;r.Content()->thirdfighting=true;r.Content()->EnterEvadeMode();assert(r.actor.motion.points.back()==4&&!r.Content()->thirdfighting);r.Content()->fourfighting=true;r.Content()->EnterEvadeMode();assert(r.actor.motion.points.back()==5&&r.actor.talks.back()==11&&!r.Content()->fourfighting);++cases;}
 {Run r;Unit pet;r.Content()->MoveInLineOfSight(&pet);assert(r.actor.talks.empty());r.actor.inRange=false;r.Begin(5);assert(r.actor.talks.empty());r.actor.inRange=true;r.Begin(5);r.Begin(5);r.Tick(5000);r.Done(5);uint32 damage=0xffffffff;r.Content()->DamageTaken(&pet,damage,0);assert(!damage);++cases;}
 {Run r;r.Begin(1);r.Begin(1);assert(r.Content()->_presentationFunctions.SizeQueue()==2);assert(r.actor.AIM_Destroy());r.Tick(30000);assert(r.actor.attacks==0&&r.target.talks.empty());++cases;}
 assert(!diagnostics);std::cout<<"PASS "<<cases<<" ASAN contexts: six actual earlier Hruthnir callback/evade/movement/LOS bodies with real FunctionProcessor/typed DeathState/locked lifecycle/AIM_Destroy/native Talk forwarding. Exact5/5/4/15/2/5s, destruction/replacement/suspended/null/alias/late clocks, ALIVE-CORPSE versus DEAD/transient, original route/point/spell/text/flags/credit endpoints and absent actor/AI/door/instance cases retained. Later point6/7 branches covered separately by retained Gossip suite; world/combat/movement/credit endpoints controlled, no full909/client claim.\n";
#endif
}
