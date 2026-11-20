#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>

using uint8 = uint8_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
enum { UNIT_STATE_EVADE, UNIT_FIELD_CREATED_BY_SPELL, SPELL_CAST_OK = 0 };
using SpellEffIndex = int;
struct SpellInfo { uint32 Id; };
struct WorldObject {};
struct Player
{
    bool alive = true, inWorld = true, pending = false;
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return inWorld; }
    bool HasPendingTeleport() const { return pending; }
};
struct Creature;
struct Unit
{
    Creature* creature = nullptr;
    Player* controller = nullptr;
    Creature* ToCreature() const { return creature; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const { return controller; }
};
struct TempSummon
{
    Unit* owner;
    Unit* GetSummoner() const { return owner; }
};
struct CreatureAI;
struct Creature : Unit
{
    Creature() { creature = this; }
    bool alive = true, combat = true, evading = false, phase = true, sameMap = true, IsAIEnabled = true;
    uint32 entry = 117230, map = 1702, createSpell = 0, health = 100, maxHealth = 100, createHealth = 100;
    Unit* victim = nullptr;
    TempSummon* summon = nullptr;
    CreatureAI* ai = nullptr;
    unsigned normalCasts = 0, triggeredCasts = 0, failures = 0, despawns = 0, delayedDespawns = 0;
    bool IsAlive() const { return alive; }
    bool isInCombat() const { return combat; }
    uint32 GetMapId() const { return map; }
    uint32 GetEntry() const { return entry; }
    uint32 GetInstanceId() const { return 42; }
    bool HasUnitState(int) const { return evading; }
    Unit* getVictim() const { return victim; }
    bool IsInMap(Player const*) const { return sameMap; }
    bool InSamePhase(Player const*) const { return phase; }
    bool isTotem() const { return summon != nullptr; }
    TempSummon* ToTempSummon() const { return summon; }
    uint32 GetUInt32Value(int) const { return createSpell; }
    void SetHealth(uint32 value) { health = value; }
    void SetMaxHealth(uint32 value) { maxHealth = value; }
    void SetCreateHealth(uint32 value) { createHealth = value; }
    CreatureAI* AI() const { return ai; }
    int CastSpell(Creature* target, uint32 spell, bool triggered)
    {
        assert(target == this);
        assert(spell == (triggered ? 243224u : 242496u));
        ++(triggered ? triggeredCasts : normalCasts);
        if (!failures)
            return SPELL_CAST_OK;
        --failures;
        return 1;
    }
    void DespawnOrUnsummon(uint32 delay = 0)
    {
        assert(delay == 0 || delay == 500);
        ++(delay ? delayedDespawns : despawns);
    }
};
struct CreatureAI
{
    explicit CreatureAI(Creature* actor) : me(actor) { actor->ai = this; }
    virtual ~CreatureAI() = default;
    virtual void Reset() {}
    virtual void EnterCombat(Unit*) {}
    virtual void UpdateAI(uint32) {}
    virtual void InitializeAI() {}
    virtual void SpellFinishCast(SpellInfo const*) {}
    Creature* me;
};
struct SmartAI : CreatureAI
{
    using CreatureAI::CreatureAI;
    unsigned pulls = 0, resets = 0, updates = 0;
    void Reset() override { ++resets; }
    void EnterCombat(Unit*) override { ++pulls; }
    void UpdateAI(uint32) override { ++updates; }
};
using ScriptedAI = CreatureAI;
struct CreatureScript
{
    explicit CreatureScript(char const*) {}
    virtual CreatureAI* GetAI(Creature*) const = 0;
};
struct Scenario
{
    uint32 id = 1266, step = 1;
    bool complete = false;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
    bool IsCompleted(bool bonus) const { assert(!bonus); return complete; }
};
struct ScenarioManager
{
    Scenario* current = nullptr;
    Scenario* GetScenario(uint32 instance) { assert(instance == 42); return current; }
};
ScenarioManager* sScenarioMgr;
unsigned warnings = 0;
uint32 urand(uint32 min, uint32 max) { assert(min == 25000 && max == 33000); return max; }
#define TC_LOG_WARN(...) (++warnings)

struct SpellScript
{
    struct Hook { void operator+=(int) {} };
    virtual bool Load() { return true; }
    virtual void Register() {}
    Unit* scriptCaster = nullptr;
    bool prevented = false;
    int value = 2;
    Unit* GetCaster() const { return scriptCaster; }
    void SetEffectValue(int next) { value = next; }
    void PreventHitDefaultEffect(SpellEffIndex index) { assert(index == 0); prevented = true; }
    Hook OnEffectHit, OnObjectAreaTargetSelect;
};
#define PrepareSpellScript(...) public:
#define SpellEffectFn(...) 0
#define SpellObjectAreaTargetSelectFn(...) 0
#include "FeltotemFelSurge.inc"

int main()
{
    Scenario scenario;
    ScenarioManager manager { &scenario };
    sScenarioMgr = &manager;
    Player player;
    Unit pet;
    pet.controller = &player;
    Creature tugar;
    tugar.victim = &pet;
    Feltotem::TugarAI boss(&tugar);

    assert(!Feltotem::ActiveTugarAttempt(&tugar));
    boss.EnterCombat(&pet);
    assert(boss.pulls == 1 && Feltotem::ActiveTugarAttempt(&tugar));
    scenario.step = 0;
    boss.UpdateAI(60000);
    assert(tugar.triggeredCasts == 0);
    scenario.step = 1;
    boss.UpdateAI(54499);
    assert(tugar.triggeredCasts == 0);
    boss.UpdateAI(1);
    assert(tugar.triggeredCasts == 1);
    boss.UpdateAI(32999);
    assert(tugar.triggeredCasts == 1);
    boss.UpdateAI(1);
    assert(tugar.triggeredCasts == 2 && boss.updates == 5);

    spell_mage_tower_fel_surge_missile missile;
    missile.scriptCaster = &tugar;
    assert(missile.Load());
    missile.Land(0);
    assert(!missile.prevented);
    spell_mage_tower_fel_surge_summon create;
    create.scriptCaster = &tugar;
    create.Summon(0);
    assert(!create.prevented && create.value == 1);

    TempSummon instance { &tugar };
    Creature totem;
    totem.entry = 121499;
    totem.createSpell = 237903;
    totem.summon = &instance;
    npc_mage_tower_fel_surge_totem::TotemAI totemAI(&totem);
    totemAI.InitializeAI();
    assert(totem.health == 1 && totem.maxHealth == 1 && totem.createHealth == 1);
    totem.failures = 1;
    totemAI.UpdateAI(1);
    totemAI.UpdateAI(249);
    assert(totem.normalCasts == 1);
    totemAI.UpdateAI(1);
    totemAI.UpdateAI(10000);
    assert(totem.normalCasts == 2 && totem.despawns == 0);

    spell_mage_tower_fel_surge damage;
    damage.scriptCaster = &totem;
    WorldObject hostile;
    std::list<WorldObject*> targets { &hostile };
    damage.FilterTargets(targets);
    assert(targets.size() == 1);
    SpellInfo surge { 242496 };
    totemAI.SpellFinishCast(&surge);
    assert(totem.delayedDespawns == 1);

    auto rejects = [&](auto& field, auto invalid)
    {
        auto previous = field;
        field = invalid;
        targets = { &hostile };
        damage.FilterTargets(targets);
        assert(targets.empty());
        field = previous;
    };
    rejects(totem.alive, false);
    rejects(tugar.alive, false);
    rejects(tugar.combat, false);
    rejects(tugar.evading, true);
    rejects(tugar.phase, false);
    rejects(tugar.sameMap, false);
    rejects(tugar.map, 1600u);
    rejects(scenario.id, 1221u);
    rejects(scenario.complete, true);
    rejects(player.alive, false);
    rejects(player.inWorld, false);
    rejects(player.pending, true);
    rejects(pet.controller, static_cast<Player*>(nullptr));

    boss.Reset();
    boss.EnterCombat(&pet);
    missile.Land(0);
    assert(missile.prevented);
    targets = { &hostile };
    damage.FilterTargets(targets);
    assert(targets.empty());
    totemAI.UpdateAI(1);
    assert(totem.despawns == 1 && boss.resets == 1);

    npc_mage_tower_fel_surge_totem::TotemAI failing(&totem);
    failing.InitializeAI();
    totem.failures = 3;
    failing.UpdateAI(1);
    failing.UpdateAI(250);
    failing.UpdateAI(250);
    assert(totem.normalCasts == 5 && warnings == 1 && totem.despawns == 2);
    std::cout << "PASS Feltotem production timers, native summon health, bounded retry, pet owner and stale attempt suppression.\n";
}
