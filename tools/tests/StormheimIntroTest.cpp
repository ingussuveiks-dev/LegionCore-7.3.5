#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
enum {ALLIANCE=469,HORDE=67,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_REWARDED=6,
QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS=16,QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR=64,QUEST_OBJECTIVE_FLAG_OPTIONAL=4,
QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=2,QUEST_SPECIAL_FLAGS_TIMED=4,GAMEOBJECT_FIELD_FLAGS=0,GO_FLAG_IN_USE=1,GO_FLAG_NOT_SELECTABLE=16,SPELL_CAST_OK=0};
struct Position {float x=0,y=0,z=0,o=0;};
struct ObjectGuid {static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct QuestObjective {int Type=0;uint32 ObjectID=0,Amount=1,Flags=0;};
struct Quest {
 uint32 id,special=0;std::vector<QuestObjective> objectives;
 auto const& GetObjectives()const{return objectives;}bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}
 bool IsRepeatable()const{return false;}bool IsAutoComplete()const{return false;}bool HasSpecialFlag(uint32 f)const{return special&f;}
};
struct QuestStore {std::map<uint32,Quest> quests;Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}} store;
auto sQuestDataStore=&store;
struct QuestStatusData {int Status=QUEST_STATUS_NONE;uint32 Timer=0;std::shared_ptr<uint8> ScriptLifetime;};
struct Player;
struct Unit {Position pos;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}};
struct Creature:Unit {uint32 entry=96644,map=1220,zone=7502;uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}};
struct GameObject:Unit {uint32 entry=244567,map=1220,zone=7502,flags=0;bool spawned=true;uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}bool isSpawned(){return spawned;}bool HasFlag(int,uint32 f){return flags&f;}};
struct Player:Unit {
 uint32 team=ALLIANCE,map=1220,packets=0,completions=0;bool world=true,alive=true,teleport=false,vehicle=false,transport=false,flight=false,flying=false,phase=true,visible=true,los=true;
 int castResult=SPELL_CAST_OK;std::vector<uint32> spells;std::vector<std::function<void()>> events;
 std::map<uint32,QuestStatusData> quests;std::map<std::pair<uint32,uint32>,uint32> data;std::set<uint32> m_RewardedQuests;
 Player* ToPlayer()override{return this;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetTeam(){return team;}uint32 GetMapId(){return map;}
 bool IsBeingTeleported(){return teleport;}bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}
 bool InSamePhase(Unit*){return phase;}bool canSeeOrDetect(Unit*){return visible;}bool IsWithinLOSInMap(Unit*){return los;}
 float GetExactDist(Position const* p){return std::sqrt((pos.x-p->x)*(pos.x-p->x)+(pos.y-p->y)*(pos.y-p->y)+(pos.z-p->z)*(pos.z-p->z));}
 float GetExactDist(Unit const* u){return GetExactDist(&u->pos);}
 QuestStatusData* getQuestStatus(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}
 int GetQuestStatus(uint32 id){auto q=getQuestStatus(id);return q?q->Status:QUEST_STATUS_NONE;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 e){return data[{q,e}];}
 void SetQuestObjectiveData(Quest const* q,QuestObjective const* o,uint32 n){data[{q->id,o->ObjectID}]=n;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32 n){assert(n==1);++packets;}
 bool HasQuestObjectiveComplete(Quest const* q,QuestObjective const& o){return data[{q->id,o.ObjectID}]>=o.Amount;}
 bool IsQuestWeekRewarded(uint32){return false;}bool IsQuestSeasonalRewarded(uint32){return false;}bool CanTakeQuest(Quest const*,bool){return true;}
 bool CanCompleteQuest(uint32);void CompleteQuest(uint32 id){quests[id].Status=QUEST_STATUS_COMPLETE;++completions;}
 int CastSpell(Player*,uint32 spell,bool){spells.push_back(spell);if(castResult==SPELL_CAST_OK)teleport=true;return castResult;}
 void AddDelayedEvent(uint64,std::function<void()> fn){events.push_back(std::move(fn));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Accept(uint32 id){quests.erase(id);quests[id].Status=QUEST_STATUS_INCOMPLETE;for(auto i=data.begin();i!=data.end();)if(i->first.first==id)i=data.erase(i);else ++i;}
 void Next(){assert(!events.empty());auto fn=std::move(events.front());events.erase(events.begin());fn();}
};
#include "QuestDelayedEvent.inc"
#include "CanCompleteQuest.inc"
struct SmartAI {Creature* me;unsigned calls=0;explicit SmartAI(Creature* c):me(c){}virtual void MoveInLineOfSight(Unit*){++calls;}virtual void IsSummonedBy(Unit*){}virtual void UpdateAI(uint32){}virtual void MovementInform(uint32,uint32){}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){}virtual void sGossipHello(Player*){}virtual void sGossipSelect(Player*,uint32,uint32){}virtual void sQuestAccept(Player*,Quest const*){}};
// Departure has its own production-handler regression harness.
namespace StormheimDeparture {void Hello(Player*,Creature*){}bool Select(Player*,Creature*,uint32,uint32){return false;}bool Accept(Player*,Creature*,Quest const*){return false;}}
// The rounds module has its own production-controller regression harness.
namespace StormheimRounds {struct Controller {explicit Controller(Creature*){}bool Start(Unit*){return false;}bool Running()const{return false;}void Update(uint32){}void Movement(uint32,uint32){}uint32 GetData(uint32)const{return 0;}void SetData(uint32,uint32){}};}
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}};
#include "StormheimIntro.inc"
int main(){
 for(auto const* j:{&StormheimIntro::AllianceJourney,&StormheimIntro::HordeJourney}){
  store.quests[j->introduction]={j->introduction,0,{{0,97377,1,0}}};store.quests[j->summons]={j->summons,0,{{0,j->reader,1,0},{0,j->arrivalCredit,1,2}}};
  Creature npc;npc.entry=j->reader;npc_stormheim_introduction ai(&npc);Player p;p.team=j->team;p.Accept(j->introduction);
  store.quests[j->introduction].special=2;ai.MoveInLineOfSight(&p);assert(p.packets==1&&p.completions==0); // Original orphan event lock.
  store.quests[j->introduction].special=0;p.Accept(j->introduction);ai.MoveInLineOfSight(&p);ai.MoveInLineOfSight(&p);assert(p.completions==1&&p.packets==2);
  p.Accept(j->summons);GameObject go;go.entry=j->portal;go_stormheim_summons_portal portal;
  portal.OnGossipHello(&p,&go);assert(p.spells.empty()&&p.events.empty()); // Must read the native letter first.
  StormheimIntro::Credit(&p,j->summons,j->reader);assert(!p.CanCompleteQuest(j->summons));
  if(j->team==ALLIANCE){store.quests[j->summons].special=2;p.data[{j->summons,j->arrivalCredit}]=1;assert(!p.CanCompleteQuest(j->summons));p.data[{j->summons,j->arrivalCredit}]=0;store.quests[j->summons].special=0;}
  portal.OnGossipHello(&p,&go);assert(p.spells.back()==j->spell&&p.events.size()==1);p.world=false;p.Next();assert(p.events.size()==1&&!p.GetQuestObjectiveData(j->summons,j->arrivalCredit));
  p.world=true;p.teleport=false;p.Next();assert(!p.GetQuestObjectiveData(j->summons,j->arrivalCredit)); // Failed teleport leaves Dalaran.
  p.teleport=false;portal.OnGossipHello(&p,&go);p.teleport=false;p.map=j->map;p.pos=j->destination;p.Next();assert(p.GetQuestStatus(j->summons)==QUEST_STATUS_COMPLETE);
  auto packets=p.packets;StormheimIntro::ConfirmArrival(&p,*j,0);assert(p.packets==packets);
  p={};p.team=j->team;p.Accept(j->summons);p.data[{j->summons,j->reader}]=1;portal.OnGossipHello(&p,&go);p.Accept(j->summons);p.data[{j->summons,j->reader}]=1;p.teleport=false;p.map=j->map;p.pos=j->destination;p.Next();assert(!p.GetQuestObjectiveData(j->summons,j->arrivalCredit));
  p={};p.team=j->team;p.Accept(j->summons);p.data[{j->summons,j->reader}]=1;p.castResult=1;portal.OnGossipHello(&p,&go);assert(p.events.empty());
  for(int reason=0;reason<17;++reason){p={};p.team=j->team;p.Accept(j->summons);p.data[{j->summons,j->reader}]=1;go.flags=0;go.spawned=true;go.map=1220;go.zone=7502;
   if(reason==0)p.team=j->team==ALLIANCE?HORDE:ALLIANCE;if(reason==1)p.alive=false;if(reason==2)p.world=false;if(reason==3)p.teleport=true;if(reason==4)p.vehicle=true;if(reason==5)p.transport=true;if(reason==6)p.flight=true;if(reason==7)p.flying=true;if(reason==8)p.phase=false;if(reason==9)p.visible=false;if(reason==10)p.los=false;if(reason==11)p.pos.x=6;if(reason==12)p.map=0;if(reason==13)go.flags=GO_FLAG_NOT_SELECTABLE;if(reason==14)go.spawned=false;if(reason==15)go.map=0;if(reason==16)go.zone=1;
   portal.OnGossipHello(&p,&go);assert(p.spells.empty()&&p.events.empty());
  }
  for(int reason=0;reason<10;++reason){p={};p.team=j->team;p.Accept(j->introduction);npc.map=1220;npc.zone=7502;
   if(reason==0)p.team=j->team==ALLIANCE?HORDE:ALLIANCE;if(reason==1)p.alive=false;if(reason==2)p.phase=false;if(reason==3)p.visible=false;if(reason==4)p.los=false;if(reason==5)p.pos.z=16;if(reason==6)npc.map=0;if(reason==7)npc.zone=1;if(reason==8)p.map=0;if(reason==9)p.flight=true;
   ai.MoveInLineOfSight(&p);assert(!p.packets);
  }
  p={};p.team=j->team;p.Accept(j->summons);p.data[{j->summons,j->reader}]=1;p.map=j->map;p.pos=j->destination;p.pos.z+=20;StormheimIntro::ConfirmArrival(&p,*j,0);assert(!p.packets);p.pos=j->destination;p.teleport=true;StormheimIntro::ConfirmArrival(&p,*j,0);assert(p.events.empty());
  p.teleport=false;player_stormheim_introduction login;login.OnLogin(&p);assert(p.GetQuestStatus(j->summons)==QUEST_STATUS_COMPLETE);auto saved=p.packets;login.OnLogin(&p);assert(p.packets==saved);
  // Independent callbacks for two players using the same shared portal.
  Player first,second;go.flags=0;go.spawned=true;go.map=1220;go.zone=7502;
  for(Player* owner:{&first,&second}){owner->team=j->team;owner->Accept(j->summons);owner->data[{j->summons,j->reader}]=1;portal.OnGossipHello(owner,&go);}
  first.teleport=false;first.map=j->map;first.pos=j->destination;first.Next();assert(first.GetQuestStatus(j->summons)==QUEST_STATUS_COMPLETE&&!second.packets);
  second.Next();assert(!second.packets);second.teleport=false;second.map=j->map;second.pos=j->destination;second.alive=false;second.Next();assert(!second.packets);second.alive=true;login.OnLogin(&second);assert(second.GetQuestStatus(j->summons)==QUEST_STATUS_COMPLETE);
 }
 // Shared markers must not change another introduction or an absent/rewarded quest.
 Player p;p.Accept(39735);p.Accept(39864);StormheimIntro::Credit(&p,39735,97377);assert(!p.GetQuestObjectiveData(39864,97377));
 for(int state:{QUEST_STATUS_NONE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){p={};p.quests[38035].Status=state;StormheimIntro::Credit(&p,38035,96663);assert(!p.packets);}
 std::cout<<"PASS: both factions, real completion and quest lifetime handlers; Dalaran arrival, letter prerequisite, acknowledged portal arrival, failed cast/teleport, duplicate/stale callbacks, wrong floor/faction/state and unrelated quest isolation.\n";
}
