#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>

enum { TYPEID_GAMEOBJECT, TYPEID_PLAYER, TYPEID_UNIT, TYPEID_DYNAMICOBJECT };
struct Guid
{
    int id;
    bool ship = false;
    bool IsMOTransport() const { return ship; }
    bool IsPlayer() const { return false; }
    bool operator<(Guid const& b) const { return id < b.id; }
};
struct WorldObject;
struct Player;
struct Transport;
using Unit = WorldObject;
struct WorldPacket { std::set<Guid> removed; };
struct UpdateData
{
    std::set<Guid> removed;
    void AddOutOfRangeGUID(Guid g) { removed.insert(g); }
    bool HasData() const { return !removed.empty(); }
    bool BuildPacket(WorldPacket* p) { p->removed = removed; return HasData(); }
};
struct Map
{
    std::map<Guid, Transport*> ships;
    Transport* GetTransport(Guid g) { auto it=ships.find(g);return it==ships.end()?nullptr:it->second; }
};
struct WorldObject
{
    Guid guid;
    bool inWorld = true, visible = true;
    Map* map = nullptr;
    Guid GetGUID() const { return guid; }
    bool IsInWorld() const { return inWorld; }
    Map* GetMap() const { return map; }
    int GetTypeId() const { return TYPEID_UNIT; }
    WorldObject* ToGameObject() { return this; }
    WorldObject* ToCreature() { return this; }
    WorldObject* ToDynObject() { return this; }
    Player* ToPlayer();
};
struct Transport : WorldObject
{
    using PassengerSet = std::set<WorldObject*>;
    PassengerSet passengers;
    PassengerSet const& GetPassengers() const { return passengers; }
};
struct Session
{
    std::vector<WorldPacket> packets;
    void SendPacket(WorldPacket* p) { packets.push_back(*p); }
};
struct Player : WorldObject
{
    Transport* ship = nullptr;
    std::set<Guid> known, extra;
    bool vehicle = false;
    Session session;
    Transport* GetTransport() { return ship; }
    bool IsOnVehicle() { return vehicle; }
    void* GetVehicleKit() { return nullptr; }
    bool HaveExtraLook(Guid g) { return extra.count(g); }
    bool canSeeOrDetect(WorldObject const* obj,bool,bool) { return obj->visible && obj->map==map; }
    void RemoveClient(Guid g) { known.erase(g); }
    void UpdateVisibilityOf(WorldObject*) { }
    void UpdateVisibilityOf(WorldObject*,UpdateData&,std::set<Unit*>&) { }
    void SendInitialVisiblePackets(Unit*) { }
    Session* GetSession() { return &session; }
};
Player* WorldObject::ToPlayer() { return static_cast<Player*>(this); }
struct ObjectAccessor { static Player* FindPlayer(Guid) { return nullptr; } };
namespace Trinity
{
struct VisibleNotifier
{
    Player& i_player;
    UpdateData i_data;
    std::set<Guid> vis_guids;
    std::set<Unit*> i_visibleNow;
    explicit VisibleNotifier(Player& p) : i_player(p),vis_guids(p.known) { }
    void SendToSelf();
};
}
using namespace Trinity;
#include "TransportSendToSelf.inc"

int main()
{
    // A transport created before self is now in known, but absent from grid
    // visits. Reconcile it through the native full visibility cleanup.
    for (bool aboard : {false,true})
    {
        Map map;Transport ship;ship.guid={55,true};ship.map=&map;map.ships[ship.guid]=&ship;
        Player p;p.map=&map;p.known.insert(ship.guid);if(aboard)p.ship=&ship;
        VisibleNotifier first(p);first.SendToSelf();
        assert(p.known.count(ship.guid) && first.i_data.removed.empty());
        VisibleNotifier repeated(p);repeated.SendToSelf();
        assert(p.known.count(ship.guid) && p.session.packets.empty());
    }
    // Removed/unloaded, phase-hidden and wrong-map ships must still clean up.
    for(int mode=0;mode<4;++mode)
    {
        Map map,other;Transport ship;ship.guid={55,true};ship.map=&map;
        map.ships[ship.guid]=&ship;Player p;p.map=&map;p.known.insert(ship.guid);
        if(mode==0)map.ships.clear();if(mode==1)ship.inWorld=false;
        if(mode==2)ship.visible=false;if(mode==3)ship.map=&other;
        VisibleNotifier gone(p);gone.SendToSelf();
        assert(!p.known.count(ship.guid) && gone.i_data.removed.count(ship.guid));
    }
    // Normal unseen objects disappear; seen passengers and vehicle extra look
    // retain their existing exceptions.
    Map map;Player p;p.map=&map;Transport ship;ship.map=&map;p.ship=&ship;
    WorldObject passenger;passenger.guid={70};passenger.map=&map;ship.passengers.insert(&passenger);
    p.known={{70},{80},{90}};p.extra.insert({90});p.vehicle=true;
    VisibleNotifier ordinary(p);ordinary.SendToSelf();
    assert(p.known.count({70}) && !p.known.count({80}) && p.known.count({90}));
    std::cout << "PASS: full native visibility cleanup retains live moving ships aboard/nearby and removes missing, hidden, unloaded and wrong-map ships; passengers/ordinary/vehicle cleanup preserved\n";
}
