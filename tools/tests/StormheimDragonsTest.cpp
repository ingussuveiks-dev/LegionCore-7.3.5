#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
using uint64=uint64_t;using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using int8=int8_t;
enum QuestStatus {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
enum {REACT_PASSIVE,UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,POINT_MOTION_TYPE=1,SPELL_AURA_MOUNTED=78};
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_DONT_REPORT};
#include "QuestStatusData.inc"
struct ObjectGuid {static ObjectGuid const Empty;unsigned value=0;bool operator<(ObjectGuid r)const{return value<r.value;}bool operator==(ObjectGuid r)const{return value==r.value;}bool operator!=(ObjectGuid r)const{return value!=r.value;}};
ObjectGuid const ObjectGuid::Empty{};
struct Position {float x=0,y=0,z=0,o=0;float GetOrientation()const{return o;}float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetExactDist(Position const* p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};
enum {QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_FLAG_OPTIONAL=4,QUEST_OBJECTIVE_FLAG_SEQUENCED=2,MOVEMENTFLAG_FALLING=1,MOVEMENTFLAG_FALLING_FAR=2,TEMPSUMMON_MANUAL_DESPAWN=8};
struct QuestObjective {int Type=0;uint32 ObjectID=0,Amount=1,Flags=0;int StorageIndex=0;};
struct Quest {std::vector<QuestObjective> objectives{{0,96466,1,0,0},{0,96467,1,2,1}};auto const& GetObjectives()const{return objectives;}} definition;
struct QuestStore {bool exists=true;Quest* GetQuestTemplate(uint32 id){assert(id==39652);return exists?&definition:nullptr;}} questStore;auto sQuestDataStore=&questStore;
struct GossipMenu {bool item=true;uint32 id=18693;uint32 GetMenuId(){return id;}void* GetItem(uint32 n){return !n&&item?this:nullptr;}};
struct PlayerMenu {GossipMenu gossip;GossipMenu& GetGossipMenu(){return gossip;}void ClearMenus(){gossip.item=false;}void SendCloseGossip(){}};
struct FakeMap {float floor=489.014f;float GetHeight(float,float,float,bool,float){return floor;}};
struct Player;struct Creature;struct ScriptedAI;
struct Unit:Position {ObjectGuid guid;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}ObjectGuid GetGUID(){return guid;}float GetDistance(Unit* u){return GetExactDist(u);}};
struct Spline {bool final=true;bool Finalized(){return final;}};
struct Vehicle {struct Info {uint32 ID=4367;} info;Info* GetVehicleInfo(){return &info;}Unit* passenger=nullptr;unsigned relocations=0;Unit* GetPassenger(int seat){assert(seat==0);return passenger;}void RelocatePassengers(){++relocations;}};
std::map<ObjectGuid,Creature*> world;unsigned serial=100;
struct Player:Unit {
 QuestStatusData quest;FakeMap geometry;uint32 zone=7541,instance=0,movement=0,packets=0;std::map<uint32,uint32> counters;PlayerMenu menu;PlayerMenu* PlayerTalkClass=&menu;std::function<Creature*()> summon;FakeMap* GetMap(){return &geometry;}uint32 GetZoneId(){return zone;}uint32 GetInstanceId(){return instance;}bool HasUnitMovementFlag(uint32 f){return movement&f;}Creature* SummonCreature(uint32,Position const&,int,int,int vehicle){assert(vehicle==4367);return summon?summon():nullptr;}uint32 GetQuestObjectiveData(uint32,uint32 id){return counters[id];}void SetQuestObjectiveData(Quest const*,QuestObjective const* o,uint32 v){counters[o->ObjectID]=v;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid g,QuestObjective const&,uint32 n){assert(g==ObjectGuid::Empty&&n==1);++packets;}bool CanCompleteQuest(uint32){return counters[96466]&&counters[96467];}void CompleteQuest(uint32){quest.Status=QUEST_STATUS_COMPLETE;}bool alive=true,inWorld=true,teleport=false,flight=false,flying=false,phase=true,visible=true,los=true,combat=false,transport=false,mounted=false,aura=true;
 unsigned map=1220,enterCalls=0,teleports=0,saves=0,controls=0;Creature* vehicle=nullptr;Unit* mover=this;Unit* view=nullptr;Spline spline;Spline* movespline=&spline;
 Position exitDest,savedPos;std::vector<int> order;
 explicit Player(unsigned id){guid={id};quest.Status=QUEST_STATUS_INCOMPLETE;}
 Player* ToPlayer()override{return this;}bool IsAlive(){return alive;}bool IsInWorld(){return inWorld;}unsigned GetMapId(){return map;}
 int GetQuestStatus(unsigned id){assert(id==39652);return quest.Status;}bool IsBeingTeleported(){return teleport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}bool isInCombat(){return combat;}bool GetTransport(){return transport;}
 Creature* GetVehicle(){return vehicle;}Unit* GetVehicleBase();Unit* GetUnitBeingMoved(){return mover;}Unit* GetViewpoint(){return view;}
 bool InSamePhase(Unit*){return phase;}bool canSeeOrDetect(Unit*){return visible;}bool IsWithinLOSInMap(Unit*){return los;}
 void SetClientControl(Unit* u,bool control){assert(control==(u==this));mover=u;view=u==this?nullptr:u;++controls;order.push_back(control?3:1);}
 void SetViewpoint(Unit*,bool apply){assert(!apply);view=nullptr;}
 void SendMovementFlagUpdate(bool value){assert(value);}void StopMoving(){}
 void RemoveAurasDueToSpell(unsigned id){assert(id==194359);aura=false;}void RemoveAurasByType(unsigned type){assert(type==SPELL_AURA_MOUNTED);mounted=false;}
 void EnterVehicle(Creature*,int seat,bool triggered){assert(seat==0 && triggered);++enterCalls;}
 void ExitVehicle();void NearTeleportTo(Position const& p){static_cast<Position&>(*this)=p;geometry.floor=p.z;spline.final=true;++teleports;}
 void SaveToDB(){++saves;savedPos=*this;}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void AddDelayedEvent(uint64,std::function<void()> callback){callback();}
 void Reaccept(){counters.clear();quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;}
};
#include "QuestDelayedEvent.inc"
struct Motion {Player* owner=nullptr;unsigned moves=0,id=0;Position destination;void MovePoint(unsigned point,Position const& p,bool generate,float speed){assert(!generate && speed==18);id=point;destination=p;++moves;if(owner){assert(owner->mover!=owner);owner->order.push_back(2);}}};
struct Creature:Unit {
 uint32 entry=92302,zone=7541,instance=0;uint64 dbGuid=0;uint32 GetEntry(){return entry;}uint64 GetDBTableGUIDLow(){return dbGuid;}uint32 GetMapId(){return 1220;}uint32 GetZoneId(){return zone;}uint32 GetInstanceId(){return instance;}bool IsInWorld(){return !gone;}Spline spline;Spline* movespline=&spline;Player* owner;ScriptedAI* ai=nullptr;bool alive=true,gone=false,personal=false,fly=false,gravity=false,active=false,hasKit=true;Motion motion;Vehicle kit;
 explicit Creature(Player& p):owner(&p){guid={++serial};world[guid]=this;motion.owner=&p;}
 bool IsAlive(){return alive;}bool IsDespawn(){return gone;}Vehicle* GetVehicleKit(){return hasKit?&kit:nullptr;}Motion* GetMotionMaster(){return &motion;}
 void DespawnOrUnsummon(unsigned=0){gone=true;}void AddPlayerInPersonnalVisibilityList(ObjectGuid g){assert(g==owner->guid);personal=true;}
 void SetReactState(unsigned){}void SetFlag(unsigned,unsigned){}void SetCanFly(bool v){fly=v;}void SetDisableGravity(bool v){gravity=v;}void setActive(bool v){active=v;}
 void NearTeleportTo(Position const& p){static_cast<Position&>(*this)=p;}void StopMoving(){}void SetFacingTo(float o2){o=o2;}
};
Unit* Player::GetVehicleBase(){return vehicle;}
struct ObjectAccessor {static Creature* GetCreature(Player& p,ObjectGuid g){auto i=world.find(g);return p.map!=1220||i==world.end()?nullptr:i->second;}};
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature* c):me(c){c->ai=this;}virtual ~ScriptedAI()=default;virtual void IsSummonedBy(Unit*){}virtual void PassengerBoarded(Unit*,int8,bool){}virtual void MovementInform(uint32,uint32){}virtual void CalcExitVehiclePos(Position&){};};
struct SmartAI:ScriptedAI {unsigned delegated=0;using ScriptedAI::ScriptedAI;virtual void sGossipHello(Player*){++delegated;}virtual void sGossipSelect(Player*,uint32,uint32){++delegated;}};
struct Template {uint32 ScriptID=1;};struct ObjectMgr {bool valid=true;Template info;Template* GetCreatureTemplate(uint32 id){assert(id==92302);return valid?&info:nullptr;}std::string GetScriptName(uint32){return "npc_stormheim_dragons";}} objectMgr;auto sObjectMgr=&objectMgr;
void Player::ExitVehicle(){if(!vehicle)return;auto bird=vehicle;bird->kit.passenger=nullptr;bird->ai->PassengerBoarded(this,0,false);vehicle=nullptr;exitDest=*bird;bird->ai->CalcExitVehiclePos(exitDest);spline.final=false;order.push_back(4);}
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnMapChanged(Player*){}virtual void OnLogout(Player*){};};
struct SpellScript {Unit* caster=nullptr;Unit* GetCaster(){return caster;}struct Hook{template<class T>void operator+=(T){}} OnCheckCast,AfterCast;virtual void Register(){};};
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(fn) &fn
#define SpellCastFn(fn) &fn
#include "DragonsRoute.inc"
#include "StormheimDragons.inc"
void Move(Unit& u,Position const& p){static_cast<Position&>(u)=p;if(auto player=u.ToPlayer())player->geometry.floor=p.z;}
struct Flight {
 Player p;Creature bird;npc_stormheim_dragons ai;
 explicit Flight(unsigned id):p(id),bird(p),ai(&bird){Move(p,StormheimDragons::Return);Move(bird,StormheimDragons::Return);StormheimDragons::starting[p.guid]={1};ai.IsSummonedBy(&p);StormheimDragons::starting.erase(p.guid);}
 ~Flight(){StormheimDragons::rides.erase(p.guid);world.erase(bird.guid);}
 void Update(unsigned diff=100){StormheimDragons::Update(&p,diff);}
 void Board(){Update();p.vehicle=&bird;bird.kit.passenger=&p;Move(p,bird);p.spline.final=false;ai.PassengerBoarded(&p,0,true);}
 void Fly(){Board();Update();assert(bird.motion.moves==0);p.spline.final=true;Update();assert(bird.motion.moves==1);}
 void Arrive(){auto r=StormheimDragons::rides.at(p.guid);Move(bird,StormheimDragons::Route[r.node]);Move(p,bird);ai.MovementInform(POINT_MOTION_TYPE,uint32(r.node+1));Update();}
 void CompleteExit(){assert(!p.vehicle);Move(p,p.exitDest);p.spline.final=true;Update();}
};
int main()
{
 player_stormheim_dragons script;
 for(unsigned faction=0;faction<2;++faction){Flight f(1+faction),other(5+faction);assert(f.bird.personal&&f.bird.active&&f.bird.fly&&f.bird.gravity);assert(!f.p.counters[96466]);
 Creature duplicate(f.p);Move(duplicate,StormheimDragons::Return);npc_stormheim_dragons duplicateAI(&duplicate);StormheimDragons::starting[f.p.guid]={1};duplicateAI.IsSummonedBy(&f.p);StormheimDragons::starting.erase(f.p.guid);assert(duplicate.gone&&StormheimDragons::rides.at(f.p.guid).bird==f.bird.guid);world.erase(duplicate.guid);
 f.Fly();assert(f.p.order[0]==1&&f.p.order[1]==2&&!f.p.counters[96466]);f.ai.MovementInform(POINT_MOTION_TYPE,1);f.Update();assert(!StormheimDragons::rides.at(f.p.guid).node);
 for(size_t n=0;n<StormheimDragons::RouteSize;++n)f.Arrive();assert(!f.p.vehicle&&f.p.mover==&f.p&&!f.p.view&&!f.p.counters[96466]);f.Update(1000);assert(StormheimDragons::rides.count(f.p.guid)&&!f.p.packets);f.CompleteExit();assert(!StormheimDragons::rides.count(f.p.guid)&&f.p.counters[96466]==1&&!f.p.counters[96467]&&f.p.quest.Status==QUEST_STATUS_INCOMPLETE&&f.p.teleports==0&&f.bird.gone);assert(StormheimDragons::rides.count(other.p.guid)&&!other.bird.gone&&!other.p.packets);
 Creature mother(f.p);mother.entry=92218;mother.dbGuid=146706157;Move(mother,{2042.73f,2662.03f,666.41f,0});npc_stormheim_dragons_giver ai(&mother);ai.sGossipHello(&f.p);assert(!f.p.counters[96467]);Move(f.p,mother);ai.sGossipHello(&f.p);ai.sGossipHello(&f.p);assert(f.p.counters[96467]==1&&f.p.packets==2&&f.p.quest.Status==QUEST_STATUS_COMPLETE);world.erase(mother.guid);}
 for(int reason=0;reason<8;++reason){Flight f(20+reason);f.Fly();f.Arrive();if(reason==0)f.p.Reaccept();if(reason==1)f.p.alive=false;if(reason==2)f.p.phase=false;if(reason==3)f.p.visible=false;if(reason==4)f.bird.alive=false;if(reason==5)f.p.ExitVehicle();if(reason==6)world.erase(f.bird.guid);f.Update(reason==7?180000:100);assert(!f.p.vehicle&&f.p.mover==&f.p&&!f.p.packets);f.CompleteExit();assert(f.p.GetExactDist(&StormheimDragons::Return)<0.01f&&!StormheimDragons::rides.count(f.p.guid)&&!f.p.packets);}
 {Flight f(40);f.Update(10000);f.Update();assert(!f.bird.motion.moves&&!f.p.packets&&StormheimDragons::CanStart(&f.p));}
 {Flight f(41);f.Fly();script.OnLogout(&f.p);assert(!f.p.vehicle&&f.p.mover==&f.p&&f.p.saves==1&&f.p.savedPos.GetExactDist(&StormheimDragons::Return)<0.01f&&!f.p.packets);}
 {Flight f(42);f.Fly();f.p.teleport=true;f.Update();assert(!f.p.vehicle&&f.p.mover==&f.p&&!f.p.teleports&&!f.p.packets);}
 {Flight f(43);f.Fly();f.p.map=1;script.OnMapChanged(&f.p);assert(!f.p.vehicle&&f.p.mover==&f.p&&!f.p.teleports&&!f.p.packets);}
 {Flight f(44);f.Fly();for(size_t n=0;n<StormheimDragons::RouteSize;++n)f.Arrive();f.p.Reaccept();f.CompleteExit();assert(!f.p.packets&&!f.p.counters[96466]);}
 {Flight f(45);f.Fly();for(size_t n=0;n<StormheimDragons::RouteSize;++n)f.Arrive();f.Update(10000);assert(f.p.teleports==1&&!f.p.counters[96466]);f.p.teleport=true;f.Update();assert(!f.p.counters[96466]);f.p.teleport=false;f.Update();assert(f.p.counters[96466]==1);}
 for(int reason=0;reason<13;++reason){Player p(50+reason);Move(p,StormheimDragons::Return);if(reason==0)p.alive=false;if(reason==1)p.quest.Status=QUEST_STATUS_COMPLETE;if(reason==2)p.map=1;if(reason==3)p.x+=20;if(reason==4)p.teleport=true;if(reason==5)p.combat=true;if(reason==6)p.transport=true;if(reason==7)p.flight=true;if(reason==8)p.movement=MOVEMENTFLAG_FALLING;if(reason==9)p.spline.final=false;if(reason==10)p.geometry.floor-=3;if(reason==11)p.instance=1;if(reason==12)p.zone=1;assert(!StormheimDragons::CanStart(&p));}
 {Player p(70);Move(p,StormheimDragons::Return);Creature source(p);Move(source,StormheimDragons::Return);source.entry=96465;source.dbGuid=146706156;npc_stormheim_dragons_giver giver(&source);unsigned summons=0;p.summon=[&]()->Creature*{++summons;return nullptr;};giver.sGossipSelect(&p,18693,0);assert(!summons);giver.sGossipHello(&p);p.Reaccept();giver.sGossipSelect(&p,18693,0);assert(!summons);giver.sGossipHello(&p);p.menu.gossip.item=false;giver.sGossipSelect(&p,18693,0);assert(!summons);p.menu.gossip.item=true;giver.sGossipSelect(&p,18693,1);assert(!summons);giver.sGossipSelect(&p,18693,0);assert(summons==1&&!p.packets&&!StormheimDragons::rides.count(p.guid));p.menu.gossip.item=true;giver.sGossipHello(&p);giver.sGossipSelect(&p,18693,0);assert(summons==2);StormheimDragons::offers.erase(p.guid);world.erase(source.guid);}
 {Player p(71);Creature mother(p);mother.entry=92218;mother.dbGuid=146706157;Move(p,mother);npc_stormheim_dragons_giver ai(&mother);ai.sGossipHello(&p);assert(!p.packets);p.counters[96466]=1;p.movement=MOVEMENTFLAG_FALLING;ai.sGossipHello(&p);assert(!p.packets);p.movement=0;mother.dbGuid=123;ai.sGossipHello(&p);assert(ai.delegated==1&&!p.packets);world.erase(mother.guid);}
 {Player p(72);Creature b(p);Move(p,StormheimDragons::Return);Move(b,StormheimDragons::Return);npc_stormheim_dragons ai(&b);ai.IsSummonedBy(&p);assert(!b.gone&&!StormheimDragons::rides.count(p.guid));world.erase(b.guid);}
 {Player p(73);Move(p,StormheimDragons::Return);Creature source(p);Move(source,StormheimDragons::Return);source.entry=96465;source.dbGuid=146706156;npc_stormheim_dragons_giver giver(&source);std::unique_ptr<Creature> ride;std::unique_ptr<npc_stormheim_dragons> ai;p.summon=[&]()->Creature*{ride=std::make_unique<Creature>(p);Move(*ride,StormheimDragons::Return);ai=std::make_unique<npc_stormheim_dragons>(ride.get());ai->IsSummonedBy(&p);return ride.get();};giver.sGossipHello(&p);giver.sGossipSelect(&p,18693,0);assert(ride&&ride->personal&&StormheimDragons::rides.at(p.guid).bird==ride->guid&&!p.packets&&StormheimDragons::starting.empty());script.OnLogout(&p);world.erase(ride->guid);world.erase(source.guid);}
 {Player p(74);Move(p,StormheimDragons::Return);definition.objectives[1].Flags=0;assert(!StormheimDragons::CanStart(&p));StormheimDragons::Credit(&p,96466);assert(!p.packets);definition.objectives[1].Flags=2;}
 {Flight f(75);f.Fly();Move(f.bird,StormheimDragons::Route[0]);Move(f.p,f.bird);f.bird.spline.final=false;f.ai.MovementInform(POINT_MOTION_TYPE,1);f.Update();assert(StormheimDragons::rides.at(f.p.guid).node==0&&!f.p.packets);f.bird.spline.final=true;f.Update();assert(StormheimDragons::rides.at(f.p.guid).node==1);}
 {Player p(76);Move(p,StormheimDragons::Return);Creature b(p);Move(b,StormheimDragons::Return);b.kit.info.ID=4133;npc_stormheim_dragons ai(&b);StormheimDragons::starting[p.guid]={1};ai.IsSummonedBy(&p);StormheimDragons::starting.erase(p.guid);assert(b.gone&&!StormheimDragons::rides.count(p.guid));world.erase(b.guid);}
 assert(StormheimDragons::rides.empty());std::cout<<"PASS: production ride/gossip/staged credit; complete boarding before mover/spline, exit before scoped credit, both faction-neutral runs, two riders, interruption/retry/cancellation/logout/transfer/timeout and unrelated actors. Vehicle/map/quest storage and completion are fixtures.\n";
}
