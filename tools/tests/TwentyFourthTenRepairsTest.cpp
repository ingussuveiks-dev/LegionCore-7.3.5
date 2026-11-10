#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
#include "Constants.inc"
struct ObjectGuid{static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
struct Position{};struct Unit;using Creature=Unit;static std::vector<uint32>spells;static uint32 spawns=0,despawns=0,binds=0;
static std::function<void()>controllerAction;
struct Motion{uint32 homes=0;template<class...T>void Clear(T...){}void MoveIdle(){}template<class...T>void MovementExpired(T...){}void MovePoint(uint32,Position){++homes;}};
struct Instance{uint32 changes=0,achievements=0,disengages=0;void SetGuidData(uint32,ObjectGuid){++changes;}void SendEncounterUnit(uint32,Unit*){++disengages;}void DoUpdateAchievementCriteria(uint32,uint32){++achievements;}};
using InstanceScript=Instance;
struct Unit{uint64 hp=100,relativeHp=10;uint32 flags=0,entry=0;bool aura=false;Unit*owner=nullptr;Instance*instance=nullptr;Motion motion;std::set<uint32>auras;uint64 GetHealth(Unit*u=nullptr){return u?relativeHp:hp;}uint64 GetMaxHealth(){return 100;}uint64 CountPctFromMaxHealth(uint32 n){return n;}void SetHealth(uint64 n){hp=n;relativeHp=std::max<uint64>(1,n/10);}float GetHealthPct(){return float(hp);}bool HealthBelowPct(uint32 n){return hp<n;}void SetFullHealth(){SetHealth(100);}void RemoveAllAuras(){aura=false;}void RemoveAurasDueToSpell(uint32){}void RemoveAllAurasOnDeath(){}void InterruptNonMeleeSpells(bool){}void StopAttack(bool=false){}void AttackStop(){}void SetFlag(uint32 field,uint32 n){if(field==UNIT_FIELD_FLAGS)flags|=n;}bool HasFlag(uint32,uint32 n){return(flags&n)!=0;}template<class...T>void ApplySpellImmune(T...){}void AddUnitState(uint32){}void SetReactState(uint32){}void SetDisplayId(uint32){}void SetStandState(uint32){}void SetTarget(ObjectGuid){}void SetUnitMovementFlags(uint32){}void KillAllDelayedEvents(){}struct{void KillAllEvents(bool){}}m_Events;uint32 GetEntry(){return entry;}ObjectGuid GetGUID(){return{};}bool HasAura(uint32 n){return auras.count(n)>0;}Unit*GetAnyOwner(){return owner;}Unit*FindNearestCreature(uint32,float){return owner;}Unit*AI(){return this;}void DoAction(int32){if(controllerAction)controllerAction();}Motion*GetMotionMaster(){return &motion;}Position GetHomePosition(){return{};}Position GetPosition(){return{};}Position GetNearPosition(float,float){return{};}Instance*GetInstanceScript(){return instance;}template<class...T>void CastSpell(T...){spells.push_back(1);}template<class...T>void SummonCreature(T...){++spawns;}void DespawnOrUnsummon(uint32){++despawns;}};
struct Events{uint32 scheduled=0,last=0,phase=0;void Reset(){scheduled=0;}template<class...T>void RescheduleEvent(uint32 n,T...){++scheduled;last=n;}template<class...T>void ScheduleEvent(uint32 n,T...){++scheduled;last=n;}void SetPhase(uint32 n){phase=n;}};
struct Base{Creature*me;Instance*instance=nullptr;Events events;bool Dead=false,despawn=false,interruptActive=false,switchPhase=false,explosion=false,defeated=false,resurrectInProgress=false,bIsUndead=false,bEventInProgress=false,explose=false;uint8 Phase=1,phase=1,uiPhase=1;uint32 casts=0,talks=0,explodes=0;bool heroic=true;explicit Base(Creature*c):me(c),instance(c->instance){}bool IsHeroicPlusRaid(){return heroic;}bool HealthBelowPct(uint32 n){return me->HealthBelowPct(n);}void DoCast(Creature*,uint32 n,bool=false){++casts;spells.push_back(n);}void DoCast(uint32 n){DoCast(me,n);}void Talk(uint32){++talks;}void ExplodeAliveGhouls(){++explodes;}void DoStopAttack(){}void BindPlayersToInstance(Creature*){++binds;}};
static void BindPlayersToInstance(Creature*){++binds;}static uint32 urand(uint32 a,uint32){return a;}static void DoScriptText(uint32,Unit*){}
static void Require(bool b,char const*m){if(!b)throw std::runtime_error(m);}
#include "Selected.inc"
template<class F>static void Hit(F&f,Unit*p,uint32&d){
#ifdef COUNTERFACTUAL
 d=std::max<uint32>(1,d/10);f.DamageTaken(p,d,0);d*=10;
#else
 f.DamageTakenAfterScaling(p,d,0);
#endif
}
int main(){try{Unit me,p,owner;Instance instance;me.instance=&instance;Frame f(&me);uint32 d=10;bool fixed=false;
#if defined(CASE_MURUTRANSITION)
 Hit(f,&p,d);fixed=!d&&f.Phase==2;
#ifndef COUNTERFACTUAL
 auto n=f.casts;for(auto ph:{2,3}){f.Phase=ph;d=100;Hit(f,&p,d);Require(!d&&f.casts==n,"pending phases protected without duplicate portal");}f.Phase=4;d=100;Hit(f,&p,d);Require(d==100,"final cleanup phase retains death");
#endif
#elif defined(CASE_HARBINGERRELEASE)
 me.owner=&owner;uint32 actions=0;controllerAction=[&]{++actions;uint32 nested=10;Hit(f,&p,nested);};Hit(f,&p,d);controllerAction={};fixed=!d&&f.Dead&&actions==1&&despawns==1;
#ifndef COUNTERFACTUAL
 d=10;Hit(f,&p,d);Require(!d&&actions==1,"release once through reentrant controller and late hit");
#endif
#elif defined(CASE_WANINGPARTICLE)
 me.owner=&owner;Hit(f,&p,d);fixed=!d&&f.despawn&&spawns==4;
#ifndef COUNTERFACTUAL
 auto n=f.casts;d=100;Hit(f,&p,d);Require(!d&&spawns==4&&f.casts==n&&f.events.scheduled==1,"four fragments/rift/native3s once");
#endif
#elif defined(CASE_ELISANDEREWIND)
 Hit(f,&p,d);fixed=!d&&f.switchPhase&&f.phase==2&&me.hp==10;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&f.phase==2,"pending rewind protected");f.switchPhase=false;d=100;Hit(f,&p,d);Require(!d&&f.phase==3&&f.switchPhase,"second rewind");d=1;Hit(f,&p,d);Require(!d,"third phase pending is protected");f.switchPhase=false;d=100;Hit(f,&p,d);Require(d==100,"active third phase may die");
#endif
#elif defined(CASE_PLASMAEXPLOSION)
 me.owner=&owner;Hit(f,&p,d);fixed=!d&&f.explosion&&despawns==1;
#ifndef COUNTERFACTUAL
 auto n=f.casts;d=100;Hit(f,&p,d);Require(!d&&f.casts==n&&despawns==1,"one explosion even with late lethal hit");me.entry=NPC_CHAOS_SPHERE;owner.auras.insert(SPELL_NATURE_INFUSION);Frame nature(&me);d=10;Hit(nature,&p,d);Require(!d&&spawns==1&&nature.casts==2,"chaos sphere and nature-only lasher branch");owner.auras.insert(SPELL_ARCANE_INFUSION);Frame combined(&me);d=10;Hit(combined,&p,d);Require(!d&&spawns==1&&combined.casts==2,"combined infusion selects spores instead of lasher");me.owner=nullptr;Frame unowned(&me);d=10;Hit(unowned,&p,d);Require(!d&&unowned.explosion,"missing owner retains local explosion/cleanup");
#endif
#elif defined(CASE_ARGENTCHALLENGE)
 Hit(f,&p,d);fixed=!d&&f.defeated&&instance.achievements==1&&binds==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&instance.achievements==1&&binds==1,"Eadric one surrender/bind");OtherFrame pal(&me);d=10;Hit(pal,&p,d);Require(!d&&pal.defeated&&instance.achievements==2&&binds==2,"Paletress same repaired native contract");d=1;Hit(pal,&p,d);Require(!d&&binds==2,"defeated late nonlethal protected");
#endif
#elif defined(CASE_BLACKKNIGHTRESURRECTION)
 f.uiPhase=PHASE_UNDEAD;Hit(f,&p,d);fixed=!d&&f.resurrectInProgress&&f.explodes==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&f.explodes==1,"pending resurrection does not re-explode ghouls");f.resurrectInProgress=false;f.uiPhase=PHASE_SKELETON;me.SetHealth(100);d=10;Hit(f,&p,d);Require(!d&&f.explodes==2,"skeleton exact lethal resurrects");f.resurrectInProgress=false;f.uiPhase=PHASE_SKELETON+1;d=100;Hit(f,&p,d);Require(d==100&&f.explodes==2,"ghost final death retained");
