#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <vector>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
struct ObjectGuid{uint64 v=0;ObjectGuid(uint64 n=0):v(n){}bool operator<(ObjectGuid const&o)const{return v<o.v;}bool operator==(ObjectGuid const&o)const{return v==o.v;}};
constexpr uint32 QUEST_STATUS_INCOMPLETE=3,TEAM_ALLIANCE=0,SPELL_MOLTEN_TOUCH_CHANNEL=249227,DATA_KROSUS=6,DATA_SKORPYRON=0,NOT_STARTED=0,IN_PROGRESS=1;
constexpr uint32 EVENT_1=1,EVENT_2=2,EVENT_3=3,EVENT_4=4,EVENT_5=5;
uint32 a_WP[6]={1,2,3,4,5,6},h_WP[6]={7,8,9,10,11,12};uint32 urand(uint32 a,uint32){return a;}
struct Queue{std::vector<std::pair<uint32,std::function<void()>>>q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,std::move(f));}void Update(uint32 n){std::vector<std::function<void()>>ready;for(auto i=q.begin();i!=q.end();)if(i->first<=n){ready.push_back(std::move(i->second));i=q.erase(i);}else{i->first-=n;++i;}for(auto&f:ready)f();}void Clear(){q.clear();}};
struct EventMap{Queue q;std::list<uint32>ready;void Reset(){q.Clear();ready.clear();}void RescheduleEvent(uint32 id,uint32 n){q.Add(n,[this,id]{ready.push_back(id);});}void Update(uint32 n){q.Update(n);}uint32 ExecuteEvent(){if(ready.empty())return 0;auto n=ready.front();ready.pop_front();return n;}};
struct Unit;using Player=Unit;using Creature=Unit;struct Base;
static uint32 bad=0,conversations=0,init=0;static bool createOK=true;
struct Position{float x,y,z;};
struct Motion{uint32 paths=0,jumps=0,lastPath=0;void Clear(){}void MovePath(uint32 n,bool){++paths;lastPath=n;}void MoveJump(float,float,float,float,float){++jumps;}};
struct Unit{ObjectGuid guid;bool alive=true,world=true,invalid=false,player=true,box=true;uint32 mapId=1,instanceId=1,quest=3,objective=0,team=0,casts=0,despawns=0,teleports=0,credits=0;Queue q;Motion motion;Base*script=nullptr;float z=3.16f;Unit(uint64 n):guid(n){}void Touch(){if(invalid)++bad;}ObjectGuid GetGUID(){Touch();return guid;}bool IsAlive(){Touch();return alive;}bool IsInWorld(){return world;}bool IsPlayer(){return player;}Player*ToPlayer(){return player?this:nullptr;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instanceId;}uint32 GetQuestStatus(uint32){Touch();return quest;}uint32 GetQuestObjectiveData(uint32,uint32){Touch();return objective;}uint32 GetEntry(){return 122390;}uint32 GetTeamId(){Touch();return team;}float GetDistance(Unit*){return 1;}float GetPositionX(){Touch();return 1;}float GetPositionY(){Touch();return 1;}float GetPositionZ(){Touch();return z;}float GetOrientation(){return 0;}bool IsWithinBox(Position,float,float,float){return box;}Base*GetInstanceScript(){return script;}void AddDelayedEvent(uint32 n,std::function<void()>f){q.Add(n,std::move(f));}void AddDelayedCombat(uint32 n,std::function<void()>f){q.Add(n,std::move(f));}template<class...T>void CastSpell(T...){Touch();++casts;}void CastSpell(Unit*u,uint32,bool=true){Touch();if(u)u->Touch();++casts;}void DespawnOrUnsummon(uint32){Touch();++despawns;}Motion*GetMotionMaster(){Touch();return &motion;}void TeleportTo(uint32,float,float,float,float){++teleports;}void NearTeleportTo(float,float,float h,float){z=h;++teleports;}void CreateConversation(uint32){++conversations;}void RemoveAura(uint32){}void KilledMonsterCredit(uint32){Touch();++credits;}};
static std::map<ObjectGuid,Unit*>units;static std::list<Player*>nearby;
namespace ObjectAccessor{Unit*GetUnit(Unit&me,ObjectGuid guid){auto i=units.find(guid);return i!=units.end()&&i->second->world&&i->second->mapId==me.mapId&&i->second->instanceId==me.instanceId?i->second:nullptr;}Unit*GetCreature(Unit&me,ObjectGuid g){auto u=GetUnit(me,g);return u&&!u->player?u:nullptr;}Player*GetPlayer(Unit&me,ObjectGuid g){auto u=GetUnit(me,g);return u&&u->player?u:nullptr;}}
void GetPlayerListInGrid(std::list<Player*>&l,Creature*,float){l=nearby;}void GetCreatureListWithEntryInGrid(std::list<Creature*>&,Player*,uint32,float){}
struct Conversation{bool CreateConversation(uint64,uint32,Player*,void*,Player&){++conversations;return createOK;}};
namespace HighGuid{struct Conversation{};}struct Generator{uint64 Generate(){return 1;}};struct Objects{template<class T>Generator*GetGenerator(){static Generator g;return &g;}}objects;auto sObjectMgr=&objects;
struct Base{Unit*me;Queue own;Base*instance=this;bool introDone=false;uint32 krosus=0,skorpyron=0,talks=0;explicit Base(Unit*u):me(u){}virtual~Base()=default;void AddDelayedEvent(uint32 n,std::function<void()>f){own.Add(n,std::move(f));}void DoCast(uint32){}void Talk(uint32,ObjectGuid={}){++talks;}void Init(){if(!me->world)++bad;++init;}uint32 GetBossState(uint32 id){return id==DATA_KROSUS?krosus:skorpyron;}};
#include "Selected.inc"
void Check(bool ok,char const*why){if(!ok){std::cerr<<why<<'\n';std::abort();}}
int main(){Unit me(1),p(2),summon(3),spider(4),cocoon(5),bystander(6);me.player=summon.player=spider.player=cocoon.player=false;for(auto u:{&me,&p,&summon,&spider,&cocoon,&bystander})units[u->guid]=u;Frame f(&me);p.script=&f;
#if defined(CASE_MOLTEN)
 f.Start(&p,&summon,&spider,&cocoon,&p);summon.invalid=true;summon.world=false;me.q.Update(1800);
#ifdef COUNTERFACTUAL
 Check(bad>0,"original stale summon dereferenced");
#else
 Check(!bad&&!me.casts,"missing summon skipped");summon.world=true;summon.invalid=false;f.Start(&p,&summon,&spider,&cocoon,&p);me.q.Update(1800);Check(me.casts==1,"authored channel retained");me.alive=false;f.Start(&p,&summon,&spider,&cocoon,&p);me.q.Update(1800);Check(me.casts==1,"dead boss skips channel");
#endif
#elif defined(CASE_BRAWLER)
 f.Start(&p,&summon,&spider,&cocoon,&p);p.invalid=true;p.world=false;summon.q.Update(47000);
#ifdef COUNTERFACTUAL
 Check(bad>0,"original faction read from departed player");
#else
 Check(!bad&&summon.motion.lastPath==1,"alliance immutable route");p.invalid=false;p.team=1;f.Start(&p,&summon,&spider,&cocoon,&p);p.invalid=true;summon.q.Update(47000);Check(!bad&&summon.motion.lastPath==7,"horde immutable route");
#endif
#elif defined(CASE_COCOON)
 f.Start(&p,&summon,&spider,&cocoon,&p);cocoon.world=false;cocoon.invalid=true;p.world=false;p.invalid=true;spider.q.Update(10000);
#ifdef COUNTERFACTUAL
 Check(bad>0,"original cocoon and caster retained");
#else
 Check(!bad&&!spider.casts&&!spider.motion.jumps,"missing cocoon skips release");cocoon.world=true;cocoon.invalid=false;p.invalid=false;f.Start(&p,&summon,&spider,&cocoon,&p);p.invalid=true;spider.q.Update(10000);Check(spider.casts==1&&cocoon.despawns==1&&!spider.motion.jumps&&!bad,"missing caster still cocoon release cleanup");p.world=true;p.invalid=false;f.Start(&p,&summon,&spider,&cocoon,&p);spider.q.Update(10000);Check(spider.casts==2&&cocoon.despawns==2&&spider.motion.jumps==1,"original jump retained");
#endif
#elif defined(CASE_TELARN)
 f.Start();me.world=false;me.q.Clear();f.own.Update(500);
#ifdef COUNTERFACTUAL
 Check(bad&&init==1,"instance callback outlives creature");
#else
 Check(!bad&&!init,"actor cancellation owns init");me.world=true;f.Start();me.q.Update(500);Check(init==1,"native500ms retained");f.Start();me.alive=false;me.q.Update(500);Check(init==1,"dead actor skipped");
#endif
#elif defined(CASE_DRAMOCK)
 f.Reset();bystander.quest=0;nearby={&p,&bystander};f.MoveInLineOfSight(&p);me.q.Update(500);p.world=false;p.invalid=true;for(uint32 d:{4000,6000,7000,7000,5000})f.UpdateAI(d);
#ifdef COUNTERFACTUAL
 Check(bad&&p.credits==1&&bystander.credits==1,"original stale/ineligible roster credit");
#else
 Check(!bad&&!p.credits&&!bystander.credits,"departed and ineligible players skipped");p.invalid=false;p.world=true;f.MoveInLineOfSight(&p);f.Reset();me.q.Update(500);Check(f.playerGUIDs.empty(),"reset invalidates pending snapshot");f.MoveInLineOfSight(&p);me.q.Update(500);p.quest=0;for(uint32 d:{4000,6000,7000,7000,5000})f.UpdateAI(d);Check(!p.credits,"abandoned quest no credit");p.quest=3;f.MoveInLineOfSight(&p);me.q.Update(500);for(uint32 d:{4000,6000,7000,7000,5000})f.UpdateAI(d);Check(p.credits==1&&f.playerGUIDs.empty(),"eligible player exactly once list consumed");f.events.RescheduleEvent(EVENT_5,1);f.UpdateAI(1);Check(p.credits==1,"duplicate finish no credit");p.objective=1;f.MoveInLineOfSight(&p);me.q.Update(500);Check(!f.useEvent,"completed objective not restarted");
#endif
#else
 f.OnPlayerEnter(&p);p.mapId=2;p.q.Update(10000);p.q.Update(10000);
#ifdef COUNTERFACTUAL
 Check(conversations>0||p.teleports>0,"original delayed callback crosses map");
#else
 Check(!conversations&&!p.teleports,"foreign map no intro/floor/teleport");p.mapId=1;uint32 casts=p.casts;f.OnPlayerEnter(&p);f.OnPlayerLeave(&p);f.OnPlayerEnter(&p);p.q.Update(5000);p.q.Update(4000);
#if defined(CASE_NIGHTHOLDENTRY)
 Check(conversations==1&&f.introDone&&p.teleports==1,"fresh entry intro exactly once plus floor");f.introDone=false;createOK=false;f.OnPlayerEnter(&p);p.q.Update(9000);Check(!f.introDone,"failed conversation retry retained");createOK=true;f.OnPlayerEnter(&p);p.q.Update(9000);Check(f.introDone,"successful retry commits intro");f.OnPlayerEnter(&p);p.box=false;uint32 n=p.teleports;p.q.Update(500);Check(p.teleports==n,"walked out of recovery box");
#elif defined(CASE_ASSAULTENTRY)
 Check(conversations==1&&p.casts==casts+3,"old entry cancelled before summon");f.OnPlayerEnter(&p);p.q.Update(5000);f.OnPlayerLeave(&p);p.q.Update(4000);Check(conversations==1,"nested stage cancelled after departure");
#else
 Check(conversations==1,"old entry cancelled newest only");p.quest=0;f.OnPlayerEnter(&p);p.quest=3;p.q.Update(2000);Check(!p.teleports,"late acceptance cancels ejection");f.OnPlayerEnter(&p);p.quest=0;p.q.Update(5000);Check(conversations==1,"abandonment cancels intro");f.OnPlayerEnter(&p);p.q.Update(2000);Check(p.teleports==1,"ineligible player original exit");
#endif
 f.OnPlayerEnter(&p);f.OnPlayerLeave(&p);uint32 previous=conversations;p.q.Update(20000);p.q.Update(20000);Check(conversations==previous,"leave cancels all pending");
#endif
#endif
 std::cout<<"PASS selected production callbacks;controlled endpoints/counterfactuals,not client traversal\n";
}
