#include <cassert>
#include <cstdint>
#include <iostream>

using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using DamageEffectType = uint32;

struct Creature;
struct Player;

struct Unit
{
    uint32 entry = 0;
    int map = 1710;
    int instance = 4;
    int phase = 1;
    bool inWorld = true;
    bool deleted = false;
    bool alive = true;
    Creature* creature = nullptr;
    Player* owner = nullptr;

    uint32 GetEntry() const { return entry; }
    int GetMapId() const { return map; }
    int GetInstanceId() const { return instance; }
    bool IsInWorld() const { return inWorld; }
    bool IsPreDelete() const { return deleted; }
    bool IsAlive() const { return alive; }
    bool IsInMap(Unit const* unit) const { return map == unit->map && instance == unit->instance; }
    bool InSamePhase(Unit const* unit) const { return phase == unit->phase; }
    Creature* ToCreature() const { return creature; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const { return owner; }
};

struct Aura
{
    Unit* caster;
    Unit* GetCaster() const { return caster; }
};

struct Creature : Unit
{
    int spawn = 0;
    unsigned health = 1;
    Aura* draw = nullptr;

    Creature() { creature = this; }
    bool IsFullHealth() const { return health == 100; }
    int GetDBTableGUIDLow() const { return spawn; }
    Aura* GetAura(uint32 aura) const { return aura == 237846 ? draw : nullptr; }
};

struct Group
{
    int guid = 9;
    bool isLFGGroup() const { return true; }
    int GetMembersCount() const { return 1; }
    int GetGUID() const { return guid; }
};

struct Player : Unit
{
    Group group;
    bool pending = false;
    Player() { owner = this; }
    Group const* GetGroup() const { return &group; }
    bool IsBeingTeleportedFar() const { return pending; }
};

struct Scenario
{
    int step = 4;
    int GetScenarioId() const { return 1281; }
    bool IsCompleted(bool) const { return step > 6; }
    int GetCurrentStep() const { return step; }
};

struct ScenarioManager
{
    Scenario scenario;
    Scenario* GetScenario(int instance) { return instance == 4 ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;

struct LfgManager
{
    int GetDungeon(int group) const { return group == 9 ? 1471 : 0; }
} lfgManager;
auto* sLFGMgr = &lfgManager;

struct SmartAI
{
    explicit SmartAI(Creature* creature) : me(creature) {}
    virtual ~SmartAI() = default;
    virtual void Reset() {}
    virtual void OnHealApplied(Unit*, uint64, uint64) {}
    virtual void DamageTaken(Unit*, uint32&, DamageEffectType) {}
    Creature* me;
};

#include "RisenThreatHeals.inc"

int main()
{
    Creature erdris;
    erdris.entry = 118488;
    erdris.spawn = 14534095;
    Aura attraction{ &erdris };
    Creature soul;
    soul.entry = 119476;
    soul.draw = &attraction;
    Player healer;
    Unit healingTotem;
    healingTotem.owner = &healer;
    RisenThreatSoulAI ai(&soul);

    for (uint32 entry = 119476; entry <= 119480; ++entry)
    {
        soul.entry = entry;
        ai.Reset();
        soul.health = 99;
        ai.OnHealApplied(&healer, 80, 80);
        assert(!ai.WasFullyHealed());
        soul.health = 100;
        ai.OnHealApplied(&healer, 80, 0);
        assert(!ai.WasFullyHealed());
        ai.OnHealApplied(&erdris, 1, 1);
        assert(!ai.WasFullyHealed());

        healer.phase = 2;
        ai.OnHealApplied(&healingTotem, 8, 1);
        assert(!ai.WasFullyHealed());
        healer.phase = 1;
        healer.pending = true;
        ai.OnHealApplied(&healingTotem, 8, 1);
        assert(!ai.WasFullyHealed());
        healer.pending = false;
        ai.OnHealApplied(&healingTotem, 8, 1);
        assert(ai.WasFullyHealed());

        erdris.spawn = 6;
        assert(!ai.WasFullyHealed());
        erdris.spawn = 14534095;
        soul.draw = nullptr;
        assert(!ai.WasFullyHealed());
        soul.draw = &attraction;

        uint32 damage = 1;
        ai.DamageTaken(&erdris, damage, 0);
        assert(!ai.WasFullyHealed());
    }

    soul.entry = 119470;
    soul.draw = nullptr;
    ai.OnHealApplied(&healer, 10, 10);
    assert(!ai.WasFullyHealed());
    scenarioManager.scenario.step = 2;
    ai.OnHealApplied(&healer, 10, 10);
    assert(ai.WasFullyHealed());
    scenarioManager.scenario.step = 3;
    assert(!ai.WasFullyHealed());
    std::cout << "PASS actual full heals, controlled healers, all five wave actors, staircase rescue and stale state guards\n";
}
