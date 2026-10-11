#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t;
enum {QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_OBJECTIVE_MONSTER=0,REACT_PASSIVE=0,POINT_MOTION_TYPE=1};
struct ObjectGuid {unsigned value=0;void Clear(){value=0;}bool operator<(ObjectGuid const&r)const{return value<r.value;}bool operator!=(ObjectGuid const&r)const{return value!=r.value;}};
struct Position {float x=0,y=0,z=0;};
struct Player;struct Creature;struct ScriptedAI;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct Vehicle {Unit* passenger=nullptr;Unit* GetPassenger(int seat){assert(seat==0);return passenger;}};
struct Motion {unsigned point=0,random=0;Position destination;void MovePoint(unsigned id,Position p){point=id;destination=p;}void MoveRandom(float){++random;}};
struct QuestObjective {int Type=0,ObjectID=100520,Amount=10;};
struct Quest {std::vector<QuestObjective> objectives{{}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore {Quest q;Quest enclave{{{0,94261,3},{0,98653,3}}};Quest const* GetQuestTemplate(unsigned id){assert(id==42104||id==39043);return id==42104?&q:&enclave;}} store;
auto sQuestDataStore=&store;
struct GroupReference {Player* player;GroupReference* following=nullptr;Player* getSource(){return player;}GroupReference* next(){return following;}};
struct Group {GroupReference* first;GroupReference* GetFirstMember(){return first;}};
struct Gossip {unsigned closes=0;void SendCloseGossip(){++closes;}};
struct Player:Unit {
 ObjectGuid guid;bool alive=true,teleport=false,visible=true,nearGroup=true;unsigned map=1220,area=7709,count=0,packets=0,drogbar=0,basilisk=0;float distance=1;std::map<unsigned,int> status;std::vector<unsigned> rescues;Group* group=nullptr;Gossip gossip;Gossip* PlayerTalkClass=&gossip;
 explicit Player(unsigned id):guid{id}{}Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}unsigned GetAreaId(){return area;}bool IsBeingTeleported(){return teleport;}
 int GetQuestStatus(unsigned q){return status[q];}float GetDistance(Creature*){return distance;}bool canSeeOrDetect(Creature*){return visible;}Group* GetGroup(){return group;}bool IsAtGroupRewardDistance(Creature*){return nearGroup;}
 int GetQuestObjectiveData(unsigned q,unsigned e){if(q==39043)return e==94261?drogbar:basilisk;assert(q==42104&&e==100520);return count;}
 void SetQuestObjectiveData(Quest const*,QuestObjective const* obj,unsigned n){if(obj->ObjectID==94261)drogbar=n;else if(obj->ObjectID==98653)basilisk=n;else count=n;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,unsigned n){assert(n==count);++packets;}bool CanCompleteQuest(unsigned q){if(q==39043)return drogbar>=3&&basilisk>=3;assert(q==42104);return count==10;}void CompleteQuest(unsigned q){status[q]=1;}
 void RewardPlayerAndGroupAtEvent(unsigned entry,Player* source){assert(source==this);rescues.push_back(entry);}
};
std::map<ObjectGuid,Creature*> world;std::vector<Creature*> allocated;unsigned serial=100;
struct Creature:Unit {
 ObjectGuid guid;unsigned entry=100520;Position pos;bool alive=true,gone=false,visible=true,samePhase=true,walk=true,gravity=false,hasKit=false,failSpawn=false,failExit=false,failBoard=false;unsigned delay=0,spawnCalls=0,boardCalls=0,react=1;Vehicle kit;Creature* parent=nullptr;Creature* refuge=nullptr;Motion motion;ScriptedAI* ai=nullptr;
 Creature(){guid={++serial};world[guid]=this;}Creature* ToCreature()override{return this;}ObjectGuid GetGUID(){return guid;}unsigned GetEntry(){return entry;}bool IsAlive(){return alive;}Position GetPosition(){return pos;}float GetPositionZ(){return pos.z;}float GetExactDist2d(float x,float y){return std::hypot(pos.x-x,pos.y-y);}Vehicle* GetVehicleKit(){return hasKit?&kit:nullptr;}Vehicle* GetVehicle(){return parent?&parent->kit:nullptr;}
 void EnterVehicle(Creature* p,int seat){assert(seat==0);++boardCalls;if(!failBoard){parent=p;p->kit.passenger=this;}}void ExitVehicle(){if(parent&&!failExit){parent->kit.passenger=nullptr;parent=nullptr;}}void SetDisableGravity(bool b){gravity=b;}
 Creature* FindNearestCreature(unsigned e,float distance,bool living){assert(e==93805&&distance==250&&living);return refuge;}bool InSamePhase(Creature*){return samePhase;}void SetReactState(unsigned r){react=r;}void SetWalk(bool w){walk=w;}Motion* GetMotionMaster(){return &motion;}
 void DespawnOrUnsummon(unsigned d=0){delay=d;if(!d){gone=true;ExitVehicle();}}void Kill(Creature* c){assert(c==this);alive=false;}
 Creature* SummonCreature(unsigned e,Position p){++spawnCalls;if(failSpawn)return nullptr;auto c=new Creature;c->entry=e;c->pos=p;allocated.push_back(c);return c;}
 ScriptedAI* AI();
};
struct ObjectAccessor {static Creature* GetCreature(Creature&,ObjectGuid guid){auto i=world.find(guid);return i==world.end()||i->second->gone?nullptr:i->second;}};
struct SpellInfo {unsigned Id;};
struct ScriptedAI {Creature* me;unsigned talks=0;explicit ScriptedAI(Creature* c):me(c){if(c)c->ai=this;}virtual ~ScriptedAI()=default;virtual void Reset(){}virtual void UpdateAI(uint32){}virtual void SpellHit(Unit*,SpellInfo const*){}virtual void MovementInform(uint32,uint32){}virtual void sGossipHello(Player*){}void Talk(unsigned,ObjectGuid){++talks;}};
ScriptedAI* Creature::AI(){static ScriptedAI fallback(nullptr);return ai?ai:&fallback;}
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogin(Player*){}};
#include "HighmountainRiverbend.inc"
int main()
{
 Player p(1),q(2),far(3),stranger(4);p.status[42104]=q.status[42104]=far.status[42104]=3;far.nearGroup=false;
 GroupReference r3{&far},r2{&q,&r3},r1{&p,&r2};Group group{&r1};p.group=&group;q.group=&group;
 Creature npc,refuge;refuge.entry=93805;npc.refuge=&refuge;npc_riverbend_evacuee ai(&npc);SpellInfo horn{210891},wrong{1};
 ai.SpellHit(&p,&wrong);ai.SpellHit(&stranger,&horn);assert(!p.count&&!ai.leaving);
 p.alive=false;ai.SpellHit(&p,&horn);p.alive=true;p.distance=12.01f;ai.SpellHit(&p,&horn);p.distance=1;p.visible=false;ai.SpellHit(&p,&horn);p.visible=true;
 p.area=7786;ai.SpellHit(&p,&horn);p.area=7709;p.map=1;ai.SpellHit(&p,&horn);p.map=1220;p.teleport=true;ai.SpellHit(&p,&horn);p.teleport=false;
 npc.refuge=nullptr;ai.SpellHit(&p,&horn);npc.refuge=&refuge;npc.samePhase=false;ai.SpellHit(&p,&horn);npc.samePhase=true;assert(!p.count&&!q.count);
 p.distance=12;ai.SpellHit(&p,&horn);assert(p.count==1&&q.count==1&&!far.count&&ai.leaving&&npc.motion.point==1&&npc.delay==60000&&!npc.walk);
 ai.SpellHit(&p,&horn);ai.SpellHit(&q,&horn);assert(p.count==1&&q.count==1);stranger.status[42104]=3;ai.SpellHit(&stranger,&horn);assert(stranger.count==1);
 ai.MovementInform(0,1);assert(npc.delay==60000);ai.MovementInform(POINT_MOTION_TYPE,1);assert(npc.delay==2000);
 p.status[42104]=0;ai.Reset();ai.SpellHit(&p,&horn);assert(p.count==1);p.status[42104]=3;p.count=0;q.count=0;
 for(unsigned i=0;i<10;++i){ai.Reset();ai.SpellHit(&p,&horn);assert(p.count==i+1&&q.count==i+1);}assert(p.status[42104]==1&&q.status[42104]==1);ai.Reset();ai.SpellHit(&p,&horn);assert(p.count==10);
 // Count persists across a simulated reconnect, without deferred credit or a stored player pointer.
 Player reconnect(1);reconnect.status[42104]=3;reconnect.count=9;ai.Reset();ai.SpellHit(&reconnect,&horn);assert(reconnect.count==10&&reconnect.status[42104]==1);
 struct Site{float x,y,z;unsigned entry;};Site sites[]={{3736.11f,4887.47f,675.735f,95051},{3649.89f,4908.65f,685.17f,94255},{3575.87f,4839.52f,700.372f,94286}};
 p.distance=1;p.status[39025]=3;
 for(auto s:sites){
  Creature hand;hand.entry=97648;hand.hasKit=true;hand.pos={s.x+0.001f,s.y,s.z};npc_grasp_of_underking_quest grasp(&hand);grasp.Reset();
  auto before=p.rescues.size();grasp.sGossipHello(&p);assert(p.rescues.size()==before); // nearby NPCs cannot substitute for an empty seat
  hand.failSpawn=true;grasp.UpdateAI(2000);assert(hand.spawnCalls==1&&!hand.kit.passenger);hand.failSpawn=false;grasp.UpdateAI(2000);assert(hand.spawnCalls==2);
  auto prisoner=hand.kit.passenger->ToCreature();assert(prisoner->entry==s.entry);grasp.UpdateAI(2000);assert(hand.spawnCalls==2);
  p.status[39025]=0;grasp.sGossipHello(&p);p.status[39025]=3;p.visible=false;grasp.sGossipHello(&p);p.visible=true;p.distance=6;grasp.sGossipHello(&p);p.distance=1;assert(p.rescues.size()==before);
  prisoner->entry=1;grasp.sGossipHello(&p);prisoner->entry=s.entry;assert(p.rescues.size()==before);
  prisoner->failExit=true;grasp.sGossipHello(&p);assert(p.rescues.size()==before&&hand.alive);prisoner->failExit=false;grasp.sGossipHello(&p);
  assert(p.rescues.size()==before+1&&p.rescues.back()==s.entry&&!prisoner->parent&&prisoner->delay==6500&&!hand.alive);
  grasp.sGossipHello(&p);grasp.UpdateAI(2000);assert(p.rescues.size()==before+1&&hand.spawnCalls==2);
  hand.alive=true;grasp.Reset();grasp.UpdateAI(2000);assert(hand.spawnCalls==3&&hand.kit.passenger!=prisoner);
 }
 Creature unrelated;unrelated.hasKit=true;npc_grasp_of_underking_quest other(&unrelated);other.UpdateAI(2000);assert(!unrelated.spawnCalls);
 player_riverbend_objectives repair;Player saved(8);saved.status[39043]=3;saved.drogbar=3;saved.basilisk=3;repair.OnLogin(&saved);assert(saved.status[39043]==1);
 saved.status[39043]=3;saved.drogbar=5;saved.basilisk=2;repair.OnLogin(&saved);assert(saved.drogbar==3&&saved.basilisk==2&&saved.status[39043]==3);
 saved.status[39043]=1;saved.drogbar=5;saved.basilisk=4;repair.OnLogin(&saved);assert(saved.drogbar==3&&saved.basilisk==3&&saved.status[39043]==1);
 saved.status[39043]=0;saved.drogbar=5;repair.OnLogin(&saved);assert(saved.drogbar==5&&saved.status[39043]==0);
 for(auto c:allocated)delete c;
 std::cout<<"PASS: extracted horn eligibility/range/phase, one credit per player/citizen, nearby group deduplication, ten warnings/completion/reconnect, actual passenger release, wrong/empty seat rejection, failed summon/exit recovery, respawn and existing 3/5+3/5 save reconciliation.\n";
}
