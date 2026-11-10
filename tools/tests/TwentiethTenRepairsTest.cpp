#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>
#include <stdexcept>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
struct ObjectGuid{uint64 v=0;static ObjectGuid const Empty;ObjectGuid(uint64 n=0):v(n){}void Clear(){v=0;}explicit operator bool()const{return v!=0;}bool operator==(ObjectGuid const&o)const{return v==o.v;}};ObjectGuid const ObjectGuid::Empty{};
struct Queue{std::vector<std::pair<uint32,std::function<void()>>>q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,f);}void Update(uint32 n){std::vector<std::function<void()>>v;for(auto i=q.begin();i!=q.end();)if(i->first<=n){v.push_back(i->second);i=q.erase(i);}else{i->first-=n;++i;}for(auto&f:v)f();}};
static uint32 homeCalls=0,lookups=0,credits=0,summonAttempts=0;struct Unit;using Player=Unit;using Creature=Unit;
struct Motion{void MoveTargetedHome(){++homeCalls;}template<class...T>void MovePoint(T...){}};
struct Map{};static Map map20;static Creature*spawnActor=nullptr;static bool spawnOK=false;
struct Unit{ObjectGuid guid;bool world=true,alive=true,player=false,aura=true;Unit*owner=nullptr,*vehicle=nullptr;uint32 mapId=1,instanceId=1,payload=0,despawns=0,objects=0;uint64 hp=100,relativeHp=10;Queue q;Motion motion;Unit(uint64 n):guid(n){}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}bool isDead(){return !alive;}bool IsInWorld(){return world;}bool HasAura(uint32){return aura;}Unit*GetVehicle(){return vehicle;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instanceId;}bool IsWithinDistInMap(Unit*,float){return true;}void AddDelayedEvent(uint32 n,std::function<void()>f){q.Add(n,f);}Unit*FindNearestCreature(uint32,float,bool=true){return spawnActor;}template<class...T>Creature*SummonCreature(T...){++summonAttempts;return spawnOK?spawnActor:nullptr;}void CastSpell(Unit*,uint32,bool=false){++payload;}Motion*GetMotionMaster(){return &motion;}void DespawnOrUnsummon(uint32=0){++despawns;}void RemoveAura(uint32){++payload;}void RemoveAurasDueToSpell(uint32){++payload;}void RemoveAllGameObjects(){++objects;}void AddAura(uint32,Unit*){}void SetDisableGravity(bool){}void SetOrientation(float){}float GetPositionX(){return 0;}float GetPositionY(){return 0;}float GetPositionZ(){return 0;}float GetOrientation(){return 0;}void setFaction(uint32){}template<class...T>void SetHomePosition(T...){}void CombatStop(bool){}void DeleteThreatList(){}uint64 GetHealth(Unit*u=nullptr){return u?relativeHp:hp;}uint32 GetTypeId(){return player?4:3;}Unit*GetOwner(){return owner;}Unit*ToPlayer(){return player?this:nullptr;}Player*GetCharmerOrOwnerPlayerOrPlayerItself(){Unit*u=this;for(int i=0;u&&i<8;++i,u=u->owner)if(u->player)return u;return nullptr;}Player*GetAffectingPlayer(){return GetCharmerOrOwnerPlayerOrPlayerItself();}void KilledMonsterCredit(uint32,ObjectGuid){++credits;}void RewardPlayerAndGroupAtEvent(uint32,Unit*){++credits;}Map*GetMap(){return &map20;}static Unit*GetUnit(Unit&,ObjectGuid);};
static std::map<uint64,Unit*>units;
Unit*Unit::GetUnit(Unit&,ObjectGuid id){++lookups;auto i=units.find(id.v);return i!=units.end()&&i->second->world?i->second:nullptr;}
namespace ObjectAccessor{Unit*GetUnit(Unit&u,ObjectGuid id){return Unit::GetUnit(u,id);}Player*GetPlayer(Unit&u,ObjectGuid id){auto p=Unit::GetUnit(u,id);return p&&p->player?p:nullptr;}Creature*GetCreature(Unit&u,ObjectGuid id){auto p=Unit::GetUnit(u,id);return p&&!p->player?p:nullptr;}}
struct Instance{ObjectGuid claim=1;ObjectGuid GetGuidData(uint32){return claim;}void SetGuidData(uint32,ObjectGuid v){claim=v;}};
struct Events{uint32 scheduled=0,last=0;template<class...T>void RescheduleEvent(uint32 n,T...){++scheduled;last=n;}};
struct Base{Creature*me;Instance script,*instance=&script;Events events;uint32 phase=0,m_uiDistanceCheckTimer=0xCDCDCDCD,m_uiOnyxiaCheckTimer=0xCDCDCDCD,m_uiChromaticCheckTimer=0xCDCDCDCD,uiResurectTimer=0xCDCDCDCD,uiResurectPhase=0xCDCDCDCD,workTimer=1,timer=1,existenceCheckTimer=1000,_existenceCheckTimer=1000,dogTimer=10000,casts=0,EventStarted=0xCDCDCDCD,Step=0xCDCDCDCD,YellTimer=0xCDCDCDCD,FireMedivhTimer=0xCDCDCDCD,FireArcanagosTimer=0xCDCDCDCD;bool onyxiaAlive=false,secondPhase=false,finalPhase=false,work=true,bDog=false;ObjectGuid trappedUnit=2,trappedPlayer=2,_trappedPlayerGUID=2,ArcanagosGUID;explicit Base(Creature*u):me(u){}void DoCast(Unit*u,uint32,bool=true){++casts;if(u)++u->payload;}void Talk(uint32){}bool UpdateVictim(){return false;}void DoMeleeAttackIfReady(){}void EnterEvadeMode(){}template<class...T>Creature*SpawnBalloon(T...){++summonAttempts;return spawnOK?spawnActor:nullptr;}};
static void DoScriptText(uint32,Unit*){}
static void Require(bool ok,char const*s){if(!ok)throw std::runtime_error(s);}
#include "Constants.inc"
#include "Selected.inc"
int main(){try{Unit me(1),p(2),other(3),pet(4),guardian(5),npcOwner(6);p.player=true;pet.owner=&p;guardian.owner=&pet;units={{1,&me},{2,&p},{3,&other},{4,&pet},{5,&guardian}};spawnActor=&other;Frame f(&me);bool fixed=false;
#if defined(CASE_NEFARIANTIMERS)
f.phase=PHASE_GROUND;f.m_uiDistanceCheckTimer=f.m_uiOnyxiaCheckTimer=1000;f.Run(1);fixed=f.m_uiChromaticCheckTimer==0xCDCDCDCD;
#ifndef COUNTERFACTUAL
spawnActor=nullptr;f.phase=PHASE_FLIGHT;f.m_uiChromaticCheckTimer=1;f.Run(1);Require(f.finalPhase&&f.events.scheduled==1,"missing prototypes schedule native landing once");auto before=f.m_uiChromaticCheckTimer;f.Run(30000);Require(f.events.scheduled==1&&f.m_uiChromaticCheckTimer==before,"consumed phase cannot wrap or reschedule");
#endif
#elif defined(CASE_ANNHYLDETIMERS)
f.Reset();fixed=!f.uiResurectTimer&&!f.uiResurectPhase;
#elif defined(CASE_TROLLRETURN)
for(int i=0;i<10;++i)f.UpdateAI(1);fixed=homeCalls==1;
#elif defined(CASE_BALLOONBOARD)
f.Start0(&p);p.q.Update(2000);fixed=p.payload==0;
#ifndef COUNTERFACTUAL
Require(f.timer==2000,"failed summon retains native next-poll delay");spawnOK=true;for(auto fn:{&Frame::Start0,&Frame::Start1,&Frame::Start2,&Frame::Start3,&Frame::Start4,&Frame::Start5,&Frame::Start6}){(f.*fn)(&p,1);p.q.Update(2000);}Require(p.payload==7,"all seven bound balloons preserve their native ride casts");f.Start0(&p);p.aura=false;p.q.Update(2000);Require(p.payload==7,"removed waiting aura cancels boarding");p.aura=true;f.Start0(&p);p.vehicle=&me;p.q.Update(2000);Require(p.payload==7,"existing vehicle cannot be stolen");p.vehicle=nullptr;f.Start0(&p);other.world=false;p.q.Update(2000);Require(p.payload==7,"missing bound balloon cannot select an unrelated replacement");
#endif
#elif defined(CASE_SHANNOXPRISON)||defined(CASE_MALORIAKFREEZE)||defined(CASE_SINDRAGOSATOMB)||defined(CASE_HAGARATOMB)
p.world=false;f.JustDied(&me);f.JustDied(&me);
#ifdef CASE_SHANNOXPRISON
fixed=!f.trappedUnit&&f.casts==1;
#elif defined(CASE_SINDRAGOSATOMB)
fixed=!f._trappedPlayerGUID&&me.objects==1;
#else
fixed=!f.trappedPlayer&&me.despawns==1;
#endif
#ifndef COUNTERFACTUAL
f.SetGUID(ObjectGuid(3),0);Require(f.released,"retired actor ignores a late binding");
#endif
#elif defined(CASE_MEDIVHSEQUENCE)
f.EventStarted=0;f.Step=0;f.StartEvent();fixed=!f.EventStarted&&!f.script.claim&&me.despawns==1;
#ifndef COUNTERFACTUAL
Frame next(&me);next.ResetState();Require(!next.EventStarted&&!next.Step&&!next.YellTimer&&!next.FireMedivhTimer&&!next.FireArcanagosTimer,"idle reset leaves all dialogue/fire clocks disabled");spawnOK=true;auto before=summonAttempts;next.StartEvent();next.StartEvent();Require(next.EventStarted&&summonAttempts==before+1,"successful StartEvent consumes one actor only");other.world=false;Require(next.NextStep(2)==9999999&&!next.EventStarted,"lost partner aborts before credit-capable dialogue");
#endif
#elif defined(CASE_CHAMPIONSURRENDER)
uint32 damage=10;
#ifdef COUNTERFACTUAL
f.DamageTaken(&p,damage,0);
#else
f.DamageTakenAfterScaling(&p,damage,0);
#endif
fixed=damage==0&&credits==1;
#ifndef COUNTERFACTUAL
damage=100;f.DamageTakenAfterScaling(&guardian,damage,0);Require(!damage&&credits==1,"reentrant/guardian final hit cannot double credit");for(Unit*killer:{&pet,&guardian}){Frame fresh(&me);damage=10;auto before=credits;fresh.DamageTakenAfterScaling(killer,damage,0);Require(!damage&&credits==before+1,"pet and nested guardian owner credit");}Frame none(&me);other.owner=&npcOwner;damage=100;none.DamageTakenAfterScaling(&other,damage,0);Require(damage==100,"nonplayer owner cannot crash or receive player progression");
#endif
#endif
#ifdef COUNTERFACTUAL
Require(!fixed,"pinned old selected body reproduces defect");
#else
Require(fixed,"repaired selected contract");
#endif
std::cout<<"PASS selected final-batch contract\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
