#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <cstdint>
#define TC_LOG_INFO(...) ((void)0)
using uint32 = uint32_t;
using int32 = int32_t;
constexpr uint32 MOVEMENTFLAG_CAN_FLY = 1, MOVEMENTFLAG_DISABLE_GRAVITY = 2,
    MOVEMENTFLAG_ROOT = 4, MOVEMENTFLAG_MASK_MOVING = 8, MOVEMENTFLAG_FALLING = 16, MOVEMENTFLAG_FALLING_FAR = 32;
constexpr uint32 UNIT_STATE_ROOT = 1;

constexpr int GAMEOBJECT_FIELD_FLAGS = 1, GO_FLAG_MAP_OBJECT = 0x100000;
struct Player;
struct WorldPacket { std::vector<int> objects; };
struct UpdateData {
    std::vector<int> objects;
    explicit UpdateData(int) {}
    bool BuildPacket(WorldPacket* packet) { packet->objects = objects; return !objects.empty(); }
};
struct WorldObject {
    int id;
    bool creature = false;
    bool IsCreature() const { return creature; }
    explicit WorldObject(int value) : id(value) {}
    void BuildCreateUpdateBlockForPlayer(UpdateData* data, Player*) const { data->objects.push_back(id); }
    int GetGUID() const { return id; }
    int GetEntry() const { return id; }
};
using Creature = WorldObject;
struct Transport : WorldObject {
    using WorldObject::WorldObject;
    std::vector<WorldObject*> passengers;
    auto const& GetPassengers() const { return passengers; }
};
struct StaticTransport : WorldObject {
    using WorldObject::WorldObject;
    bool inWorld = true, mapObject = true, visible = true;
    bool IsInWorld() const { return inWorld; }
    bool HasFlag(int, int) const { return mapObject; }
};
struct Session {
    std::vector<WorldPacket> packets;
    void SendPacket(WorldPacket* packet) { packets.push_back(*packet); }
};
struct Player : WorldObject {
    Player() : WorldObject(1) {}
    Session session;
    Transport* transport = nullptr;
    std::set<int> known;
    int mapId = 0;
    uint32 flags = 0, state = 0;
    bool alive = true, vehicle = false, rootAura = false, selfCreateProtected = false;
    bool inWorld = true;
    int visibilityRefreshes = 0;
    float distance = 50;
    int32 _mapObjectArrivalMap = -1;
    uint32 _mapObjectArrivalWait = 0, _mapObjectArrivalSettle = 0;
    bool _mapObjectArrivalOwnRoot = false;
    int GetMapId() const { return mapId; }
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return inWorld; }
    Creature* FindNearestCreature(uint32, float) { return nullptr; }
    void UpdateVisibilityForPlayer() { assert(_mapObjectArrivalMap == -1); ++visibilityRefreshes; }
    bool CanFly() const { return flags & MOVEMENTFLAG_CAN_FLY; }
    bool IsLevitating() const { return flags & MOVEMENTFLAG_DISABLE_GRAVITY; }
    bool GetVehicle() const { return vehicle; }
    bool HasUnitState(uint32 mask) const { return state & mask; }
    bool HasUnitMovementFlag(uint32 mask) const { return flags & mask; }
    void RemoveUnitMovementFlag(uint32 mask) { flags &= ~mask; }
    void AddUnitMovementFlag(uint32 mask) { flags |= mask; }
    void AddUnitState(uint32 mask) { state |= mask; }
    float GetPositionZ() const { return 747.6f; }
    float GetExactDist(WorldObject*) const { return distance; }
    void SetFallInformation(int, float) { }
    void SetDisableGravity(bool disable) { if(disable) flags |= MOVEMENTFLAG_DISABLE_GRAVITY; else flags &= ~MOVEMENTFLAG_DISABLE_GRAVITY; }
    void SetControlled(bool, uint32) { if(!rootAura) { state &= ~UNIT_STATE_ROOT; flags &= ~MOVEMENTFLAG_ROOT; } }
    void BeginMapObjectArrival();
    void NotifyMapObjectArrivalLoaded(int32 mapId);
    void UpdateMapObjectArrival(uint32 diff);
    void EndMapObjectArrival(bool refreshVisibility = false);
    bool ShouldDeferMapObjectArrivalVisibility(WorldObject const* target);
    void BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target)
    {
        selfCreateProtected = IsLevitating() && HasUnitMovementFlag(MOVEMENTFLAG_ROOT);
        WorldObject::BuildCreateUpdateBlockForPlayer(data, target);
    }
    Transport* GetTransport() const { return transport; }
    bool HaveAtClient(WorldObject const* object) const { return known.count(object->id) != 0; }
    bool canSeeOrDetect(StaticTransport const* object, bool, bool) const { return object->visible; }
    void AddClient(int guid) { known.insert(guid); }
    Session* GetSession() { return &session; }
};
struct Map {
    std::map<int, StaticTransport*> m_StaticTransports;
    void SendInitSelf(Player* player);
    int GetId() const { return 0; }
};
#include "BeginMapObjectArrival.inc"
#include "NotifyMapObjectArrivalLoaded.inc"
#include "UpdateMapObjectArrival.inc"
#include "EndMapObjectArrival.inc"
#include "ShouldDeferMapObjectArrivalVisibility.inc"
#include "SendInitSelf.inc"

