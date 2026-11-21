#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <vector>

using uint32 = std::uint32_t;
using uint16 = std::uint16_t;
using int32 = std::int32_t;
using ObjectGuid = uint32;
using SpellEffIndex = unsigned;

constexpr float INTERACTION_DISTANCE = 5.0f;
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_DONT_REPORT };

namespace GameTime
{
    uint32 now = 0;
    uint32 GetGameTimeMS() { return now; }
}

struct WorldLocation
{
    uint32 map = 0;
    float x = 0, y = 0, z = 0;
    uint32 GetMapId() const { return map; }
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; }
};

struct SpellTargetPosition
{
    uint32 target_mapId;
    float target_X, target_Y, target_Z;
};

struct GossipMenu
{
    uint32 menu = 20778;
    bool visible = true;
    uint32 GetMenuId() const { return menu; }
    GossipMenu const& GetGossipMenu() const { return *this; }
    GossipMenu const* GetItem(uint32) const { return visible ? this : nullptr; }
};

struct Creature;
struct Player
{
    explicit Player(ObjectGuid guid) : id(guid) {}
    ObjectGuid id;
    Creature* npc = nullptr;
    GossipMenu shown;
    GossipMenu* PlayerTalkClass = &shown;
    WorldLocation transfer;
    std::set<uint32> markers;
    std::vector<uint32> queuedGossip;
    unsigned currency = 100, selections = 0, debitCalls = 0, markerCalls = 0;
    uint16 scenario = 962;
    bool pending = false, alive = true, present = true, sameMap = true, samePhase = true, nearby = true, questActive = true;
    Player* ToPlayer() { return this; }
    ObjectGuid GetGUID() const { return id; }
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return present; }
    bool HasPendingTeleport() const { return pending; }
    bool IsBeingTeleportedFar() const { return pending; }
    WorldLocation const& GetTeleportDest() const { return transfer; }
    bool HasCurrency(uint32 currencyId, uint32 amount) const { assert(currencyId == 1226); return currency >= amount; }
    void ModifyCurrency(uint32 currencyId, int32 amount, bool, bool exact)
    {
        assert(currencyId == 1226 && amount == -100 && exact && currency >= 100);
        currency -= 100;
        ++debitCalls;
    }
    void SetRewardedQuest(uint32 marker) { markers.insert(marker); ++markerCalls; }
    uint16 GetScenarioId() const { return scenario; }
    void SetScenarioId(uint16 value) { scenario = value; }
    Creature* FindNearestCreature(uint32 entry, float distance, bool) const
    {
        assert(entry == 117871 && distance == INTERACTION_DISTANCE);
        return nearby ? npc : nullptr;
    }
};

struct SmartAI;
struct Creature
{
    uint32 map = 1220, id = 12742696;
    SmartAI* ai = nullptr;
    bool IsAlive() const { return true; }
    bool IsInWorld() const { return true; }
    uint32 GetMapId() const { return map; }
    uint32 GetDBTableGUIDLow() const { return id; }
    bool IsInMap(Player const* player) const { return player->sameMap; }
    bool InSamePhase(Player const* player) const { return player->samePhase; }
    bool IsWithinDistInMap(Player const* player, float) const { return player->nearby; }
    SmartAI* AI() const { return ai; }
};

struct SmartAI
{
    explicit SmartAI(Creature* creature) : me(creature) {}
    virtual ~SmartAI() = default;
    virtual void sGossipSelect(Player* player, uint32 menu, uint32 selection)
    {
        ++player->selections;
        if (menu == 20778 && selection < 14)
        {
            player->SetScenarioId(1204);
            player->queuedGossip.push_back(selection);
        }
    }
    Creature* me;
};

