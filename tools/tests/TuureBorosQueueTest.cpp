#include <cassert>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <memory>
#include <map>
#include <vector>
#include <functional>
#include <iostream>
#include <algorithm>
using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
enum {TYPEID_PLAYER=4,UNIT_STATE_STUNNED=8,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_FIELD_FLAGS=1,
 UNIT_FLAG_NOT_SELECTABLE=1,UNIT_FLAG_IMMUNE_TO_PC=2,UNIT_FLAG_IMMUNE_TO_NPC=4,UNIT_FLAG_NOT_ATTACKABLE_1=8};
#include "Functions.inc"
struct Creature;struct Unit {uint32 type=4;uint64 guid=1;virtual ~Unit()=default;uint32 GetTypeId()const{return type;}uint64 GetGUID()const{return guid;}};
struct Player:Unit {uint32 credits=0;void UpdateAchievementCriteria(uint32 type,uint32 asset){assert(type==92&&asset==50562);++credits;}};
struct CreatureAI {virtual ~CreatureAI()=default;virtual void Reset(){};virtual void MoveInLineOfSight(Unit*){};virtual void EnterEvadeMode(){};virtual void UpdateLifecycle(uint32){};virtual void AttackStart(Unit*){}};
struct GameObject {uint64 GetGUID()const{return 249469;}};
struct InstanceScript {uint32 doors=0;void HandleGameObject(uint64 guid,bool open){assert(guid==249469&&open);++doors;}};
struct Creature:Unit {
 bool world=true,alive=true,preDelete=false,m_AI_locked=false,near=true;uint32 entry=106134,moves=0,flags=15,conversations=0;
 CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;Creature* jeims=nullptr;Creature* enemy=nullptr;Creature* target=nullptr;Player* nearest=nullptr;GameObject* door=nullptr;InstanceScript* instance=nullptr;FunctionProcessor functions;
 Creature(){type=3;}CreatureAI* AI(){return primary;}bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool IsPreDelete()const{return preDelete;}float GetHealthPct()const{return 100;}
 bool IsWithinDistInMap(Unit*,float distance){assert(distance==20);return near;}
 void ClearUnitState(uint32){}void RemoveAura(uint32 id){assert(id==204237);}
 void AddDelayedEvent(uint64 ms,std::function<void()>&& f){functions.AddDelayedEvent(ms,std::move(f));}
 Creature* FindNearestCreature(uint32 id,float distance,bool alive=true){assert(alive);if(id==106137){assert(distance==100);return jeims;}if(id==106201){assert(distance==40);return enemy;}assert(id==106221&&distance==40);return target;}
 GameObject* FindNearestGameObject(uint32 id,float distance){assert(id==249469&&distance==30);return door;}InstanceScript* GetInstanceScript(){return instance;}
 Player* FindNearestPlayer(float distance){assert(distance==100);return nearest;}
 void RemoveFlag(uint32 field,uint32 mask){assert(field==1&&mask==15);flags&=~mask;}
 void CreateConversation(uint32 id){assert(id==2009);++conversations;}
 Creature* GetMotionMaster(){return this;}void MovePoint(uint32 id,float x,float y,float z){assert(id==2&&x==34.70f&&y==1201.05f&&z==-65.06f);++moves;}
 void Tick(uint32 diff){CreatureAI* i_AI=primary;
#include "Lifecycle.inc"
 functions.Update(diff);}
};
#include "Recover.inc"
struct EventMap {void Reset(){}};
struct Boros:CreatureAI {
 Creature* me;EventMap events;FunctionProcessor _functions,_entryFunctions;uint64 _entryGeneration=0;bool _pendingHealRecovery=false,wait_heal=false,_introDone=false,firstfighting=false;
 std::vector<uint32> talks,casts;uint32 attacks=0;explicit Boros(Creature* c):me(c){c->primary=this;}
 void CancelIntroHeal(){}void CompleteIntroHeal(){} // separately tested full heal/lease
 void Talk(uint32 id,uint64=0){talks.push_back(id);}void DoCast(uint32 id){assert(id==212629);casts.push_back(id);}void AttackStart(Unit*)override{++attacks;}
#include "Methods.inc"
#include "Arrival.inc"
};
struct Run {Player player;Creature actor,jeims,enemy,target;GameObject door;InstanceScript instance;std::unique_ptr<Boros> ai;
 Run(){actor.jeims=&jeims;jeims.enemy=&enemy;actor.target=&target;actor.door=&door;actor.instance=&instance;ai=std::make_unique<Boros>(&actor);jeims.primary=ai.get();}
 void Tick(uint32 ms){actor.Tick(ms);}
};
int main(){
#ifdef BASELINE
 {Run r;r.ai->ArrivedFirst();assert(r.actor.functions.SizeQueue()==2);r.ai->Reset();r.Tick(3000);assert(r.actor.moves==1&&r.ai->casts.size()==1);}
 {Run r;r.ai->MoveInLineOfSight(&r.player);r.ai->Reset();r.Tick(4000);assert(std::count(r.ai->talks.begin(),r.ai->talks.end(),1u)==1);}
 {Run r;r.ai->ArrivedFirst();r.ai.reset();r.actor.primary=nullptr;assert(r.actor.functions.SizeQueue()==2);/* Do not execute the now-dangling baseline callback. */}
 std::cout<<"PASS labelled473d023c counterfactual: old Unit-owned movement/greeting still execute after AI Reset; queued borrowed callbacks remain after AI destruction. Freed baseline callback deliberately not invoked.\n";
#else
 uint32 variants=0;
 for(int mode=0;mode<4;++mode)for(int missing=0;missing<4;++missing){Run r;CreatureAI temporary;if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}if(mode==2)r.actor.i_disabledAI=r.ai.get();if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();}
  if(missing==1)r.actor.door=nullptr;if(missing==2)r.actor.jeims=nullptr;if(missing==3)r.jeims.enemy=nullptr;
  r.ai->ArrivedFirst();r.ai->MoveInLineOfSight(&r.player);assert(!r.actor.functions.SizeQueue()&&r.ai->_entryFunctions.SizeQueue()==3);
  r.Tick(1999);assert(r.ai->casts.empty()&&!r.actor.moves);r.Tick(1);assert(r.ai->casts.size()==1&&std::count(r.ai->talks.begin(),r.ai->talks.end(),3u)==1);
  assert(r.instance.doors==uint32(missing!=1)&&r.ai->attacks==uint32(missing!=2&&missing!=3));
  r.Tick(999);assert(!r.actor.moves);r.Tick(1);assert(r.actor.moves==1);r.Tick(999);assert(std::count(r.ai->talks.begin(),r.ai->talks.end(),1u)==0);r.Tick(1);assert(std::count(r.ai->talks.begin(),r.ai->talks.end(),1u)==1);
  r.Tick(20000);assert(r.actor.moves==1&&r.ai->casts.size()==1);++variants;
 }
 {Run r;r.ai->ArrivedFirst();r.ai->MoveInLineOfSight(&r.player);r.ai->Reset();r.Tick(20000);assert(!r.actor.moves&&r.ai->casts.empty()&&r.ai->talks.size()==1);r.ai->ArrivedFirst();r.Tick(3000);assert(r.actor.moves==1&&r.ai->casts.size()==1);}
 {Run r;r.ai->ArrivedFirst();r.ai->Reset();r.ai->ArrivedFirst();r.Tick(3000);assert(r.actor.moves==1&&r.ai->casts.size()==1);}
 {Run r;r.ai->ArrivedFirst();r.ai->MoveInLineOfSight(&r.player);r.ai.reset();r.actor.primary=nullptr;assert(!r.actor.functions.SizeQueue());r.Tick(20000);r.ai=std::make_unique<Boros>(&r.actor);r.Tick(20000);assert(!r.actor.moves&&r.ai->casts.empty());}
 for(bool reset:{false,true}){Run r;r.ai->firstfighting=true;r.ai->EnterEvadeMode();r.ai->_functions.Update(1000);assert(r.actor.conversations==1&&!r.ai->firstfighting&&r.ai->_entryFunctions.SizeQueue()==1);if(reset)r.ai->Reset();r.Tick(15999);assert(r.target.flags==15);r.Tick(1);assert(r.target.flags==(reset?15u:0u));r.Tick(16000);assert(r.actor.conversations==1);}
 {Run r;r.actor.target=nullptr;r.ai->firstfighting=true;r.ai->EnterEvadeMode();r.ai->_functions.Update(1000);r.Tick(16000);assert(r.actor.conversations==1);}
 std::cout<<"PASS actual greeting, point1 callback body, full evade/reset/lifecycle with native FunctionProcessor and Creature locked lifecycle: "<<variants<<" primary/disabled/alias/null x missing actor/door cases; exact2s cast/3s motion/4s speech/16s release, Reset cancellation/same-tick retry, freed/replaced AI and no victim/stun gate. Native motion/door/spell/credit endpoints are adapters; later recipient/stage/follow/client remains open.\n";
#endif
}
