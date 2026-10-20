#include <cassert>
#include <iostream>
#include <cstdint>
using uint32 = uint32_t;
constexpr int GAMEOBJECT_TYPE_TEXT = 9;
struct Packet { int guid; };
namespace WorldPackets { namespace GameObject {
struct PageText {
    Packet packet;
    explicit PageText(int guid) : packet{guid} {}
    Packet const* Write() { return &packet; }
};
}}
struct Unit {
    bool player = true;
    int packets = 0, lastGuid = 0;
    bool IsPlayer() { return player; }
    Unit* ToPlayer() { assert(player); return this; }
    void SendDirectMessage(Packet const* packet) { ++packets; lastGuid = packet->guid; }
};
struct Conditions {
    bool allowed = true;
    int checks = 0;
    uint32 lastCondition = 0;
    bool IsPlayerMeetingCondition(Unit*, uint32 condition) {
        ++checks; lastCondition = condition; return allowed;
    }
} conditions;
auto sConditionMgr = &conditions;
struct GameObjectTemplate { struct { uint32 pageID = 5234, conditionID1 = 0; } text; };
struct Object {
    GameObjectTemplate data;
    bool hasInfo = true;
    GameObjectTemplate const* GetGOInfo() { return hasInfo ? &data : nullptr; }
    int GetGUID() { return 131774; }
    void Use(Unit* user) {
        switch (GAMEOBJECT_TYPE_TEXT) {
#include "TextUse.inc"
        }
    }
};
int main() {
    Object blanket;
    Unit player;
    blanket.Use(&player);
    assert(player.packets == 1 && player.lastGuid == 131774);
    blanket.Use(&player); // Optional inspection is repeatable.
    assert(player.packets == 2);
    conditions.allowed = false;
    blanket.data.text.conditionID1 = 123;
    blanket.Use(&player);
    assert(player.packets == 2 && conditions.lastCondition == 123);
    conditions.allowed = true;
    blanket.data.text.pageID = 0;
    blanket.Use(&player);
    assert(player.packets == 2);
    Unit pet;
    pet.player = false;
    int checks = conditions.checks;
    blanket.Use(&pet);
    assert(!pet.packets && conditions.checks == checks);
    blanket.hasInfo = false;
    blanket.Use(&player);
    assert(player.packets == 2 && conditions.checks == checks);
    std::cout << "PASS: production text-object dispatch, GUID, repeat, missing page/template, condition and non-player gates\n";
}
