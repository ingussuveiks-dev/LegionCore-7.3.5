#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <functional>
#include <list>
#include <map>
#include <vector>
#include <string>
#include <iostream>
using uint8=uint8_t;using int8=int8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using ObjectGuid=struct Guid;using CriteriaTimedTypes=uint32;
struct Guid{uint64 value=0;Guid()=default;Guid(uint64 v):value(v){}void Clear(){value=0;}bool operator==(Guid const&o)const{return value==o.value;}bool operator!=(Guid const&o)const{return !(*this==o);}bool operator<(Guid const&o)const{return value<o.value;}};
struct Unit;using Player=Unit;using Creature=Unit;using WorldObject=Unit;struct Base;using InstanceScript=Base;
struct Queue{std::vector<std::pair<uint32,std::function<void()>>>q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,std::move(f));}void Update(uint32 n){std::vector<std::function<void()>>ready;for(auto it=q.begin();it!=q.end();)if(it->first<=n){ready.push_back(std::move(it->second));it=q.erase(it);}else{it->first-=n;++it;}for(auto&f:ready)f();}void Clear(){q.clear();}};
static uint32 invalidAccess=0,conversations=0,achievements=0;static bool createOK=true;
struct Position{float x=10,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}};
struct Motion{uint32 paths=0;void MovePath(uint32,bool,int,int){++paths;}};
struct Achievement{void StartTimedAchievement(CriteriaTimedTypes,uint32){++achievements;}};
struct Unit{Guid guid;bool player=true,alive=true,world=true,invalid=false;uint32 mapId=1,instanceId=1,casts=0,removed=0,despawned=0,controls=0,restores=0,summonAttempts=0;Queue queue;Unit*vehicle=nullptr,*owner=nullptr;Base*script=nullptr;Motion motion;Achievement achievement;Position pos;Unit(uint64 g=1):guid(g){}Guid GetGUID(){if(invalid)++invalidAccess;return guid;}bool IsPlayer(){return player;}Unit*ToPlayer(){if(!player)++invalidAccess;return this;}bool IsAlive(){if(invalid)++invalidAccess;return alive;}bool IsInWorld(){return world;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instanceId;}Base*GetInstanceScript(){return script;}Unit*GetVehicleBase(){return vehicle;}void SetClientControl(Unit*,bool apply){if(invalid)++invalidAccess;if(apply)++restores;else ++controls;}void AddDelayedEvent(uint32 n,std::function<void()>f){queue.Add(n,std::move(f));}Position GetPosition(){if(invalid)++invalidAccess;return pos;}float GetPositionX(){return pos.x;}float GetPositionY(){return pos.y;}float GetPositionZ(){return pos.z;}template<class...T>void CastSpell(T...){if(invalid||!world)++invalidAccess;++casts;}void CastSpell(Position,uint32,bool){if(invalid||!world)++invalidAccess;++casts;}void RemoveAurasDueToSpell(uint32){if(invalid)++invalidAccess;++removed;}void RemoveFlag(uint32,uint32){}void DespawnOrUnsummon(uint32=0){++despawned;}Unit*GetAnyOwner(){return owner;}bool HasAura(uint32){return false;}Motion*GetMotionMaster(){return &motion;}Creature*SummonCreature(uint32,Position,Guid,uint32,uint32){if(!world)++invalidAccess;++summonAttempts;return nullptr;}Achievement*GetAchievementMgr(){return &achievement;}};
static std::map<Guid,Unit*>units;
namespace ObjectAccessor{Unit*GetUnit(Unit&me,Guid g){auto it=units.find(g);return it!=units.end()&&it->second->world&&it->second->mapId==me.mapId&&it->second->instanceId==me.instanceId?it->second:nullptr;}Unit*GetCreature(Unit&me,Guid g){auto u=GetUnit(me,g);return u&&!u->player?u:nullptr;}Unit*GetPlayer(Unit&me,Guid g){auto u=GetUnit(me,g);return u&&u->player?u:nullptr;}}
struct Map{uint32 id=1,instanceId=1;std::vector<Unit*>players;uint32 GetId(){return id;}uint32 GetInstanceId(){return instanceId;}template<class F>void ApplyOnEveryPlayer(F f){for(auto p:players)f(p);}};
struct SpellInfo{uint32 Id=246347;};struct Conversation{bool CreateConversation(uint64,uint32,Unit*,void*,Unit&){++conversations;return createOK;}};
namespace HighGuid{struct Conversation{};}struct Generator{uint64 Generate(){return 1;}};struct ObjMgr{template<class T>Generator*GetGenerator(){static Generator g;return &g;}}objects;auto sObjectMgr=&objects;
struct Scenario{Achievement achievement;Achievement&GetAchievementMgr(){return achievement;}}scenario;struct ScenarioManager{Scenario*GetScenario(uint32){return &scenario;}}scenarios;auto sScenarioMgr=&scenarios;
uint32 urand(uint32 a,uint32){return a;}int irand(int a,int){return a;}float frand(float a,float){return a;}
struct Base{Unit*me;Map ownMap,*instance=&ownMap;bool convers=false;uint32 bossState=0,talks=0,events=0;bool isEmpowered[2]{false,true};std::map<Guid,std::vector<Guid>>addsBoundsOfFel;Unit*selection=nullptr;std::vector<Conversation*>retained;Base(Unit*u):me(u){}virtual~Base()=default;uint32 GetBossState(uint32){return bossState;}void Talk(uint32,Guid){++talks;}void DoCast(Unit*u,uint32,bool){if(u->invalid||!u->world)++invalidAccess;++u->casts;}Unit*SelectTarget(uint32,uint32,float,bool){return selection;}};
#include "Constants.inc"
#include "Selected.inc"
void Check(bool ok,char const*why){if(!ok){std::cerr<<why<<'\n';std::abort();}}
int main(){Unit me(1),p(2),summon(3);me.player=summon.player=false;units[me.guid]=&me;units[p.guid]=&p;units[summon.guid]=&summon;Frame f(&me);p.pos.x=20;p.script=&f;me.owner=&p;f.ownMap.players={&p};f.selection=&p;
#if defined(CASE_PYLON)
 Unit living(4),dead(5),pet(6);dead.alive=false;pet.player=false;std::list<Unit*>targets{&living,&dead,&pet};f.FilterTargets(targets);
#ifdef COUNTERFACTUAL
 Check(invalidAccess==1,"original nonplayer cast dereference counterfactual");
#else
 Check(targets.size()==1&&targets.front()==&dead&&!invalidAccess,"only dead players");targets={nullptr,&pet};f.FilterTargets(targets);Check(targets.empty(),"null/nonplayer empty");
#endif
#elif defined(CASE_QUILLS)
 f.Start(&p,&summon);p.invalid=true;p.world=false;units.erase(p.guid);me.queue.Update(5000);
#ifdef COUNTERFACTUAL
 Check(invalidAccess>0,"original retained target read");
#else
 Check(!invalidAccess&&me.queue.q.empty(),"missing target no stale position");p.invalid=false;p.world=true;units[p.guid]=&p;f.Start(&p,&summon);me.queue.Update(5000);Check(!me.queue.q.empty(),"existing target keeps authored volley");me.queue.Update(10000);Check(me.casts>0,"volley retained");uint32 previous=me.casts;me.alive=false;f.Start(&p,&summon);me.queue.Update(5000);Check(me.casts==previous&&me.queue.q.empty(),"dead boss no secondary volley");me.alive=true;
#endif
#elif defined(CASE_HELLFIRE) || defined(CASE_BOUNDS)
 f.Start(&p,&summon);me.world=false;me.queue.Clear();p.queue.Update(3000);summon.queue.Update(3000);
#ifdef COUNTERFACTUAL
 Check(invalidAccess>0,"foreign queue executes against retired boss");
#else
 Check(!invalidAccess,"boss-owned callback cancelled on retirement");me.world=true;f.Start(&p,&summon);me.queue.Update(3000);
#if defined(CASE_BOUNDS)
 Check(p.casts>0&&me.summonAttempts==2&&me.casts>0,"original bounds endpoints");
#else
 Check(me.casts>0&&summon.removed==1&&summon.despawned==1,"original hellfire endpoint and cleanup");
#endif
 me.alive=false;uint32 previous=me.casts,attempts=me.summonAttempts;f.Start(&p,&summon);me.queue.Update(3000);Check(me.casts==previous&&me.summonAttempts==attempts,"dead boss no combat payload");me.alive=true;p.world=false;summon.world=false;uint32 calls=me.casts;f.Start(&p,&summon);me.queue.Update(3000);Check(me.casts==calls,"missing delayed actor skips payload");
#endif
#elif defined(CASE_SEATINTRO)
 f.OnPlayerEnter(&p);p.mapId=2;p.queue.Update(3500);
#ifdef COUNTERFACTUAL
 Check(conversations==1&&f.convers,"original crosses map and consumes intro");
#else
 Check(!conversations&&!f.convers,"foreign map rejected");p.mapId=1;createOK=false;f.OnPlayerEnter(&p);p.queue.Update(3500);Check(conversations==1&&!f.convers,"failed create leaves retry");createOK=true;f.OnPlayerEnter(&p);f.OnPlayerEnter(&p);p.queue.Update(3500);Check(conversations==2&&f.convers,"queued entries exactly once");
#endif
#elif defined(CASE_STYLEGREETING)
 f.Start(&p);p.invalid=true;p.world=false;units.erase(p.guid);me.queue.Update(2000);
#ifdef COUNTERFACTUAL
 Check(invalidAccess==1&&f.talks==1,"original delayed GUID reads retired player");
#else
 Check(!invalidAccess&&!f.talks,"departed contestant skipped");p.invalid=false;p.world=true;units[p.guid]=&p;for(uint32 n=0;n<9;++n){f.Start(&p);me.queue.Update(2000);}Check(f.talks==9,"all identical authored greeting sites");
#endif
#elif defined(CASE_STORMDRAGON)
 p.vehicle=&me;f.PassengerBoarded(&p,0,true);p.vehicle=nullptr;f.PassengerBoarded(&p,0,false);me.queue.Update(2000);me.queue.Update(3000);
#ifdef COUNTERFACTUAL
 Check(p.controls==2&&me.motion.paths==2,"original removal schedules duplicate stale control/flight");
#else
 Check(!p.controls&&!me.motion.paths,"remove cancels pending boarding");p.vehicle=&me;f.PassengerBoarded(&p,0,true);f.PassengerBoarded(&p,0,true);me.queue.Update(2000);Check(p.controls==1,"only one issued control");p.vehicle=nullptr;f.PassengerBoarded(&p,0,false);me.queue.Update(3000);Check(p.restores==1&&!me.motion.paths,"exit restores control and cancels path");p.vehicle=&me;f.PassengerBoarded(&p,0,true);me.queue.Update(2000);me.queue.Update(3000);Check(p.controls==2&&me.motion.paths==1,"fresh boarding retains2s+3s route");
#endif
#elif defined(CASE_HEARTHSTATION) || defined(CASE_SPIDER)
 f.IsSummonedBy(&p);p.invalid=true;p.world=false;units.erase(p.guid);me.queue.Update(100);
#ifdef COUNTERFACTUAL
 Check(invalidAccess>0,"original delayed owner access");
#else
 Check(!invalidAccess&&!p.casts,"departed owner skipped");p.invalid=false;p.world=true;units[p.guid]=&p;f.IsSummonedBy(&p);me.queue.Update(100);Check(p.casts==1,"original ride spell retained");
#if defined(CASE_HEARTHSTATION)
 SpellInfo spell;f.SpellFinishCast(&spell);me.queue.Update(1500);Check(p.removed==2,"both owned finish auras removed");me.owner=nullptr;f.UpdateAI(1000);Check(me.despawned==1&&f.despawn_timer==1000,"deterministic orphan timeout");
#else
 Check(f.check==200,"existing spider retry unchanged");
#endif
#endif
#elif defined(CASE_STYLETIMER)
 f.Start();p.instanceId=2;p.queue.Update(500);
#ifdef COUNTERFACTUAL
 Check(achievements==2,"original timer follows player into another instance");
#else
 Check(!achievements,"foreign instance no timed criteria");p.instanceId=1;f.Start();p.queue.Update(500);Check(achievements==2,"original player and scenario timer endpoints");
#endif
#endif
 std::cout<<"PASS selected source callback regression;controlled payloads and counterfactual instrumentation,not client traversal\n";
}
