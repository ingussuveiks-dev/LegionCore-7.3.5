#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <vector>

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
    explicit WorldObject(int value) : id(value) {}
    void BuildCreateUpdateBlockForPlayer(UpdateData* data, Player*) const { data->objects.push_back(id); }
    int GetGUID() const { return id; }
};
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
    int GetMapId() const { return 0; }
    Transport* GetTransport() const { return transport; }
    bool HaveAtClient(WorldObject const* object) const { return known.count(object->id) != 0; }
    bool canSeeOrDetect(StaticTransport const* object, bool, bool) const { return object->visible; }
    void AddClient(int guid) { known.insert(guid); }
    Session* GetSession() { return &session; }
};
struct Map {
    std::map<int, StaticTransport*> m_StaticTransports;
    void SendInitSelf(Player* player);
};
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

    // Already known cities must not be created twice; unseen/inactive objects stay hidden.
    map.SendInitSelf(&player);
    assert((player.session.packets.back().objects == std::vector<int>{1}));

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
    std::cout << "PASS: city before player in the same packet; phase/despawn/duplicate guards; boat passengers and ordinary maps.\n";
}
