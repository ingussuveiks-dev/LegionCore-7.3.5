#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <vector>
#define TC_LOG_INFO(...) ((void)0)
#define TC_LOG_DEBUG(...) ((void)0)
using uint32 = uint32_t;
using int32 = int32_t;
using ObjectGuid = int;
constexpr uint32 MOVEMENTFLAG_CAN_FLY=1, MOVEMENTFLAG_DISABLE_GRAVITY=2,
    MOVEMENTFLAG_ROOT=4, MOVEMENTFLAG_MASK_MOVING=8, MOVEMENTFLAG_FALLING=16, MOVEMENTFLAG_FALLING_FAR=32;
constexpr uint32 UNIT_STATE_ROOT=1;
constexpr int SPELL_AURA_DISABLE_GRAVITY=1, GAMEOBJECT_FIELD_FLAGS=1, GO_FLAG_MAP_OBJECT=0x100000;
struct Player; struct Map;
struct Position {
    float x=0,y=0,z=747.6f;
    float GetPositionX() const{return x;} float GetPositionY() const{return y;} float GetPositionZ() const{return z;}
    float GetExactDist(Position const* p) const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}
};
struct WorldPacket {std::vector<int> objects;};
struct UpdateData {
    std::vector<int> objects; explicit UpdateData(int){}
    bool BuildPacket(WorldPacket* p){p->objects=objects;return !objects.empty();}
};
struct WorldObject:Position {
    int id; bool creature=false, inWorld=true, visible=true;
    explicit WorldObject(int n):id(n){}
    bool IsCreature()const{return creature;} bool IsInWorld()const{return inWorld;}
    int GetGUID()const{return id;}
    void BuildCreateUpdateBlockForPlayer(UpdateData* d,Player*)const{d->objects.push_back(id);}
};
using Creature=WorldObject;
struct Model {
    float height=747.6f; bool enabled=true,visible=true;
    float getHeight(float,float,float z,float range,std::set<uint32> const&,bool)const{
        return enabled&&visible&&height>=z-range&&height<=z?height:-std::numeric_limits<float>::infinity();
    }
};
struct GameObject:WorldObject {
    using WorldObject::WorldObject;
    bool mapObject=true;
    Model* m_model=nullptr;
    bool HasFlag(int,int)const{return mapObject;}
    float GetVisibilityRange()const{return 90;}
};
using StaticTransport=GameObject;
struct Transport:GameObject {
    using GameObject::GameObject; std::vector<WorldObject*> passengers;
    auto const& GetPassengers()const{return passengers;}
};
struct Session {std::vector<WorldPacket> packets;void SendPacket(WorldPacket* p){packets.push_back(*p);}};
struct Destination {int map=0;int GetMapId()const{return map;}};
struct Player:WorldObject {
    Player():WorldObject(1){}
    Session session; Transport* transport=nullptr;
    std::set<int> known; int mapId=0;
    uint32 flags=0,state=0;
    bool alive=true,vehicle=false,rootAura=false,gravityAura=false,selfCreateProtected=false,farTeleport=false;
    int visibilityRefreshes=0,gravityOrders=0,rootOrders=0;
    Destination dest;
    int32 _mapObjectArrivalMap=-1,_mapObjectArrivalLoadedMap=-1;
    uint32 _mapObjectArrivalWait=0,_mapObjectArrivalSettle=0;
    bool _mapObjectArrivalOwnRoot=false;
    ObjectGuid _mapObjectArrivalTransport=0;
    int GetMapId()const{return mapId;} bool IsAlive()const{return alive;}
    bool IsBeingTeleportedFar()const{return farTeleport;} auto const& GetTeleportDest()const{return dest;}
    void UpdateVisibilityForPlayer(){assert(_mapObjectArrivalMap==-1);++visibilityRefreshes;}
    bool CanFly()const{return flags&MOVEMENTFLAG_CAN_FLY;} bool IsLevitating()const{return flags&MOVEMENTFLAG_DISABLE_GRAVITY;}
    bool GetVehicle()const{return vehicle;} Transport* GetTransport()const{return transport;}
    bool HasUnitState(uint32 m)const{return state&m;} bool HasUnitMovementFlag(uint32 m)const{return flags&m;}
    bool HasAuraType(int)const{return gravityAura;}
    void RemoveUnitMovementFlag(uint32 m){flags&=~m;} void AddUnitMovementFlag(uint32 m){flags|=m;} void AddUnitState(uint32 m){state|=m;}
    void SetFallInformation(int,float){}
    void SetDisableGravity(bool enable){++gravityOrders;if(enable)flags|=MOVEMENTFLAG_DISABLE_GRAVITY;else flags&=~MOVEMENTFLAG_DISABLE_GRAVITY;}
    void SetControlled(bool enable,uint32){++rootOrders;if(enable){state|=UNIT_STATE_ROOT;flags|=MOVEMENTFLAG_ROOT;}else if(!rootAura){state&=~UNIT_STATE_ROOT;flags&=~MOVEMENTFLAG_ROOT;}}
    void BeginMapObjectArrival(bool notifyMovement=false);
    void NotifyMapObjectArrivalLoading(int32 mapId);
    void NotifyMapObjectArrivalLoaded(int32 mapId);
    bool IsMapObjectArrivalProtected()const;
    bool movieHeld=false;
    bool IsMovieMovementHeld()const{return movieHeld;}
    void UpdateMapObjectArrival(uint32 diff);
    void EndMapObjectArrival(bool refreshVisibility=false);
    bool ShouldDeferMapObjectArrivalVisibility(WorldObject const* target);
    bool HaveAtClient(WorldObject const* p)const{return known.count(p->id)!=0;}
    bool canSeeOrDetect(WorldObject const* p,bool,bool)const{return p->visible;}
    void AddClient(int n){known.insert(n);} Session* GetSession(){return &session;}
    float GetSightRange()const{return 90;}
    std::set<uint32> const& GetPhases()const{static std::set<uint32> p={1};return p;}
    void BuildCreateUpdateBlockForPlayer(UpdateData* d,Player* p){selfCreateProtected=IsMapObjectArrivalProtected()&&IsLevitating()&&HasUnitMovementFlag(MOVEMENTFLAG_ROOT);WorldObject::BuildCreateUpdateBlockForPlayer(d,p);}
    Player* GetUnitBeingMoved(){return this;} Player* ToPlayer(){return this;}
};
struct DynamicTreeCallback {GameObject* go=nullptr;};
struct Tree {
    GameObject* floor=nullptr;float height=747.6f;bool* gridLoaded=nullptr;
    float getHeight(float,float,float z,float range,std::set<uint32>const&,bool playerRules,DynamicTreeCallback* callback=nullptr)const{
        if(!floor||!floor->visible||!floor->inWorld||(gridLoaded&&!*gridLoaded)||height<z-range||height>z)
            return -std::numeric_limits<float>::infinity();
        if(callback){assert(playerRules);callback->go=floor;}
        return height;
    }
};
struct CellCoord{};
namespace Trinity {
    CellCoord ComputeCellCoord(float,float){return {};}
    struct GameObjectInRangeCheck {
        Position dest;float range;
        GameObjectInRangeCheck(float x,float y,float z,float r):range(r){dest.x=x;dest.y=y;dest.z=z;}
        bool operator()(GameObject* go){return dest.GetExactDist(go)<=range;}
    };
    template<class Check>struct GameObjectListSearcher{
        std::list<GameObject*>& objects;Check& check;
        GameObjectListSearcher(Player*,std::list<GameObject*>& o,Check& c):objects(o),check(c){}
    };
    template<class T>T& makeGridVisitor(T& t){return t;}
}
struct Map {
    std::map<int,StaticTransport*> m_StaticTransports;
    std::vector<Transport*> _transports;
    std::vector<GameObject*> grid;bool gridLoaded=false;Tree _dynamicTree;
    int id=0;float staticFloor=-std::numeric_limits<float>::infinity();
    Map(){_dynamicTree.gridLoaded=&gridLoaded;}
    int GetId()const{return id;}
    float GetHeight(float,float,float,bool,float)const{return staticFloor;}
    float GetHeight(std::set<uint32>const& phases,float x,float y,float z,bool vmap=true,float range=50,DynamicTreeCallback* callback=nullptr)const;
    void BuildPlayerArrivalObjects(Player*,Position const&,UpdateData&,bool notifyMovement=false);
    void SendInitSelf(Player*);
};
struct Cell {
    explicit Cell(CellCoord){}
    template<class T>void Visit(CellCoord,T& visitor,Map& map,float,float,float){
        map.gridLoaded=true;
        for(auto go:map.grid)if(visitor.check(go))visitor.objects.push_back(go);
    }
};
#include "BeginMapObjectArrival.inc"
#include "NotifyMapObjectArrivalLoading.inc"
#include "NotifyMapObjectArrivalLoaded.inc"
#include "IsMapObjectArrivalProtected.inc"
#include "UpdateMapObjectArrival.inc"
#include "EndMapObjectArrival.inc"
#include "ShouldDeferMapObjectArrivalVisibility.inc"
#include "GetArrivalHeight.inc"
#include "BuildPlayerArrivalObjects.inc"
#include "SendInitSelf.inc"
using Unit=Player;
struct MovementInfo {uint32 flags=0;};
struct WorldSession {
    Player* _player;int relocations=0;
    void RelocateMover(MovementInfo& movementInfo){
#include "ArrivalRelocatePrefix.inc"
        ++relocations;mover->flags=movementInfo.flags;
    }
};
int main(){
    Map map;Player player;GameObject floor(777),phasedOut(2),despawned(3);
    StaticTransport city(246854);phasedOut.visible=false;despawned.inWorld=false;
    map.grid={&floor,&phasedOut,&despawned};map.m_StaticTransports.emplace(city.id,&city);
    map._dynamicTree.floor=&floor;
    map.SendInitSelf(&player);
    assert((player.session.packets.back().objects==std::vector<int>{777,246854,1}));
    assert(player.selfCreateProtected&&map.gridLoaded&&player.known==std::set<int>({777,246854}));
    // Native Map::GetHeight must forward the actual callback, not convert it
    // to its preceding bool argument. This assertion failed with the old code.
    DynamicTreeCallback callback;
    assert(map.GetHeight(player.GetPhases(),0,0,749.6f,true,5,&callback)==747.6f&&callback.go==&floor);
    map.staticFloor=748;callback.go=nullptr;
    assert(map.GetHeight(player.GetPhases(),0,0,749.6f,true,5,&callback)==748&&!callback.go);
    map.staticFloor=-std::numeric_limits<float>::infinity();
    player.EndMapObjectArrival();map.SendInitSelf(&player);
    assert((player.session.packets.back().objects==std::vector<int>{1})&&player.selfCreateProtected);
    // An ordinary GO on an unrelated map, not the old entry/map special case.
    Map other;other.id=1220;other.grid={&floor};other._dynamicTree.floor=&floor;
    Player otherPlayer;otherPlayer.mapId=1220;other.SendInitSelf(&otherPlayer);
    assert((otherPlayer.session.packets.back().objects==std::vector<int>{777,1})&&otherPlayer.selfCreateProtected);
    // Supporting geometry with a distant origin must still precede self.
    GameObject large(88);large.x=500;Map big;big._dynamicTree.floor=&large;
    Player bigPlayer;big.SendInitSelf(&bigPlayer);
    assert((bigPlayer.session.packets.back().objects==std::vector<int>{88,1})&&bigPlayer.selfCreateProtected);
    // No floor / an airborne destination must not grant levitation.
    for(int mode=0;mode<4;++mode){
        Map air;Player p;GameObject hidden(55);air._dynamicTree.floor=&hidden;
        if(mode==0)air._dynamicTree.floor=nullptr;
        if(mode==1)air._dynamicTree.height=p.z-190;
        if(mode==2)hidden.visible=false;
        if(mode==3)hidden.inWorld=false;
        air.SendInitSelf(&p);assert(!p.selfCreateProtected&&p.flags==0);
    }
    // Same-map teleport: objects first, explicit movement orders for known self.
    Player near;Position dest;dest.x=1000;floor.x=1000;Map nearMap;nearMap.grid={&floor};nearMap._dynamicTree.floor=&floor;
    UpdateData data(0);nearMap.BuildPlayerArrivalObjects(&near,dest,data,true);
    assert(data.objects==std::vector<int>{777}&&near.gravityOrders==1&&near.rootOrders==1);
    assert(near.IsMapObjectArrivalProtected());
    near.NotifyMapObjectArrivalLoaded(0);assert(near._mapObjectArrivalSettle==8000);
    MovementInfo stale;WorldSession session{&near};session.RelocateMover(stale);
    assert(!session.relocations&&near.IsLevitating());
    near.movieHeld=true;
    near.UpdateMapObjectArrival(8000);session.RelocateMover(stale);assert(!session.relocations);
    near.movieHeld=false;session.RelocateMover(stale);assert(session.relocations==1);
    // Short movement on an already-visible platform must not root combat blinks.
    near.x=dest.x;UpdateData shortBlink(0);nearMap.BuildPlayerArrivalObjects(&near,dest,shortBlink,true);
    assert(!near.IsMapObjectArrivalProtected()&&shortBlink.objects.empty());
    // A known model may stream again after a long teleport within the same map.
    near.x=0;UpdateData longBlink(0);nearMap.BuildPlayerArrivalObjects(&near,dest,longBlink,true);
    assert(near.IsMapObjectArrivalProtected()&&longBlink.objects.empty());near.EndMapObjectArrival();
    floor.x=0;
    Player late;WorldObject npc(99),pet(416),otherObserver(200);npc.creature=pet.creature=true;
    late.flags=MOVEMENTFLAG_MASK_MOVING|MOVEMENTFLAG_FALLING;late.BeginMapObjectArrival();
    assert(late.ShouldDeferMapObjectArrivalVisibility(&npc)&&late.ShouldDeferMapObjectArrivalVisibility(&pet));
    assert(!late.ShouldDeferMapObjectArrivalVisibility(&floor)&&!late.ShouldDeferMapObjectArrivalVisibility(&otherObserver));
    late.known.insert(pet.id);assert(!late.ShouldDeferMapObjectArrivalVisibility(&pet));
    late.UpdateMapObjectArrival(20000);late.NotifyMapObjectArrivalLoaded(1);assert(!late._mapObjectArrivalSettle);
    late.NotifyMapObjectArrivalLoaded(0);late.UpdateMapObjectArrival(7000);late.NotifyMapObjectArrivalLoaded(0);
    assert(late._mapObjectArrivalSettle==1000);late.UpdateMapObjectArrival(1000);
    assert(late.flags==0&&late.state==0&&late.visibilityRefreshes==1);late.EndMapObjectArrival(true);assert(late.visibilityRefreshes==1);
    // Early, wrong-map, duplicate and next-load notifications.
    Player early;early.NotifyMapObjectArrivalLoaded(0);early.BeginMapObjectArrival();assert(early._mapObjectArrivalSettle==8000);
    early.EndMapObjectArrival();early.NotifyMapObjectArrivalLoading(0);early.BeginMapObjectArrival();assert(!early._mapObjectArrivalSettle);
    early.NotifyMapObjectArrivalLoaded(1);assert(!early._mapObjectArrivalSettle);
    early.farTeleport=true;early.dest.map=1;early.NotifyMapObjectArrivalLoaded(1);assert(!early._mapObjectArrivalSettle&&early._mapObjectArrivalLoadedMap==1);
    Player timeout;timeout.BeginMapObjectArrival();timeout.UpdateMapObjectArrival(29999);assert(timeout.IsLevitating());timeout.UpdateMapObjectArrival(1);assert(timeout.flags==0&&timeout.state==0);
    for(int mode=0;mode<4;++mode){
        Player p;Transport boat(9);if(mode==0)p.flags=MOVEMENTFLAG_CAN_FLY;if(mode==1)p.flags=MOVEMENTFLAG_DISABLE_GRAVITY;
        if(mode==2)p.vehicle=true;if(mode==3)p.alive=false;
        auto original=p.flags;p.BeginMapObjectArrival();p.EndMapObjectArrival();assert(p._mapObjectArrivalMap==-1&&p.flags==original);
    }
    for(int mode=0;mode<6;++mode){
        Player p;p.BeginMapObjectArrival();Transport boat(10);
        if(mode==0)p.EndMapObjectArrival();if(mode==1)p.mapId=1220;if(mode==2)p.alive=false;if(mode==3)p.vehicle=true;
        if(mode==4){p.inWorld=false;p.EndMapObjectArrival(true);}if(mode==5)p.transport=&boat;
        p.UpdateMapObjectArrival(1);assert(p.flags==0&&p._mapObjectArrivalMap==-1&&p.visibilityRefreshes==0);
    }
    Player rooted;rooted.flags=MOVEMENTFLAG_ROOT;rooted.state=UNIT_STATE_ROOT;
    rooted.BeginMapObjectArrival();rooted.EndMapObjectArrival();assert(rooted.flags==MOVEMENTFLAG_ROOT&&rooted.state==UNIT_STATE_ROOT);
    Player newAura;newAura.BeginMapObjectArrival();newAura.rootAura=newAura.gravityAura=true;newAura.EndMapObjectArrival();
    assert(newAura.flags==(MOVEMENTFLAG_ROOT|MOVEMENTFLAG_DISABLE_GRAVITY));
    Map normal;Player passenger;Transport boat(5);WorldObject known(6),unknown(7);
    boat.passengers={&passenger,&known,&unknown};passenger.transport=&boat;passenger.known.insert(6);normal.SendInitSelf(&passenger);
    assert((passenger.session.packets.back().objects==std::vector<int>{5,1,6}));
    // A moving ship can be pending insertion into the dynamic tree on the
    // first map update. Its real model still identifies the supporting floor.
    Map shipMap;Player sailor;Transport ship(55);Model deck;
    ship.m_model=&deck;shipMap._transports.push_back(&ship);sailor.transport=&ship;
    shipMap.SendInitSelf(&sailor);
    assert(sailor.selfCreateProtected);
    assert((sailor.session.packets.back().objects==std::vector<int>{55,1}));
    assert(sailor.ShouldDeferMapObjectArrivalVisibility(&npc));
    sailor.NotifyMapObjectArrivalLoaded(0);sailor.UpdateMapObjectArrival(7999);
    assert(sailor.IsMapObjectArrivalProtected());
    WorldSession shipSession{&sailor};MovementInfo staleShip;shipSession.RelocateMover(staleShip);
    assert(!shipSession.relocations&&sailor.transport==&ship);
    sailor.UpdateMapObjectArrival(1);
    assert(!sailor.IsMapObjectArrivalProtected()&&sailor.transport==&ship&&sailor.visibilityRefreshes==1);
    // Normal disembark, a different ship, and a vehicle cancel ownership.
    for(int mode=0;mode<3;++mode){
        Player p;Transport a(90),b(91);p.transport=&a;p.BeginMapObjectArrival();
        if(mode==0)p.transport=nullptr;if(mode==1)p.transport=&b;if(mode==2)p.vehicle=true;
        p.UpdateMapObjectArrival(1);assert(p._mapObjectArrivalMap==-1&&p.flags==0);
    }
    // No guessed ship floors: disabled/hidden models and deliberate air poses
    // must not get gravity protection just because a transport exists nearby.
    for(int mode=0;mode<3;++mode){
        Map m;Player p;Transport t(99);Model model;
        t.m_model=&model;m._transports.push_back(&t);
        if(mode==0)model.enabled=false;if(mode==1)t.visible=false;if(mode==2)p.z+=20;
        m.SendInitSelf(&p);assert(!p.IsMapObjectArrivalProtected());
    }
    std::cout<<"PASS: extracted native geometry-before-self, actual floor callback, ordinary/distant/known/hidden floors, same-map explicit gravity/root, stale ACK rejection, early/late/duplicate loading, timeouts, movement and aura ownership cleanup.\n";
}
