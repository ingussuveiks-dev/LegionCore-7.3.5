#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint8=uint8_t; using uint32=uint32_t; using int32=int32_t; using uint64=uint64_t;
enum QuestStatus {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
enum {UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP,UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE=2,
 UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,REACT_PASSIVE,CHAT_MSG_MONSTER_WHISPER,TEMPSUMMON_TIMED_DESPAWN};
#include "QuestStatusData.inc"
struct ObjectGuid {unsigned value=0;bool operator<(ObjectGuid const& r)const{return value<r.value;}};
using GuidUnorderedSet=std::set<ObjectGuid>;
struct Position {float x=0,y=0,z=0,o=0;};
namespace Movement {using PointsArray=std::vector<Position>;}
class Quest;struct Player;struct Creature;struct ScriptedAI;
struct Unit:Position {
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}
 Position const& GetPosition(){return *this;}
 float GetDistance(float a,float b,float c){return std::sqrt((x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c));}
 float GetDistance(Unit* u){return GetDistance(u->x,u->y,u->z);}
};
std::map<ObjectGuid,Creature*> creatures;std::map<ObjectGuid,Player*> players;
std::vector<std::unique_ptr<Creature>> allocated;unsigned serial=100;
struct Player:Unit {
 ObjectGuid guid;QuestStatusData quest;bool online=true,alive=true,teleport=false,vehicle=false,transport=false,flight=false,flying=false,phase=true,los=true,failSpawn=false;
 unsigned map=1220,credits=0,closed=0;std::map<unsigned,int> progress;
 struct Talk {Player* p;void SendCloseGossip(){++p->closed;}} talk{this};Talk* PlayerTalkClass=&talk;
 explicit Player(unsigned id):guid{id}{players[guid]=this;quest.Status=QUEST_STATUS_INCOMPLETE;}
 Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}
 bool IsInWorld(){return online;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}
 bool IsBeingTeleported(){return teleport;}bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}
 bool InSamePhase(Unit*){return phase;}bool IsWithinDistInMap(Unit* c,float r){return map==1220&&GetDistance(c)<=r;}bool IsWithinLOSInMap(Unit*){return los;}
 bool canSeeOrDetect(Creature*);
 int GetQuestStatus(unsigned q){assert(q==42088);return quest.Status;}
 int GetQuestObjectiveData(unsigned q,unsigned entry){assert(q==42088);return progress[entry];}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void AddDelayedEvent(uint64,std::function<void()> callback){callback();}
 void UpdateVisibilityOf(Creature*){}
 void KilledMonsterCredit(unsigned entry,ObjectGuid){assert(entry==106115||entry==106153||entry==106191);assert(!progress[entry]);progress[entry]=1;++credits;if(credits==3)quest.Status=QUEST_STATUS_COMPLETE;}
 Creature* SummonCreature(unsigned,Position const&,unsigned,unsigned,int,ObjectGuid);
 void Reaccept(){quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;progress.clear();credits=0;}
};
#include "QuestDelayedEvent.inc"
struct Spline {bool done=false;bool Finalized(){return done;}};
struct Creature:Unit {
 unsigned entry;ObjectGuid guid;Player* owner=nullptr;ScriptedAI* ai=nullptr;
 bool alive=true,gone=false,IsAIEnabled=true,personal=false,gossip=true,walk=true,failAura=false,failLaunch=false;
 unsigned aura=0,launches=0;Spline spline;Spline* movespline=&spline;GuidUnorderedSet _hideForGuid;
#include "EvacuationVisibility.inc"
 Creature(unsigned e,Player* p=nullptr):entry(e),guid{++serial},owner(p){creatures[guid]=this;}
 ~Creature();
 unsigned GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}ScriptedAI* AI(){return ai;}
 Creature* ToTempSummon(){return owner?this:nullptr;}Unit* GetSummoner(){return owner;}
 bool IsAlive(){return alive;}bool IsDespawn(){return gone;}void DespawnOrUnsummon(){gone=true;}
 void AddPlayerInPersonnalVisibilityList(ObjectGuid p){assert(owner&&p.value==owner->guid.value);personal=true;}
 void RemoveFlag(unsigned f,unsigned){assert(f==UNIT_FIELD_NPC_FLAGS);gossip=false;}void SetFlag(unsigned,unsigned){}void SetReactState(unsigned){}
 void CastSpell(Creature* c,unsigned id,bool triggered){assert(c==this&&triggered);if(!failAura)aura=id;}
 bool HasAura(unsigned id){return aura==id;}void SetWalk(bool v){walk=v;}
};
bool Player::canSeeOrDetect(Creature* c){return !c->ShouldHideFor(guid)&&(!c->personal||c->owner==this);}
struct ObjectAccessor {
 static Player* GetPlayer(Creature&,ObjectGuid g){auto it=players.find(g);return it==players.end()?nullptr:it->second;}
 static Creature* GetCreature(Creature&,ObjectGuid g){auto it=creatures.find(g);return it==creatures.end()?nullptr:it->second;}
};
struct ScriptedAI {
 Creature* me;explicit ScriptedAI(Creature* c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;
 virtual void AttackStart(Unit*){}virtual void MoveInLineOfSight(Unit*){}virtual void sGossipSelect(Player*,uint32,uint32){}
 virtual void IsSummonedBy(Unit*){}virtual void UpdateAI(uint32){}
};
Creature::~Creature(){creatures.erase(guid);if(owner)delete ai;}
struct TextMgr {unsigned calls=0;void SendChat(Creature*,unsigned group,ObjectGuid p,unsigned type){assert(group==0&&players.count(p)&&type==CHAT_MSG_MONSTER_WHISPER);++calls;}} texts;
auto sCreatureTextMgr=&texts;
namespace Movement {struct MoveSplineInit {
 Creature& c;PointsArray points;explicit MoveSplineInit(Creature& v):c(v){}
 void MovebyPath(PointsArray const& p){points=p;}void SetWalk(bool walk){assert(!walk);}
 int Launch(){assert(points.size()>2);if(c.failLaunch)return 0;++c.launches;return 25000;}
};}
#include "HighmountainEvacuation.inc"
Creature* Player::SummonCreature(unsigned entry,Position const& pos,unsigned,unsigned duration,int,ObjectGuid viewer)
{
 assert(duration==45000&&viewer.value==guid.value);if(failSpawn)return nullptr;
 auto c=std::make_unique<Creature>(entry,this);static_cast<Position&>(*c)=pos;
 new npc_highmountain_evacuation(c.get());c->AI()->IsSummonedBy(this);auto ptr=c.get();allocated.push_back(std::move(c));return ptr;
}
void Move(Unit& u,Position const& p){static_cast<Position&>(u)=p;}
int main()
{
 using namespace SnowmaneEvacuation;
 Creature red(106115),tarvim(106153),nefu(106191);Move(red,RedPath.front());Move(tarvim,TarvimPath.front());
 npc_highmountain_evacuation r(&red),t(&tarvim),n(&nefu);Player p(1),q(2);Move(p,red);Move(q,red);
 // The real visibility methods remove just one viewer, preserving everyone else.
 red.AddToHideList(p.guid);red.AddToHideList(q.guid);red.RemoveFromHideList(p.guid);assert(!red.ShouldHideFor(p.guid)&&red.ShouldHideFor(q.guid));red.RemoveFromHideList(q.guid);
 for(unsigned cause=0;cause<11;++cause){
  if(cause==0)p.quest.Status=QUEST_STATUS_NONE;if(cause==1)p.alive=false;if(cause==2)p.phase=false;if(cause==3)p.los=false;
  if(cause==4)p.map=1;if(cause==5)p.x+=8;if(cause==6)p.teleport=true;if(cause==7)p.vehicle=true;
  if(cause==8)p.progress[106115]=1;if(cause==9)p.failSpawn=true;if(cause==10)p.transport=true;
  r.sGossipSelect(&p,19560,0);assert(allocated.empty()&&!p.credits&&!red.ShouldHideFor(p.guid));
  p.Reaccept();p.alive=p.phase=p.los=true;p.map=1220;Move(p,red);p.teleport=p.vehicle=p.failSpawn=p.transport=false;
 }
 r.sGossipSelect(&p,19561,0);r.sGossipSelect(&p,19560,1);assert(allocated.empty());
 r.sGossipSelect(&p,19560,0);auto a=allocated.back().get();assert(a->personal&&!a->gossip&&red.ShouldHideFor(p.guid)&&!red.ShouldHideFor(q.guid));
 r.sGossipSelect(&p,19560,0);assert(allocated.size()==1);a->AI()->sGossipSelect(&p,19560,0);assert(allocated.size()==1);
 r.sGossipSelect(&q,19560,0);auto b=allocated.back().get();assert(b!=a&&b->owner==&q);
 a->AI()->UpdateAI(1999);assert(!p.credits&&!a->launches);a->AI()->UpdateAI(1);assert(p.credits==1&&a->aura==210246&&a->launches==1&&!q.credits);
 a->AI()->UpdateAI(1);assert(p.credits==1);a->spline.done=true;a->AI()->UpdateAI(1);assert(a->gone);
 r.UpdateAI(2000);assert(red.ShouldHideFor(p.guid)); // Keep source hidden after success.
 q.Reaccept();b->AI()->UpdateAI(1);r.UpdateAI(1000);assert(b->gone&&!red.ShouldHideFor(q.guid)&&red.ShouldHideFor(p.guid));
 for(unsigned cause=0;cause<7;++cause){
  q.Reaccept();Move(q,red);r.sGossipSelect(&q,19560,0);b=allocated.back().get();
  if(cause==0)b->failAura=true;if(cause==1)b->failLaunch=true;if(cause==2)q.alive=false;
  if(cause==3)q.online=false;if(cause==4)q.map=1;if(cause==5)q.phase=false;if(cause==6)q.x+=250;
  b->AI()->UpdateAI(2000);r.UpdateAI(1000);assert(b->gone&&!q.credits&&!red.ShouldHideFor(q.guid));
  q.online=q.alive=q.phase=true;q.map=1220;
 }
 Move(p,tarvim);t.sGossipSelect(&p,19561,0);b=allocated.back().get();b->AI()->UpdateAI(2000);assert(b->aura==210304&&p.credits==2&&!p.progress[106191]);
 Move(p,nefu);unsigned spawns=unsigned(allocated.size());n.sGossipSelect(&p,19565,0);assert(p.credits==3&&p.quest.Status==QUEST_STATUS_COMPLETE&&allocated.size()==spawns&&!nefu.ShouldHideFor(p.guid));
 n.sGossipSelect(&p,19565,0);assert(p.credits==3);Move(p,*b);b->AI()->UpdateAI(1);assert(!b->gone); // Last credit must not cancel a departure.
 p.Reaccept();r.UpdateAI(1000);t.UpdateAI(2000);assert(!red.ShouldHideFor(p.guid)&&!tarvim.ShouldHideFor(p.guid)&&b->gone);
 Move(p,red);r.sGossipSelect(&p,19560,0);a=allocated.back().get();a->AI()->UpdateAI(44000);r.UpdateAI(1000);assert(a->gone&&!p.credits&&!red.ShouldHideFor(p.guid));
 allocated.clear();
 std::cout<<"PASS: production gossip guards, independent viewers, exact objectives, duplicate clicks, native pickup spells, successful launch before credit, quest reaccept, aura/launch/spawn failures, death/logout/map/phase/range/timeouts and Old Nefu staying home.\n";
}