constexpr std::array<uint32, 7> Markers{45547, 46033, 45548, 45635, 45546, 45636, 45549};
struct GossipOption { uint32 OptionIndex; uint32 Conditions; };
struct GossipDataStore
{
    std::vector<std::pair<int, GossipOption>> rows;
    GossipDataStore()
    {
        for (uint32 selection = 0; selection < 14; ++selection)
            rows.push_back({0, {selection, selection}});
    }
    auto GetGossipMenuItemsMapBounds(uint32 menu) const
    {
        assert(menu == 20778);
        return std::make_pair(rows.begin(), rows.end());
    }
};
struct ConditionManager
{
    bool IsObjectMeetToConditions(Player const* player, Creature const* source, uint32 selection) const
    {
        assert(source && source->GetDBTableGUIDLow() == 12742696);
        bool const rewarded = player->markers.count(Markers[selection / 2]) != 0;
        return player->questActive && (selection % 2 == 0 ? rewarded : !rewarded);
    }
};
struct SpellManager
{
    std::map<uint32, SpellTargetPosition> positions;
    SpellTargetPosition const* GetSpellTargetPosition(uint32 id) const
    {
        auto const it = positions.find(id);
        return it == positions.end() ? nullptr : &it->second;
    }
};

GossipDataStore gossipData;
ConditionManager conditions;
SpellManager spells;
auto* sGossipDataStore = &gossipData;
auto* sConditionMgr = &conditions;
auto* sSpellMgr = &spells;

struct Hook { void operator+=(int) {} };
struct SpellScript
{
    struct SpellInfo { uint32 Id = 0; } info;
    Player* caster = nullptr;
    Player* hit = nullptr;
    bool skippedCurrency = false;
    bool skippedEffect = false;
    Hook OnCheckCast, OnCast, OnEffectHitTarget, AfterCast;
    virtual ~SpellScript() = default;
    virtual void Register() = 0;
    Player* GetCaster() const { return caster; }
    Player* GetHitUnit() const { return hit; }
    SpellInfo const* GetSpellInfo() const { return &info; }
    void PreventCurrencyReagentConsumption() { skippedCurrency = true; }
    void PreventHitDefaultEffect(SpellEffIndex) { skippedEffect = true; }
};

#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(...) 0
#define SpellCastFn(...) 0
#define SpellEffectFn(...) 0

#include "XylemEntry.inc"

enum class TeleportResult { Accepted, Rejected, WrongDestination };

bool Dispatch(Player& player, uint32 id, TeleportResult result, Player* suppliedTarget = nullptr)
{
    spell_mage_tower_victoria_admission cast;
    cast.caster = &player;
    cast.hit = suppliedTarget ? suppliedTarget : &player;
    cast.info.Id = id;
    if (cast.CheckAdmission() != SPELL_CAST_OK)
        return false;

    cast.TakeAdmissionRequest();
    cast.CheckTeleportTarget(0);
    if (!cast.skippedEffect && result != TeleportResult::Rejected)
    {
        SpellTargetPosition const* dest = spells.GetSpellTargetPosition(id);
        player.transfer = {dest->target_mapId, dest->target_X, dest->target_Y, dest->target_Z};
        if (result == TeleportResult::WrongDestination)
            player.transfer.x += 2;
        player.pending = true;
    }
    cast.FinishAdmission();
    assert(cast.skippedCurrency);
    return !cast.skippedEffect;
}

