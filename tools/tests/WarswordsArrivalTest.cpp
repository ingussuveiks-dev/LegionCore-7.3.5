#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <vector>
using uint32 = uint32_t;
enum { CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1 };
struct InstanceScript;
struct Player;
struct Map
{
    struct Reference { Player* player; Player* getSource() const { return player; } };
    struct PlayerList : std::list<Reference> { bool isEmpty() const { return empty(); } } players;
} homeMap, otherMap;
struct Scenario
{
    uint32 identity = 944, step = 0;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioManager
{
    Scenario scenario;
    bool available = true;
    Scenario* GetScenario(uint32 instanceId) { return available && (instanceId == 42 || instanceId == 43) ? &scenario : nullptr; }
} manager;
auto* sScenarioMgr = &manager;
struct Player
{
    bool world = true, contact = true, alive = true, teleporting = false, rewarded = false, owned = false;
    uint32 klass = CLASS_WARRIOR, status = QUEST_STATUS_INCOMPLETE, mapId = 1511, instanceId = 42;
    Map* map = &homeMap;
    InstanceScript* script = nullptr;
    std::vector<std::function<void()>> pending;
    std::vector<uint32> spells;
    uint32 jumps = 0, conversations = 0;
    bool IsInWorld() const { return world; }
    bool CanContact() const { return contact; }
    bool IsAlive() const { return alive; }
    bool IsBeingTeleported() const { return teleporting; }
    uint32 getClass() const { return klass; }
    Map* GetMap() const { return map; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instanceId; }
    InstanceScript* GetInstanceScript() const { return script; }
    bool GetQuestRewardStatus(uint32 quest) const { assert(quest == 40043); return rewarded; }
    uint32 GetQuestStatus(uint32 quest) const { assert(quest == 40043); return status; }
    bool HasItemCount(uint32 item, uint32 count, bool bank) const { assert(item == 128908 && count == 1 && bank); return owned; }
    void AddDelayedEvent(uint32 delay, std::function<void()> callback) { assert(delay == 3000); pending.push_back(std::move(callback)); }
    void Flush() { auto callbacks = std::move(pending); pending.clear(); for (auto& callback : callbacks) callback(); }
    void CastSpell(Player* target, uint32 spell) { assert(target == this && (spell == 205534 || spell == 189695)); spells.push_back(spell); }
    void CastSpell(float horizontal, float vertical, float height, uint32 spell) { assert(horizontal == 3386.46f && vertical == 1406.94f && height == 68.94f && spell == 205744); spells.push_back(spell); }
    float GetOrientation() const { return 0.7f; }
    Player* GetMotionMaster() { return this; }
    void MoveJump(float horizontal, float vertical, float height, float orientation, float speed, float verticalSpeed)
    {
        assert(horizontal == 3386.46f && vertical == 1406.94f && height == 68.94f && orientation == GetOrientation() && speed == 3 && verticalSpeed == 3);
        ++jumps;
    }
    void CreateConversation(uint32 conversation) { assert(conversation == 838); ++conversations; }
};
struct InstanceMap : Map
{
    uint32 GetInstanceId() const { return 42; }
    PlayerList const& GetPlayers() const { return players; }
};
struct InstanceScript
{
    InstanceMap* instance;
    explicit InstanceScript(InstanceMap* map) : instance(map) { }
    virtual ~InstanceScript() = default;
    virtual void Initialize() { }
    virtual void OnPlayerEnter(Player*) { }
    virtual void Update(uint32) { }
};
#include "Policy.inc"
struct Instance : InstanceScript
{
    uint32 outrotimer = 0, count = 0, count_wave = 0;
    bool jump = false, myscenario = false;
    explicit Instance(InstanceMap* map) : InstanceScript(map) { Initialize(); }
#include "Initialize.inc"
#include "OnPlayerEnter.inc"
#include "Update.inc"
};
void Invalidate(Player& player, InstanceScript* alternative, uint32 variant)
{
    switch (variant)
    {
        case 0: player.world = false; break;
        case 1: player.contact = false; break;
        case 2: player.alive = false; break;
        case 3: player.teleporting = true; break;
        case 4: player.klass = 3; break;
        case 5: player.map = nullptr; break;
        case 6: player.mapId = 1220; player.map = &otherMap; break;
        case 7: player.instanceId = 43; break;
        case 8: player.script = alternative; break;
        case 9: player.rewarded = true; break;
        case 10: player.status = 0; break;
        case 11: player.status = 5; break;
        case 12: manager.available = false; break;
        case 13: manager.scenario.identity = 973; break;
    }
}
int main()
{
    for (uint32 variant = 0; variant < 14; ++variant)
    {
        manager = {}; InstanceMap map; Instance instance(&map), alternative(&map); Player player; player.script = &instance;
        instance.OnPlayerEnter(&player); assert(player.pending.size() == 1);
        Invalidate(player, &alternative, variant); player.Flush();
#ifdef BASELINE
        assert(player.jumps == uint32(variant != 4));
#else
        assert(!player.jumps && player.spells.empty() && !instance.myscenario && instance.jump);
#endif
    }
    for (uint32 step = 0; step <= 8; ++step) for (bool owned : {false, true})
    {
        manager = {}; manager.scenario.step = step; InstanceMap map; Instance instance(&map); Player player; player.script = &instance; player.owned = owned;
        instance.OnPlayerEnter(&player); player.Flush();
#ifdef BASELINE
        assert(player.jumps == 1);
#else
        assert(player.jumps == uint32(step == 0) && instance.myscenario);
        assert(instance.jump == (step != 0));
#endif
    }
    for (uint32 variant = 0; variant < 16; ++variant)
    {
        manager = {}; InstanceMap map; Instance instance(&map), alternative(&map); Player player; player.script = &instance;
        map.players.push_back({&player}); instance.OnPlayerEnter(&player); player.Flush();
        assert(player.spells.size() == 2 && player.jumps == 1 && !instance.jump);
        instance.Update(4999); assert(player.spells.size() == 2 && !player.conversations);
        if (variant < 14) Invalidate(player, &alternative, variant);
        if (variant == 14) manager.scenario.step = 1;
        if (variant == 15) manager.scenario.step = 7;
        instance.Update(1); instance.Update(5000);
#ifdef BASELINE
        assert(player.spells.size() == 3 && player.conversations == 1);
#else
        assert(player.spells.size() == 2 && !player.conversations && instance.jump);
#endif
    }
    {
        manager = {}; InstanceMap map; Instance instance(&map); Player first, second, unrelated;
        first.script = second.script = unrelated.script = &instance; second.status = QUEST_STATUS_COMPLETE; unrelated.klass = 3;
        for (Player* player : {&first, &second, &unrelated, static_cast<Player*>(nullptr)}) map.players.push_back({player});
        instance.OnPlayerEnter(nullptr); instance.OnPlayerEnter(&first); first.Flush();
        instance.OnPlayerEnter(&second); second.Flush(); instance.OnPlayerEnter(&unrelated); unrelated.Flush();
#ifdef BASELINE
        assert(!instance.myscenario && !instance.jump);
#else
        assert(instance.myscenario && !instance.jump && first.jumps == 1 && second.jumps == 1 && !unrelated.jumps);
        instance.Update(5000); instance.Update(5000);
        assert(first.conversations == 1 && second.conversations == 1 && !unrelated.conversations && instance.jump);
        assert(first.spells.size() == 3 && second.spells.size() == 3 && unrelated.spells.empty());
#endif
    }
#ifdef BASELINE
    std::cout << "BASELINE: pinned9fe2be7b requests arrival for13 invalid deferred contexts, every later stage and16 invalid landing contexts; non-Warrior clears shared myscenario.\n";
#else
    std::cout << "PASS: actual arrival policy/callback/update reject14 invalid deferred contexts and16 late landing contexts, retain18 stage/ownership bootstrap cases including fresh owned-item replay, active incomplete/complete eligible group arrivals and exactly-once landing requests; unrelated entrants cannot disable the instance. Controlled casts/movement/timer, not native packet/flight/rendering or persisted client traversal.\n";
#endif
}
