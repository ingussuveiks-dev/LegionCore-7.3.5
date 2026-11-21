#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

using uint8 = std::uint8_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

struct ObjectGuid
{
    uint64 value = 0;
    bool IsEmpty() const { return value == 0; }
    bool operator==(ObjectGuid const& other) const { return value == other.value; }
    bool operator!=(ObjectGuid const& other) const { return !(*this == other); }
};

struct Unit
{
    ObjectGuid guid;
    bool alive = true;
    bool inWorld = true;
    bool deleted = false;
    ObjectGuid GetGUID() const { return guid; }
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return inWorld; }
    bool IsPreDelete() const { return deleted; }
};

struct Creature : Unit
{
    uint32 entry;
    uint64 spawn;
    uint32 GetEntry() const { return entry; }
    uint64 GetDBTableGUIDLow() const { return spawn; }
};

struct Group
{
    ObjectGuid guid;
    uint32 members = 1;
    bool lfg = true;
    bool isLFGGroup() const { return lfg; }
    uint32 GetMembersCount() const { return members; }
    ObjectGuid GetGUID() const { return guid; }
};

struct Player : Unit
{
    uint32 map = 1698;
    Group* group = nullptr;
    bool pending = false;
    unsigned exits = 0;
    std::function<void()> departure;
    static inline unsigned removals = 0;
    Group* GetGroup() const { return group; }
    uint32 GetMapId() const { return map; }
    bool HasPendingTeleport() const { return pending; }
    void AddDelayedEvent(uint32, std::function<void()> action) { departure = std::move(action); }
    static void RemoveFromGroup(Group*, ObjectGuid) { ++removals; }
};

struct PlayerRef
{
    Player* player;
    Player* getSource() const { return player; }
};

struct InstanceMap
{
    uint32 id;
    std::map<uint64, Creature*> creatures;
    std::vector<PlayerRef> players;
    uint32 GetInstanceId() const { return id; }
    std::vector<PlayerRef> const& GetPlayers() const { return players; }
    Creature* GetCreature(ObjectGuid const& guid) const
    {
        auto found = creatures.find(guid.value);
        return found == creatures.end() ? nullptr : found->second;
    }
};

struct InstanceScript
{
    explicit InstanceScript(InstanceMap* map) : instance(map) {}
    virtual ~InstanceScript() = default;
    virtual void OnCreatureCreate(Creature*) {}
    virtual void OnUnitDeath(Unit*) {}
    virtual void OnPlayerEnter(Player*) {}
    virtual void OnPlayerLeave(Player*) {}
    virtual void Update(uint32) {}
    virtual uint32 GetData(uint32) const { return 0; }
    InstanceMap* instance;
};

struct InstanceMapScript
{
    InstanceMapScript(char const*, uint32) {}
    virtual ~InstanceMapScript() = default;
    virtual InstanceScript* GetInstanceScript(InstanceMap*) const = 0;
};

struct Scenario
{
    uint32 id = 1259;
    bool completed = false;
    uint32 GetScenarioId() const { return id; }
    bool IsCompleted(bool) const { return completed; }
};

struct ScenarioManager
{
    std::map<uint32, Scenario> scenarios;
    Scenario* GetScenario(uint32 map)
    {
        auto found = scenarios.find(map);
        return found == scenarios.end() ? nullptr : &found->second;
    }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;

struct LfgManager
{
    uint32 GetDungeon(ObjectGuid guid) const { return guid.value < 100 ? 1440 : 0; }
    void TeleportPlayer(Player* player, bool out)
    {
        assert(out);
        ++player->exits;
        player->pending = true;
    }
} lfgManager;
auto* sLFGMgr = &lfgManager;

unsigned exhausted = 0;
void TC_LOG_ERROR(char const*, char const*, uint32) { ++exhausted; }

#include "HighlordsReturn.inc"

int main()
{
    Creature velen{ { { 10 } }, 117042, 14534061 };
    Creature korvas{ { { 11 } }, 116880, 14534062 };
    Creature otherVelen{ { { 12 } }, 117042, 7 };
    Creature boss{ { { 13 } }, 117198, 14534049 };
    Group solo{ { 90 } };
    Group different{ { 91 } };
    Group unrelated{ { 100 } };
    Player challenger;
    challenger.group = &solo;
    challenger.alive = false;
    Player observer;
    observer.group = &unrelated;

    InstanceMap first{ 1, {{10, &velen}, {11, &korvas}}, {{ &challenger }, { &observer }} };
    InstanceMap second{ 2, {{10, &velen}, {11, &korvas}}, {} };
    scenarioManager.scenarios.emplace(1, Scenario{});
    scenarioManager.scenarios.emplace(2, Scenario{});
    instance_mage_tower_highlords_return::HighlordsReturnInstance active(&first);
    instance_mage_tower_highlords_return::HighlordsReturnInstance separate(&second);

    active.OnCreatureCreate(&velen);
    active.OnCreatureCreate(&otherVelen);
    assert(!active.GetData(1));
    active.OnCreatureCreate(&korvas);
    assert(active.GetData(1));
    active.OnUnitDeath(&boss);
    active.OnUnitDeath(&otherVelen);
    assert(active.GetData(1));
    active.Update(500);
    assert(!challenger.exits);

    separate.OnCreatureCreate(&velen);
    separate.OnCreatureCreate(&korvas);
    scenarioManager.scenarios[2].completed = true;
    separate.OnUnitDeath(&velen);
    assert(separate.GetData(1));

    velen.alive = false;
    assert(!active.GetData(1));
    active.OnUnitDeath(&velen);
    velen.alive = true;
    assert(!active.GetData(1));
    assert(separate.GetData(1));
    challenger.pending = true;
    active.Update(1);
    assert(!challenger.exits);
    challenger.pending = false;
    active.Update(500);
    assert(challenger.exits == 1 && observer.exits == 0);
    active.OnUnitDeath(&korvas);
    for (int tick = 0; tick < 20; ++tick)
        active.Update(500);
    assert(challenger.exits == 1 && exhausted == 1);
    active.OnPlayerEnter(&challenger);
    challenger.pending = false;
    active.Update(500);
    assert(challenger.exits == 2);

    active.OnPlayerLeave(&challenger);
    challenger.departure();
    assert(Player::removals == 0);
    challenger.map = 1220;
    challenger.group = &different;
    challenger.departure();
    assert(Player::removals == 0);
    challenger.group = &solo;
    challenger.departure();
    assert(Player::removals == 1);

    std::cout << "PASS living actor gate, ally death, isolated attempts, bounded native exit and post-exit solo group cleanup\n";
}
