#include <algorithm>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <stdexcept>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
#include "Constants.inc"
struct ObjectGuid{static ObjectGuid const Empty;uint64 v=0;ObjectGuid(uint64 n=0):v(n){}void Clear(){v=0;}explicit operator bool()const{return v!=0;}bool operator==(ObjectGuid const&o)const{return v==o.v;}bool operator!=(ObjectGuid const&o)const{return v!=o.v;}bool operator<(ObjectGuid const&o)const{return v<o.v;}};
ObjectGuid const ObjectGuid::Empty{};
struct Queue{std::vector<std::pair<uint32,std::function<void()>>>q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,f);}void Update(uint32 n){std::vector<std::function<void()>>v;for(auto i=q.begin();i!=q.end();)if(i->first<=n){v.push_back(i->second);i=q.erase(i);}else{i->first-=n;++i;}for(auto&f:v)f();}};
struct Unit;using Player=Unit;using Creature=Unit;struct EventObject;struct InstanceScript;
struct Map{InstanceScript*script=nullptr;std::map<uint64,Unit*>units;std::map<uint64,EventObject*>objects;std::map<uint32,int>states;Creature*GetCreature(ObjectGuid);EventObject*GetEventObject(ObjectGuid);template<class...T>void LoadGrid(T...){} };using InstanceMap=Map;
static std::vector<Unit*>allocated;static uint32 credits=0,loots=0,spawns=0,despawns=0,conversations=0;static int failSpawn=-1;static bool castOK=true,conversationOK=true;static std::vector<uint32>casts;static std::map<uint32,uint32>convoCounts;
struct Motion{template<class...T>void MovePoint(T...){}void MoveTargetedHome(){}};
struct HostileReference{ObjectGuid guid;ObjectGuid getUnitGuid(){return guid;}};struct Threat{std::list<HostileReference*>refs;auto getThreatList(){return refs;}};
struct Unit{ObjectGuid guid;bool world=true,alive=true,player=false,combat=false,gossip=true;Unit*owner=nullptr;Map*map=nullptr;uint64 hp=100,relativeHp=10;uint32 payload=0;Queue q;Motion motion;Threat threat;
 explicit Unit(uint64 n):guid(n){}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}bool IsInWorld(){return world;}bool isInCombat(){return combat;}uint64 GetHealth(Unit*u=nullptr){return u?relativeHp:hp;}uint64 GetMaxHealth(){return 100;}void SetHealth(uint64 n){hp=n;}void SetFullHealth(){hp=100;}bool HealthBelowPct(uint32 n){return hp<n;}uint32 GetEntry(){return 66138;}uint32 GetTypeId(){return player?4:3;}bool IsPlayer(){return player;}Player*ToPlayer(){return player?this:nullptr;}Unit*GetAnyOwner(){return owner;}Unit*GetOwner(){return owner;}
 Player*GetAffectingPlayer(){if(player)return this;if(owner&&owner->player)return owner;if(owner&&owner->owner&&owner->owner->player)return owner->owner;return nullptr;}
 Player*GetCharmerOrOwnerPlayerOrPlayerItself(){return player?this:owner&&owner->player?owner:nullptr;}
 Map*GetMap(){return map;}InstanceScript*GetInstanceScript(){return map?map->script:nullptr;}void AddDelayedEvent(uint32 n,std::function<void()>f){q.Add(n,f);}int CastSpell(Unit*,uint32 n,bool=false){casts.push_back(n);++payload;return castOK?0:1;}void CreateConversation(uint32 n){++conversations;++convoCounts[n];}bool HasAura(uint32){return false;}void GetCreatureListWithEntryInGrid(std::list<Creature*>&,uint32,float){}
 static Player*GetPlayer(Unit&u,ObjectGuid n){auto*p=u.map->GetCreature(n);return p&&p->player?p:nullptr;}void KilledMonsterCredit(uint32,ObjectGuid={}){++credits;}void RewardPlayerAndGroupAtEvent(uint32,Unit*){++credits;}void GeneratePersonalLoot(Unit*,Player*p){if(p&&p->player)++loots;}void DespawnOrUnsummon(uint32=0){++despawns;}void setFaction(uint32){}void StopAttack(){combat=false;}void HandleEmoteCommand(uint32){}void CombatStop(bool){combat=false;}void DeleteThreatList(){}void RemoveAllAuras(){}void SetVisible(bool){}void SetReactState(uint32){}void setActive(bool){}template<class...T>void SetFlag(T...){gossip=true;}template<class...T>void RemoveFlag(T...){gossip=false;}void SetInCombatWith(Unit*){combat=true;}template<class...T>void SetHomePosition(T...){}Unit const&GetPosition(){return *this;}float GetPositionX(){return 0;}float GetPositionY(){return 0;}float GetPositionZ(){return 0;}float GetOrientation(){return 0;}Motion*GetMotionMaster(){return &motion;}Unit*AI(){return this;}Unit*GetAI(){return this;}void Talk(uint32){}void ZoneTalk(uint32){++payload;}void DoAction(uint32){}Threat&getThreatManager(){return threat;}
};
Creature*Map::GetCreature(ObjectGuid n){auto i=units.find(n.v);return i!=units.end()&&i->second->world?i->second:nullptr;}
struct Position{float x,y,z,o;};
struct EventObject{Map*map;uint32 entry;ObjectGuid guid;std::set<ObjectGuid>affectedPlayers,affectedPermamentPlayers;EventObject(Map*m,uint32 e):map(m),entry(e),guid(e){map->objects[e]=this;}ObjectGuid GetGUID(){return guid;}Map*GetMap(){return map;}uint32 GetEntry(){return entry;}Position GetPosition(){return{};}template<class...T>Creature*SummonCreature(T...){++spawns;if(int(spawns)==failSpawn)return nullptr;auto*p=new Unit(10000+spawns);allocated.push_back(p);p->map=map;map->units[p->guid.v]=p;return p;}};
EventObject*Map::GetEventObject(ObjectGuid n){auto i=objects.find(n.v);return i==objects.end()?nullptr:i->second;}
namespace ObjectAccessor{Player*FindPlayer(Map*m,ObjectGuid n){auto p=m->GetCreature(n);return p&&p->player?p:nullptr;}Unit*GetUnit(Unit&u,ObjectGuid n){return u.map->GetCreature(n);}}
struct InstanceScript{Map*instance;Queue q;explicit InstanceScript(Map*m):instance(m){m->script=this;}virtual~InstanceScript(){}void AddDelayedEvent(uint32 n,std::function<void()>f){q.Add(n,f);}int GetBossState(uint32 n){return instance->states[n];}ObjectGuid GetGuidData(uint32 n){return n;}};
enum class HighGuid{Conversation};struct Generator{uint64 Generate(){return 1;}};struct ObjectMgr{template<HighGuid>Generator*GetGenerator(){static Generator g;return &g;}};static ObjectMgr obj,*sObjectMgr=&obj;
struct Conversation{template<class...T>bool CreateConversation(uint64,uint32 n,Player*,T...){if(conversationOK){++conversations;++convoCounts[n];delete this;return true;}return false;}};
static float frand(float a,float){return a;}
struct Events{template<class...T>void RescheduleEvent(T...){}};struct Summons{void DespawnAll(){}};
struct Base{Creature*me;bool EventProgress=true,bEvent_1=false,bEvent_2=false,ChargeEvent=false,_evadeCheck=true;uint32 uiChargeTimer=0,uiShieldBreakerTimer=0,uiDefendTimer=0;Events events;Summons summons;explicit Base(Creature*u):me(u){}void Talk(uint32){}void DoCast(uint32){}void EnterEvadeMode(){me->combat=false;}void AttackStart(Unit*){}void DoZoneInCombat(){throw std::runtime_error("outdoor zone-combat call");}};
static void DoScriptText(int32,Unit*){}
static void Require(bool ok,char const*s){if(!ok)throw std::runtime_error(s);}
#include "Header.inc"
#include "Selected.inc"
static void Hit(Frame&f,Unit*k,uint32&d){
#if !defined(CASE_ANTORUSINTRO)&&!defined(CASE_TSULONGEVADE)
#ifdef COUNTERFACTUAL
 f.DamageTaken(k,d,0);
#else
 f.DamageTakenAfterScaling(k,d,0);
#endif
#else
 (void)f;(void)k;(void)d;
#endif
}
int main(){try{Map map,otherMap;Unit me(1),p(2),other(3),pet(4),guardian(5),npc(6);p.player=other.player=true;pet.owner=&p;guardian.owner=&pet;for(Unit*u:{&me,&p,&other,&pet,&guardian,&npc}){u->map=&map;map.units[u->guid.v]=u;}Frame f(&me);bool fixed=false;uint32 d=10;
#if defined(CASE_ANTORUSINTRO)
 EventObject eo(&map,801),next(&map,806);
#ifdef COUNTERFACTUAL
 InstanceScript inst(&map),second(&otherMap);f.OnTrigger(&p,&eo,true);f.OnTrigger(&p,&next,true);p.q.Update(1000);fixed=casts.size()==1;
#else
 AntorusIntroInstance inst(&map),second(&otherMap);
 auto trigger=[&](EventObject&e,Player*target=nullptr){if(!target)target=&p;f.OnTrigger(target,&e,true);e.affectedPlayers.insert(target->guid);e.affectedPermamentPlayers.insert(target->guid);};
 trigger(eo);trigger(next);inst.q.Update(1000);fixed=inst.IntroPlayed(801)&&inst.IntroPlayed(806)&&casts.size()==1;
 trigger(eo);inst.q.Update(1000);Require(convoCounts[5524]==1,"repeat entry cannot repeat accepted conversation");
 Unit p2(20);p2.player=true;p2.map=&otherMap;otherMap.units[20]=&p2;EventObject eo2(&otherMap,801);trigger(eo2,&p2);second.q.Update(1000);Require(convoCounts[5524]==2,"separate raids own separate progress");
 EventObject gate(&map,803);trigger(gate);Require(gate.affectedPermamentPlayers.count(p.guid),"native dispatcher records rejection after callback");inst.q.Update(1);Require(!gate.affectedPermamentPlayers.count(p.guid)&&!gate.affectedPlayers.count(p.guid),"failed prerequisite clears both native admission sets");map.states[DATA_HASABEL]=DONE;trigger(gate);Require(inst.IntroPlayed(803),"real prerequisite then successful actor accepts");
 EventObject fail(&map,808);castOK=false;trigger(fail);inst.q.Update(1000);inst.q.Update(1);Require(!inst.IntroPlayed(808)&&!fail.affectedPermamentPlayers.count(p.guid),"failed cast retries");castOK=true;trigger(fail);inst.q.Update(1000);Require(inst.IntroPlayed(808),"cast retry succeeds");
 EventObject leave(&map,811);trigger(leave);p.map=&otherMap;inst.q.Update(1000);p.map=&map;inst.q.Update(1);Require(!inst.IntroPlayed(811)&&!leave.affectedPermamentPlayers.count(p.guid),"map change cancels player step and rearms native source");trigger(leave);p.alive=false;inst.q.Update(1000);p.alive=true;inst.q.Update(1);Require(!inst.IntroPlayed(811),"death cancels delayed player effects");
 EventObject trio(&map,810);uint32 before=spawns,clean=despawns;failSpawn=int(before+2);trigger(trio);inst.q.Update(1);Require(spawns==before+3&&despawns==clean+2&&!inst.IntroPlayed(810),"partial trio cleaned before retry");failSpawn=-1;castOK=false;trigger(trio);inst.q.Update(1000);inst.q.Update(1);before=spawns;castOK=true;trigger(trio);inst.q.Update(1000);Require(inst.IntroPlayed(810)&&spawns==before,"failed visual retry does not duplicate accepted trio");
 EventObject ordered(&map,812);trigger(ordered);inst.q.Update(1000);Require(!inst.IntroPlayed(812)&&casts.back()==250700,"native first cast precedes50s followup");castOK=false;inst.q.Update(50000);inst.q.Update(1);castOK=true;auto count=std::count(casts.begin(),casts.end(),uint32(250700));trigger(ordered);inst.q.Update(1000);inst.q.Update(50000);Require(inst.IntroPlayed(812)&&std::count(casts.begin(),casts.end(),uint32(250700))==count,"second-stage retry retains first receipt");
 EventObject intro(&map,800);conversationOK=false;trigger(intro);inst.q.Update(1);Require(!inst.IntroPlayed(800),"failed initial conversation not consumed");conversationOK=true;trigger(intro);conversationOK=false;inst.q.Update(7000);inst.q.Update(1);conversationOK=true;trigger(intro);inst.q.Update(7000);Require(inst.IntroPlayed(800)&&convoCounts[5545]==1&&convoCounts[5522]==1,"accepted first conversation retained across second failure");
 EventObject world(&map,802);trigger(world);p.map=&otherMap;inst.q.Update(8000);inst.q.Update(1);p.map=&map;Require(!inst.IntroPlayed(802),"missing world boss rearms");Unit hasabel(NPC_HASABEL);hasabel.map=&map;map.units[NPC_HASABEL]=&hasabel;trigger(world);p.map=&otherMap;inst.q.Update(8000);p.map=&map;Require(inst.IntroPlayed(802)&&hasabel.payload==1,"shared world event survives initiator departure");
 EventObject imonar(&map,807);Unit boss(NPC_IMONAR);boss.map=&map;map.units[NPC_IMONAR]=&boss;conversationOK=false;trigger(imonar);p.map=&otherMap;inst.q.Update(9000);p.map=&map;Require(inst.IntroPlayed(807)&&boss.payload==1,"accepted world intro completes despite optional conversation and departed initiator");
 for(Unit*u:allocated)delete u;
#endif
#elif defined(CASE_TSULONGEVADE)
 f.Reset();fixed=!f._evadeCheck;
#elif defined(CASE_CITYVALIANT)
#ifdef COUNTERFACTUAL
 f.DamageTaken(&p,d,0);fixed=d==0;
#else
 f.Reset();Require(f.StartChallenge(&p),"native gossip starts actual challenger");Hit(f,&other,d);Require(!d&&p.payload==0,"other player cannot steal victory");d=10;Hit(f,&guardian,d);fixed=!d&&p.payload==1;d=100;Hit(f,&p,d);Require(!d&&p.payload==1,"repeat hit no duplicate mark");f.Reset();Require(f.StartChallenge(&p),"new challenge after reset");f.KilledUnit(&pet);Require(f.challenger==p.guid,"pet loss is not trainee death");f.KilledUnit(&p);Require(!f.challenger&&f.defeated&&me.gossip&&!me.combat,"challenger death retires round and restores retry gossip");Require(f.StartChallenge(&p),"new round after loss without extra reset");me.combat=false;p.alive=false;Require(!f.StartChallenge(&p),"dead challenger rejected");p.alive=true;p.map=&otherMap;Require(!f.StartChallenge(&p),"foreign map rejected");
#endif
#elif defined(CASE_TOURNAMENTHELPER)
 Hit(f,&p,d);fixed=d==9;
#ifndef COUNTERFACTUAL
 me.relativeHp=1;d=1;Hit(f,&p,d);Require(!d,"one-health exact lethal never underflows");me.relativeHp=10;d=9;Hit(f,&p,d);Require(d==9,"ordinary sublethal unchanged");
#endif
#elif defined(CASE_ARGENTVALIANT)||defined(CASE_CHENGCHALLENGE)
 me.owner=&p;Hit(f,&guardian,d);fixed=!d&&credits==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&pet,d);Require(!d&&credits==1,"pet/reentrant hit cannot double-credit");Frame fresh(&me);d=10;Hit(fresh,&p,d);Require(!d&&credits==2,"actual player exact lethal");Frame invalid(&me);me.owner=nullptr;d=100;Hit(invalid,&npc,d);Require(credits==2,"unowned NPC cannot gain progression");
#endif
#else
 HostileReference h0{npc.guid},h1{pet.guid},h2{p.guid},h3{other.guid},h4{ObjectGuid(999)},h5{guardian.guid};me.threat.refs={&h0,&h1,&h4,&h2,&h5,&h3};Hit(f,&guardian,d);fixed=!d&&credits==2&&loots==1;
#ifndef COUNTERFACTUAL
 d=100;Hit(f,&p,d);Require(!d&&credits==2&&loots==1,"one victory/loot despite repeated final hits");Frame invalid(&me);d=10;Hit(invalid,&npc,d);Require(!d&&credits==2&&loots==1,"unowned lethal damage preserves actor without player progression");
#endif
#endif
#ifdef COUNTERFACTUAL
 Require(!fixed,"pinned old selected handler reproduces defect");
#else
 Require(fixed,"repaired production contract");
#endif
 std::cout<<"PASS selected next-ten contract\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
