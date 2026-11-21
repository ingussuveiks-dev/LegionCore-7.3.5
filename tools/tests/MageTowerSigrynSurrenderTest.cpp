#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

struct InstanceMap
{
    uint32 id = 57;
    uint32 GetInstanceId() const { return id; }
};
struct Player
{
    InstanceMap* map = nullptr;
    bool present = true;
    bool removed = false;
    bool far = false;
    bool samePhase = true;
    bool IsInWorld() const { return present; }
    bool IsPreDelete() const { return removed; }
    bool IsBeingTeleportedFar() const { return far; }
};
struct Unit
{
    Player* challenger = nullptr;
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const { return challenger; }
};
struct Creature
{
    InstanceMap* map;
    uint32 entry;
    uint64 spawn;
    uint64 health = 10000000;
    uint64 maximum = 10000000;
    uint64 baseMaximum = 10000000;
    bool alive = true;
    bool present = true;
    bool nativeAura = true;

    InstanceMap* GetMap() const { return map; }
    uint32 GetEntry() const { return entry; }
    uint64 GetDBTableGUIDLow() const { return spawn; }
    uint64 GetGUID() const { return spawn + 1000; }
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return present; }
    bool HasAura(uint32 spell, uint64 caster) const { return nativeAura && spell == 239056 && caster == GetGUID(); }
    bool IsInMap(Player const* player) const { return map == player->map; }
    bool InSamePhase(Player const* player) const { return player->samePhase; }
    uint64 GetHealth(Unit const*) const { return health; }
    uint64 GetMaxHealth(Unit const*) const { return maximum; }
    uint64 GetMaxHealth() const { return baseMaximum; }
};
struct Scenario
{
    uint32 id = 1265;
    uint32 step = 1;
    bool completed = false;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
    bool IsCompleted(bool) const { return completed; }
};
struct ScenarioMgr
{
    Scenario scenario;
    Scenario* GetScenario(uint32 id) { return id == 57 ? &scenario : nullptr; }
} scenarios;
auto* sScenarioMgr = &scenarios;

struct InstanceScript
{
    explicit InstanceScript(InstanceMap* map) : instance(map) {}
    virtual ~InstanceScript() = default;
    virtual void OnCreatureDamageTaken(Creature*, Unit*, uint32&) {}
    InstanceMap* instance;
};
struct InstanceMapScript
{
    InstanceMapScript(char const*, uint32 map) { assert(map == 1703); }
    virtual ~InstanceMapScript() = default;
    virtual InstanceScript* GetInstanceScript(InstanceMap*) const = 0;
};

#include "GodQueenInstance.inc"

int main()
{
    InstanceMap map;
    InstanceMap foreignMap{60};
    Player player{&map};
    Unit direct{&player}, pet{&player}, guardian{&player}, unowned;
    instance_mage_tower_god_queen::GodQueenInstance instance(&map);

    Creature sigryn{&map, 116484, 387451};
    Creature faljar{&map, 116496, 387454};
    Creature jarl{&map, 116499, 387453};
    std::array<Creature*, 3> const actors{&sigryn, &faljar, &jarl};
    std::array<uint32, 3> const nativeThresholds{20, 23, 27};
    for (size_t i = 0; i < actors.size(); ++i)
    {
        Creature* actor = actors[i];
        for (Unit* source : {&direct, &pet, &guardian})
        {
            uint32 amount = 125000;
            instance.OnCreatureDamageTaken(actor, source, amount);
            assert(amount == 125000);
            amount = std::numeric_limits<uint32>::max();
            instance.OnCreatureDamageTaken(actor, source, amount);
            assert(amount == 9900000 && amount < actor->health);
            uint64 const remaining = actor->health - amount;
            assert(remaining && remaining * 100 / actor->maximum < nativeThresholds[i]);
        }
    }

    jarl.maximum = 100000000;
    jarl.health = 100000000;
    jarl.baseMaximum = 1000000;
    uint32 burst = 100000000;
    instance.OnCreatureDamageTaken(&jarl, &guardian, burst);
    assert(burst == 99000000 && (jarl.health - burst) * jarl.baseMaximum / jarl.maximum == 10000);
    jarl.health = 1000000;
    burst = 1000000;
    instance.OnCreatureDamageTaken(&jarl, &guardian, burst);
    assert(burst == 0);

    auto remainsUnchanged = [&instance](Creature& target, Unit& source)
    {
        uint32 amount = 25000000;
        instance.OnCreatureDamageTaken(&target, &source, amount);
        assert(amount == 25000000);
    };
    remainsUnchanged(sigryn, unowned);
    sigryn.spawn = 100;
    remainsUnchanged(sigryn, pet);
    sigryn.spawn = 387451;
    sigryn.nativeAura = false;
    remainsUnchanged(sigryn, guardian);
    sigryn.nativeAura = true;
    sigryn.map = &foreignMap;
    remainsUnchanged(sigryn, direct);
    sigryn.map = &map;
    player.far = true;
    remainsUnchanged(sigryn, pet);
    player.far = false;
    player.samePhase = false;
    remainsUnchanged(sigryn, guardian);
    player.samePhase = true;
    scenarios.scenario.step = 0;
    remainsUnchanged(sigryn, direct);
    scenarios.scenario.step = 1;
    scenarios.scenario.completed = true;
    remainsUnchanged(sigryn, direct);
    scenarios.scenario.completed = false;
    burst = 25000000;
    instance.OnCreatureDamageTaken(&sigryn, &guardian, burst);
    assert(burst == 9900000);

    std::cout << "PASS all three native surrender thresholds under player, pet and guardian hits, including extreme scaling and retry\n";
}
