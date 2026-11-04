// Actual complete transfer/removal/coordinate bodies, real std::set/std::vector
// under ASAN. Map, script, Player travel and lifetime endpoints are controlled.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
using uint32=uint32_t;
struct ObjectGuid{uint64_t low=0,high=0;ObjectGuid(uint64_t value=0):low(value){}bool operator==(ObjectGuid const& r)const{return low==r.low&&high==r.high;}bool operator!=(ObjectGuid const& r)const{return !(*this==r);}bool operator<(ObjectGuid const& r)const{return high<r.high||(high==r.high&&low<r.low);}};
enum{TYPEID_PLAYER=1,TYPEID_UNIT=2,TYPEID_DYNAMICOBJECT=3,TYPEID_AREATRIGGER=4,TELE_TO_NOT_LEAVE_TRANSPORT=8};
#define ASSERT(x) assert(x)
struct Position{float x=1,y=2,z=3,o=.25f;static float NormalizeOrientation(float v){while(v<0)v+=6.2831853f;while(v>=6.2831853f)v-=6.2831853f;return v;}void GetPosition(float& a,float& b,float& c,float& d)const{a=x;b=y;c=z;d=o;}};
struct Movement{struct Trans{Position Pos;void Reset(){Pos={};}}transport;};
struct Transport;struct Player;struct Unit;
struct WorldObject{virtual ~WorldObject()=default;uint32 type=TYPEID_PLAYER;ObjectGuid guid=0;Transport* transport=nullptr;Movement m_movementInfo;uint32 removed=0;ObjectGuid GetGUID()const{return guid;}uint32 GetTypeId()const{return type;}bool IsPlayer()const{return type==TYPEID_PLAYER;}Player* ToPlayer();Unit* ToUnit();void SetTransport(Transport* t){transport=t;}Transport* GetTransport()const{return transport;}void AddObjectToRemoveList(){++removed;}void SetTratsport(Transport*,Unit*);};
struct Unit:WorldObject{using ControlList=std::set<ObjectGuid>;ControlList m_Controlled;Unit* vehicle=nullptr;uint32 nearCalls=0;Position last;std::function<void()> nearHook;Unit* GetVehicleBase(){return vehicle;}void NearTeleportTo(float x,float y,float z,float o){++nearCalls;last={x,y,z,o};if(nearHook)nearHook();}};
struct Player:Unit{bool alive=true,change=false,accepted=true;uint32 farCalls=0,resurrections=0,lastMap=0,lastOptions=0;std::function<void()> farHook;bool IsAlive()const{return alive;}void ResurrectPlayer(float value){assert(value==1);alive=true;++resurrections;}bool IsChangeMap()const{return change;}bool TeleportTo(uint32 map,float x,float y,float z,float o,uint32 flags){++farCalls;lastMap=map;lastOptions=flags;last={x,y,z,o};if(farHook)farHook();return accepted;}};
Player* WorldObject::ToPlayer(){return IsPlayer()?static_cast<Player*>(this):nullptr;}Unit* WorldObject::ToUnit(){return type==TYPEID_PLAYER||type==TYPEID_UNIT?static_cast<Unit*>(this):nullptr;}void WorldObject::SetTratsport(Transport* t,Unit* owner){assert(!t&&owner);}
static std::map<ObjectGuid,Unit*> registry;
namespace ObjectAccessor{Unit* GetUnit(Player&,ObjectGuid guid){auto f=registry.find(guid);return f==registry.end()?nullptr:f->second;}}
struct Scripts{std::function<void(Transport*,Player*)> hook;uint32 removed=0;void OnRemovePassenger(Transport* t,Player* p){++removed;if(hook)hook(t,p);}}scripts;auto sScriptMgr=&scripts;
struct Map{uint32 id=1,removes=0,adds=0;uint32 GetId()const{return id;}template<class T>void RemoveFromMap(T* object,bool erase){assert(!erase);++removes;object->SetMap(nullptr);}template<class T>void AddToMap(T* object){assert(object->GetMap()==this);++adds;}};
struct Maps{Map next{2};Map* CreateBaseMap(uint32 id){next.id=id;return &next;}}maps;auto sMapMgr=&maps;
struct TransportBase{
#include "TransportPosition.inc"
};
struct Transport{using PassengerSet=std::set<WorldObject*>;PassengerSet _passengers,_staticPassengers;Map* map;Position pos;uint32 unloaded=0,updated=0;explicit Transport(Map* m):map(m){}Map* GetMap(){return map;}void SetMap(Map* value){map=value;}void UnloadStaticPassengers(){++unloaded;}void Relocate(float x,float y,float z,float o){pos={x,y,z,o};}void UpdatePosition(float x,float y,float z,float o){++updated;Relocate(x,y,z,o);}void Add(Player* p){_passengers.insert(p);p->transport=this;registry[p->guid]=p;}void RemovePassenger(WorldObject* passenger);bool TeleportTransport(uint32 map,float x,float y,float z,float o);};
#include "TransportRemove.inc"
#include "TransportTeleport.inc"
struct Fixture{Map old;Transport ship{&old};std::vector<std::unique_ptr<Player>> owners;std::vector<Player*> sorted;Fixture(uint32 n){registry.clear();scripts={};maps.next={2};for(uint32 i=0;i<n;++i){auto p=std::make_unique<Player>();p->guid=i+1;ship.Add(p.get());owners.push_back(std::move(p));}for(auto p:ship._passengers)sorted.push_back(p->ToPlayer());}~Fixture(){scripts.hook={};registry.clear();}void Retire(Player* p){ship.RemovePassenger(p);registry.erase(p->guid);for(auto& v:owners)if(v.get()==p){v.reset();break;}}};
int main(int argc,char** argv){
#ifdef TRANSPORT_BASELINE
 Fixture f(1);if(argc>1&&std::string(argv[1])=="near"){f.sorted[0]->nearHook=[&]{f.ship.RemovePassenger(f.sorted[0]);};std::cout<<"COUNTERFACTUAL original same-map callback erases current set node\n"<<std::flush;f.ship.TeleportTransport(1,10,20,30,0);}else{f.sorted[0]->type=TYPEID_UNIT;std::cout<<"COUNTERFACTUAL original map-change NPC removal erases current set node\n"<<std::flush;f.ship.TeleportTransport(2,10,20,30,0);}return 3;
#else
 (void)argc;(void)argv;uint32 cases=0;
 for(bool far:{false,true})for(float angle:{0.f,1.5707963f,3.14f})for(uint32 mode=0;mode<13;++mode){
  Fixture f(4);auto first=f.sorted[0],second=f.sorted[1],third=f.sorted[2],last=f.sorted[3];
  if(mode==1){first->accepted=false;first->nearHook=[&]{f.ship.RemovePassenger(first);};}
  if(mode==2){first->change=true;first->nearHook=[&]{f.ship.RemovePassenger(first);};}
  if(mode==3){first->alive=false;}
  if(mode==4){first->accepted=false;first->m_Controlled={second->guid,third->guid};second->type=third->type=TYPEID_UNIT;first->nearHook=[&]{f.ship.RemovePassenger(first);};}
  if(mode==5){auto remove=[&]{f.ship.RemovePassenger(second);};first->farHook=first->nearHook=remove;}
  if(mode==6){auto retire=[&]{f.Retire(second);};first->farHook=first->nearHook=retire;}
  if(mode==7){auto rebind=[&]{f.ship.RemovePassenger(second);registry.erase(second->guid);second->guid.high=99;f.ship.Add(second);};first->farHook=first->nearHook=rebind;}
  if(mode==8){auto join=[&]{auto p=std::make_unique<Player>();p->guid=100;f.ship.Add(p.get());f.owners.push_back(std::move(p));};first->farHook=first->nearHook=join;}
  if(mode==9){first->type=TYPEID_DYNAMICOBJECT;second->type=TYPEID_AREATRIGGER;third->type=TYPEID_UNIT;}
  Unit vehicle;if(mode==10){vehicle.transport=&f.ship;first->vehicle=&vehicle;}
  if(mode==11){first->accepted=false;auto leaveAll=[&]{auto snapshot=f.ship._passengers;for(auto object:snapshot)f.ship.RemovePassenger(object);};first->farHook=first->nearHook=leaveAll;}
  if(mode==12){first->accepted=false;auto self=[&]{f.ship.RemovePassenger(first);};first->farHook=first->nearHook=self;}
  bool result=f.ship.TeleportTransport(far?2:1,10,20,30,angle);assert(result==far);
  assert(far?(f.old.removes==1&&maps.next.adds==1&&f.ship.GetMap()==&maps.next&&f.ship.unloaded==1):(f.ship.updated==1&&f.ship.GetMap()==&f.old));
  for(auto const& p:f.owners)if(p){assert(p->nearCalls<=1&&p->farCalls<=1);if(p->farCalls){assert(p->lastOptions==TELE_TO_NOT_LEAVE_TRANSPORT&&p->lastMap==2);assert(std::abs(p->last.z-33)<.001f);}}
  for(auto const& p:f.owners)if(p&&(p->farCalls||p->nearCalls)){
   assert(std::abs(p->last.x-(10+std::cos(angle)-2*std::sin(angle)))<.001f);
   assert(std::abs(p->last.y-(20+std::sin(angle)+2*std::cos(angle)))<.001f);
   assert(std::abs(p->last.z-33)<.001f);
   assert(std::abs(p->last.o-Position::NormalizeOrientation(angle+.25f))<.001f);
  }
  if(mode==3&&far)assert(first->resurrections==1&&first->alive);
  if(mode==4)assert(!f.ship._passengers.count(first)&&!f.ship._passengers.count(second)&&!f.ship._passengers.count(third));
  if(mode==5||mode==7)assert(second->farCalls==0&&second->nearCalls==0);
  if(mode==8)assert(f.owners.back()->nearCalls==0&&f.owners.back()->farCalls==0);
  if(mode==9)assert(far?(first->removed==1&&second->removed==1&&!f.ship._passengers.count(third)):(first->nearCalls==0&&second->nearCalls==0&&third->nearCalls==0));
  if(mode==10&&!far)assert(first->nearCalls==0);
  if(mode==11)assert(f.ship._passengers.empty()&&last->farCalls==0&&last->nearCalls==0);
  ++cases;
 }
 {Fixture f(0);assert(f.ship.TeleportTransport(2,0,0,0,0));++cases;}
 std::cout<<"PASS "<<cases<<" actual transfer/removal/coordinate ASAN contexts: map change/same map, rejected/accepted travel, owner+pet+guardian removal, retired/rebound future actor, late joins, empty set, dynamic/area cleanup, vehicle deferral and dead-player resurrection. Engine map/script/travel/lifetime services controlled; not a client ship crossing\n";
#endif
}
