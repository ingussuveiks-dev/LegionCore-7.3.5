#include <algorithm>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <functional>
#include <stdexcept>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;using AuraRemoveMode=uint32;
struct Position{float x=0,y=0,z=0,o=0;void SimplePosXYRelocationByAngle(Position&p,float,float)const{p=*this;}};
#include "Constants.inc"
struct ObjectGuid{uint64 v=0;ObjectGuid(uint64 n=0):v(n){}};
struct SpellInfo{uint32 Id;};
struct Queue{std::vector<std::pair<uint32,std::function<void()>>>q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,f);}void Update(uint32 n){std::vector<std::function<void()>>v;for(auto i=q.begin();i!=q.end();)if(i->first<=n){v.push_back(i->second);i=q.erase(i);}else{i->first-=n;++i;}for(auto&f:v)f();}};
struct Area{float radius=4,scale=0;uint32 changes=0;float GetRadius(){return radius;}void SetSphereScale(float n,uint32,bool){scale=n;++changes;}};
static uint32 despawns=0,spawns=0,ownerActions=0,notifications=0,lastEntry=0;static bool castOK=true;static std::vector<uint32>spells;static std::function<void(uint32)>onCast;
struct Unit;using Player=Unit;using Creature=Unit;using TempSummon=Unit;
struct Motion{void Initialize(){}};
struct Unit{ObjectGuid guid;bool player=false,alive=true,IsAIEnabled=true,ai=true,temp=false;Unit*owner=nullptr;uint64 hp=100,relativeHp=10;uint32 entry=0,data=0,display=0;Motion motion;Queue q;std::set<uint32>auras;std::map<uint32,Area*>areas;explicit Unit(uint64 n):guid(n){}uint64 GetHealth(Unit*u=nullptr){return u?relativeHp:hp;}uint64 GetMaxHealth(){return 100;}void SetHealth(uint64 n){hp=n;relativeHp=std::max<uint64>(1,n/10);}void SetFullHealth(){SetHealth(100);}Unit*GetAnyOwner(){return owner;}Unit*ToCreature(){return player?nullptr:this;}bool IsCreature(){return !player;}bool IsAlive(){return alive;}Unit*ToTempSummon(){return temp?this:nullptr;}Unit*GetSummoner(){return owner;}Unit*GetAI(){return player?nullptr:ai?this:nullptr;}Unit*AI(){return GetAI();}uint32 GetEntry(){return entry;}uint32 GetData(uint32){return data;}void DoAction(int32){++ownerActions;}void SummonedCreatureDies(Unit*,Unit*){++notifications;}bool HasAura(uint32 n){return auras.count(n)>0;}void RemoveAurasDueToSpell(uint32 n){auras.erase(n);}void RemoveAurasAllDots(){}void RemoveAllAuras(){auras.clear();}void InterruptNonMeleeSpells(bool){}void StopAttack(bool=true){}void AttackStop(){}void StopMoving(){}void SetDisplayId(uint32 n){display=n;}template<class...T>void SetFlag(T...){}Area*GetAreaObject(uint32 n){auto i=areas.find(n);return i==areas.end()?nullptr:i->second;}int CastSpell(Unit*u,uint32 n,bool=false){spells.push_back(n);if(castOK&&u)u->auras.insert(n);if(onCast)onCast(n);return castOK?0:1;}void DespawnOrUnsummon(uint32=0){++despawns;}void AddDelayedEvent(uint32 n,std::function<void()>f){q.Add(n,f);}void AddDelayedCombat(uint32 n,std::function<void()>f){q.Add(n,f);}Motion*GetMotionMaster(){return &motion;}Position GetPosition(){return{};}template<class...T>Unit*SummonCreature(T...){++spawns;return nullptr;}};
struct EntryCheckPredicate{uint32 entry;explicit EntryCheckPredicate(uint32 n):entry(n){lastEntry=n;}};
struct Summons{uint32 actions=0;template<class P>void DoAction(int32,P){++actions;}};
struct Events{uint32 scheduled=0,last=0;void Reset(){}template<class...T>void RescheduleEvent(uint32 n,T...){++scheduled;last=n;}};
struct Base{Creature*me;bool heroic=true,mythic=false,moltenRemnants=false,eruption=false,isMythicModule=false,pulse=false,restart=false,died=false,haywire=false,ascendance=false,explosion=false,feignDeath=false,_dying=false;uint32 moveTimer=500,casts=0,talks=0,selfActions=0;float apocalypsisSize=0;Events events;Summons summons;explicit Base(Creature*c):me(c){}bool IsHeroicPlusRaid(){return heroic;}bool IsMythicRaid(){return mythic;}void Talk(uint32){++talks;}void ZoneTalk(uint32){++talks;}void DoAction(int32){++selfActions;}void EnterEvadeMode(){}void DoCast(uint32 n){++casts;if(castOK)me->auras.insert(n);}void DoCast(Unit*u,uint32 n,bool=true){++casts;if(castOK)u->auras.insert(n);}void DoCastAOE(uint32 n){DoCast(n);}};
static float frand(float a,float){return a;}
static void Require(bool ok,char const*s){if(!ok)throw std::runtime_error(s);}
#include "Selected.inc"
static void Hit(Frame&f,Unit*u,uint32&d){
#ifdef COUNTERFACTUAL
 d=std::max<uint32>(1,d/10);f.DamageTaken(u,d,0);d*=10;
#else
 f.DamageTakenAfterScaling(u,d,0);
#endif
}
int main(){try{Unit me(1),p(2),owner(3);p.player=true;Frame f(&me);uint32 d=10;bool fixed=false;
#if defined(CASE_AGGRAMARREMNANTS)
 Hit(f,&p,d);fixed=!d&&f.moltenRemnants&&!f.moveTimer;
#ifndef COUNTERFACTUAL
 auto n=spells.size();d=10;Hit(f,&p,d);Require(!d&&spells.size()==n,"molten remnants once");Frame normal(&me);normal.heroic=false;d=10;Hit(normal,&p,d);Require(d==10&&!normal.moltenRemnants,"normal ember death retained");
#endif
#elif defined(CASE_ARGUSPULSE)
 Hit(f,&p,d);fixed=!d&&f.pulse;
#ifndef COUNTERFACTUAL
 auto n=spells.size();f.OnApplyOrRemoveAura(SPELL_INITIALIZATION_SEQUENCE,AURA_REMOVE_BY_EXPIRE,false);d=10;Hit(f,&p,d);Require(!d&&spells.size()==n,"lethal and expiry share one native pulse");Area source,target;me.areas[SPELL_APOCALYPSIS_MODULE_AT]=&source;me.areas[SPELL_APOCALYPSIS_ZONE_AT]=&target;auto frame=std::make_unique<Frame>(&me);frame->isMythicModule=true;onCast=[&](uint32 id){if(id==SPELL_APOCALYPSIS_MODULE_REMOVE){SpellInfo info{id};frame->SpellFinishCast(&info);}};d=10;Hit(*frame,&p,d);Require(!d&&frame->apocalypsisSize==4,"source radius snapshotted before synchronous finish");onCast={};frame.reset();me.q.Update(100);Require(target.scale==4&&target.changes==1,"unit-owned100ms callback survives destroyed AI frame");
#endif
#elif defined(CASE_EONARWIPE)
 Hit(f,&p,d);fixed=!d&&f.restart&&f.summons.actions==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&f.summons.actions==1&&f.talks==1,"one native wipe action/speech");
#endif
#elif defined(CASE_WORLDBREAKERWEAPONS)
 me.owner=&owner;me.entry=NPC_WORLDBREAKER_ANNIHILATOR;Hit(f,&p,d);fixed=!d&&f.died&&f.selfActions==1&&ownerActions==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&ownerActions==1&&f.selfActions==1,"one destruction/owner transition");Frame mythic(&me);mythic.mythic=true;me.owner=&p;me.entry=NPC_WORLDBREAKER_DECIMATOR;d=10;Hit(mythic,&p,d);Require(!d&&mythic.haywire&&mythic.casts==1&&ownerActions==1,"mythic other weapon and player owner safe");
