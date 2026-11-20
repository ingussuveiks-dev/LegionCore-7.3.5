#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <utility>

using uint32 = uint32_t;
using uint16 = uint16_t;
using int32 = int32_t;

struct WorldLocation
{
    uint32 map = 1673;
    float x = 764.04f, y = 1333.36f, z = 267.23f;
    uint32 GetMapId() const { return map; }
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; }
};

struct SpellTargetPosition
{
    uint32 target_mapId = 1673;
    float target_X = 764.04f, target_Y = 1333.36f, target_Z = 267.23f;
};

struct GossipMenu
{
    uint32 menu = 20778;
    bool visible = true;
    uint32 GetMenuId() const { return menu; }
    GossipMenu const& GetGossipMenu() const { return *this; }
    GossipMenu const* GetItem(uint32) const { return visible ? this : nullptr; }
};

struct Player
{
    GossipMenu shown;
    GossipMenu* PlayerTalkClass = &shown;
    WorldLocation transfer;
    unsigned currency = 100, selections = 0, debitCalls = 0, markerCalls = 0;
    uint16 scenario = 962;
    bool paidAllowed = true, freeAllowed = true, rewarded = false;
    bool accepted = true, pending = false, alive = true, present = true, sameMap = true, samePhase = true;
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return present; }
    bool HasPendingTeleport() const { return pending; }
    bool IsBeingTeleportedFar() const { return pending; }
    WorldLocation const& GetTeleportDest() const { return transfer; }
    bool HasCurrency(uint32 id, uint32 amount) const { assert(id == 1226); return currency >= amount; }
    void ModifyCurrency(uint32 id, int32 amount, bool, bool exact)
    {
        assert(id == 1226 && amount == -100 && exact && currency >= 100);
        currency -= 100;
        ++debitCalls;
    }
    bool GetQuestRewardStatus(uint32 id) const { assert(id == 45635); return rewarded; }
    void SetRewardedQuest(uint32 id) { assert(id == 45635); rewarded = true; ++markerCalls; }
    uint16 GetScenarioId() const { return scenario; }
    void SetScenarioId(uint16 value) { scenario = value; }
};

struct Creature
{
    uint32 map = 1220, id = 12742696;
    uint32 GetMapId() const { return map; }
    uint32 GetDBTableGUIDLow() const { return id; }
    bool IsInMap(Player const* player) const { return player->sameMap; }
    bool InSamePhase(Player const* player) const { return player->samePhase; }
};

struct SmartAI
{
    explicit SmartAI(Creature* creature) : me(creature) {}
    virtual void sGossipSelect(Player* player, uint32 menu, uint32 selection)
    {
        ++player->selections;
        if (menu == 20778 && (selection == 6 || selection == 7))
        {
            player->SetScenarioId(1204);
            player->pending = player->accepted;
        }
    }
    Creature* me;
};

struct GossipOption
{
    uint32 OptionIndex;
    bool Conditions;
};
struct GossipDataStore
{
    std::array<std::pair<int, GossipOption>, 2> rows{{ {0,{6,true}}, {0,{7,false}} }};
    auto GetGossipMenuItemsMapBounds(uint32 menu)
    {
        assert(menu == 20778);
        return std::make_pair(rows.begin(), rows.end());
    }
};
struct ConditionManager
{
    bool IsObjectMeetToConditions(Player const* player, Creature const* source, bool paid) const
    {
        assert(source && source->GetDBTableGUIDLow() == 12742696);
        return paid ? player->paidAllowed && player->rewarded : player->freeAllowed && !player->rewarded;
    }
};
struct SpellManager
{
    SpellTargetPosition position;
    SpellTargetPosition const* GetSpellTargetPosition(uint32 id) const { assert(id == 229196); return &position; }
};

GossipDataStore gossipData;
ConditionManager conditions;
SpellManager spells;
auto* sGossipDataStore = &gossipData;
auto* sConditionMgr = &conditions;
auto* sSpellMgr = &spells;

#include "XylemEntry.inc"

int main()
{
    using namespace ClosingTheEye;
    Creature victoria;
    VictoriaAI ai(&victoria);
    Player player;
    ai.sGossipSelect(&player, VictoriaMenu, FreeEntry);
    assert(player.rewarded && player.markerCalls == 1 && player.currency == 100 && player.selections == 1 && player.scenario == 1204);
    ai.sGossipSelect(&player, VictoriaMenu, FreeEntry);
    assert(player.markerCalls == 1 && player.selections == 1);

    player.pending = false;
    ai.sGossipSelect(&player, VictoriaMenu, FreeEntry);
    assert(player.markerCalls == 1 && player.selections == 1);
    ai.sGossipSelect(&player, VictoriaMenu, PaidEntry);
    ai.sGossipSelect(&player, VictoriaMenu, PaidEntry);
    assert(player.currency == 0 && player.debitCalls == 1 && player.selections == 2);

    Player denied;
    denied.accepted = false;
    ai.sGossipSelect(&denied, VictoriaMenu, FreeEntry);
    assert(!denied.rewarded && denied.currency == 100 && denied.selections == 1 && denied.scenario == 962);
    denied.rewarded = true;
    ai.sGossipSelect(&denied, VictoriaMenu, PaidEntry);
    assert(denied.currency == 100 && denied.selections == 2);
    denied.currency = 99;
    ai.sGossipSelect(&denied, VictoriaMenu, PaidEntry);
    assert(denied.currency == 99 && denied.selections == 2);

    Player other;
    other.transfer.x += 2;
    ai.sGossipSelect(&other, VictoriaMenu, FreeEntry);
    assert(!other.rewarded && other.selections == 1);
    other.pending = false;
    other.shown.visible = false;
    ai.sGossipSelect(&other, VictoriaMenu, FreeEntry);
    assert(!other.rewarded && other.selections == 1);
    ai.sGossipSelect(&other, VictoriaMenu, 8);
    assert(!other.rewarded && other.selections == 2 && other.currency == 100);
    std::cout << "PASS free-once marker, exactly-once fee, rejected transfers, live conditions and sibling options.\n";
}
