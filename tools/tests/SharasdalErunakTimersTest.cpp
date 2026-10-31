#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <iostream>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
using Milliseconds=std::chrono::milliseconds;
uint32 urand(uint32 low,uint32){return low;}int32 irand(int32 low,int32){return low;}
#include "EventMap.inc"
#include "Functions.inc"
enum{TYPEID_UNIT,TYPEID_PLAYER,UNIT_STATE_STUNNED=8,UNIT_STATE_CASTING=16,WAYPOINT_MOTION_TYPE=2};
struct Player;struct Creature;struct CreatureAI;
struct Unit{virtual ~Unit()=default;virtual uint32 GetTypeId()const{return TYPEID_UNIT;}virtual Player* ToPlayer(){return nullptr;}};
struct Player:Unit{std::vector<uint32> credits;uint32 GetTypeId()const override{return TYPEID_PLAYER;}Player* ToPlayer()override{return this;}void KilledMonsterCredit(uint32 id){credits.push_back(id);}void CastSpell(Unit*,uint32){}};
struct InstanceScript{uint32 step=0;uint32 getScenarionStep()const{return step;}};
struct Motion{uint32 paths=0;void MovePath(uint32 id,bool,int32=0,int32=0){assert(id==10993811);++paths;}};
struct Creature:Unit{
 CreatureAI* i_AI=nullptr;CreatureAI* i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;
 bool AIM_Destroy();void Lifecycle(uint32 diff);
 FunctionProcessor functions;uint32 states=0;float z=245,hp=100;bool near=true,world=true,preDelete=false,alive=true,victim=false;
 uint32 attacks=0;std::function<void()> onPresentation;std::vector<uint32> talks,conversations,casts;Motion motion;InstanceScript instance;Creature* grash=nullptr;Creature* adel=nullptr;Creature* brute=nullptr;
 CreatureAI* AI(){return i_AI;}bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}
 float GetHealthPct()const{return hp;}uint64 GetHealth(Unit*)const{return uint64(hp);}void RemoveAura(uint32 id){assert(id==208093);}void AddAura(uint32 id,Creature*){assert(id==208093);}
 void ClearUnitState(uint32 state){states&=~state;}void AddUnitState(uint32 state){states|=state;}bool HasUnitState(uint32 state)const{return (states&state)!=0;}
 float GetPositionX()const{return 0;}float GetPositionY()const{return 0;}float GetPositionZ()const{return z;}float GetOrientation()const{return 0;}void SetHomePosition(float,float,float,float){}
 InstanceScript* GetInstanceScript(){return &instance;}Motion* GetMotionMaster(){return &motion;}bool IsWithinDistInMap(Unit*,float)const{return near;}
 Creature* FindNearestCreature(uint32 id,float,bool){return id==105298?grash:id==104853?adel:id==102792?brute:nullptr;}Player* FindNearestPlayer(float){return nullptr;}
 void CreateConversation(uint32 id){conversations.push_back(id);if(onPresentation)onPresentation();}void AddDelayedEvent(uint64 delay,std::function<void()>&& f){functions.AddDelayedEvent(delay,std::move(f));}
};
struct CreatureAI{
 Creature* me;bool m_MoveInLineOfSight_locked=false;explicit CreatureAI(Creature* actor):me(actor){}virtual ~CreatureAI()=default;
 virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void EnterEvadeMode(){}virtual void OnHealApplied(Unit*,uint32,int32){}virtual void UpdateLifecycle(uint32){}
 virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}virtual void MovementInform(uint32,uint32){}virtual void MoveInLineOfSight(Unit*){}virtual void UpdateAI(uint32){}
 virtual void Talk(uint32 id){me->talks.push_back(id);if(me->onPresentation)me->onPresentation();}virtual void AttackStart(Unit*){++me->attacks;me->victim=true;}
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
struct CreatureScript{explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual CreatureAI* GetAI(Creature*)const=0;};
#include "Recovery.inc"
#include "Actor.inc"
using Erunak=npc_erunak_stonespeaker::npc_erunak_stonespeakerAI;
struct Run{
 Creature actor,grash,adel,brute;Player player;CreatureAI grashAI{&grash},adelAI{&adel},temporary{&actor};
 Run(bool upper){actor.z=upper?797.f:245.f;actor.i_AI=new Erunak(&actor);actor.grash=&grash;actor.adel=&adel;actor.brute=&brute;grash.i_AI=&grashAI;adel.i_AI=&adelAI;}
 ~Run(){if(actor.i_disabledAI&&actor.i_disabledAI!=actor.i_AI)delete actor.i_disabledAI;actor.i_disabledAI=nullptr;if(actor.i_AI!=&temporary)delete actor.i_AI;}
 Erunak* Content(){return static_cast<Erunak*>(actor.i_disabledAI?actor.i_disabledAI:actor.i_AI);}
 void Begin(){actor.i_AI->MoveInLineOfSight_Safe(&player);}
 void Tick(uint32 diff,bool ai=true){actor.Lifecycle(diff);actor.functions.Update(diff);if(ai&&actor.i_AI)actor.i_AI->UpdateAI(diff);}
 uint32 Output(bool upper)const{return uint32(upper?actor.conversations.size():actor.talks.size());}
 void Suspend(bool current=true,bool alias=false){auto content=actor.i_AI;actor.i_disabledAI=content;actor.i_AI=alias?content:current?&temporary:nullptr;}
};
int main(){
#ifdef BASELINE
 for(bool upper:{false,true}){Run r(upper);r.Begin();assert(r.actor.functions.SizeQueue()==1);assert(r.actor.AIM_Destroy());assert(r.actor.functions.SizeQueue()==1);/* The known dangling pointer is deliberately not invoked. */}
 std::cout<<"PASS EXPECTED BROKEN c3dd68ad5 counterfactual: both actual intro callbacks remain in native Unit FunctionProcessor after native AIM_Destroy deletes their captured AI. Dangling pointers not invoked; no fabricated runtime crash or client claim.\n";
#else
 unsigned cases=0;
 for(bool upper:{false,true}){
  uint32 delay=upper?5000:3000;
  for(uint32 mode=0;mode<7;++mode){Run r(upper);r.Begin();auto content=r.Content();assert(r.actor.functions.SizeQueue()==0&&content->_functions.SizeQueue()==1);
   if(mode==1)r.actor.states|=UNIT_STATE_STUNNED;if(mode==2)r.actor.states|=UNIT_STATE_CASTING;
   if(mode==3)r.Suspend();if(mode==4)r.Suspend(false);if(mode==5)r.Suspend(true,true);if(mode==6)content->Reset();
   r.Tick(delay-1);assert(r.Output(upper)==0);r.Tick(1);assert(r.Output(upper)==1);r.Tick(10000);assert(r.Output(upper)==1);
   if(upper){assert(r.actor.conversations==std::vector<uint32>{1868}&&r.actor.motion.paths==1&&r.grash.motion.paths==1&&r.adel.motion.paths==1&&r.player.credits.empty());}
   else{assert(r.actor.talks==std::vector<uint32>{0}&&r.player.credits==std::vector<uint32>{105805}&&content->firstbattle);assert(r.actor.attacks==(mode==4?0u:1u));}
   ++cases;
  }
  for(uint32 elapsed:{0u,1u,1500u,2999u}){Run r(upper);r.Begin();r.Tick(elapsed,false);assert(!r.Output(upper));assert(r.actor.AIM_Destroy());r.actor.functions.Update(10000);assert(!r.Output(upper)&&r.actor.functions.Empty());r.actor.i_AI=new Erunak(&r.actor);r.Tick(10000);assert(!r.Output(upper));r.Begin();r.Tick(delay);assert(r.Output(upper)==1);++cases;}
  {Run r(upper);r.Begin();r.actor.m_AI_locked=true;assert(!r.actor.AIM_Destroy());r.actor.m_AI_locked=false;r.Tick(delay);assert(r.Output(upper)==1);++cases;}
  {Run r(upper);r.Begin();uint32 unrelated=0;r.actor.AddDelayedEvent(delay,[&]{++unrelated;});assert(r.actor.AIM_Destroy());r.Tick(delay,false);assert(unrelated==1&&!r.Output(upper));++cases;}
  {Run r(upper);r.Begin();r.Begin();r.Tick(delay);r.Begin();r.Tick(delay);assert(r.Output(upper)==1);++cases;}
  {Run r(upper);Unit pet;r.actor.i_AI->MoveInLineOfSight_Safe(&pet);assert(!r.Output(upper)&&r.actor.functions.SizeQueue()==0&&r.Content()->_functions.SizeQueue()==0);r.Begin();r.Tick(delay);assert(r.Output(upper)==1);++cases;}
  {Run r(upper);r.actor.near=false;r.Begin();r.Tick(delay);assert(!r.Output(upper));r.actor.near=true;r.Begin();r.Tick(delay);assert(r.Output(upper)==1);++cases;}
  {Run r(upper);r.Begin();r.Tick(delay/2,false);r.Tick(delay-delay/2,false);assert(r.Output(upper)==1);++cases;}
  {Run r(upper);r.Begin();auto content=r.Content();content->wait_heal=true;r.actor.states|=UNIT_STATE_STUNNED;r.actor.hp=41;content->OnHealApplied(&r.player,1,1);r.Tick(delay);assert(!content->wait_heal&&!content->_pendingHealRecovery&&!r.actor.HasUnitState(UNIT_STATE_STUNNED)&&r.Output(upper)==1);++cases;}
  {Run r(upper);r.actor.grash=nullptr;r.actor.adel=nullptr;r.actor.brute=nullptr;r.Begin();r.Tick(delay);assert(r.Output(upper)==1&&!r.actor.attacks);++cases;}
  {Run r(upper);r.Begin();r.actor.onPresentation=[&]{assert(r.actor.m_AI_locked&&!r.actor.AIM_Destroy());};r.Tick(delay);assert(r.Output(upper)==1&&!r.actor.m_AI_locked);++cases;}
  {Run r(upper);r.Begin();r.Tick(delay+1000);assert(r.Output(upper)==1);r.Tick(0);assert(r.Output(upper)==1);++cases;}
 }
 std::cout<<"PASS "<<cases<<" actual full Erunak/native FunctionProcessor+EventMap+SafeLOS+locked lifecycle+AIM_Destroy contexts: exact3s/5s once, stunned/casting/no victim/no UpdateAI, suspended/current-null/alias once, Reset, destroy/replace/retry, unrelated Unit work retained, pet presence and post-heal40pct recovery. Native service boundaries controlled; existing admission/waypoint/nearest actor policies not certified, no client/full1066 claim.\n";
#endif
}
