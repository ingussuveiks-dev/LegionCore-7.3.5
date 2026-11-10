#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <vector>
#include <iostream>
using uint8=uint8_t;using int32=int32_t;using uint32=uint32_t;using uint64=uint64_t;
struct ObjectGuid{uint64 v=0;static ObjectGuid const Empty;ObjectGuid(uint64 n=0):v(n){}bool operator<(ObjectGuid const&o)const{return v<o.v;}bool operator==(ObjectGuid const&o)const{return v==o.v;}};ObjectGuid const ObjectGuid::Empty{};
struct Queue{std::vector<std::pair<uint32,std::function<void()>>>q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,std::move(f));}void Update(uint32 n){std::vector<std::function<void()>>ready;for(auto i=q.begin();i!=q.end();)if(i->first<=n){ready.push_back(std::move(i->second));i=q.erase(i);}else{i->first-=n;++i;}for(auto&f:ready)f();}void Clear(){q.clear();}};
struct EventMap{uint32 due=0,id=0;void Reset(){due=id=0;}void RescheduleEvent(uint32 n,uint32 d){id=n;due=d;}void Update(uint32 d){due=due>d?due-d:0;}uint32 ExecuteEvent(){if(due)return 0;auto n=id;id=0;return n;}};
struct Unit;using Player=Unit;using Creature=Unit;struct Base;using InstanceScript=Base;
static uint32 bad=0,conversations=0;static Creature*helper=nullptr;static bool spawnOK=true,spawnFigures=false;Creature*Spawn(uint32);
struct Position{float x=1,y=1,z=1;};struct Motion{uint32 points=0;template<class...T>void MovePoint(T...){++points;}void MoveFall(){}};
struct Group{bool leader=true;bool IsLeader(ObjectGuid){return leader;}};
struct Map{struct Ref{Player*p;Player*getSource()const{return p;}};struct PlayerList:std::list<Ref>{bool isEmpty()const{return empty();}}players;uint32 difficulty=23;uint32 GetDifficultyID(){return difficulty;}PlayerList const&GetPlayers(){return players;}Creature*GetCreature(ObjectGuid);};
struct AI{Unit*me;uint32 enters=0,talks=0;void EnterCombat(Unit*);void Talk(uint32,ObjectGuid={}){++talks;}void SetFlyMode(bool){}void DoCast(uint32);};
struct Unit{ObjectGuid guid;bool alive=true,world=true,invalid=false,player=true,combat=false,hasAura=true;uint32 mapId=1,instanceId=1,quest=3,casts=0,despawns=0,repop=0,lastRemoved=0,anim=0,react=0,summonAttempts=0;float distance=1;Queue q;Motion motion;Base*script=nullptr;Group*group=nullptr;Map*map=nullptr;AI ai{this};Unit*owner=nullptr;Unit(uint64 n):guid(n){}void Touch(){if(invalid||!world)++bad;}ObjectGuid GetGUID(){Touch();return guid;}bool IsAlive(){Touch();return alive;}bool IsInWorld(){return world;}bool IsPlayer(){return player;}Player*ToPlayer(){return player?this:nullptr;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instanceId;}uint32 GetQuestStatus(uint32){Touch();return quest;}template<class...T>float GetDistance(T...){Touch();return distance;}Base*GetInstanceScript(){return script;}void AddDelayedEvent(uint32 n,std::function<void()>f){Touch();q.Add(n,std::move(f));}void KillAllDelayedEvents(){q.Clear();}template<class...T>void CastSpell(T...){Touch();++casts;}void CastSpell(Unit*u,uint32,bool=false){Touch();if(u)u->Touch();++casts;}void RemoveAura(uint32 s){Touch();lastRemoved=s;}void RemoveAurasDueToSpell(uint32 s){RemoveAura(s);}bool HasAura(uint32){return hasAura;}bool HasUnitState(uint32){return false;}bool isInCombat(){return combat;}void RepopAtGraveyard(){++repop;}void TeleportTo(uint32,float,float,float,float){}void CreateConversation(uint32){++conversations;}Map*GetMap(){return map;}Group*GetGroup(){return group;}void RemoveFlag(uint32,uint32){}AI*AI(){Touch();return &ai;}void DespawnOrUnsummon(uint32){Touch();++despawns;}Motion*GetMotionMaster(){Touch();return &motion;}float GetPositionX(){Touch();return 1;}float GetPositionY(){Touch();return 1;}float GetPositionZ(){Touch();return 1;}float GetOrientation(){return 0;}void SetWalk(bool){}void SetAnimKitId(uint32 n){Touch();anim=n;}void SetReactState(uint32 n){Touch();react=n;}void SetFacingTo(Unit*u){Touch();u->Touch();}void setRegeneratingHealth(bool){}Unit*GetAnyOwner(){return owner;}Creature*FindNearestCreature(uint32,float,bool){return nullptr;}template<class...T>Creature*SummonCreature(uint32 id,T...){Touch();++summonAttempts;return Spawn(id);}};
void AI::EnterCombat(Unit*){me->Touch();++enters;me->combat=true;}void AI::DoCast(uint32 s){me->CastSpell(me,s);}
static std::map<ObjectGuid,Unit*>units;static std::list<std::unique_ptr<Unit>>spawned;Creature*Spawn(uint32){if(!spawnOK)return nullptr;if(!spawnFigures){helper->world=true;helper->invalid=false;return helper;}spawned.emplace_back(new Unit(100+spawned.size()));spawned.back()->player=false;units[spawned.back()->guid]=spawned.back().get();return spawned.back().get();}
Creature*Map::GetCreature(ObjectGuid g){auto i=units.find(g);return i==units.end()?nullptr:i->second;}
namespace ObjectAccessor{Unit*GetUnit(Unit&me,ObjectGuid guid){auto i=units.find(guid);return i!=units.end()&&i->second->world&&i->second->mapId==me.mapId&&i->second->instanceId==me.instanceId?i->second:nullptr;}Creature*GetCreature(Unit&me,ObjectGuid g){auto u=GetUnit(me,g);return u&&!u->player?u:nullptr;}Player*GetPlayer(Unit&me,ObjectGuid g){auto u=GetUnit(me,g);return u&&u->player?u:nullptr;}}
struct Base{Unit*me;Map map;Base*instance=this;bool IntroDone=false;uint32 bossState=3,resets=0,phase=0,last_spell=229008,casts=0;std::vector<uint32>spellOrder;EventMap events;explicit Base(Unit*u):me(u){me->map=&map;map.players.push_back({u});}virtual~Base()=default;uint32 GetBossState(uint32){return bossState;}ObjectGuid GetGuidData(uint32);Creature*GetCreature(ObjectGuid g){return map.GetCreature(g);}void _Reset(){++resets;}bool UpdateVictim(){return true;}void DoMeleeAttackIfReady(){}void DoCast(uint32 id){me->Touch();++casts;spellOrder.push_back(id);}void AttackStart(Unit*u){me->Touch();me->AI()->EnterCombat(u);}void Talk(uint32){} };
uint32 urand(uint32 a,uint32){return a;}
#include "Constants.inc"
#include "Selected.inc"
ObjectGuid Base::GetGuidData(uint32){
#if defined(CASE_AVATARINTRO)
 return ObjectGuid(3);
#else
 return ObjectGuid(5);
#endif
}
void Check(bool ok,char const*why){if(!ok){std::cerr<<why<<'\n';std::abort();}}
int main(){Unit me(1),p(2),other(3),visual(4),illidan(5);me.player=other.player=visual.player=illidan.player=false;for(auto u:{&me,&p,&other,&visual,&illidan})units[u->guid]=u;Frame f(&me);p.script=&f;p.map=&f.map;f.map.players.clear();f.map.players.push_back({&p});helper=&other;me.owner=&p;
#if defined(CASE_VALENTRY)||defined(CASE_MAWENTRY)
 f.OnPlayerEnter(&p);p.instanceId=2;p.q.Update(5000);
#ifdef COUNTERFACTUAL
 Check(conversations||p.repop,"original delayed entry crosses instance");
#else
 Check(!conversations&&!p.repop,"foreign instance refused");p.instanceId=1;f.OnPlayerEnter(&p);f.OnPlayerLeave(&p);f.OnPlayerEnter(&p);p.q.Update(5000);
#if defined(CASE_VALENTRY)
 Check(conversations==1,"newest entry only");p.quest=0;f.OnPlayerEnter(&p);p.quest=3;p.q.Update(2000);f.OnPlayerEnter(&p);p.quest=0;p.q.Update(5000);Check(conversations==1,"late quest changes cancel old branch");
#else
 Check(p.repop==1,"newest recovery only");f.OnPlayerEnter(&p);p.distance=30;p.q.Update(500);Check(p.repop==1,"walked away no recovery");p.distance=1;f.OnPlayerEnter(&p);f.bossState=0;p.q.Update(500);Check(p.repop==1,"late boss recheck");
#endif
 f.OnPlayerEnter(&p);f.OnPlayerLeave(&p);auto before=conversations+p.repop;p.q.Update(20000);Check(conversations+p.repop==before,"leave cancels callbacks");
#endif
#elif defined(CASE_STYLEEFFECTS)
 f.QueueStageEffect(&p);p.world=false;p.q.Update(500);me.q.Update(500);
#ifdef COUNTERFACTUAL
 Check(bad&&p.casts==1,"original player-owned cue follows departure");
#else
 Check(!bad&&!p.casts,"departed contestant skipped");p.world=true;f.QueueStageEffect(&p);f.ResetEffects();me.q.Update(500);Check(!p.casts,"Reset cancels old cue");for(int n=0;n<20;++n){f.QueueStageEffect(&p);me.q.Update(500);}Check(p.casts==20,"all twenty authored callsites use shared cue");
#endif
#elif defined(CASE_INQUISITION)
 me.combat=true;f.Start(&other,&p);p.world=false;p.invalid=true;other.q.Update(100);
#ifdef COUNTERFACTUAL
 Check(other.ai.enters==1,"original retained attacker passed to partner");
#else
 Check(!other.ai.enters&&!bad,"missing attacker skipped");p.world=true;p.invalid=false;f.Start(&other,&p);me.combat=false;other.q.Update(100);Check(!other.ai.enters,"evaded source no delayed pull");me.combat=true;p.player=false;f.Start(&other,&p);other.q.Update(100);Check(other.ai.enters==1,"living pet/guardian attacker retained");p.player=true;f.Start(&other,&p);other.q.Update(100);Check(other.ai.enters==1,"already engaged partner no repeat");f.Start(&other,nullptr);other.q.Update(100);Check(other.ai.enters==1,"null attacker no capture");
#endif
#elif defined(CASE_NIGHTBANE)
 spawnOK=false;f.sGossipSelect(&p,0,0);
#ifdef COUNTERFACTUAL
 Check(p.casts==1,"original achievement consumed failed summon");spawnOK=true;f.sGossipSelect(&p,0,0);other.world=false;other.invalid=true;me.q.Update(75000);Check(bad,"original starter callback retains missing nightbane");
#else
 Check(!f.cinematicStarted&&!p.casts,"failed summon retains admission/no achieve");spawnOK=true;f.sGossipSelect(&p,0,0);f.sGossipSelect(&p,0,0);Check(p.casts==1&&me.summonAttempts==2,"one accepted cinematic after retry");other.world=false;other.invalid=true;me.q.Update(75000);me.q.Update(6000);Check(!bad&&me.despawns==1,"missing nightbane skips unsafe casts but starter cleans up");
#endif
#elif defined(CASE_AVATARINTRO)
 f.MoveInLineOfSight(&p);me.world=false;me.invalid=true;me.q.Clear();other.q.Update(27000);
#ifdef COUNTERFACTUAL
 Check(bad,"original Kiljaeden queue dereferences retired Avatar");
#else
 Check(!bad,"missing Avatar no stale cast");other.q.Update(13000);Check(other.casts==2,"Kiljaeden self cleanup independent of missing Avatar");me.world=true;me.invalid=false;f.IntroDone=false;f.MoveInLineOfSight(&p);other.q.Update(27000);me.q.Update(3000);Check(me.anim==12758,"authored delayed animation");me.q.Update(10000);Check(me.casts==1,"authored sword visual through current AI");
#endif
#elif defined(CASE_KHADGAR)
 f.Start(&other);me.world=false;me.invalid=true;me.q.Clear();other.q.Update(2000);
#ifdef COUNTERFACTUAL
 Check(bad,"foreign Khadgar queue touches retired boss");
#else
 Check(!bad,"boss-owned sequence cancels at destruction");me.world=true;me.invalid=false;spawnOK=false;f.Start(&other);me.q.Update(2000);Check(other.lastRemoved==227671&&other.ai.talks==1&&me.summonAttempts==1,"original cleanup speech and figure attempt");spawnOK=spawnFigures=true;f.Start(&other);me.q.Update(2000);uint32 colors=0;for(auto const&u:spawned)colors+=u->casts;Check(spawned.size()==5&&colors==8,"all four original color branches and five distinct figures");me.alive=false;uint32 previous=me.summonAttempts;f.Start(&other);me.q.Update(2000);Check(me.summonAttempts==previous,"dead boss no new figures");
#endif
#elif defined(CASE_OSOINTRO)
 f.Reset();me.world=false;me.invalid=true;me.q.Clear();other.q.Update(6000);
#ifdef COUNTERFACTUAL
 Check(bad,"helper queue retained retired Oso");
#else
 Check(!bad,"Oso owns intro callback");me.world=true;me.invalid=false;f.Reset();f.Reset();other.world=false;me.q.Update(6000);Check(me.ai.enters==1&&f.resets==3,"new attempt only;missing optional helper does not block combat");spawnOK=false;f.Reset();me.q.Update(6000);Check(me.ai.enters==2,"failed helper spawn retains six-second readiness");spawnOK=true;f.Reset();me.alive=false;me.q.Update(6000);Check(me.ai.enters==2,"dead boss cannot start attack");
#endif
#elif defined(CASE_RAYDVISUAL)
 f.Start(&visual);f.last_spell=999;visual.q.Update(6000);
#ifdef COUNTERFACTUAL
 Check(visual.lastRemoved==999,"original delayed mutable member read");
#else
 Check(visual.lastRemoved==229008,"only originally applied morph removed");
#endif
#elif defined(CASE_RAYDROTATION)
 f.events.RescheduleEvent(EVENT_1,7000);
#ifdef COUNTERFACTUAL
 for(int n=0;n<10;++n)f.UpdateAI(12000);Check(f.phase==10&&7000+10*11000<120000,"11th array access becomes out of bounds before round deadline;counterfactual stops before UB");
#else
 for(uint32 n=0;n<32;++n){f.UpdateAI(12000);Check(f.spellOrder.back()==Spells_Rayd[n%10]&&f.phase<10,"bounded paired rotation preserves authored order");}Check(f.casts==32,"rotation continues through repeated cycles");
#endif
#endif
 std::cout<<"PASS selected production source;instrumented counterfactuals/controlled endpoints,not client traversal\n";
}
