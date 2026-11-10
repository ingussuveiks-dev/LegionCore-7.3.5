#include <algorithm>
#include <cstdint>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <stdexcept>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
#include "Constants.inc"
struct ObjectGuid{uint64 v=0;static ObjectGuid const Empty;ObjectGuid(uint64 n=0):v(n){}void Clear(){v=0;}explicit operator bool()const{return v!=0;}bool operator==(ObjectGuid const&o)const{return v==o.v;}bool operator!=(ObjectGuid const&o)const{return v!=o.v;}};ObjectGuid const ObjectGuid::Empty{};
struct Unit;using Player=Unit;using Creature=Unit;static std::map<uint64,Unit*>units;static std::vector<uint32>spells;static uint32 credits=0,evades=0,despawns=0,actions=0;static Unit*victoryTarget=nullptr,*playerFixture=nullptr;static bool castOK=true;
struct Motion{void Clear(){}void MoveTargetedHome(){}template<class...T>void MovePoint(T...){}template<class...T>void MoveFollow(T...){}void MovementExpired(bool){}};
struct Unit{ObjectGuid guid;bool player=false,alive=true,world=true,IsAIEnabled=true,ai=true,combat=false;Unit*owner=nullptr;uint64 hp=100,relativeHp=10;uint32 mapId=1,entry=0,display=0;std::set<uint32>auras;Motion motion;explicit Unit(uint64 n):guid(n){}ObjectGuid GetGUID(){return guid;}ObjectGuid GetOwnerGUID(){return owner?owner->guid:ObjectGuid{};}bool IsControlledByPlayer(){return GetAffectingPlayer()!=nullptr;}Player*GetAffectingPlayer(){if(player)return this;if(owner&&owner->player)return owner;if(owner&&owner->owner&&owner->owner->player)return owner->owner;return nullptr;}Player*ToPlayer(){return player?this:nullptr;}bool IsAlive(){return alive;}bool IsInWorld(){return world;}uint32 GetMap(){return mapId;}uint32 GetEntry(){return entry;}uint64 GetHealth(Unit*u=nullptr){return u?relativeHp:hp;}uint64 GetMaxHealth(){return 100;}void SetHealth(uint64 n){hp=n;relativeHp=std::max<uint64>(1,n/10);}bool HealthBelowPct(uint32 n){return hp<n;}uint64 CountPctFromMaxHealth(uint32 n){return n;}void SetFullHealth(){SetHealth(100);}bool HasAura(uint32 n){return auras.count(n)>0;}void RemoveAura(uint32 n){auras.erase(n);}void RemoveAurasDueToSpell(uint32 n){RemoveAura(n);}void RemoveAllAuras(){auras.clear();}void RemoveAurasAllDots(){}void InterruptNonMeleeSpells(bool){}void CastStop(){}void StopAttack(bool=true,bool=false){combat=false;}void AttackStop(){combat=false;}void CombatStop(bool=true){combat=false;}void RestoreFaction(){}void setFaction(uint32){}void SetReactState(uint32,uint32=0){}void SetDisplayId(uint32 n){display=n;}uint32 GetNativeDisplayId(){return 1;}void SetWalk(bool){}void HandleEmoteCommand(uint32){}template<class...T>void SetFlag(T...){}template<class...T>void RemoveFlag(T...){}template<class...T>void SetHomePosition(T...){}void DespawnOrUnsummon(uint32=0){++despawns;}Motion*GetMotionMaster(){return &motion;}float GetPositionX(){return 0;}float GetPositionY(){return 0;}float GetPositionZ(){return 0;}float GetOrientation(){return 0;}float GetFollowAngle(){return 0;}float GetDistance(Unit*){return 20;}int CastSpell(Unit*u,uint32 n,bool=false,uint32=0,uint32=0,ObjectGuid={}){spells.push_back(n);if(u)u->auras.insert(n);if(u&&u->player)victoryTarget=u;return castOK?0:1;}void RemoveGameObject(uint32,bool){}void KilledMonsterCredit(uint32){++credits;}Unit*AI(){return ai?this:nullptr;}void DoAction(int32){++actions;}void IsSummonedBy(Unit*){++actions;}Unit*FindNearestCreature(uint32 n,float,bool=true){for(auto const&v:units)if(v.second->entry==n&&v.second->alive&&v.second->world)return v.second;return nullptr;}};
namespace ObjectAccessor{Player*GetPlayer(Unit&u,ObjectGuid n){auto i=units.find(n.v);return i!=units.end()&&i->second->player&&i->second->world&&i->second->mapId==u.mapId?i->second:nullptr;}}
static void GetCreatureListWithEntryInGrid(std::list<Creature*>&v,Unit*u,uint32 n,float){for(auto const&p:units)if(p.second!=u&&p.second->entry==n)v.push_back(p.second);}
static void GetPlayerListInGrid(std::list<Player*>&v,Unit*,float){if(playerFixture)v.push_back(playerFixture);}
static uint32 urand(uint32 a,uint32){return a;}
struct Map{std::map<uint32,ObjectGuid>guids;Creature*GetCreature(ObjectGuid n){auto i=units.find(n.v);return i==units.end()?nullptr:i->second;}};
struct Instance{Map*instance;uint32 data=0,boss=0;void SetData(uint32,uint32){++data;}void SetBossState(uint32,uint32){++boss;}ObjectGuid GetGuidData(uint32 n){return instance->guids[n];}};
struct Events{uint32 phase=0,scheduled=0,last=0;void Reset(){}void SetPhase(uint32 n){phase=n;}bool IsInPhase(uint32 n){return phase==n;}template<class...T>void ScheduleEvent(uint32 n,T...){++scheduled;last=n;}template<class...T>void RescheduleEvent(uint32 n,T...){++scheduled;last=n;}};
struct Base{Creature*me;Map map;Instance script{&map},*instance=&script;Events events;bool heroic=true,moveComplete=false,hp75=false,hp50=false,hp30=false,cankill=false,lose=false,m_bIsDuelInProgress=true,LightningCharge=false,Resurrect=false,ResurrectCheck=false,fakeDeath=false,sayDeath=false,EncounterFinish=false,stillInFight=true,eventEmote=false;uint32 moveTimer=0xCDCDCDCD,phase=0,casts=0,talks=0;ObjectGuid targetMove,m_uiDuelerGUID=2;explicit Base(Creature*u):me(u){}bool IsHeroic(){return heroic;}bool UpdateVictim(){return false;}void EnterEvadeMode(){++evades;me->combat=false;m_bIsDuelInProgress=false;m_uiDuelerGUID.Clear();}void Talk(uint32){++talks;}void DoCast(uint32 n){++casts;me->auras.insert(n);}void DoCast(Unit*u,uint32 n,bool=true){++casts;u->auras.insert(n);}};
static void Require(bool ok,char const*s){if(!ok)throw std::runtime_error(s);}
#include "Selected.inc"
template<class F>static void Hit(F&f,Unit*who,uint32&d){
#ifdef COUNTERFACTUAL
 // The old hook sees raw damage before native normalization. Model multiplier0.1.
 d=std::max<uint32>(1,d/10);f.DamageTaken(who,d,0);d*=10;
#else
 f.DamageTakenAfterScaling(who,d,0);
#endif
}
int main(){try{Unit me(1),p(2),other(3),pet(4),guardian(5),a(6),b(7);p.player=other.player=true;pet.owner=&p;guardian.owner=&pet;units={{1,&me},{2,&p},{3,&other},{4,&pet},{5,&guardian},{6,&a},{7,&b}};playerFixture=&p;Frame f(&me);uint32 d=10;bool fixed=false;
#if defined(CASE_ENERGIZEDPHOENIX)
 Hit(f,&guardian,d);fixed=!d&&me.display==66325;
#ifndef COUNTERFACTUAL
 auto n=spells.size();d=100;Hit(f,&p,d);Require(!d&&spells.size()==n&&f.sleeping&&f.moveComplete,"sleep consumed once;no continued movement poll");me.RemoveAura(SPELL_SLEEP_ADDS);f.IsSummonedBy(&me);Require(!f.sleeping&&!f.moveComplete&&f.moveTimer==2000,"native wake rearms original2s move timer");f.Reset();Require(f.moveComplete&&!f.moveTimer&&!f.sleeping,"unsummoned reset disables movement clock");
#endif
#elif defined(CASE_ARTRUISRELEASE)
 Hit(f,&guardian,d);fixed=!d;
#ifndef COUNTERFACTUAL
 Require(f.releasePending&&!f.hp30,"missing actors cannot consume release");a.entry=NPC_JALOOT;b.entry=NPC_ZEPHIK;b.IsAIEnabled=false;f.UpdateAI(1);Require(!f.hp30&&!actions,"both actors need active AI");b.IsAIEnabled=true;f.UpdateAI(1);Require(f.hp30&&!f.releasePending&&actions==2,"actual update preamble retries even under shield");f.UpdateAI(1);d=10;Hit(f,&p,d);Require(!d&&actions==2&&!f.releasePending,"accepted release stays committed while shield holds");f.DoAction(ACTION_1);d=10;Hit(f,&p,d);Require(d==10&&f.cankill,"native selected branch unlock permits real kill");f.Reset();Require(!f.hp30&&!f.cankill&&!f.releasePending,"abandon/reset clears pending work");
#endif
#elif defined(CASE_KORAGHANOMALY)
 Hit(f,&p,d);fixed=f.m_Exploded&&spells.size()==2&&d==10;
#ifndef COUNTERFACTUAL
 auto n=spells.size();d=10;Hit(f,&guardian,d);Require(spells.size()==n&&d==10,"explosion once while native death damage remains");Frame silent(&me);me.auras.insert(SuppressionFieldSilence);d=10;Hit(silent,&pet,d);Require(!silent.m_Exploded&&spells.size()==n&&d==10,"native silence field suppresses death explosion");
#endif
#elif defined(CASE_BROKENDUEL)
 Hit(f,&guardian,d);fixed=!d&&f.lose&&victoryTarget==&p;
#ifndef COUNTERFACTUAL
 auto n=spells.size();auto c=credits;d=100;Hit(f,&pet,d);Require(!d&&spells.size()==n&&credits==c,"late pet hit cannot repeat victory/credit");Frame outsider(&me);d=10;Hit(outsider,&other,d);Require(!d&&!outsider.lose,"foreign challenger rejected");d=10;Hit(outsider,nullptr,d);Require(!d&&!outsider.lose,"null/unowned damage cannot steal round");Frame lost(&me);p.mapId=2;d=10;Hit(lost,&guardian,d);Require(!d&&!lost.lose,"departed owner cannot claim progression");lost.Tick();Require(!lost.m_bIsDuelInProgress,"missing challenger abandons through native evade");p.mapId=1;Frame dead(&me);p.alive=false;d=10;Hit(dead,&guardian,d);Require(!d&&!dead.lose,"dead owner cannot consume victory");p.alive=true;Frame idle(&me);idle.m_bIsDuelInProgress=false;d=9;Hit(idle,&p,d);Require(d==9,"ordinary non-duel damage unchanged");
#ifdef CASE_BROKENDUEL
 Require(credits==1,"native108722 credit reaches player owner once");
#endif
#endif
#elif defined(CASE_NEPTULONFAILURE)
 Hit(f,&p,d);fixed=!d&&evades==1;
#elif defined(CASE_BLAZEEGG)
 f.events.SetPhase(PHASE_BLAZE);Hit(f,&pet,d);fixed=!d&&f.events.IsInPhase(PHASE_EGG);
#ifndef COUNTERFACTUAL
 auto n=f.casts;d=10;Hit(f,&guardian,d);Require(!d&&f.casts==n,"egg cannot transform twice");
#endif
#elif defined(CASE_SIAMATCHARGE)
 Hit(f,&pet,d);fixed=!d&&f.LightningCharge&&f.events.scheduled==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&guardian,d);Require(!d&&f.events.scheduled==1,"charge/death clock scheduled once");Frame normal(&me);normal.heroic=false;d=10;Hit(normal,&p,d);Require(d==10&&!normal.LightningCharge,"normal servant retains ordinary death");Frame absent(&me);absent.instance=nullptr;d=10;Hit(absent,&p,d);Require(!d&&absent.events.scheduled==1,"missing instance cannot crash death cleanup");
#endif
#elif defined(CASE_WHITEMANERESURRECTION)
 Hit(f,&p,d);fixed=!d&&f.Resurrect&&f.ResurrectCheck;
#ifndef COUNTERFACTUAL
 auto n=f.casts;d=10;Hit(f,&guardian,d);Require(!d&&f.casts==n,"resurrection hold is independent of repeated hits");f.MovementInform(POINT_MOTION_TYPE,1);Require(!f.ResurrectCheck,"actual authored arrival releases hold");d=100;Hit(f,&p,d);Require(d==100,"real final death allowed after resurrection");me.SetFullHealth();Frame1 durand(&me);d=10;Hit(durand,&pet,d);Require(!d&&durand.fakeDeath&&durand.sayDeath,"Durand scaled exact hit fakes death");auto talks=durand.talks;d=10;Hit(durand,&guardian,d);Require(!d&&durand.talks==talks,"Durand fake-death speech once");
#endif
#elif defined(CASE_SNOWDRIFTSURRENDERS)
 Hit(f,&guardian,d);fixed=!d&&f.EncounterFinish&&f.script.boss==1;
#ifndef COUNTERFACTUAL
 d=10;Hit(f,&p,d);Require(!d&&f.script.boss==1,"boss completion once");me.SetFullHealth();Frame1 novice(&me);d=10;Hit(novice,&pet,d);Require(!d&&!novice.stillInFight&&novice.script.data==1&&novice.events.scheduled==1,"novice tracked surrender once");d=100;Hit(novice,&guardian,d);Require(!d&&novice.script.data==1&&novice.events.scheduled==1,"late hits cannot restart novice jump");me.SetFullHealth();Frame2 mini(&me);d=10;Hit(mini,&guardian,d);Require(!d&&!mini.stillInFight&&mini.script.data==1,"miniboss counted once");d=100;Hit(mini,&p,d);Require(!d&&mini.script.data==1,"retired miniboss survives late hits");Frame2 absent(&me);absent.instance=nullptr;d=10;Hit(absent,&p,d);Require(!d&&!absent.stillInFight,"missing instance/position cleanup safe");
#endif
#elif defined(CASE_FELMYSTFLIGHT)
 f.phase=PHASE_FLIGHT;Hit(f,&pet,d);fixed=!d;
#ifndef COUNTERFACTUAL
 f.phase=PHASE_GROUND;d=10;Hit(f,&p,d);Require(d==10,"native ground phase allows ordinary lethal death");
#endif
#endif
#ifdef COUNTERFACTUAL
 Require(!fixed,"pinned raw hook reproduces missed final normalized lethal transition");
#else
 Require(fixed,"repaired selected contract");
#endif
 std::cout<<"PASS selected second-five contract\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
