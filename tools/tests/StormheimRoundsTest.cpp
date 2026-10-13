#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
enum {ALLIANCE=469,HORDE=67,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,
QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS=16,QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR=64,QUEST_OBJECTIVE_FLAG_OPTIONAL=4,
QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=2,QUEST_SPECIAL_FLAGS_TIMED=4,UNIT_FIELD_NPC_FLAGS=0,REACT_PASSIVE=0,TEMPSUMMON_TIMED_DESPAWN=1,POINT_MOTION_TYPE=1,CHAT_MSG_MONSTER_WHISPER=1};
struct ObjectGuid {uint32 id=0;bool operator<(ObjectGuid r)const{return id<r.id;}bool operator==(ObjectGuid r)const{return id==r.id;}bool operator!=(ObjectGuid r)const{return id!=r.id;}static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct Position {float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}void Relocate(float a,float b,float c){x=a;y=b;z=c;}};
struct Player;class Creature;
struct WorldObject {Position pos;virtual ~WorldObject()=default;float GetExactDist(Position const* p)const{return std::sqrt((pos.x-p->x)*(pos.x-p->x)+(pos.y-p->y)*(pos.y-p->y)+(pos.z-p->z)*(pos.z-p->z));}float GetExactDist(WorldObject const* p)const{return GetExactDist(&p->pos);}};
class Unit:public WorldObject {public:virtual Player* ToPlayer(){return nullptr;}};
struct AI {virtual ~AI()=default;virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){}};
struct Motion {uint32 id=0,calls=0;Position destination;void MovePoint(uint32 token,float x,float y,float z,bool generatePath){assert(!generatePath);id=token;destination={x,y,z,0};++calls;}};
class Creature:public Unit {public:
 uint32 entry=0,map=0;ObjectGuid guid,personal;bool alive=true,despawn=false,combat=true,walk=false,active=false;AI* ai=nullptr;Motion motion;
 uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}bool IsDespawn(){return despawn;}auto AI()->::AI*{return ai;}
 void DespawnOrUnsummon(){despawn=true;}void StopMoving(){}Motion* GetMotionMaster(){return &motion;}
 void CombatStop(bool){combat=false;}void SetReactState(int){}void SetWalk(bool b){walk=b;}void setActive(bool b){active=b;}void SetUInt32Value(int,uint32){}
 void AddPlayerInPersonnalVisibilityList(ObjectGuid g){personal=g;}int CastSpell(Creature*,uint32,bool){return 0;}
};
struct GameObject:WorldObject {uint32 entry=247491,map=1;bool spawned=true;uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}bool isSpawned(){return spawned;}};
struct QuestObjective {int Type;uint32 ObjectID,Amount=1,Flags=0;};
struct Quest {uint32 id;std::vector<QuestObjective> objectives;auto const& GetObjectives()const{return objectives;}bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}bool IsRepeatable()const{return false;}bool IsAutoComplete()const{return false;}bool HasSpecialFlag(uint32)const{return false;}};
struct Store {std::map<uint32,Quest> quests;Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}} store;auto sQuestDataStore=&store;
struct QuestStatusData {int Status=0;uint32 Timer=0;std::shared_ptr<uint8> ScriptLifetime;};
struct Player:Unit {
 ObjectGuid guid;uint32 team=ALLIANCE,map=0,packets=0,summons=0;bool world=true,alive=true,teleport=false,vehicle=false,transport=false,flight=false,flying=false,phase=true,visible=true,los=true,combat=false,failSummon=false,failCast=false;
 std::map<uint32,QuestStatusData> quests;std::set<uint32> m_RewardedQuests,auras;std::map<std::pair<uint32,uint32>,uint32> progress;std::map<uint32,std::vector<ObjectGuid>> summonList;std::vector<std::function<void()>> events;
 Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}uint32 GetTeam(){return team;}uint32 GetMapId(){return map;}
 bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}bool isInCombat(){return combat;}
 bool InSamePhase(WorldObject*){return phase;}bool canSeeOrDetect(WorldObject*){return visible;}bool IsWithinLOSInMap(WorldObject*){return los;}
 bool IsWithinDistInMap(WorldObject* object,float distance){return GetExactDist(object)<=distance+1.5f;}
 auto GetSummonList(uint32 e){return &summonList[e];}Creature* SummonCreature(uint32,float,float,float,float,uint32,uint32);
 bool HasAura(uint32 a){return auras.count(a)!=0;}void RemoveAurasDueToSpell(uint32 a){auras.erase(a);}int CastSpell(Player*,uint32 a,bool){if(failCast)return 1;auras.insert(a);return 0;}
 QuestStatusData* getQuestStatus(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}int GetQuestStatus(uint32 q){auto s=getQuestStatus(q);return s?s->Status:0;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 o){return progress[{q,o}];}void SetQuestObjectiveData(Quest const* q,QuestObjective const* o,uint32 n){progress[{q->id,o->ObjectID}]=n;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32 n){assert(n==1);++packets;}
 bool HasQuestObjectiveComplete(Quest const* q,QuestObjective const& o){return GetQuestObjectiveData(q->id,o.ObjectID)>=o.Amount;}
 bool IsQuestWeekRewarded(uint32){return false;}bool IsQuestSeasonalRewarded(uint32){return false;}bool CanTakeQuest(Quest const*,bool){return true;}bool CanCompleteQuest(uint32);void CompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_COMPLETE;}void SetQuestStatus(uint32 q,int state){quests[q].Status=state;}
 void AddDelayedEvent(uint64,std::function<void()> fn){events.push_back(std::move(fn));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Accept(uint32 q){quests.erase(q);quests[q].Status=3;for(auto i=progress.begin();i!=progress.end();)if(i->first.first==q)i=progress.erase(i);else ++i;}
 void Next(){assert(!events.empty());auto fn=std::move(events.front());events.erase(events.begin());fn();}
};
#include "QuestDelayedEvent.inc"
#include "CanCompleteQuest.inc"
#include "RoundsHeader.inc"
std::map<ObjectGuid,Player*> players;std::map<ObjectGuid,Creature*> creatures;uint32 nextGuid=100;
namespace ObjectAccessor {Player* GetPlayer(Creature&,ObjectGuid g){auto i=players.find(g);return i==players.end()?nullptr:i->second;}Creature* GetCreature(Player&,ObjectGuid g){auto i=creatures.find(g);return i==creatures.end()?nullptr:i->second;}}
struct WayPoint {uint32 id;float x,y,z;};using WPPath=std::unordered_map<uint32,WayPoint*>;
struct Paths {std::map<uint32,WPPath> paths;WPPath* GetPath(uint32 p){auto i=paths.find(p);return i==paths.end()?nullptr:&i->second;}} paths;auto sSmartWaypointMgr=&paths;
struct Texts {unsigned lines=0;void SendChat(Creature*,uint32,ObjectGuid,int){++lines;}} texts;auto sCreatureTextMgr=&texts;
#include "Rounds.inc"
struct ActorAI:AI {StormheimRounds::Controller controller;explicit ActorAI(Creature* me):controller(me){}uint32 GetData(uint32 id)const override{return controller.GetData(id);}void SetData(uint32 id,uint32 v)override{controller.SetData(id,v);}};
Creature* Player::SummonCreature(uint32 e,float x,float y,float z,float o,uint32,uint32){if(failSummon)return nullptr;auto npc=new Creature;npc->entry=e;npc->map=map;npc->guid={nextGuid++};npc->pos={x,y,z,o};npc->ai=new ActorAI(npc);creatures[npc->guid]=npc;summonList[e].push_back(npc->guid);++summons;static_cast<ActorAI*>(npc->ai)->controller.Start(this);return npc;}
auto& Controller(Creature* c){return static_cast<ActorAI*>(c->ai)->controller;}
void Register(Player& p,uint32 id,StormheimRounds::Route const& r){p.guid={id};p.team=r.team;p.map=r.map;p.pos=r.start;p.Accept(r.quest);players[p.guid]=&p;}
void NativeProgress(Player& p,StormheimRounds::Route const& r,uint32 objective){assert(p.GetQuestStatus(r.quest)==3);p.progress[{r.quest,objective}]=1;if(p.CanCompleteQuest(r.quest))p.CompleteQuest(r.quest);}
void Tick(Player& p,Creature* c,bool move=true){p.pos=c->pos;Controller(c).Update(1000);if(move&&c->motion.calls){c->pos=c->motion.destination;p.pos=c->pos;Controller(c).Movement(POINT_MOTION_TYPE,c->motion.id);}}
void Drive(Player& p,Creature* c,uint32 readyKey){for(unsigned n=0;n<120&&!c->AI()->GetData(readyKey);++n)Tick(p,c);assert(c->AI()->GetData(readyKey));}
int main(){
 for(auto const* r:{&StormheimRounds::AllianceRoute,&StormheimRounds::HordeRoute}){
  store.quests[r->quest]={r->quest,{{r->team==ALLIANCE?0:2,r->inspection,1,0},{0,r->salute,1,2},{0,r->finalNpc,1,2}}};
  for(uint32 subset=0;subset<8;++subset){Player check;check.Accept(r->quest);for(auto pair:{std::make_pair(1u,r->inspection),std::make_pair(2u,r->salute),std::make_pair(4u,r->finalNpc)})if(subset&pair.first)check.progress[{r->quest,pair.second}]=1;assert(check.CanCompleteQuest(r->quest)==(subset==7));}
  for(uint32 part=0;part<3;++part){auto point=new WayPoint{1,r->start.x+float((part+1)*3),r->start.y,r->start.z};paths.paths[r->firstPath+part][1]=point;}
  Player p;Register(p,r->quest,*r);p.combat=true;StormheimRounds::Ensure(&p);assert(!p.summons);p.combat=false;p.failSummon=true;StormheimRounds::Ensure(&p);assert(!p.summons);p.failSummon=false;StormheimRounds::Ensure(&p);
  Creature* guide=StormheimRounds::Existing(&p,*r);assert(guide&&guide->personal==p.guid&&guide->walk&&!guide->combat);
  StormheimRounds::Ensure(&p);assert(p.summons==1);assert(!StormheimRounds::CanSalute(&p));
  auto& run=Controller(guide);run.Update(1000);auto token=guide->motion.id;run.Movement(POINT_MOTION_TYPE,token);assert(!guide->AI()->GetData(r->quest+100000)); // Early movement callback cannot finish a path.
  run.Update(31000);assert(guide->motion.id!=token&&!guide->AI()->GetData(r->quest+100000));guide->pos=guide->motion.destination;run.Movement(POINT_MOTION_TYPE,token);assert(!guide->AI()->GetData(r->quest+100000));run.Movement(POINT_MOTION_TYPE,guide->motion.id);assert(guide->AI()->GetData(r->quest+100000));
  Creature gun;gun.entry=110898;gun.map=0;gun.pos=p.pos;GameObject catapult;catapult.pos=p.pos;
  if(r->team==ALLIANCE){assert(StormheimRounds::CanInspect(&p,&gun));gun.pos.z+=5.2f;assert(StormheimRounds::CanInspect(&p,&gun));gun.pos.z+=20;assert(!StormheimRounds::CanInspect(&p,&gun));gun.pos=p.pos;gun.entry=110712;assert(!StormheimRounds::CanInspect(&p,&gun));gun.entry=110898;}else assert(StormheimRounds::CanInspectCatapult(&p,&catapult));
  for(unsigned n=0;n<50;++n)Tick(p,guide);assert(!p.HasAura(r->bar)&&!p.GetQuestObjectiveData(r->quest,r->finalNpc));
  if(r->team==ALLIANCE){StormheimRounds::Inspection cast;gun.pos=p.pos;assert(cast.Check(&p,&gun)&&!p.GetQuestObjectiveData(r->quest,r->inspection));p.los=false;cast.Hit(&p,&gun);assert(!p.GetQuestObjectiveData(r->quest,r->inspection));p.los=true;cast.Hit(&p,&gun);cast.Hit(&p,&gun);assert(p.packets==1);}else NativeProgress(p,*r,r->inspection);p.failCast=true;
  Drive(p,guide,r->quest+200000);for(unsigned n=0;n<40;++n)Tick(p,guide);assert(!p.HasAura(r->bar));p.failCast=false;Tick(p,guide);assert(p.HasAura(r->bar)&&StormheimRounds::CanSalute(&p));
  p.los=false;assert(!StormheimRounds::CanSalute(&p));p.los=true;p.combat=true;assert(!StormheimRounds::CanSalute(&p));p.combat=false;
  Creature finalNpc;finalNpc.entry=r->finalNpc;finalNpc.map=r->map;finalNpc.guid={nextGuid++};finalNpc.pos=p.pos;creatures[finalNpc.guid]=&finalNpc;auto initialEvents=p.events.size();StormheimRounds::FinalHello(&p,&finalNpc);assert(p.events.size()==initialEvents); // Still no final stage.
  NativeProgress(p,*r,r->salute);p.RemoveAurasDueToSpell(r->bar);for(unsigned n=0;n<80&&guide->AI()->GetData(r->quest)!=4;++n)Tick(p,guide);assert(guide->AI()->GetData(r->quest)==4&&!p.GetQuestObjectiveData(r->quest,r->finalNpc));
  finalNpc.pos=p.pos;finalNpc.pos.z+=20;StormheimRounds::FinalHello(&p,&finalNpc);assert(guide->AI()->GetData(r->quest)==4);finalNpc.pos=p.pos;
  p.events.clear();StormheimRounds::FinalHello(&p,&finalNpc);StormheimRounds::FinalHello(&p,&finalNpc);assert(guide->AI()->GetData(r->quest)==5&&p.events.size()==1&&!p.GetQuestObjectiveData(r->quest,r->finalNpc));
  p.Next();if(r->team==ALLIANCE){assert(!p.GetQuestObjectiveData(r->quest,r->finalNpc));p.Next();}assert(p.GetQuestStatus(r->quest)==QUEST_STATUS_COMPLETE&&p.packets==(r->team==ALLIANCE?2u:1u));run.Update(1000);assert(guide->despawn);
  // Re-entry after saved inspection resumes without inventing salute/final credit.
  Player retry;Register(retry,r->quest+10,*r);retry.progress[{r->quest,r->inspection}]=1;retry.progress[{r->quest,r->finalNpc}]=1;StormheimRounds::Ensure(&retry);assert(!retry.GetQuestObjectiveData(r->quest,r->finalNpc));auto recovered=StormheimRounds::Existing(&retry,*r);assert(recovered&&recovered->AI()->GetData(r->quest)==2);
  retry.Accept(r->quest);retry.auras.insert(r->bar);Controller(recovered).Update(1000);assert(recovered->despawn&&retry.HasAura(r->bar));StormheimRounds::Ensure(&retry);assert(!retry.HasAura(r->bar));
  auto fresh=StormheimRounds::Existing(&retry,*r);assert(fresh&&fresh!=recovered);retry.world=false;Controller(fresh).Update(1000);assert(fresh->despawn);retry.world=true;StormheimRounds::Ensure(&retry);assert(StormheimRounds::Existing(&retry,*r));
  // A missing path does not consume arrival; restoring it permits retry.
  Player missing;Register(missing,r->quest+20,*r);auto saved=paths.paths[r->firstPath];paths.paths.erase(r->firstPath);StormheimRounds::Ensure(&missing);auto waiting=StormheimRounds::Existing(&missing,*r);for(unsigned n=0;n<35;++n)Controller(waiting).Update(1000);assert(!waiting->motion.calls&&!waiting->AI()->GetData(r->quest+100000));paths.paths[r->firstPath]=saved;Drive(missing,waiting,r->quest+100000);
  auto oldToken=waiting->motion.id;missing.combat=true;Controller(waiting).Update(1000);Controller(waiting).Movement(POINT_MOTION_TYPE,oldToken);missing.combat=false;
  assert(StormheimRounds::CanStart(&missing,r->team==ALLIANCE?69333:69334));assert(!StormheimRounds::CanStart(&missing,r->team==ALLIANCE?69334:69333));
  // Two players never share their guide or objective storage.
  assert(StormheimRounds::Existing(&missing,*r)!=StormheimRounds::Existing(&retry,*r));assert(!retry.GetQuestObjectiveData(r->quest,r->salute));
  if(r->team==ALLIANCE){StormheimRounds::Inspection stale;gun.pos=missing.pos;assert(stale.Check(&missing,&gun));missing.Accept(r->quest);Controller(waiting).Update(1000);StormheimRounds::Ensure(&missing);auto newGuide=StormheimRounds::Existing(&missing,*r);Drive(missing,newGuide,r->quest+100000);gun.pos=missing.pos;assert(!stale.Check(&missing,&gun));stale.Hit(&missing,&gun);assert(!missing.GetQuestObjectiveData(r->quest,r->inspection));}
  // Missing finale actor restores retry; an abandoned queued dialogue cannot credit a new attempt.
  Player finale;Register(finale,r->quest+30,*r);finale.progress[{r->quest,r->inspection}]=1;finale.progress[{r->quest,r->salute}]=1;StormheimRounds::Ensure(&finale);auto lastGuide=StormheimRounds::Existing(&finale,*r);for(unsigned n=0;n<100&&lastGuide->AI()->GetData(r->quest)!=4;++n)Tick(finale,lastGuide);assert(lastGuide->AI()->GetData(r->quest)==4);
  finale.events.clear();finalNpc.pos=finale.pos;StormheimRounds::FinalHello(&finale,&finalNpc);creatures.erase(finalNpc.guid);finale.Next();assert(lastGuide->AI()->GetData(r->quest)==4&&!finale.packets);creatures[finalNpc.guid]=&finalNpc;StormheimRounds::FinalHello(&finale,&finalNpc);finale.Accept(r->quest);finale.Next();assert(!finale.packets);
  Creature other;other.entry=r->guide;other.map=1220;StormheimRounds::Controller unrelated(&other);assert(!unrelated.Start(&missing));
  players.erase(p.guid);players.erase(retry.guid);players.erase(missing.guid);players.erase(finale.guid);
 }
 std::cout<<"PASS: production personal rounds controller, both factions, actual completion and quest lifetime, movement acknowledgement, all three required actions, failed casts/summons/paths, combat, re-entry, stale lifetime, independent owners and later-map exclusion.\n";
}
