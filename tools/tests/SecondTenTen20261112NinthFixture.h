#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#ifdef SECOND_NINTH_NATIVE_CLOCK
#include "EventMap.h"
#endif
namespace NinthClock {
using PhaseFelmyst=uint32_t;using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using ObjectGuid=uint64_t;
#include "NinthClockLabels.inc"
static void Require(bool value,char const*message){if(!value)throw std::runtime_error(message);}
static uint32 urand(uint32 first,uint32){return first;}
#ifdef SECOND_NINTH_NATIVE_CLOCK
using EventMap=::EventMap;
#else
struct EventMap{uint64_t now=0;std::multimap<uint64_t,uint32>due;void Reset(){now=0;due.clear();}void Update(uint32 diff){now+=diff;}void ScheduleEvent(uint32 id,uint32 delay){due.emplace(now+delay,id);}void RescheduleEvent(uint32 id,uint32 delay){for(auto i=due.begin();i!=due.end();)if(i->second==id)i=due.erase(i);else++i;ScheduleEvent(id,delay);}uint32 ExecuteEvent(){if(due.empty()||due.begin()->first>now)return 0;auto id=due.begin()->second;due.erase(due.begin());return id;}};
#endif
struct Position{float GetPositionX(){return 0;}float GetPositionZ(){return 0;}};static Position posHatchersWay[2][4];
struct Unit{virtual~Unit()=default;};
struct InstanceScript{uint32 state=IN_PROGRESS;uint32 GetBossState(uint32){return state;}};
struct Motion{void MoveChase(Unit*){}void MovePoint(uint32,float,float,float){}};
struct Creature:Unit{uint32 entry=0;uint32 GetEntry(){return entry;}void SetFlag(uint32,uint32){}void SetDisableGravity(bool){}void SetFloatValue(uint32,float){}void SetUnitMovementFlags(uint32){}void CastStop(uint32){}void RemoveAurasDueToSpell(uint32 id){auras.erase(id);}void RemoveAura(uint32 id){auras.erase(id);}template<class...T>void SetSpeed(T...){}Unit*victim=nullptr;bool combat=true,casting=false;uint32 healthPct=100,despawns=0;InstanceScript*instance=nullptr;std::vector<uint32>casts;std::set<uint32>auras;Motion motion;bool isInCombat(){return combat;}bool HasAura(uint32 id){return auras.count(id)!=0;}bool HasUnitState(uint32){return casting;}bool HealthBelowPct(uint32 pct){return healthPct<pct;}void CastSpell(Unit*,uint32 id,bool=false){casts.push_back(id);auras.insert(id);}void AddAura(uint32 id,Unit*p){CastSpell(p,id,true);}void DespawnOrUnsummon(){++despawns;}void DisappearAndDie(){++despawns;}Unit*getVictim(){return victim;}Unit*SelectNearestTarget(float){return victim;}void SetReactState(uint32){}float GetPositionY(){return 0;}Motion*GetMotionMaster(){return &motion;}void Attack(Unit*p,bool){victim=p;}void _AddCreatureSpellCooldown(uint32,int64_t){}};
struct TaskScheduler{uint32 elapsed=0;void Update(uint32 diff){elapsed+=diff;}};
struct ClockAI{Creature*me;bool victimAllowed=true;EventMap events;struct{void DespawnAll(){}}summons;explicit ClockAI(Creature*p):me(p){}bool UpdateVictim(){return victimAllowed&&me->victim;}void _Reset(){events.Reset();}void _EnterCombat(){}void DoMeleeAttackIfReady(){}void DoSpellAttackIfReady(uint32){}void DoCast(Unit*p,uint32 id,bool=false){me->CastSpell(p,id);}void DoCast(uint32 id){me->CastSpell(me,id);}void DoCastAOE(uint32 id){me->CastSpell(me,id);}void DoCastVictim(uint32 id,bool=false){me->CastSpell(me->victim,id);}void Talk(uint32){}Unit*GetPlayerAtMinimumRange(float){return me->victim;}Unit*SelectTarget(uint32,uint32,float=0,bool=false){return me->victim;}Unit*SelectTarget(uint32,uint32,int){return me->victim;}};
static uint32 time(void*){return 1;}static int NonTankTargetSelector(Creature*){return 0;}
namespace ObjectAccessor{static Unit*GetUnit(Creature&,ObjectGuid){return nullptr;}}

#include "NinthSelected.inc"
#include "SecondTenTen20261112NinthCases.inc"
}
