#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
enum {ALLIANCE=469,HORDE=67,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_REWARDED=6,
CHAT_MSG_MONSTER_WHISPER=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct ObjectGuid {
 uint32 value=0; bool IsEmpty()const{return !value;}void Clear(){value=0;}
 bool operator==(ObjectGuid b)const{return value==b.value;}bool operator<(ObjectGuid b)const{return value<b.value;}
};
namespace std {template<>struct hash<ObjectGuid>{size_t operator()(ObjectGuid g)const{return g.value;}};}
struct Creature;struct Player;
struct Quest {uint32 id;uint32 GetQuestId()const{return id;}};
struct QuestStatusData {int Status=QUEST_STATUS_INCOMPLETE;std::shared_ptr<uint8> ScriptLifetime;};
struct Group {ObjectGuid guid{200};bool lfg=true;bool isLFGGroup(){return lfg;}ObjectGuid GetGUID(){return guid;}};
struct GossipMenu {uint32 id=19355;bool exists=true;uint32 GetMenuId(){return id;}void* GetItem(uint32){return exists?this:nullptr;}};
struct Talk {uint32 closes=0;GossipMenu gossip;GossipMenu& GetGossipMenu(){return gossip;}void SendCloseGossip(){++closes;}};
struct Player {
 ObjectGuid guid{1};uint32 team=ALLIANCE,map=0,instance=10,spellCount=0,criteriaCount=0,menus=0;
 bool world=true,alive=true,teleport=false,combat=false,vehicle=false,transport=false,flight=false,flying=false,phase=true,visible=true,los=true,movie=false;
 float distance=2;Group* group=nullptr;Talk talk;Talk* PlayerTalkClass=&talk;
 int castResult=0;
 uint32 scenarioId=0;uint32 GetScenarioId(){return scenarioId;}void SetScenarioId(uint32 id){scenarioId=id;}
 std::map<uint32,QuestStatusData> quests;std::vector<std::function<void()>> events;
 ObjectGuid GetGUID(){return guid;}uint32 GetTeam(){return team;}uint32 GetMapId(){return map;}uint32 GetInstanceId(){return instance;}
 bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}bool isInCombat(){return combat;}
 bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}
 bool InSamePhase(Creature*){return phase;}bool canSeeOrDetect(Creature*){return visible;}bool IsWithinLOSInMap(Creature*){return los;}
 float GetExactDist(Creature*){return distance;}Group* GetGroup(){return group;}
 bool isWatchingMovie(){return movie;}void setWatchinMovie(bool v){movie=v;}
 int CastSpell(Player*,uint32 spell,bool){assert(spell==(team==HORDE?228560u:186528u));++spellCount;if(!castResult)movie=true;return castResult;}
 void PrepareGossipMenu(Creature*,uint32 menuId,bool){talk.gossip.id=menuId;}void SendPreparedGossip(Creature*){++menus;}
 QuestStatusData* getQuestStatus(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}
 int GetQuestStatus(uint32 q){auto s=getQuestStatus(q);return s?s->Status:QUEST_STATUS_NONE;}
 uint32 finalCredit=0;uint32 GetQuestObjectiveData(uint32,uint32 entry){assert(entry==113320);return finalCredit;}
 void SetQuestStatus(uint32 q,int status){quests[q].Status=status;}
 void Accept(uint32 q){quests[q]={QUEST_STATUS_INCOMPLETE,{}};}
 void AddDelayedEvent(uint64,std::function<void()>&& f){events.push_back(std::move(f));}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Next(){assert(!events.empty());auto f=std::move(events.front());events.erase(events.begin());f();}
 void Drain(){while(!events.empty())Next();}
 void UpdateAchievementCriteria(uint32 type,uint32 asset){assert(type==92&&asset==45340);++criteriaCount;}
};
#include "QuestDelayedEvent.inc"
namespace StormheimFleet {void Accepted(Player*){}}
struct Template {uint32 GossipMenuId=19355;};
struct Creature {
 ObjectGuid guid{100};uint32 entry=96644,map=0,instance=10;bool alive=true;Template data;Creature* companion=nullptr;
 ObjectGuid GetGUID(){return guid;}uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}uint32 GetInstanceId(){return instance;}
 bool IsAlive(){return alive;}Template const* GetCreatureTemplate(){return &data;}
 Creature* FindNearestCreature(uint32 actorEntry,float,bool){assert(actorEntry==94420);return companion;}
};
namespace ObjectAccessor {
 std::map<ObjectGuid,Creature*> creatures;std::map<ObjectGuid,Player*> players;
 template<class T>Creature* GetCreature(T&,ObjectGuid g){return creatures[g];}Player* FindPlayer(ObjectGuid g){return players[g];}
}
namespace lfg {enum {LFG_STATE_NONE,LFG_STATE_QUEUED,LFG_STATE_DUNGEON,PLAYER_ROLE_DAMAGE=8};using LfgDungeonSet=std::set<uint32>;}
struct Lfg {
 uint32 dungeon=969,joins=0,teleports=0,last=0,leaves=0;std::map<ObjectGuid,uint32> states;std::set<uint32> selected;
 uint32 GetDungeon(ObjectGuid){return dungeon;}uint32 GetQueueId(ObjectGuid,uint32=0){return 1;}
 uint32 GetState(ObjectGuid g,uint32){return states[g];}
 void TeleportPlayer(Player*,bool out){assert(!out);++teleports;}
 void JoinLfg(Player*,uint32 role,lfg::LfgDungeonSet& slots){assert(role==8&&slots.size()==1);last=*slots.begin();++joins;}
 lfg::LfgDungeonSet const& GetSelectedDungeons(ObjectGuid,uint32){return selected;}
 void LeaveLfg(ObjectGuid,uint32){++leaves;}
} lfgMgr;auto sLFGMgr=&lfgMgr;
struct Scenario {uint32 id=824,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} scenario;
struct ScenarioMgr {Scenario* GetScenario(uint32){return &scenario;}} scenarioMgr;auto sScenarioMgr=&scenarioMgr;
struct Text {std::vector<std::pair<uint32,uint32>> lines;void SendChat(Creature* c,uint32 group,ObjectGuid owner,uint32){assert(!owner.IsEmpty());lines.emplace_back(c->entry,group);}} texts;auto sCreatureTextMgr=&texts;
struct SmartAI {Creature* me;uint32 calls=0;explicit SmartAI(Creature* c):me(c){}virtual void sGossipHello(Player*){++calls;}virtual void UpdateAI(uint32){++calls;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogout(Player*){}virtual void OnLogin(Player*){}virtual void OnUpdate(Player*,uint32){}};
#include "Departure.inc"
int main(){
 using namespace StormheimDeparture;
 for(auto const* r:{&AllianceRoute,&HordeRoute}){
  pending.clear();queued.clear();lfgMgr={};Player p;p.team=r->team;p.map=r->map;p.Accept(r->quest);Creature npc;npc.entry=r->npc;npc.map=r->map;npc.data.GossipMenuId=r->team==ALLIANCE?19355:19342;ObjectAccessor::creatures[npc.guid]=&npc;
  assert(Eligible(&p,&npc));Hello(&p,&npc);assert(p.menus==1);assert(!Select(&p,&npc,999,0));assert(!Select(&p,&npc,npc.data.GossipMenuId,7));
  npc.map=1475;assert(!Select(&p,&npc,npc.data.GossipMenuId,0));npc.map=r->map;
  p.talk.gossip.exists=false;assert(Select(&p,&npc,npc.data.GossipMenuId,0)&&p.events.empty());p.talk.gossip.exists=true;p.talk.gossip.id=999;assert(Select(&p,&npc,npc.data.GossipMenuId,0)&&p.events.empty());p.talk.gossip.id=npc.data.GossipMenuId;
  assert(Select(&p,&npc,npc.data.GossipMenuId,0));Start(&p,&npc);assert(p.events.size()==2);
  p.Next();p.Next();assert(!lfgMgr.joins&&p.events.size()==1);p.movie=false;p.Next();assert(lfgMgr.joins==1&&lfgMgr.last==r->dungeon&&p.GetQuestStatus(r->quest)==QUEST_STATUS_INCOMPLETE);
  // Queued and unrelated groups are preserved; an active matching instance is recoverable.
  Group g;p.group=&g;lfgMgr.dungeon=r->dungeon;lfgMgr.states[g.guid]=lfg::LFG_STATE_DUNGEON;assert(Queue(&p,*r)&&lfgMgr.teleports==1);
  lfgMgr.dungeon=123;assert(!Queue(&p,*r));g.lfg=false;assert(!Queue(&p,*r));p.group=nullptr;lfgMgr.states[p.guid]=lfg::LFG_STATE_QUEUED;auto spells=p.spellCount;Start(&p,&npc);assert(p.spellCount==spells&&p.events.empty());lfgMgr.states.clear();
  // Acceptance boundaries and native Alliance Rogers departure.
  Quest q{r->quest};Creature giver=npc;if(r->team==ALLIANCE)giver.entry=96663;assert(Accept(&p,&giver,&q));if(r->team==ALLIANCE)assert(p.events.empty());p.Drain();
  for(int reason=0;reason<19;++reason){
   p.Accept(r->quest);p.world=p.alive=p.phase=p.visible=p.los=true;p.teleport=p.combat=p.vehicle=p.transport=p.flight=p.flying=false;p.team=r->team;p.map=r->map;p.distance=2;npc.alive=true;npc.map=r->map;npc.entry=r->npc;p.group=nullptr;
   switch(reason){case 0:p.world=false;break;case 1:p.alive=false;break;case 2:p.teleport=true;break;case 3:p.combat=true;break;case 4:p.vehicle=true;break;case 5:p.transport=true;break;case 6:p.flight=true;break;case 7:p.flying=true;break;case 8:p.phase=false;break;case 9:p.visible=false;break;case 10:p.los=false;break;case 11:p.team=r->team==ALLIANCE?HORDE:ALLIANCE;break;case 12:p.map=1475;break;case 13:p.distance=7;break;case 14:npc.alive=false;break;case 15:npc.map=1475;break;case 16:npc.entry=94419;break;case 17:p.quests[r->quest].Status=QUEST_STATUS_COMPLETE;break;case 18:p.quests[r->quest].Status=QUEST_STATUS_REWARDED;break;}
   assert(!Eligible(&p,&npc));Start(&p,&npc);assert(p.events.empty());
  }
  p.world=p.alive=p.phase=p.visible=p.los=true;p.team=r->team;p.map=r->map;p.distance=2;npc.alive=true;npc.map=r->map;npc.entry=r->npc;p.Accept(r->quest);
  Start(&p,&npc);p.Accept(r->quest);Start(&p,&npc);auto before=lfgMgr.joins;p.movie=false;p.Drain();assert(lfgMgr.joins==before+1); // Old acceptance callbacks cannot consume new attempt.
  Start(&p,&npc);p.quests.erase(r->quest);p.Drain();assert(lfgMgr.joins==before+1);
  p.Accept(r->quest);Start(&p,&npc);p.distance=8;p.Drain();assert(lfgMgr.joins==before+1);p.distance=2;
  p.Accept(r->quest);Start(&p,&npc);p.Next();if(r->team==HORDE){Continue(&p,npc.guid,pending[p.guid].generation,0);assert(!p.movie);}p.Drain();
  p.Accept(r->quest);p.castResult=1;before=lfgMgr.joins;Start(&p,&npc);p.Drain();assert(lfgMgr.joins==before+1);p.castResult=0;
  assert(p.GetQuestStatus(r->quest)==QUEST_STATUS_INCOMPLETE);player_stormheim_departure logout;logout.OnLogout(&p);assert(!pending.count(p.guid));
  p.quests[r->quest].Status=QUEST_STATUS_COMPLETE;logout.OnLogin(&p);assert(p.GetQuestStatus(r->quest)==QUEST_STATUS_INCOMPLETE);
  p.quests[r->quest].Status=QUEST_STATUS_COMPLETE;p.finalCredit=1;logout.OnLogin(&p);assert(p.GetQuestStatus(r->quest)==QUEST_STATUS_COMPLETE);
  p.quests[r->quest].Status=QUEST_STATUS_REWARDED;p.finalCredit=0;logout.OnLogin(&p);assert(p.GetQuestStatus(r->quest)==QUEST_STATUS_REWARDED);
  p.Accept(r->quest);Start(&p,&npc);p.movie=false;p.Drain();assert(queued.count(p.guid));lfgMgr.states[p.guid]=lfg::LFG_STATE_QUEUED;lfgMgr.selected={r->dungeon};p.Accept(r->quest);UpdateQueue(&p);assert(lfgMgr.leaves==1&&!queued.count(p.guid));
  Start(&p,&npc);assert(p.events.empty()); // unrelated/current queue remains untouched by a new departure request
  lfgMgr.states.clear();Start(&p,&npc);p.movie=false;p.Drain();lfgMgr.states[p.guid]=lfg::LFG_STATE_QUEUED;lfgMgr.selected={123};p.Accept(r->quest);UpdateQueue(&p);assert(lfgMgr.leaves==1);
 }
 // The report owns one acceptance, validates both actors on every line,
 // retains the gossip flag for retry, and awards only native event 45340.
 Creature sylvanas;sylvanas.entry=94419;sylvanas.map=1475;Creature nathanos;nathanos.entry=94420;nathanos.map=1475;nathanos.guid={101};sylvanas.companion=&nathanos;
 Player p;p.team=HORDE;p.map=1475;p.Accept(39801);ObjectAccessor::players[p.guid]=&p;ObjectAccessor::creatures[nathanos.guid]=&nathanos;scenario={};texts.lines.clear();Report report(&sylvanas);
 report.Start(&p);report.Start(&p);assert(texts.lines.size()==1);report.Update(4499);assert(texts.lines.size()==1);for(int i=0;i<5;++i)report.Update(60000);
 assert(texts.lines.size()==5&&p.criteriaCount==1&&p.GetQuestStatus(39801)==QUEST_STATUS_INCOMPLETE);scenario.step=1;report.Start(&p);assert(report.owner.IsEmpty());
 scenario.step=0;report.Start(&p);p.Accept(39801);report.Update(60000);assert(report.owner.IsEmpty()&&p.criteriaCount==1);report.Start(&p);nathanos.alive=false;report.Update(1);assert(report.owner.IsEmpty());nathanos.alive=true;
 report.Start(&p);p.combat=true;report.Update(1);assert(report.owner.IsEmpty());p.combat=false;
 report.Start(&p);ObjectAccessor::players[p.guid]=nullptr;report.Update(1);assert(report.owner.IsEmpty());ObjectAccessor::players[p.guid]=&p;
 sylvanas.companion=nullptr;report.Start(&p);assert(report.owner.IsEmpty());sylvanas.companion=&nathanos;nathanos.instance=11;report.Start(&p);assert(report.owner.IsEmpty());nathanos.instance=10;
 p.team=ALLIANCE;report.Start(&p);assert(report.owner.IsEmpty());p.team=HORDE;
 npc_stormheim_fleet_report ai(&sylvanas);scenario.id=826;ai.sGossipHello(&p);assert(ai.calls==1);sylvanas.map=1220;scenario.id=824;ai.UpdateAI(1);assert(ai.calls==2);
 std::cout<<"PASS: native faction queues, matching-instance recovery, wrong queues/groups, movie skip/fallback, duplicate and stale acceptance callbacks, failed interaction/retry, opening report chronology and actor loss/combat/abandonment, unrelated scenario delegation; no final quest credit.\n";
}
