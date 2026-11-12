#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#ifdef SECOND_EIGHTH_NATIVE_CLOCK
#include "EventMap.h"
#endif
namespace EighthClock {
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using ObjectGuid=uint64_t;
#include "ClockLabels.inc"
static void Require(bool value,char const*message){if(!value)throw std::runtime_error(message);}
static uint32 urand(uint32 first,uint32){return first;}
#ifdef SECOND_EIGHTH_NATIVE_CLOCK
using EventMap=::EventMap;
#else
struct EventMap{uint64_t now=0;std::multimap<uint64_t,uint32>due;void Reset(){now=0;due.clear();}void Update(uint32 diff){now+=diff;}void ScheduleEvent(uint32 id,uint32 delay){due.emplace(now+delay,id);}void RescheduleEvent(uint32 id,uint32 delay){for(auto i=due.begin();i!=due.end();)if(i->second==id)i=due.erase(i);else++i;ScheduleEvent(id,delay);}uint32 ExecuteEvent(){if(due.empty()||due.begin()->first>now)return 0;auto id=due.begin()->second;due.erase(due.begin());return id;}};
#endif
struct Position{float GetPositionX(){return 0;}float GetPositionZ(){return 0;}};static Position posHatchersWay[2][4];
struct Unit{virtual~Unit()=default;};
struct InstanceScript{uint32 state=IN_PROGRESS;uint32 GetBossState(uint32){return state;}};
struct Motion{void MoveChase(Unit*){}void MovePoint(uint32,float,float,float){}};
struct Creature:Unit{Unit*victim=nullptr;bool combat=true,casting=false;uint32 healthPct=100,despawns=0;InstanceScript*instance=nullptr;std::vector<uint32>casts;std::set<uint32>auras;Motion motion;bool isInCombat(){return combat;}bool HasAura(uint32 id){return auras.count(id)!=0;}bool HasUnitState(uint32){return casting;}bool HealthBelowPct(uint32 pct){return healthPct<pct;}void CastSpell(Unit*,uint32 id,bool=false){casts.push_back(id);auras.insert(id);}void AddAura(uint32 id,Unit*p){CastSpell(p,id,true);}void DespawnOrUnsummon(){++despawns;}void DisappearAndDie(){++despawns;}Unit*getVictim(){return victim;}Unit*SelectNearestTarget(float){return victim;}void SetReactState(uint32){}float GetPositionY(){return 0;}Motion*GetMotionMaster(){return &motion;}void Attack(Unit*p,bool){victim=p;}void _AddCreatureSpellCooldown(uint32,int64_t){}};
struct TaskScheduler{uint32 elapsed=0;void Update(uint32 diff){elapsed+=diff;}};
struct ClockAI{Creature*me;bool victimAllowed=true;EventMap events;explicit ClockAI(Creature*p):me(p){}bool UpdateVictim(){return victimAllowed&&me->victim;}void _Reset(){events.Reset();}void _EnterCombat(){}void DoMeleeAttackIfReady(){}void DoSpellAttackIfReady(uint32){}void DoCast(Unit*p,uint32 id,bool=false){me->CastSpell(p,id);}void DoCast(uint32 id){me->CastSpell(me,id);}void DoCastAOE(uint32 id){me->CastSpell(me,id);}void DoCastVictim(uint32 id,bool=false){me->CastSpell(me->victim,id);}void Talk(uint32){}Unit*GetPlayerAtMinimumRange(float){return me->victim;}Unit*SelectTarget(uint32,uint32,float=0,bool=false){return me->victim;}Unit*SelectTarget(uint32,uint32,int){return me->victim;}};
static uint32 time(void*){return 1;}static int NonTankTargetSelector(Creature*){return 0;}
namespace ObjectAccessor{static Unit*GetUnit(Creature&,ObjectGuid){return nullptr;}}
// The production shared selector/rescheduler is extracted in full. Other
// instance/spatial/dialogue endpoints remain controlled explicitly.
struct HighSeasClockBase:ClockAI{using ClockAI::ClockAI;ObjectGuid GetLowestFriendGUID(){return 0;}
#include "HighSeasClockHelper.inc"
};
#include "SelectedClock.inc"
static uint32 Count(Creature const&me,uint32 spell){return uint32(std::count(me.casts.begin(),me.casts.end(),spell));}
static void RunClockCase(int test){Creature me;Unit victim;me.victim=&victim;InstanceScript instance;me.instance=&instance;
switch(test){
case 0:{EightClock0 ai(&me);ai.Reset();ai.UpdateAI(0);for(int i=0;i<44;++i)ai.UpdateAI(1000);Require(Count(me,SPELL_FORCE_BLAST)>=2&&Count(me,SPELL_GRAVITY_STRIKE)>=1&&Count(me,SPELL_GRIEVOUS_WHIRL)>=2&&Count(me,SPELL_SHADOW_STRIKE)>=2,"Evolved zealot must dispatch all four authored combat timers");ai.Reset();me.casts.clear();me.combat=false;ai.UpdateAI(60000);Require(me.casts.empty(),"An idle zealot cannot dispatch combat spells");break;}
case 1:{EightClock1 ai(&me);ai.instance=&instance;ai.Reset();ai.EnterCombat(&victim);ai.UpdateAI(7999);Require(!Count(me,SPELL_FIERY_SLASH),"Fiery Slash cannot execute before its authored minimum deadline");ai.UpdateAI(1);Require(Count(me,SPELL_FIERY_SLASH)==1,"Blazing construct must execute its first authored Fiery Slash");me.casting=true;ai.UpdateAI(8000);Require(Count(me,SPELL_FIERY_SLASH)==1,"An active cast defers due Fiery Slash dispatch");me.casting=false;ai.UpdateAI(0);Require(Count(me,SPELL_FIERY_SLASH)==2,"A deferred Fiery Slash retains elapsed combat time");instance.state=DONE;ai.UpdateAI(8000);Require(me.despawns==1&&Count(me,SPELL_FIERY_SLASH)==2,"Encounter cleanup cannot dispatch another construct attack");break;}
case 2:{EightClock2 ai(&me);ai.instance=&instance;ai.Reset();ai.EnterCombat(&victim);ai.UpdateAI(6999);Require(!Count(me,SPELL_FLAMEBUFFET),"Hatchling cannot attack before its authored deadline");ai.UpdateAI(1);Require(Count(me,SPELL_FLAMEBUFFET)==1,"Janalai hatchling must dispatch its authored Flame Buffet");instance.state=DONE;ai.UpdateAI(60000);Require(me.despawns==1&&Count(me,SPELL_FLAMEBUFFET)==1,"Hatchling cleanup remains ahead of combat dispatch");break;}
case 3:{EightClock3 ai(&me);ai.Reset();for(int i=0;i<25;++i)ai.UpdateAI(1000);Require(Count(me,SPELL_SUNDERING_BITE)>=3&&Count(me,SPELL_SHATTERING_STONE)>=2&&Count(me,SPELL_FOCUSED_ASSAULT)>=3,"Stone Quilen must dispatch and repeat all three native attacks");auto count=me.casts.size();ai.victimAllowed=false;ai.UpdateAI(60000);Require(count==me.casts.size(),"No-victim Quilen path cannot dispatch combat events");ai.Reset();ai.victimAllowed=true;me.healthPct=9;ai.UpdateAI(0);ai.UpdateAI(0);Require(Count(me,SPELL_PROTECTIVE_FENZY)==1,"Protective Frenzy remains single application while present");break;}
case 4:{EightClock4 ai(&me);ai.Reset();ai.EnterCombat(&victim);ai.UpdateAI(1499);Require(!Count(me,SPELL_COUNTER_SHOT),"Counter Shot retains the authored first delay");ai.UpdateAI(1);Require(Count(me,SPELL_COUNTER_SHOT)==1,"Horde captain must dispatch its first Counter Shot");ai.UpdateAI(10600);Require(Count(me,SPELL_COUNTER_SHOT)==2,"Counter Shot repeat must use the same shared queue as dispatch");ai.Reset();ai.UpdateAI(60000);Require(Count(me,SPELL_COUNTER_SHOT)==2,"Captain reset clears pending combat repeats");ai.EnterCombat(&victim);ai.EnterCombat(&victim);ai.UpdateAI(1500);Require(Count(me,SPELL_COUNTER_SHOT)==3,"Repeated combat notification cannot duplicate the first Counter Shot");break;}
case 5:{EightClock5 ai(&me);ai.Reset();ai.EnterCombat(&victim);ai.UpdateAI(4999);Require(!Count(me,SPELL_DEVOUR_ENSLAVEMENT),"Scenario dispel retains its native first deadline");ai.UpdateAI(1);Require(Count(me,SPELL_DEVOUR_ENSLAVEMENT)==1,"Black Harvest felhunter must dispatch its authored first dispel");ai.UpdateAI(10000);Require(Count(me,SPELL_DEVOUR_MAGIC)==1,"Felhunter independent magic dispel retains its deadline");ai.Reset();ai.EnterCombat(&victim);me.casting=true;ai.UpdateAI(5000);me.casting=false;ai.UpdateAI(0);Require(Count(me,SPELL_DEVOUR_ENSLAVEMENT)==2,"Deferred dispel retains elapsed time during another cast");break;}
case 6:{EightClock6 ai(&me);ai.Reset();ai.EnterCombat(&victim);for(int i=0;i<90;++i)ai.UpdateAI(1000);Require(Count(me,SPELL_PUMMEL)>=3,"Pummel must repeat its own event after the first interrupt");Require(Count(me,SPELL_MORTALSTRIKE)>=5,"Independent Mortal Strike cadence remains active");break;}
case 7:{EightClock7 ai(&me);ai.EnterCombat(&victim);for(int i=0;i<90;++i)ai.UpdateAI(1000);Require(Count(me,SPELL_SHADOW_POWER)==3,"Gyrokill Shadow Power must repeat under its own event");Require(Count(me,SPELL_SAW_BLADE)==4,"Shadow Power must not add another Saw Blade schedule");break;}
default:throw std::runtime_error("unknown combat clock case");}}
}
