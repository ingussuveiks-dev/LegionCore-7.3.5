#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>
#include <iostream>
using uint32=uint32_t;using uint64=uint64_t;using DamageEffectType=uint32;
#include "Functions.inc"
enum{TYPEID_UNIT,TYPEID_PLAYER,UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK=1,POINT_MOTION_TYPE=2,UNIT_STATE_STUNNED=4,UNIT_STATE_CASTING=8,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct ObjectGuid{uint64 low=0,high=0;bool operator==(ObjectGuid const& other)const{return low==other.low&&high==other.high;}};
struct Unit{virtual ~Unit()=default;virtual uint32 GetTypeId()const{return TYPEID_UNIT;}};
struct Player:Unit{ObjectGuid guid{17,0x1234567800000011};std::vector<uint32> credits;uint32 GetTypeId()const override{return TYPEID_PLAYER;}ObjectGuid GetGUID()const{return guid;}void UpdateAchievementCriteria(uint32 type,uint32 id){assert(type==92);credits.push_back(id);}};
struct Motion{uint32 jumps=0,points=0;void MoveJump(float,float,float,float,float,float){++jumps;}void MovePoint(uint32 id,float,float,float){assert(id==1);++points;}};
struct CreatureAI;
struct Creature:Unit{
 CreatureAI* i_AI=nullptr;CreatureAI* i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;
 FunctionProcessor functions;uint32 states=0,flags=UNIT_NPC_FLAG_SPELLCLICK,despawns=0,melee=0;bool near=true,victim=false;float y=1700;Motion motion;Player* nearest=nullptr;
 std::vector<std::pair<uint32,ObjectGuid>> talks;std::vector<uint32> casts;std::function<void()> onTalk;
 CreatureAI* AI(){return i_AI;}bool AIM_Destroy();void Lifecycle(uint32 diff);
 void RemoveFlag(uint32,uint32 flag){flags&=~flag;}void SetFlag(uint32,uint32 flag){flags|=flag;}
 void SetHomePosition(float,float,float,float){}float GetPositionX()const{return 1;}float GetPositionY()const{return y;}float GetPositionZ()const{return 1;}float GetOrientation()const{return 0;}
 bool IsWithinDistInMap(Unit*,float)const{return near;}bool HasUnitState(uint32 flag)const{return (states&flag)!=0;}
 Motion* GetMotionMaster(){return &motion;}Player* FindNearestPlayer(float){return nearest;}void DespawnOrUnsummon(uint32 delay){assert(delay==3000);++despawns;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& f){functions.AddDelayedEvent(delay,std::move(f));}
};
struct CreatureAI{
 Creature* me;explicit CreatureAI(Creature* actor):me(actor){}virtual ~CreatureAI()=default;virtual void Reset(){}virtual void UpdateLifecycle(uint32){}virtual void UpdateAI(uint32){}virtual void MovementInform(uint32,uint32){}virtual void MoveInLineOfSight(Unit*){}virtual void SetData(uint32,uint32){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
 void Talk(uint32 id,ObjectGuid guid={}){me->talks.emplace_back(id,guid);if(me->onTalk)me->onTalk();}
};
void Creature::Lifecycle(uint32 diff){
#include "Lifecycle.inc"
}
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(condition,...) assert(condition)
#include "AIDestroy.inc"
struct ScriptedAI:CreatureAI{using CreatureAI::CreatureAI;bool UpdateVictim(){return me->victim;}void DoCast(uint32 id){me->casts.push_back(id);}void DoMeleeAttackIfReady(){++me->melee;}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual CreatureAI* GetAI(Creature*)const=0;};
#include "Actor.inc"
using Nine=npc_nine_czi::npc_nine_cziAI;
struct Run{
 Creature actor;Player player;CreatureAI temporary{&actor};
 Run(){actor.i_AI=new Nine(&actor);actor.nearest=&player;}
 ~Run(){if(actor.i_disabledAI&&actor.i_disabledAI!=actor.i_AI)delete actor.i_disabledAI;actor.i_disabledAI=nullptr;if(actor.i_AI!=&temporary)delete actor.i_AI;}
 Nine* Content(){return static_cast<Nine*>(actor.i_disabledAI?actor.i_disabledAI:actor.i_AI);}
 void Begin(uint32 path){if(path==0)Content()->MovementInform(POINT_MOTION_TYPE,1);else if(path==1)Content()->MoveInLineOfSight(&player);else Content()->SetData(1,1);}
 void Tick(uint32 diff,bool combat=true){actor.Lifecycle(diff);actor.functions.Update(diff);if(combat&&actor.i_AI)actor.i_AI->UpdateAI(diff);}
 void Suspend(bool current=true,bool alias=false){auto old=actor.i_AI;actor.i_disabledAI=old;actor.i_AI=alias?old:current?&temporary:nullptr;}
};
void AssertFinished(Run const& r,uint32 path){
 if(path==0){assert(r.actor.talks.size()==2&&r.actor.talks[0].first==5&&r.actor.talks[1].first==6&&r.actor.talks[1].second==r.player.guid);assert(r.actor.flags&1);assert(r.actor.casts==std::vector<uint32>{193721});assert(r.player.credits.empty()&&!r.actor.motion.points&&!r.actor.despawns);}
 if(path==1){assert(r.actor.talks.size()==2&&r.actor.talks[0].first==7&&r.actor.talks[1].first==8&&r.actor.talks[1].second==r.player.guid);assert(r.player.credits==std::vector<uint32>{46902});assert(r.actor.despawns==1&&r.actor.casts.empty()&&!r.actor.motion.points);}
 if(path==2){assert(r.actor.talks.size()==2&&r.actor.talks[0].first==3&&r.actor.talks[1].first==4);assert(r.player.credits==std::vector<uint32>{46734});assert(r.actor.motion.jumps==1&&r.actor.motion.points==1);assert(r.actor.casts==std::vector<uint32>{193248});assert(!r.actor.despawns);}
}
int main(){
#ifdef BASELINE
 for(uint32 path=0;path<3;++path){Run r;r.Begin(path);assert(r.actor.functions.SizeQueue()==(path==2?2u:1u));assert(r.actor.AIM_Destroy());assert(r.actor.functions.SizeQueue()==(path==2?2u:1u));}
 std::cout<<"PASS EXPECTED BROKEN aa1513802 counterfactual: four actual Nine Czi callbacks survive native AIM_Destroy in Unit queue with dangling AI captures; departure also captures Player*. Dangling callbacks deliberately not invoked; no runtime crash or client claim.\n";
#else
 uint32 cases=0;
 for(uint32 path=0;path<3;++path){
  for(uint32 mode=0;mode<10;++mode){Run r;r.Begin(path);auto content=r.Content();assert(r.actor.functions.SizeQueue()==0&&content->_presentationFunctions.SizeQueue()==(path==2?2u:1u));
   if(mode==1)r.actor.states|=UNIT_STATE_STUNNED;if(mode==2)r.actor.states|=UNIT_STATE_CASTING;if(mode==3)r.Suspend();if(mode==4)r.Suspend(false);if(mode==5)r.Suspend(true,true);if(mode==6)content->Reset();if(mode==7)r.actor.victim=true;
   uint32 delay=path==2?3000:4000;r.Tick(delay-1,mode!=8);assert(r.actor.talks.size()==1);r.Tick(mode==9?1001u:1u,mode!=8);assert(r.actor.talks.size()==2);
   if(path==2){assert(!r.actor.motion.points);r.Tick(mode==9?2999u:3999u);assert(!r.actor.motion.points);r.Tick(1);}
   r.Tick(10000);AssertFinished(r,path);++cases;
  }
  for(uint32 elapsed:{0u,1u,2000u,2999u}){Run r;r.Begin(path);r.Tick(elapsed,false);auto initialTalks=r.actor.talks.size();auto initialCredits=r.player.credits.size();assert(r.actor.AIM_Destroy());r.Tick(10000,false);assert(r.actor.talks.size()==initialTalks&&r.player.credits.size()==initialCredits&&!r.actor.motion.points&&!r.actor.despawns);r.actor.i_AI=new Nine(&r.actor);r.Tick(10000);assert(r.actor.talks.size()==initialTalks);++cases;}
  {Run r;r.Begin(path);uint32 other=0;r.actor.AddDelayedEvent(5000,[&]{++other;});assert(r.actor.AIM_Destroy());r.Tick(10000,false);assert(other==1&&r.actor.talks.size()==1);++cases;}
  {Run r;r.Begin(path);r.actor.onTalk=[&]{assert(r.actor.m_AI_locked&&!r.actor.AIM_Destroy());};r.Tick(7000);r.actor.onTalk={};AssertFinished(r,path);++cases;}
  {Run r;r.Begin(path);assert(r.actor.AIM_Destroy());r.actor.talks.clear();r.player.credits.clear();r.actor.casts.clear();r.actor.motion={};r.actor.i_AI=new Nine(&r.actor);r.Tick(10000);assert(r.actor.talks.empty());r.Begin(path);r.Tick(7000);AssertFinished(r,path);++cases;}
 }
 // The original player can be destroyed and a different nearest player found.
 // The departure text still stores its original full128-bit target, no dereference.
 for(bool replacement:{false,true}){Run r;auto transient=std::make_unique<Player>();transient->guid={0x1234000000000055,0xfeedabdc00000099};auto guid=transient->guid;r.actor.nearest=transient.get();r.Content()->MoveInLineOfSight(transient.get());assert(transient->credits==std::vector<uint32>{46902});transient.reset();r.actor.nearest=replacement?&r.player:nullptr;r.Tick(4000);assert(r.actor.talks.back().first==8&&r.actor.talks.back().second==guid&&r.actor.despawns==1&&r.player.credits.empty());++cases;}
 {Run r;r.Begin(1);r.Begin(1);r.Tick(4000);r.Begin(1);r.Tick(4000);AssertFinished(r,1);++cases;}
 {Run r;Unit pet;r.Content()->MoveInLineOfSight(&pet);assert(!r.Content()->_introDone&&r.actor.talks.empty());r.Begin(1);r.Tick(4000);AssertFinished(r,1);++cases;}
 {Run r;r.actor.near=false;r.Begin(1);assert(!r.Content()->_introDone&&r.actor.talks.empty());r.actor.near=true;r.Begin(1);r.Tick(4000);AssertFinished(r,1);++cases;}
 {Run r;r.actor.y=1800;r.Begin(1);r.Tick(10000);assert(r.actor.talks.size()==1&&r.actor.talks[0].first==1&&r.player.credits.empty()&&!r.actor.despawns);++cases;}
 {Run r;r.actor.nearest=nullptr;r.Begin(1);r.Tick(10000);assert(r.Content()->_introDone&&r.actor.talks.empty()&&!r.actor.despawns);++cases;}
 {Run r;r.Content()->SetData(2,1);r.Content()->SetData(1,2);r.Tick(10000);assert(r.actor.talks.empty()&&r.player.credits.empty()&&r.actor.casts.empty());++cases;}
 {Run r;r.actor.nearest=nullptr;r.Begin(0);r.Tick(4000);assert(r.actor.talks.size()==1&&r.actor.flags&1&&r.actor.casts==std::vector<uint32>{193721});++cases;}
 {Run r;r.actor.nearest=nullptr;r.Begin(2);r.Tick(7000);assert(r.player.credits.empty()&&r.actor.talks.size()==2&&r.actor.motion.points==1);++cases;}
 {Run r;uint32 damage=0xffffffff;r.Content()->DamageTaken(&r.player,damage,0);assert(!damage);++cases;}
 std::cout<<"PASS "<<cases<<" actual full Nine Czi/native FunctionProcessor+locked Creature lifecycle+AIM_Destroy ASAN contexts: four3s/4s/4s/7s timers, immutable full GUID, player destruction/replacement, suspended/current-null/alias clocks, stun/casting/no-victim/no UpdateAI, exact timing/late tick/reset/repeat, destruction/replacement, unrelated Unit work, callback lock, unchanged spell/credit/motion endpoints. Native world/network services controlled; admission/nearest-player policies and client/full Monk intro remain separate.\n";
#endif
}
