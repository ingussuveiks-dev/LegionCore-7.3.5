#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <crtdbg.h>
using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using int8=int8_t;
enum {HORDE=67,ALLIANCE=469,UNIT_FIELD_NPC_FLAGS=1,UNIT_FIELD_FLAGS=2,UNIT_NPC_FLAG_GOSSIP=1,UNIT_NPC_FLAG_QUESTGIVER=2,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,REACT_PASSIVE=0,POINT_MOTION_TYPE=8,TEMPSUMMON_TIMED_DESPAWN=1,INTERACTION_DISTANCE=5};
#define TC_LOG_INFO(...) (void)0
struct ObjectGuid {uint64 n=0;bool operator<(ObjectGuid b)const{return n<b.n;}bool operator==(ObjectGuid b)const{return n==b.n;}explicit operator bool()const{return n!=0;}};
struct Position {float x=0,y=0,z=0,o=0;float GetExactDist(Position const* p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};
struct Spline {bool done=true;bool Finalized(){return done;}};
struct Player;struct Creature;struct SmartAI;
struct Map{} stormheim,otherMap;
struct Unit:Position {ObjectGuid guid;bool world=true,alive=true;uint32 mapId=1220;Spline spline;Spline* movespline=&spline;virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}ObjectGuid GetGUID(){return guid;}bool IsAlive(){return alive;}bool IsInWorld(){return world;}Map* GetMap(){return mapId==1220?&stormheim:&otherMap;}void StopMoving(){spline.done=true;}};
struct Gossip {uint32 id=18762;bool item=true;uint32 GetMenuId(){return id;}void* GetItem(uint32){return item?this:nullptr;}};
struct Menu {Gossip gossip;unsigned closed=0;Gossip& GetGossipMenu(){return gossip;}void SendCloseGossip(){++closed;}};
struct Vehicle {Unit* passenger=nullptr;unsigned relocations=0;Unit* GetPassenger(int){return passenger;}void RelocatePassengers(){++relocations;}};
struct Motion {Creature* owner=nullptr;uint32 point=0,moves=0;Position destination;void MovePoint(uint32,Position const&,bool,float);};
struct Creature:Unit {uint32 entry=97250;uint64 dbGuid=340420;std::map<int,uint32> flags{{UNIT_FIELD_NPC_FLAGS,3}};SmartAI* ai=nullptr;Creature* source=nullptr;Vehicle kit;bool hasKit=true,fly=false,gravity=false,active=false;uint32 despawn=0;ObjectGuid viewer;Motion motion{this};
 uint32 GetEntry(){return entry;}uint64 GetDBTableGUIDLow(){return dbGuid;}bool HasFlag(int f,uint32 bit){return flags[f]&bit;}void SetFlag(int f,uint32 b){flags[f]|=b;}void RemoveFlag(int f,uint32 b){flags[f]&=~b;}void SetReactState(int){}void setActive(bool b){active=b;}void SetCanFly(bool b){fly=b;}void SetDisableGravity(bool b){gravity=b;}Motion* GetMotionMaster(){return &motion;}Vehicle* GetVehicleKit(){return hasKit?&kit:nullptr;}Position GetPosition(){return *this;}SmartAI* AI(){return ai;}void DespawnOrUnsummon(uint32=0);Creature* SummonCreature(uint32,Position const&,int,uint32,int,ObjectGuid);
};
struct Player:Unit {uint32 team=HORDE,instance=0,zone=7541;bool phase=true,near=true,teleport=false,transport=false,flight=false,combat=false;ObjectGuid charmer;Creature* vehicle=nullptr;Creature* requested=nullptr;Unit* moved=this;Unit* view=nullptr;Menu menu;Menu* PlayerTalkClass=&menu;uint32 exits=0,teleports=0,controls=0,updates=0;Position exitDestination;std::vector<std::function<void()>> events;
 Player* ToPlayer()override{return this;}uint32 GetTeam(){return team;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instance;}uint32 GetZoneId(){return zone;}bool InSamePhase(Creature*){return phase;}bool IsWithinDistInMap(Creature*,float){return near;}bool IsBeingTeleported(){return teleport;}Vehicle* GetVehicle(){return vehicle?vehicle->GetVehicleKit():nullptr;}Creature* GetVehicleBase(){return vehicle;}bool GetTransport(){return transport;}bool isInFlight(){return flight;}bool isInCombat(){return combat;}ObjectGuid GetCharmerGUID(){return charmer;}Unit* GetUnitBeingMoved(){return moved;}Unit* GetViewpoint(){return view;}
 void SetClientControl(Unit* u,bool on){assert(on==(u==this));moved=u;view=u==this?nullptr:u;++controls;}void SetViewpoint(Unit* u,bool on){assert(!on&&view==u);view=nullptr;}void SendMovementFlagUpdate(bool){++updates;}void EnterVehicle(Creature* c,int,bool){requested=c;}void ExitVehicle(Position const* =nullptr);void NearTeleportTo(Position const& p){assert(!vehicle&&spline.done);static_cast<Position&>(*this)=p;++teleports;}void AddDelayedEvent(uint32,std::function<void()> f){events.push_back(std::move(f));}
};
struct SmartAI {Creature* me;unsigned delegated=0;explicit SmartAI(Creature* c):me(c){c->ai=this;}virtual ~SmartAI()=default;virtual void sGossipSelect(Player*,uint32,uint32){++delegated;}virtual void SetGUID(ObjectGuid const&,int32=0){}virtual void PassengerBoarded(Unit*,int8,bool){++delegated;}virtual void JustDied(Unit*){++delegated;}virtual void SummonedCreatureDespawn(Creature*){++delegated;}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){++delegated;}};
std::map<ObjectGuid,Player*> players;std::map<ObjectGuid,Creature*> creatures;
namespace ObjectAccessor {Player* FindPlayer(ObjectGuid id){auto i=players.find(id);return i==players.end()||!i->second->world?nullptr:i->second;}Player* GetPlayer(Creature& c,ObjectGuid id){auto p=FindPlayer(id);return p&&p->GetMap()==c.GetMap()?p:nullptr;}Creature* GetCreature(Creature&,ObjectGuid id){auto i=creatures.find(id);return i==creatures.end()||!i->second->world?nullptr:i->second;}}
#include "BatPath.inc"
#include "BatRider.inc"
void Motion::MovePoint(uint32 id,Position const& p,bool path,float speed){assert(!path&&speed==24.0f);assert(owner->kit.passenger&&owner->kit.relocations&&owner->fly&&owner->gravity);auto rider=owner->kit.passenger->ToPlayer();assert(rider&&rider->moved==owner&&rider->spline.done);point=id;destination=p;++moves;owner->spline.done=false;}
void Player::ExitVehicle(Position const* p){if(!vehicle)return;auto c=vehicle;++exits;c->AI()->PassengerBoarded(this,0,false);assert(vehicle==c);vehicle=nullptr;c->kit.passenger=nullptr;exitDestination=p?*p:c->GetPosition();spline.done=false;}
void Creature::DespawnOrUnsummon(uint32 delay){despawn=delay?delay:1;if(kit.passenger)kit.passenger->ToPlayer()->ExitVehicle();world=false;if(source)source->AI()->SummonedCreatureDespawn(this);}
bool failSummon=false,failKit=false;uint64 serial=1000;std::vector<std::unique_ptr<Creature>> summons;std::vector<std::unique_ptr<SmartAI>> ais;
Creature* Creature::SummonCreature(uint32 id,Position const& p,int type,uint32 lifetime,int vehicleId,ObjectGuid owner){if(failSummon)return nullptr;assert(id==97250&&type==TEMPSUMMON_TIMED_DESPAWN&&lifetime==90000&&!vehicleId);auto c=std::make_unique<Creature>();static_cast<Position&>(*c)=p;c->dbGuid=0;c->source=this;c->viewer=owner;c->guid={++serial};c->hasKit=!failKit;auto result=c.get();creatures[c->guid]=result;auto ai=std::make_unique<npc_stormheim_foothold_batrider>(result);ais.push_back(std::move(ai));summons.push_back(std::move(c));return result;}
using AI=npc_stormheim_foothold_batrider;
struct Run {Creature source;AI ai{&source};Player p;Run(){source.guid={++serial};static_cast<Position&>(source)=StormheimBatRider::Source;creatures[source.guid]=&source;p.guid={++serial};players[p.guid]=&p;}Creature* Start(){ai.sGossipSelect(&p,18762,0);return p.requested;}};
void Board(Player& p,Creature* c){assert(c&&p.requested==c);p.vehicle=c;c->kit.passenger=&p;p.spline.done=false;c->AI()->PassengerBoarded(&p,0,true);}
void Fly(Player& p,Creature* c){Board(p,c);c->AI()->UpdateAI(100);assert(!c->motion.moves);p.spline.done=true;c->AI()->UpdateAI(100);assert(c->motion.moves==1&&p.moved==c&&p.updates==1);}
void Arrive(Creature* c){auto a=static_cast<AI*>(c->AI());static_cast<Position&>(*c)=c->motion.destination;c->spline.done=true;a->MovementInform(POINT_MOTION_TYPE,c->motion.point);}
void FinishExit(Player& p,Creature* c){static_cast<Position&>(p)=p.exitDestination;p.spline.done=true;c->AI()->UpdateAI(100);}
int main(){_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 for(unsigned bad=0;bad<21;++bad){Run r;auto& p=r.p;if(bad==0)p.team=ALLIANCE;if(bad==1)p.world=false;if(bad==2)p.alive=false;if(bad==3)p.mapId=1;if(bad==4)p.instance=1;if(bad==5)p.zone=1;if(bad==6)p.phase=false;if(bad==7)p.near=false;if(bad==8)p.teleport=true;if(bad==9)p.transport=true;if(bad==10)p.flight=true;if(bad==11)p.combat=true;if(bad==12)p.charmer={1};if(bad==13)p.menu.gossip.id=1;if(bad==14)p.menu.gossip.item=false;if(bad==15)r.source.dbGuid=1;if(bad==16)r.source.entry=1;if(bad==17)r.source.alive=false;if(bad==18)r.source.flags[UNIT_FIELD_NPC_FLAGS]=2;if(bad==19)p.vehicle=&r.source;if(bad==20)r.source.mapId=1;r.ai.sGossipSelect(&p,18762,0);assert(!p.requested&&!p.menu.closed);}
 {Run r;r.ai.sGossipSelect(nullptr,18762,0);r.ai.sGossipSelect(&r.p,1,0);assert(r.ai.delegated==1);}
 {Run r;failSummon=true;assert(!r.Start()&&!r.p.menu.closed);failSummon=false;assert(r.Start());}
 {Run r;failKit=true;r.Start();assert(!r.p.requested);failKit=false;assert(r.Start());}
 {Run r;auto c=r.Start();auto a=static_cast<AI*>(c->AI());assert(c->viewer==r.p.guid&&c->active&&!c->flags[UNIT_FIELD_NPC_FLAGS]);r.ai.sGossipSelect(&r.p,18762,0);assert(r.ai.flights.size()==1);c->AI()->UpdateAI(100);assert(!c->motion.moves&&!r.p.controls);Fly(r.p,c);a->MovementInform(0,100);a->MovementInform(8,999);a->MovementInform(8,100);assert(c->motion.moves==1);for(int i=0;i<4;++i)Arrive(c);assert(a->stage==AI::Stage::Exiting&&r.p.exits==1&&r.p.moved==&r.p&&!r.p.view&&!c->despawn);a->UpdateAI(100);assert(!c->despawn&&!r.p.teleports);FinishExit(r.p,c);assert(c->despawn&&r.p.GetExactDist(&StormheimBatRider::Landing)<.001f&&!r.p.teleports&&r.source.world&&r.source.flags[UNIT_FIELD_NPC_FLAGS]==3&&r.ai.flights.empty());}
 {Run r;auto c=r.Start();c->AI()->UpdateAI(10000);assert(static_cast<AI*>(c->AI())->stage==AI::Stage::Exiting&&!c->motion.moves);c->AI()->UpdateAI(100);assert(c->despawn&&!r.p.teleports&&r.p.moved==&r.p);}
 for(int mode=0;mode<4;++mode){Run r;auto c=r.Start();Fly(r.p,c);if(mode==0)r.p.ExitVehicle();if(mode==1)c->AI()->UpdateAI(60000);if(mode==2){r.p.ExitVehicle();r.p.mapId=1;}if(mode==3){r.p.ExitVehicle();r.p.teleport=true;}assert(r.p.moved==&r.p&&!r.p.view);c->AI()->UpdateAI(100);assert(!r.p.teleports);FinishExit(r.p,c);assert(c->despawn&&r.p.teleports==unsigned(mode<2));}
 {Run r;auto c=r.Start();Fly(r.p,c);r.p.world=false;c->AI()->UpdateAI(100);assert(c->despawn&&r.p.moved==&r.p&&!r.p.view&&!r.p.teleports);}
 {Run r;auto c=r.Start();Fly(r.p,c);r.p.ExitVehicle();Creature other;r.p.vehicle=&other;r.p.moved=&other;r.p.view=&other;c->AI()->UpdateAI(100);assert(c->despawn&&!r.p.teleports&&r.p.moved==&other);r.p.vehicle=nullptr;}
 {Run r;auto c=r.Start();Fly(r.p,c);c->AI()->JustDied(nullptr);assert(r.p.moved==&r.p&&!r.p.events.empty());r.p.spline.done=true;auto events=std::move(r.p.events);for(auto& event:events)event();assert(r.p.teleports==1);}
 {Run r;auto c=r.Start();Fly(r.p,c);r.p.ExitVehicle();c->AI()->UpdateAI(5000);assert(c->despawn&&r.p.teleports==1);} // Stalled exit spline is bounded.
 {Run r;auto c=r.Start();Fly(r.p,c);static_cast<Position&>(*c)=c->motion.destination;c->spline.done=true;c->AI()->UpdateAI(100);assert(c->motion.moves==2);} // Actual endpoint retry, not a timer credit.
 {Run r;Player other;other.guid={++serial};players[other.guid]=&other;auto first=r.Start();r.ai.sGossipSelect(&other,18762,0);assert(other.requested&&other.requested!=first&&r.ai.flights.size()==2);}
 std::cout<<"PASS: production Bat-Rider script;21 admission rejections, viewer-scoped per-player runs/deduplication, summon/vehicle/boarding failure retries, completed boarding before mover handoff, four actual endpoints, callback retry, normal exit wait, early exit/timeout/source recovery, logout/map-transfer/new-vehicle isolation, forced-death recovery and bounded stalled exit. Spell/vehicle join, movement, event timing, lookup, packets and pet-frame/client rendering are adapters.\n";
}