int main()
{
    using namespace MageTowerAdmission;
    Creature victoria;
    VictoriaAI ai(&victoria);
    victoria.ai = &ai;
    constexpr std::array<uint32, 7> Maps{1616, 1710, 1703, 1673, 1698, 1684, 1702};
    constexpr std::array<uint32, 14> Spells{235527, 237660, 235557, 235194, 237622, 237664, 229196, 229196,
        233380, 233379, 231857, 237663, 233804, 233808};
    for (uint32 selection = 0; selection < Spells.size(); ++selection)
    {
        assert(AdmissionSpell(selection) == Spells[selection]);
        spells.positions.emplace(Spells[selection], SpellTargetPosition{Maps[selection / 2], 38, 59, 71});
    }

    for (uint32 route = 0; route < Maps.size(); ++route)
    {
        Player player(1 + route);
        player.npc = &victoria;
        uint32 const paid = route * 2;
        uint32 const free = paid + 1;
        ai.sGossipSelect(&player, VictoriaMenu, paid);
        assert(player.selections == 0);
        ai.sGossipSelect(&player, VictoriaMenu, free);
        ai.sGossipSelect(&player, VictoriaMenu, free);
        assert(player.queuedGossip == std::vector<uint32>{free});
        assert(player.currency == 100 && player.markers.empty() && !player.pending);
        assert(!Dispatch(player, Spells[(free + 2) % Spells.size()], TeleportResult::Accepted));
        GameTime::now += 200;
        assert(Dispatch(player, Spells[free], TeleportResult::Accepted));
        assert((player.markers == std::set<uint32>{Markers[route]} && player.markerCalls == 1));
        assert(player.transfer.map == Maps[route] && player.currency == 100);
        assert(!Dispatch(player, Spells[free], TeleportResult::Accepted));
        player.pending = false;
        ai.sGossipSelect(&player, VictoriaMenu, free);
        assert(player.selections == 1);
        player.SetScenarioId(962);
        ai.sGossipSelect(&player, VictoriaMenu, paid);
        ai.sGossipSelect(&player, VictoriaMenu, paid);
        assert(player.selections == 2 && player.currency == 100);
        GameTime::now += 200;
        assert(Dispatch(player, Spells[paid], TeleportResult::Rejected));
        assert(player.currency == 100 && player.debitCalls == 0 && player.scenario == 962);
        ai.sGossipSelect(&player, VictoriaMenu, paid);
        GameTime::now += 200;
        assert(Dispatch(player, Spells[paid], TeleportResult::Accepted));
        assert(player.currency == 0 && player.debitCalls == 1 && player.markerCalls == 1);
        player.pending = false;
        player.currency = 99;
        ai.sGossipSelect(&player, VictoriaMenu, paid);
        assert(player.selections == 3 && !Dispatch(player, Spells[paid], TeleportResult::Accepted));
    }

    Player stale(100);
    stale.npc = &victoria;
    ai.sGossipSelect(&stale, VictoriaMenu, 7);
    GameTime::now += RequestLifetime;
    assert(!Dispatch(stale, 229196, TeleportResult::Accepted));
    ai.sGossipSelect(&stale, VictoriaMenu, 7);
    stale.nearby = false;
    assert(!Dispatch(stale, 229196, TeleportResult::Accepted));
    stale.nearby = true;
    stale.questActive = false;
    assert(!Dispatch(stale, 229196, TeleportResult::Accepted));
    stale.questActive = true;
    assert(Dispatch(stale, 229196, TeleportResult::WrongDestination));
    assert(stale.markers.empty() && stale.currency == 100);
    stale.pending = false;
    stale.shown.visible = false;
    ai.sGossipSelect(&stale, VictoriaMenu, 7);
    assert(!Dispatch(stale, 229196, TeleportResult::Accepted));
    stale.shown.visible = true;
    ai.sGossipSelect(&stale, VictoriaMenu, 7);
    Player other(202);
    assert(!Dispatch(stale, 229196, TeleportResult::Accepted, &other));
    assert(stale.markers.empty() && stale.currency == 100);
    ai.sGossipSelect(&stale, VictoriaMenu, 20);
    assert(stale.selections == 4);

    Player warlock(333);
    warlock.npc = &victoria;
    ai.sGossipSelect(&warlock, VictoriaMenu, 13);
    assert(Dispatch(warlock, 233808, TeleportResult::Accepted) && warlock.markers.count(45549) == 1);
    warlock.pending = false;
    ai.sGossipSelect(&warlock, VictoriaMenu, 11);
    assert(Dispatch(warlock, 237663, TeleportResult::Accepted) && warlock.markers.count(45636) == 1);
    assert(warlock.currency == 100 && warlock.markerCalls == 2);
    std::cout << "PASS seven free/paid admission pairs, delayed dispatch, native denial, one debit, stale requests and unrelated gossip\n";
}