int main() {
    Map map;
    Player player;
    StaticTransport city(246854), phasedOut(2), despawned(3), elevator(4);
    phasedOut.visible = false;
    despawned.inWorld = false;
    elevator.mapObject = false;
    for (auto object : {&city, &phasedOut, &despawned, &elevator})
        map.m_StaticTransports.emplace(object->id, object);
    map.SendInitSelf(&player);
    assert(player.session.packets.size() == 1);
    assert((player.session.packets.back().objects == std::vector<int>{246854, 1}));
    assert(player.known == std::set<int>{246854});
    assert(player.selfCreateProtected); // No vulnerable first frame before a later movement packet.

    // Already known cities must not be created twice; unseen/inactive objects stay hidden.
    map.SendInitSelf(&player);
    assert((player.session.packets.back().objects == std::vector<int>{1}));

    Player late;
    WorldObject khadgar(113986), pet(416), floor(246854), otherPlayer(200);
    khadgar.creature = pet.creature = true;
    assert(!late.ShouldDeferMapObjectArrivalVisibility(&khadgar));
    late.flags = MOVEMENTFLAG_MASK_MOVING | MOVEMENTFLAG_FALLING;
    late.BeginMapObjectArrival();
    assert(late.ShouldDeferMapObjectArrivalVisibility(&khadgar));
    assert(late.ShouldDeferMapObjectArrivalVisibility(&pet));
    assert(!late.ShouldDeferMapObjectArrivalVisibility(&floor));
    assert(!late.ShouldDeferMapObjectArrivalVisibility(&otherPlayer));
    late.known.insert(pet.id);
    assert(!late.ShouldDeferMapObjectArrivalVisibility(&pet)); // Existing units must still get cleanup.
    late.known.erase(pet.id);
    Player otherObserver;
    assert(!otherObserver.ShouldDeferMapObjectArrivalVisibility(&khadgar));
    assert(late.IsLevitating() && late.HasUnitMovementFlag(MOVEMENTFLAG_ROOT));
    assert(!late.HasUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING | MOVEMENTFLAG_FALLING));
    late.UpdateMapObjectArrival(20000); // Slow loading does not consume the streaming grace.
    late.NotifyMapObjectArrivalLoaded(1); // Old-map notification cannot release the hold.
    assert(late._mapObjectArrivalSettle == 0);
    late.NotifyMapObjectArrivalLoaded(0);
    late.UpdateMapObjectArrival(7000);
    late.NotifyMapObjectArrivalLoaded(0); // Duplicate notification does not extend the deadline.
    assert(late._mapObjectArrivalSettle == 1000);
    assert(late.visibilityRefreshes == 0 && late.ShouldDeferMapObjectArrivalVisibility(&khadgar));
    late.UpdateMapObjectArrival(1000);
    assert(late.visibilityRefreshes == 1 && !late.ShouldDeferMapObjectArrivalVisibility(&khadgar));
    assert(!late.ShouldDeferMapObjectArrivalVisibility(&pet));
    late.EndMapObjectArrival(true);
    assert(late.visibilityRefreshes == 1);
    assert(late.flags == 0 && late.state == 0 && late._mapObjectArrivalMap == -1);
    late.NotifyMapObjectArrivalLoaded(0);
    assert(late._mapObjectArrivalSettle == 0);

    Player missing;
    missing.BeginMapObjectArrival();
    missing.UpdateMapObjectArrival(29999);
    assert(missing.IsLevitating());
    missing.UpdateMapObjectArrival(1);
    assert(missing.flags == 0 && missing.state == 0);
    assert(missing.visibilityRefreshes == 1);

    for (int mode = 0; mode != 5; ++mode)
    {
        Player skipped;
        Transport otherBoat(9);
        if (mode == 0) skipped.flags = MOVEMENTFLAG_CAN_FLY;
        if (mode == 1) skipped.flags = MOVEMENTFLAG_DISABLE_GRAVITY;
        if (mode == 2) skipped.vehicle = true;
        if (mode == 3) skipped.transport = &otherBoat;
        if (mode == 4) skipped.alive = false;
        uint32 original = skipped.flags;
        skipped.BeginMapObjectArrival();
        skipped.EndMapObjectArrival();
        assert(skipped._mapObjectArrivalMap == -1 && skipped.flags == original);
    }
    for (int mode = 0; mode != 6; ++mode)
    {
        Player leaving;
        leaving.BeginMapObjectArrival();
        if(mode == 0) leaving.EndMapObjectArrival(); // logout/map removal cleanup
        if(mode == 1) leaving.mapId = 1220;
        if(mode == 2) leaving.alive = false;
        if(mode == 3) leaving.vehicle = true;
        if(mode == 4) { leaving.inWorld = false; leaving.EndMapObjectArrival(true); }
        Transport departingBoat(10);
        if(mode == 5) leaving.transport = &departingBoat;
        leaving.UpdateMapObjectArrival(1);
        assert(leaving.flags == 0 && leaving._mapObjectArrivalMap == -1);
        assert(leaving.visibilityRefreshes == 0);
    }
    Player rooted;
    rooted.state = UNIT_STATE_ROOT;
    rooted.flags = MOVEMENTFLAG_ROOT;
    rooted.BeginMapObjectArrival();
    rooted.EndMapObjectArrival();
    assert(rooted.flags == MOVEMENTFLAG_ROOT && rooted.state == UNIT_STATE_ROOT);
    Player newlyRooted;
    newlyRooted.BeginMapObjectArrival();
    newlyRooted.rootAura = true;
    newlyRooted.EndMapObjectArrival();
    assert(newlyRooted.flags == MOVEMENTFLAG_ROOT);

    Map distantMap;
    distantMap.m_StaticTransports.emplace(city.id, &city);
    Player distant;
    distant.distance = 1000;
    distantMap.SendInitSelf(&distant);
    assert(!distant.selfCreateProtected);

    // Ordinary maps and existing boat/passenger initialization retain their behavior.
    map.m_StaticTransports.clear();
    Transport boat(5);
    WorldObject passenger(6), unknown(7);
    boat.passengers = {&player, &passenger, &unknown};
    player.transport = &boat;
    player.known.insert(6);
    map.SendInitSelf(&player);
    assert((player.session.packets.back().objects == std::vector<int>{5, 1, 6}));
    player.transport = nullptr;
    map.SendInitSelf(&player);
    assert((player.session.packets.back().objects == std::vector<int>{1}));
    std::cout << "PASS: protected first create; loading/streaming deadlines; missing and duplicate notifications; flight, roots, death, logout, map/vehicle changes; city visibility and ordinary transports.\n";
}
