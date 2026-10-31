#include <cassert>
#include <cstdint>
#include <map>
#include <list>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <chrono>
#include <iostream>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
using Milliseconds=std::chrono::milliseconds;
uint32 urand(uint32 low,uint32){return low;}int32 irand(int32 low,int32){return low;}
#include "EventMap.inc"
#include "Functions.inc"
enum{TYPEID_UNIT,TYPEID_PLAYER,UNIT_STATE_STUNNED=8,UNIT_STATE_CASTING=16,POINT_MOTION_TYPE=1,REACT_AGGRESSIVE=2,UNIT_FIELD_NPC_FLAGS=3,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct Creature;struct CreatureAI;
struct Unit{virtual ~Unit()=default;virtual uint32 GetTypeId()const{return TYPEID_UNIT;}};
struct Player:Unit{uint32 GetTypeId()const override{return TYPEID_PLAYER;}void UpdateAchievementCriteria(uint32,uint32){assert(false);}};
struct GameObject{uint32 phases=0;void SetPhaseMask(uint32 mask,bool update){assert(mask==2&&update);++phases;}};
struct Motion{std::vector<uint32>points;void MovePoint(uint32 id,float x,float y,float z){
 if(id==2)assert(x==-1017.80f&&y==-3048.89f&&z==12.82f);
 else if(id==3)assert(x==-1018.74f&&y==-2994.68f&&z==12.30f);
 else if(id==4)assert(x==-1017.72f&&y==-2933.42f&&z==19.48f);
 else assert(false);points.push_back(id);}};
struct Creature:Unit{
 CreatureAI*i_AI=nullptr,*i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true,near=true,victim=false;
 uint32 states=0;FunctionProcessor functions;Motion motion;std::vector<GameObject*>walls;GameObject*door=nullptr;
 std::vector<uint32>talks,casts;std::function<void(uint32)>onTalk;
 bool AIM_Destroy();void Lifecycle(uint32 diff);CreatureAI*AI(){return i_AI;}
 void SetReactState(uint32 state){assert(state==REACT_AGGRESSIVE);}void SetWalk(bool walk){assert(!walk);}
 bool IsWithinDistInMap(Unit*,float distance){assert(distance==20.f);return near;}bool HasUnitState(uint32 state){return (states&state)!=0;}
 bool HasAura(uint32 id){assert(id==204779);return false;}void SetHomePosition(float,float,float,float){}
 float GetPositionX()const{return 0;}float GetPositionY()const{return 0;}float GetPositionZ()const{return 0;}float GetOrientation()const{return 0;}
 Motion*GetMotionMaster(){return &motion;}void SetUInt32Value(uint32 field,uint32 value){assert(field==UNIT_FIELD_NPC_FLAGS&&value==1);}
 Creature*FindNearestCreature(uint32,float,bool){return nullptr;}Player*FindNearestPlayer(float){return nullptr;}
 GameObject*FindNearestGameObject(uint32 id,float distance){assert(id==246395&&distance==50.f);return door;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&&fn){functions.AddDelayedEvent(delay,std::move(fn));}
};
void GetGameObjectListWithEntryInGrid(std::list<GameObject*>&out,Creature*actor,uint32 id,float range){assert(id==246321&&range==35.f);out.assign(actor->walls.begin(),actor->walls.end());}
struct CreatureAI{
 Creature*me;bool m_MoveInLineOfSight_locked=false;explicit CreatureAI(Creature*actor):me(actor){}virtual~CreatureAI()=default;
 virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void DoAction(int32){}virtual void DamageDealt(Unit*,uint32&,DamageEffectType){}
 virtual void SetData(uint32,uint32){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}virtual void MovementInform(uint32,uint32){}
 virtual void MoveInLineOfSight(Unit*){}virtual void UpdateAI(uint32){}virtual void UpdateLifecycle(uint32){}
 virtual void sGossipSelect(Player*,uint32,uint32){}
 virtual void Talk(uint32 id){me->talks.push_back(id);if(me->onTalk)me->onTalk(id);}
 void MoveInLineOfSight_Safe(Unit*);
};
void Creature::Lifecycle(uint32 diff){
#include "Lifecycle.inc"
}
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(condition,...) assert(condition)
#include "AIDestroy.inc"
#include "SafeLOS.inc"
struct ScriptedAI:CreatureAI{using CreatureAI::CreatureAI;bool UpdateVictim(){return me->victim;}void DoCast(uint32 id){me->casts.push_back(id);}void DoMeleeAttackIfReady(){}};
// Current plan admission/refresh is exercised by its separate native suite.
namespace SheilunPlan {void Refresh(Creature*){} void Start(Player*,Creature*,uint32,uint32){}}
#include "Actor.inc"
using Taran=npc_tazhan_chuAI;
struct Run{
 Creature actor;Player player;Unit pet;CreatureAI temporary{&actor};GameObject a,b;
 Run(){actor.i_AI=new Taran(&actor);actor.walls={&a,&b};}
 ~Run(){if(actor.i_disabledAI&&actor.i_disabledAI!=actor.i_AI)delete actor.i_disabledAI;if(actor.i_AI!=&temporary)delete actor.i_AI;}
 Taran*Content(){return static_cast<Taran*>(actor.i_disabledAI?actor.i_disabledAI:actor.i_AI);}
 void Begin(bool wall){if(wall)Content()->MovementInform(POINT_MOTION_TYPE,1);else Content()->MoveInLineOfSight_Safe(&player);}
 void Tick(uint32 diff,bool update=true){actor.Lifecycle(diff);actor.functions.Update(diff);if(update&&actor.i_AI)actor.i_AI->UpdateAI(diff);}
 void Suspend(uint32 mode){auto content=actor.i_AI;actor.i_disabledAI=content;actor.i_AI=mode==0?&temporary:mode==1?nullptr:content;}
 void Verify(bool wall){if(wall){assert(actor.talks==std::vector<uint32>({3,4}));assert(actor.casts==std::vector<uint32>{202053});assert(actor.motion.points==std::vector<uint32>{2});assert(a.phases==1&&b.phases==1);}else{assert(actor.talks==std::vector<uint32>({0,1}));assert(actor.casts.empty()&&actor.motion.points.empty()&&!a.phases&&!b.phases);}}
};
int main(){
#ifdef BASELINE
 for(bool wall:{false,true}){Run r;r.Begin(wall);assert(r.actor.functions.SizeQueue()==1);assert(r.actor.AIM_Destroy());assert(r.actor.functions.SizeQueue()==1);/* Prove the escaped capture; do not invoke the dangling pointer. */}
 std::cout<<"PASS EXPECTED BROKEN d52af1bc7 counterfactual: both actual4s/5s captures remain in the native Creature/Unit queue after native AIM_Destroy deletes their AI. Dangling callbacks deliberately not invoked; no simulated crash or client proof.\n";
#else
 unsigned cases=0;
 for(bool wall:{false,true}){
  uint32 delay=wall?4000:5000;
  for(uint32 mode=0;mode<8;++mode){Run r;r.Begin(wall);assert(r.actor.functions.SizeQueue()==0&&r.Content()->_presentationFunctions.SizeQueue()==1);
   if(mode==1)r.actor.states=UNIT_STATE_STUNNED;if(mode==2)r.actor.states=UNIT_STATE_CASTING;
   if(mode>=3&&mode<=5)r.Suspend(mode-3);if(mode==6)r.Content()->Reset();if(mode==7)r.actor.IsAIEnabled=false;
   r.Tick(delay-1);assert(r.actor.talks.size()==1);r.Tick(1);r.Verify(wall);r.Tick(10000);r.Verify(wall);++cases;
  }
  for(uint32 elapsed:{0u,1u,2000u,3999u}){Run r;r.Begin(wall);r.Tick(elapsed,false);assert(r.actor.talks.size()==1);assert(r.actor.AIM_Destroy());r.actor.functions.Update(10000);assert(r.actor.talks.size()==1&&r.actor.functions.Empty());r.actor.i_AI=new Taran(&r.actor);r.Tick(10000);assert(r.actor.talks.size()==1);r.actor.talks.clear();r.actor.casts.clear();r.Begin(wall);r.Tick(delay);r.Verify(wall);++cases;}
  for(uint32 suspended=0;suspended<3;++suspended){Run r;r.Begin(wall);r.Suspend(suspended);r.Tick(1);auto content=r.Content();r.actor.i_AI=content;r.actor.i_disabledAI=nullptr;/* Restore the content AI before native destruction, as required by its real assertion. */assert(r.actor.AIM_Destroy());r.Tick(10000,false);assert(r.actor.talks.size()==1);++cases;}
  {Run r;r.Begin(wall);r.actor.m_AI_locked=true;assert(!r.actor.AIM_Destroy());r.actor.m_AI_locked=false;r.Tick(delay);r.Verify(wall);++cases;}
  {Run r;r.Begin(wall);uint32 unrelated=0;r.actor.AddDelayedEvent(delay,[&]{++unrelated;});assert(r.actor.AIM_Destroy());r.Tick(delay,false);assert(unrelated==1&&r.actor.talks.size()==1);++cases;}
  {Run r;r.Begin(wall);r.actor.onTalk=[&](uint32 id){if(id==(wall?4u:1u))r.Content()->Reset();};r.Tick(delay);r.Verify(wall);r.Tick(delay);r.Verify(wall);++cases;}
 }
 // Old rescue queue remains paused by the original stunned UpdateAI guard.
 {Run r;r.Content()->SetData(1,1);r.actor.states=UNIT_STATE_STUNNED;r.Begin(false);r.Tick(5000);assert(r.actor.talks==std::vector<uint32>({0,1})&&r.actor.motion.points.empty()&&r.Content()->_functions.SizeQueue()==1);r.actor.states=0;r.Tick(999);assert(r.actor.motion.points.empty());r.Tick(1);assert(r.actor.talks==std::vector<uint32>({0,1,8})&&r.actor.motion.points==std::vector<uint32>{3});++cases;}
 {Run r;r.Content()->MoveInLineOfSight_Safe(&r.pet);assert(r.actor.talks.empty());r.actor.near=false;r.Begin(false);assert(r.actor.talks.empty());r.actor.near=true;r.Begin(false);r.Begin(false);r.Tick(5000);r.Verify(false);++cases;}
 {Run r;r.actor.walls.clear();r.Begin(true);r.Tick(4000);assert(r.actor.motion.points==std::vector<uint32>{2}&&!r.a.phases&&!r.b.phases);++cases;}
 // Both deadlines share one lifecycle clock; UpdateAI must not double it.
 {Run r;r.Begin(false);r.Begin(true);r.Tick(3999);assert(r.actor.talks==std::vector<uint32>({0,3}));r.Tick(1);assert(r.actor.talks==std::vector<uint32>({0,3,4}));r.Tick(999);assert(r.actor.talks.size()==3);r.Tick(1);assert(r.actor.talks==std::vector<uint32>({0,3,4,1}));++cases;}
 {Run r;r.Begin(false);r.actor.onTalk=[&](uint32 id){if(id==1)r.Content()->MovementInform(POINT_MOTION_TYPE,1);};r.Tick(5000);assert(r.actor.talks==std::vector<uint32>({0,1,3}));r.Tick(3999);assert(r.actor.motion.points.empty());r.Tick(1);assert(r.actor.motion.points==std::vector<uint32>{2});++cases;}
 {Run r;r.Content()->MovementInform(99,1);assert(r.actor.casts.empty());r.Content()->MovementInform(POINT_MOTION_TYPE,2);assert(r.actor.talks==std::vector<uint32>{5});++cases;}
 std::cout<<"PASS "<<cases<<" actual full Taran constructor/reset/LOS/movement/rescue/combat methods + native FunctionProcessor/EventMap/SafeLOS/current-and-suspended lifecycle/AIM_Destroy under ASAN: exact4s/5s, stunned/casting/suspended/alias/reset clocks, deletion/replacement, locked destruction, independent Unit event retention, reentrant reset/enqueue, missing walls, nonplayer/range/once admission, unchanged stunned rescue clock. Opening/wound endpoints controlled and independently tested; no client, successful barrier cast or full story claim.\n";
#endif
}
