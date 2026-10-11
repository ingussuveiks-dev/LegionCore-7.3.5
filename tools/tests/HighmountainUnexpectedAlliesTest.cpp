#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
enum QuestStatus {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
enum {REACT_PASSIVE,UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,CHAT_MSG_MONSTER_WHISPER=15,POINT_MOTION_TYPE=1,TEMPSUMMON_TIMED_DESPAWN=1};
#include "QuestStatusData.inc"
struct ObjectGuid {unsigned value=0;bool operator<(ObjectGuid const& r)const{return value<r.value;}};
struct Position {float x=0,y=0,z=0,o=0;float GetExactDist(Position const* p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};
struct Player;struct Creature;struct ScriptedAI;using TempSummon=Creature;
struct Unit:Position {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return o;}float GetDistance(Unit* u){return GetExactDist(u);}};
struct QuestObjective {unsigned ID;int StorageIndex;};
struct Quest {std::vector<QuestObjective> objectives{{280678,0},{280843,1},{280680,2}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest q;Quest const* GetQuestTemplate(unsigned id){assert(id==39456);return &q;}} store;
auto sQuestDataStore=&store;
std::map<ObjectGuid,Creature*> world;std::vector<Creature*> allocated;unsigned serial=100;
struct Player:Unit {
 ObjectGuid guid;QuestStatusData quest;bool alive=true,teleport=false,flight=false,flying=false,vehicle=false,phase=true,visible=true,los=true,failSpawn=false;
 unsigned map=1220,packets=0,completions=0;std::vector<ObjectGuid> summons;
 struct TalkClass {void SendCloseGossip(){}} talk;TalkClass* PlayerTalkClass=&talk;
 explicit Player(unsigned id):guid{id}{quest.Status=QUEST_STATUS_INCOMPLETE;}
 Player* ToPlayer()override{return this;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}ObjectGuid GetGUID(){return guid;}
 int GetQuestStatus(unsigned id){assert(id==39456);return quest.Status;}
 bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}bool GetVehicle(){return vehicle;}
 bool InSamePhase(Unit*){return phase;}bool canSeeOrDetect(Unit*){return visible;}bool IsWithinDistInMap(Unit* u,float d){return GetDistance(u)<=d;}bool IsWithinLOSInMap(Unit*){return los;}
 int GetQuestObjectiveData(Quest const*,int index){return quest.ObjectiveData[index];}
 void SetQuestObjectiveData(Quest const*,QuestObjective const* obj,unsigned count){quest.ObjectiveData[obj->StorageIndex]=count;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,unsigned){++packets;}
 bool CanCompleteQuest(unsigned){return quest.ObjectiveData[0] && quest.ObjectiveData[1];}
 void CompleteQuest(unsigned){quest.Status=QUEST_STATUS_COMPLETE;++completions;}
 auto GetSummonList(unsigned entry){assert(entry==99624);return &summons;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void AddDelayedEvent(uint64,std::function<void()> callback){callback();}
 Creature* SummonCreature(unsigned,float,float,float,float,unsigned,unsigned,ObjectGuid);
 void Reaccept(){quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;}
};
#include "QuestDelayedEvent.inc"
struct Motion {unsigned moves=0;Position destination;void MovePoint(unsigned id,Position const& p){assert(id==1);destination=p;++moves;}};
struct Creature:Unit {
 unsigned entry;ObjectGuid guid;Player* owner;ScriptedAI* ai=nullptr;bool alive=true,gone=false,personal=false,walk=false;Motion motion;
 Creature(unsigned e,Player* p):entry(e),guid{++serial},owner(p){world[guid]=this;if(p)p->summons.push_back(guid);}
 bool IsAlive(){return alive;}bool IsDespawn(){return gone;}TempSummon* ToTempSummon(){return owner?this:nullptr;}Unit* GetSummoner(){return owner;}
 ObjectGuid GetGUID(){return guid;}ScriptedAI* AI(){return ai;}void DespawnOrUnsummon(){gone=true;}
 void AddPlayerInPersonnalVisibilityList(ObjectGuid g){assert(owner && g.value==owner->guid.value);personal=true;}
 void SetReactState(unsigned){}void SetFlag(unsigned,unsigned){}void SetWalk(bool value){walk=value;}Motion* GetMotionMaster(){return &motion;}void SetFacingToObject(Creature*){}
 Creature* FindNearestCreature(unsigned e,float range,bool){for(auto const& pair:world){auto c=pair.second;if(c->entry==e && c->alive && !c->gone && GetDistance(c)<=range)return c;}return nullptr;}
};
struct ObjectAccessor {static Creature* GetCreature(Player&,ObjectGuid g){auto i=world.find(g);return i==world.end()?nullptr:i->second;}};
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature* c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void MoveInLineOfSight(Unit*){}virtual void sGossipHello(Player*){}virtual void sGossipSelect(Player*,uint32,uint32){}virtual void IsSummonedBy(Unit*){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}virtual uint32 GetData(uint32)const{return 0;}};
struct TextMgr {std::vector<unsigned> recipients;void SendChat(Creature*,unsigned,ObjectGuid p,unsigned type){assert(type==CHAT_MSG_MONSTER_WHISPER);recipients.push_back(p.value);}} texts;
auto sCreatureTextMgr=&texts;
#include "HighmountainUnexpectedAllies.inc"
void Move(Unit& u,Position const& p){static_cast<Position&>(u)=p;}
Creature* Player::SummonCreature(unsigned entry,float x,float y,float z,float o,unsigned,unsigned,ObjectGuid g)
{
 assert(entry==99624 && g.value==guid.value);if(failSpawn)return nullptr;
 auto c=new Creature(entry,this);Move(*c,{x,y,z,o});allocated.push_back(c);new npc_unexpected_allies_ritual(c);c->AI()->IsSummonedBy(this);return c;
}
int main()
{
 Player p(1),q(2);Move(p,UnexpectedAllies::Start);Move(q,UnexpectedAllies::Start);
 Creature guide(99619,nullptr),mayla(93826,nullptr);Move(guide,UnexpectedAllies::Start);Move(mayla,UnexpectedAllies::Destination);
 npc_unexpected_allies_guide start(&guide);
 p.los=false;start.sGossipSelect(&p,18971,0);assert(allocated.empty());p.los=true;
 start.MoveInLineOfSight(&p);assert(p.quest.ObjectiveData[0]==1 && !p.quest.ObjectiveData[1] && p.completions==0);
 start.sGossipSelect(&p,1,0);assert(allocated.empty());p.failSpawn=true;start.sGossipSelect(&p,18971,0);assert(allocated.empty());p.failSpawn=false;
 start.sGossipSelect(&p,18971,0);auto actor=UnexpectedAllies::Existing(&p);assert(actor && actor->personal && actor->walk && actor->motion.moves==1);
 start.sGossipSelect(&p,18971,0);assert(allocated.size()==1);
 start.sGossipSelect(&q,18971,0);assert(UnexpectedAllies::Existing(&q)!=actor && q.quest.ObjectiveData[0]==1);
 actor->AI()->MovementInform(POINT_MOTION_TYPE,1);assert(!static_cast<npc_unexpected_allies_ritual*>(actor->AI())->arrived);
 actor->AI()->UpdateAI(120000);assert(actor->gone && !p.quest.ObjectiveData[1]); // Path timeout cannot grant credit.
 start.sGossipSelect(&p,18971,0);actor=UnexpectedAllies::Existing(&p);assert(actor && !actor->gone);
 Move(*actor,UnexpectedAllies::Destination);Move(p,UnexpectedAllies::Destination);
 actor->AI()->MovementInform(POINT_MOTION_TYPE,1);actor->AI()->UpdateAI(2000);
 for(unsigned i=0;i<5;++i){actor->AI()->UpdateAI(5000);assert(!p.quest.ObjectiveData[1] && !p.completions);}
 actor->AI()->UpdateAI(5000);assert(p.quest.ObjectiveData[0]==1 && p.quest.ObjectiveData[1]==1 && p.quest.ObjectiveData[2]==1 && p.completions==1 && actor->gone);
 assert(!q.quest.ObjectiveData[1] && !q.completions);
 auto other=UnexpectedAllies::Existing(&q);q.Reaccept();other->AI()->UpdateAI(1);assert(other->gone && !q.quest.ObjectiveData[1]);
 Move(q,UnexpectedAllies::Start);start.sGossipSelect(&q,18971,0);other=UnexpectedAllies::Existing(&q);assert(other);
 Move(*other,UnexpectedAllies::Destination);Move(q,UnexpectedAllies::Destination);other->AI()->MovementInform(POINT_MOTION_TYPE,1);
 mayla.alive=false;other->AI()->UpdateAI(2000);assert(other->gone && !q.quest.ObjectiveData[1]);mayla.alive=true;
 for(int cause=0;cause<4;++cause){
  q.Reaccept();Move(q,UnexpectedAllies::Start);start.sGossipSelect(&q,18971,0);other=UnexpectedAllies::Existing(&q);assert(other);
  if(cause==0)q.alive=false;if(cause==1)q.map=1;if(cause==2)q.phase=false;if(cause==3)q.x+=50;
  other->AI()->UpdateAI(1);assert(other->gone && !q.quest.ObjectiveData[1]);q.alive=true;q.map=1220;q.phase=true;
 }
 for(auto c:allocated){delete c->ai;delete c;}
 std::cout<<"PASS: distinct meet/witness/wait objectives, guarded and retryable gossip, two isolated owners, duplicate clicks, real arrival before timed dialogue, no early completion, path timeout, missing Mayla, quest lifetime, death/map/phase/range cancellation.\n";
}
