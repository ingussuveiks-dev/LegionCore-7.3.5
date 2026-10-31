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
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;
using Milliseconds=std::chrono::milliseconds;
uint32 urand(uint32 low,uint32){return low;}
#include "EventMap.inc"
#include "Functions.inc"
enum{UNIT_STATE_CASTING=1,SELECT_TARGET_RANDOM=0,REACT_PASSIVE=0,REACT_AGGRESSIVE=1,REACT_ATTACK_OFF=2};
struct SpellInfo{uint32 Id;};
struct Unit{virtual ~Unit()=default;};
struct CreatureAI;
struct Creature:Unit{
 CreatureAI* i_AI=nullptr;CreatureAI* i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;bool AIM_Destroy();
 FunctionProcessor functions;bool combat=true,despawn=false,world=true,casting=false,threatEmpty=false;uint32 react=REACT_AGGRESSIVE;
 Unit target;Unit* victim=&target;std::vector<uint32> casts,talks;bool failCast=false;uint32 attempts=0;
 bool isInCombat()const{return combat;}bool IsDespawn()const{return despawn;}bool IsInWorld()const{return world;}
 uint32 GetReactState()const{return react;}bool HasReactState(uint32 state)const{return react==state;}
 Unit* SelectVictim(){return victim;}Unit* getVictim(){return victim;}
 struct Threat{bool empty;bool isThreatListEmpty()const{return empty;}};Threat getThreatManager(){return {threatEmpty};}
 bool HasUnitState(uint32)const{return casting;}
 void CastSpell(Unit*,uint32 id){++attempts;if(!failCast)casts.push_back(id);}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& fn){functions.AddDelayedEvent(delay,std::move(fn));}
};
struct CreatureAI{
 Creature* me;explicit CreatureAI(Creature* actor):me(actor){}virtual ~CreatureAI()=default;
 virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void JustDied(Unit*){}virtual void SpellHitTarget(Unit*,SpellInfo const*){}virtual void UpdateAI(uint32){}
 bool UpdateVictim();void AttackStart(Unit* target){me->victim=target;}void EnterEvadeMode(){me->combat=false;Reset();}
};
#include "Victim.inc"
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(condition,...) assert(condition)
#include "AIDestroy.inc"
struct ScriptedAI:CreatureAI{
 using CreatureAI::CreatureAI;Unit* SelectTarget(uint32,uint32){return me->victim;}
 void Talk(uint32 id){me->talks.push_back(id);}void DoCast(uint32 id){me->CastSpell(me,id);}void DoMeleeAttackIfReady(){}
};
struct CreatureScript{explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual CreatureAI* GetAI(Creature*)const=0;};
#include "Actor.inc"
using Boss=npc_kraliss::npc_kralissAI;
struct Run{
 Creature boss;std::unique_ptr<Boss> ai=std::make_unique<Boss>(&boss);
 Run(){ai->EnterCombat(&boss.target);}
 void Tick(uint32 diff){boss.functions.Update(diff);ai->UpdateAI(diff);}
 void BeginRoar(){Tick(18000);assert(boss.talks.size()==1&&boss.casts.empty());}
 uint32 Roars(){uint32 n=0;for(auto spell:boss.casts)if(spell==209748)++n;return n;}
};
int main(){
#ifdef BASELINE
 {Run r;r.BeginRoar();r.ai->Reset();r.Tick(1000);assert(r.Roars()==1);}
 {Run r;r.BeginRoar();r.boss.combat=false;r.ai->JustDied(nullptr);r.Tick(1000);assert(r.Roars()==1);}
 {Run r;r.BeginRoar();r.ai.reset();assert(r.boss.functions.SizeQueue()==1);/* Do not invoke the dangling captured pointer. */}
 std::cout<<"PASS COUNTERFACTUAL267d273dc: actual full Kraliss/native FunctionProcessor dispatch casts after Reset/death-combat-stop and retains raw AI capture after AI destruction. Dangling pointer deliberately not invoked; no fabricated crash/client claim.\n";
#else
 unsigned cases=0;
 {Run r;r.BeginRoar();r.Tick(999);assert(!r.Roars());r.Tick(1);assert(r.Roars()==1);r.Tick(999);assert(r.Roars()==1);r.Tick(1);assert(r.boss.casts.size()==3&&r.boss.casts[1]==209764&&r.boss.casts[2]==209755);++cases;}
 for(uint32 elapsed:{0u,1u,500u,999u}){Run r;r.BeginRoar();r.Tick(elapsed);r.ai->Reset();r.Tick(10000);assert(!r.Roars()&&r.ai->events.Empty());++cases;}
 {Run r;r.BeginRoar();r.boss.combat=false;r.ai->JustDied(nullptr);r.Tick(10000);assert(!r.Roars()&&r.boss.functions.Empty());++cases;}
 for(uint32 absent:{0u,1u,2u,3u}){Run r;r.BeginRoar();
  if(absent==0)r.boss.world=false;if(absent==1)r.boss.despawn=true;if(absent==2)r.boss.combat=false;if(absent==3)r.boss.victim=nullptr;
  r.Tick(10000);assert(!r.Roars());r.ai->Reset();r.boss.world=true;r.boss.despawn=false;r.boss.combat=true;r.boss.victim=&r.boss.target;r.Tick(1000);assert(!r.Roars());++cases;
 }
 {Run r;r.BeginRoar();r.boss.casting=true;r.Tick(1000);assert(!r.Roars());r.boss.casting=false;r.Tick(0);assert(r.Roars()==1);r.Tick(0);assert(r.Roars()==1);++cases;}
 {Run r;r.BeginRoar();r.boss.failCast=true;r.Tick(1000);assert(!r.Roars()&&r.boss.attempts==1);r.Tick(0);assert(r.boss.attempts==1);r.boss.failCast=false;r.Tick(1000);r.Tick(16000);assert(r.boss.talks.size()==2);r.Tick(1000);assert(r.Roars()==1);++cases;}
 {Run r;r.BeginRoar();r.ai.reset();r.boss.functions.Update(10000);assert(r.boss.functions.Empty()&&!r.Roars());r.ai=std::make_unique<Boss>(&r.boss);r.ai->EnterCombat(&r.boss.target);r.Tick(1000);assert(!r.Roars());++cases;}
 {Creature boss;boss.i_AI=new Boss(&boss);boss.i_AI->EnterCombat(&boss.target);boss.i_AI->UpdateAI(18000);assert(boss.talks.size()==1);
  boss.m_AI_locked=true;assert(!boss.AIM_Destroy()&&boss.i_AI);boss.m_AI_locked=false;assert(boss.AIM_Destroy()&&!boss.i_AI&&!boss.IsAIEnabled);
  boss.functions.Update(10000);assert(boss.functions.Empty()&&boss.casts.empty());++cases;
 }
 {Run r;uint32 unrelated=0;r.boss.AddDelayedEvent(19000,[&]{++unrelated;});r.BeginRoar();r.ai->Reset();r.Tick(1000);assert(unrelated==1&&!r.Roars());++cases;}
 {Run r;r.BeginRoar();r.boss.react=REACT_PASSIVE;r.boss.threatEmpty=true;r.Tick(1000);assert(!r.Roars()&&r.ai->events.Empty());++cases;}
 {Run r;r.BeginRoar();r.boss.react=REACT_ATTACK_OFF;r.Tick(1000);assert(!r.Roars());r.ai->Reset();r.boss.react=REACT_AGGRESSIVE;r.Tick(1000);assert(!r.Roars());++cases;}
 {Run r;r.BeginRoar();r.boss.functions.Update(10000);assert(!r.Roars()&&r.boss.functions.Empty());/* Suspended AI is not given UpdateAI. */
  r.ai->UpdateAI(1000);assert(r.Roars()==1);++cases;
 }
 {Run r;Unit pet;r.boss.victim=&pet;r.BeginRoar();r.Tick(1000);assert(r.Roars()==1);++cases;}
 {Run r;r.BeginRoar();r.Tick(1000);r.ai->Reset();r.ai->EnterCombat(&r.boss.target);r.Tick(17999);assert(r.boss.talks.size()==1&&r.Roars()==1);r.Tick(1);assert(r.boss.talks.size()==2&&r.Roars()==1);r.Tick(1000);assert(r.Roars()==2);++cases;}
 {Run r;SpellInfo a{20508},b{209755},other{123};r.ai->SpellHitTarget(&r.boss.target,&a);r.ai->SpellHitTarget(&r.boss.target,&b);r.ai->SpellHitTarget(&r.boss.target,&other);assert(r.boss.casts==std::vector<uint32>({209757,209757}));++cases;}
 std::cout<<"PASS "<<cases<<" actual full Kraliss/native EventMap+FunctionProcessor+UpdateVictim contexts:18s speech/1s roar/20s random spells, reset/death/world/despawn/no-victim/cast deferral/native passive evade, failed cast no replay, AI destruction/replacement/retry, unrelated Unit queue retained, pet victim and unchanged hit spells. Victim/Spell/Creature services controlled; no actual client combat/presentation/full1066 claim.\n";
#endif
}