#endif
#elif defined(CASE_INGVARRESURRECTION)
 Hit(f,&p,d);fixed=!d&&f.bIsUndead&&f.bEventInProgress;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d,"pending ritual protects corpse");f.bEventInProgress=false;d=100;Hit(f,&p,d);Require(d==100,"active undead final death retained");
#endif
#elif defined(CASE_KELESETHSKELETON)
 Hit(f,&p,d);fixed=!d&&me.HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE)&&f.events.scheduled==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&f.events.scheduled==1,"late pet hit cannot reschedule resurrection");me.flags=0;d=100;Hit(f,&p,d);Require(!d&&f.events.scheduled==1,"new restored life starts one fresh event after reset");
#endif
#elif defined(CASE_LIVINGAMBER)
 Hit(f,&p,d);fixed=!d&&f.explose&&f.casts==4;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&f.casts==4,"one explosion and original consuming-spell cleanup retained");
#endif
#else
#error Unknown case
#endif
#ifdef COUNTERFACTUAL
 Require(!fixed,"pinned raw-hook counterfactual must expose scaled lethal defect");std::cout<<"PASS expected-broken pinned control\n";
#else
 Require(fixed,"repaired final-damage contract");std::cout<<"PASS repaired selected lifecycle;source ASAN,not full encounter/client traversal\n";
#endif
 return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