#endif
#elif defined(CASE_HYRJAASCENDANCE)
 me.owner=&owner;Hit(f,&p,d);fixed=!d&&f.ascendance&&ownerActions==1;
#ifndef COUNTERFACTUAL
 auto n=f.casts;d=100;Hit(f,&p,d);Require(!d&&f.casts==n&&ownerActions==1,"ascendance once");Frame restored(&me);owner.data=1;restored.Reset();Require(restored.ascendance,"restored spirit latches ascendance");n=ownerActions;d=10;Hit(restored,&p,d);Require(!d&&ownerActions==n,"restored spirit cannot recount");Frame playerOwner(&me);me.owner=&p;playerOwner.Reset();d=10;Hit(playerOwner,&p,d);Require(!d&&playerOwner.ascendance&&ownerActions==n,"player owner cannot be cast to creature AI");
#endif
#elif defined(CASE_ILGYNOTHICHOR)
 Hit(f,&p,d);fixed=!d&&f.explosion&&f.events.scheduled==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&f.events.scheduled==1&&f.casts==1,"explosion/native3s cleanup once");
#endif
#elif defined(CASE_DESOLATERESIDUE)
 me.owner=&owner;Hit(f,&p,d);fixed=!d&&f.died&&notifications==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&notifications==1&&me.q.q.size()==1,"eruption/notification/cleanup once");me.q.Update(5000);Require(despawns==1,"original5s cleanup");Unit residue(4);{auto frame=std::make_unique<Frame>(&residue);d=10;Hit(*frame,&p,d);}residue.q.Update(5000);Require(despawns==2,"unit-owned cleanup survives destroyed AI frame");
#endif
#elif defined(CASE_FESTERFACEGOO)
 Hit(f,&p,d);fixed=!d&&f.casts==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&f.casts==1,"accepted recongealing aura gates repeats");me.auras.clear();castOK=false;Frame retry(&me);d=10;Hit(retry,&p,d);d=10;Hit(retry,&p,d);Require(!d&&retry.casts==2,"failed cast retains native retry");
#endif
#elif defined(CASE_FALLENCRUSADER)
 Hit(f,&p,d);fixed=!d&&f.feignDeath;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&f.casts==1,"native fake death once");
#endif
#elif defined(CASE_COLLAPSINGSTAR)
 Hit(f,&p,d);fixed=!d&&f._dying;
#ifndef COUNTERFACTUAL
 auto n=despawns;d=100;Hit(f,&p,d);Require(!d&&despawns==n,"pending star cannot die early or respawn hole");Frame owned(&me);me.temp=true;me.owner=&owner;d=10;Hit(owned,&p,d);Require(!d&&spawns==1,"native summon owner creates one hole");Frame wrong(&me);me.owner=&p;d=10;Hit(wrong,&p,d);Require(!d&&spawns==1,"non-creature owner cannot spawn a controller-dependent hole");
#endif
#endif
#ifdef COUNTERFACTUAL
 Require(!fixed,"pinned raw hook misses normalized lethal transition");
#else
 Require(fixed,"repaired selected transition");
#endif
 std::cout<<"PASS selected third-five contract\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
