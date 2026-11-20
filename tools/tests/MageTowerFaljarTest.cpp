#include <cassert>
#include <cstdint>
#include <iostream>
#include <iterator>

using uint32 = uint32_t;
enum { UNIT_STATE_EVADE, CURRENT_GENERIC_SPELL };
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_DONT_REPORT };
struct SpellInfo { uint32 Id; };
struct Spell
{
    SpellInfo info;
    SpellInfo const* GetSpellInfo() const { return &info; }
};
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
    Player* controller = nullptr;
    Creature* actor = nullptr;
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const { return controller; }
    Creature* ToCreature() const { return actor; }
};
struct CreatureAI;
struct Creature : Unit
{
    Creature() { actor = this; }
    bool alive = true, combat = true, evading = false, friendly = false;
    bool sameMap = true, samePhase = true, IsAIEnabled = true, shielded = false, failCast = false;
    uint32 map = 1703;
    Unit* victim = nullptr;
    CreatureAI* ai = nullptr;
    Spell const* casting = nullptr;
    unsigned shieldCasts = 0, interruptions = 0;

    uint32 GetMapId() const { return map; }
    uint32 GetEntry() const { return 116496; }
    uint32 GetInstanceId() const { return 31; }
    uint32 GetGUID() const { return 44; }
    bool IsAlive() const { return alive; }
    bool isInCombat() const { return combat; }
    bool HasUnitState(int) const { return evading; }
    Unit* getVictim() const { return victim; }
    bool IsInMap(Player*) const { return sameMap; }
    bool InSamePhase(Player*) const { return samePhase; }
    bool IsFriendlyTo(Player*) const { return friendly; }
    CreatureAI* AI() const { return ai; }
    Spell const* GetCurrentSpell(int) const { return casting; }
    bool IsNonMeleeSpellCast(bool) const { return casting != nullptr; }

    SpellCastResult CastSpell(Creature* target, uint32 spell, bool triggered)
    {
        assert(target == this && spell == 237949 && !triggered);
        ++shieldCasts;
        if (failCast)
            return SPELL_FAILED_DONT_REPORT;
        shielded = true;
        return SPELL_CAST_OK;
    }

    void InterruptSpell(int slot, bool, bool, uint32 filter)
    {
        assert(slot == CURRENT_GENERIC_SPELL && filter == 237952);
        if (casting && casting->info.Id == filter)
        {
            casting = nullptr;
            ++interruptions;
        }
    }

    void RemoveAurasDueToSpell(uint32 spell, uint32 owner)
    {
        assert(spell == 237949 && owner == GetGUID());
        shielded = false;
    }
};

struct CreatureAI
{
    explicit CreatureAI(Creature* creature) : me(creature) { me->ai = this; }
    virtual ~CreatureAI() = default;
    Creature* me;
};
struct SmartAI : CreatureAI
{
    using CreatureAI::CreatureAI;
    virtual void Reset() { ++resets; }
    virtual void EnterCombat(Unit*) { ++pulls; }
    virtual void UpdateAI(uint32) { ++updates; }
    virtual void SetData(uint32, uint32) { ++dataCalls; }
    virtual void JustDied(Unit*) { ++deaths; }
    unsigned resets = 0, pulls = 0, updates = 0, dataCalls = 0, deaths = 0;
};
struct CreatureScript
{
    explicit CreatureScript(char const*) {}
    virtual CreatureAI* GetAI(Creature*) const = 0;
};
struct Scenario
{
    uint32 id = 1265, step = 1;
    bool complete = false;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
    bool IsCompleted(bool bonus) const { assert(!bonus); return complete; }
};
struct ScenarioManager
{
    Scenario* active;
    Scenario* GetScenario(uint32 instance) { assert(instance == 31); return active; }
};
ScenarioManager* sScenarioMgr;
unsigned warnings = 0;
#define TC_LOG_WARN(...) (++warnings)
struct SpellScript
{
    struct Hook { void operator+=(int) {} };
    Hook OnCheckCast;
    Unit* scriptCaster = nullptr;
    Unit* GetCaster() const { return scriptCaster; }
    virtual void Register() {}
};
#define PrepareSpellScript(...) public:
#define SpellCheckCastFn(...) 0
#include "MageTowerFaljar.inc"

int main()
{
    Creature faljar;
    GodQueen::FaljarAI ai(&faljar);
    Scenario scenario;
    ScenarioManager manager { &scenario };
    sScenarioMgr = &manager;
    Player player;
    Unit pet { &player, nullptr };
    faljar.victim = &pet;
    spell_mage_tower_ancestral_blast blast;
    blast.scriptCaster = &faljar;
    Spell shadowbolt { { 237890 } }, blastCast { { 237952 } };

    assert(blast.CheckAttempt() == SPELL_FAILED_DONT_REPORT);
    ai.EnterCombat(&pet);
    ai.UpdateAI(98399);
    assert(ai.updates == 1 && faljar.shieldCasts == 0);
    faljar.casting = &shadowbolt;
    ai.UpdateAI(1);
    assert(faljar.shieldCasts == 0);
    faljar.casting = nullptr;
    ai.UpdateAI(1);
    assert(faljar.shieldCasts == 1 && faljar.shielded && ai.updates == 2);
    assert(blast.CheckAttempt() == SPELL_CAST_OK);
    ai.UpdateAI(5);
    assert(ai.updates == 2); // Native follow-up gets time to start.
    faljar.casting = &blastCast;
    ai.UpdateAI(5);
    assert(ai.updates == 3 && blast.CheckAttempt() == SPELL_CAST_OK);

    auto rejects = [&](auto& field, auto invalid)
    {
        auto previous = field;
        field = invalid;
        assert(blast.CheckAttempt() == SPELL_FAILED_DONT_REPORT);
        field = previous;
    };
    rejects(faljar.alive, false);
    rejects(faljar.combat, false);
    rejects(faljar.evading, true);
    rejects(faljar.friendly, true);
    rejects(faljar.sameMap, false);
    rejects(faljar.samePhase, false);
    rejects(faljar.map, 1702u);
    rejects(player.alive, false);
    rejects(player.inWorld, false);
    rejects(player.pending, true);
    rejects(pet.controller, static_cast<Player*>(nullptr));
    rejects(scenario.complete, true);
    rejects(scenario.step, 0u);
    rejects(scenario.id, 1221u);

    faljar.casting = nullptr; // Player or pet interrupts through the native engine.
    ai.UpdateAI(69194);
    assert(!faljar.shielded && faljar.shieldCasts == 1);
    ai.UpdateAI(1);
    assert(faljar.shieldCasts == 2 && blast.CheckAttempt() == SPELL_CAST_OK);
    faljar.casting = &blastCast;
    ai.UpdateAI(10);
    ai.SetData(3, 3);
    assert(ai.dataCalls == 1 && faljar.interruptions == 1 && !faljar.shielded);
    assert(blast.CheckAttempt() == SPELL_FAILED_DONT_REPORT);

    ai.Reset();
    ai.EnterCombat(&pet);
    ai.UpdateAI(98400);
    assert(faljar.shielded);
    ai.UpdateAI(250); // Native follow-up does not start.
    assert(!faljar.shielded && warnings == 1 && blast.CheckAttempt() == SPELL_FAILED_DONT_REPORT);
    faljar.failCast = true;
    ai.UpdateAI(14750);
    assert(faljar.shieldCasts == 4 && !faljar.shielded);
    ai.UpdateAI(999);
    assert(faljar.shieldCasts == 4);
    faljar.failCast = false;
    ai.UpdateAI(1);
    assert(faljar.shieldCasts == 5 && blast.CheckAttempt() == SPELL_CAST_OK);
    ai.JustDied(&pet);
    assert(ai.deaths == 1 && ai.resets == 1 && ai.pulls == 2 && !faljar.shielded);
    assert(blast.CheckAttempt() == SPELL_FAILED_DONT_REPORT);
    std::cout << "PASS Faljar production cadence, native-cast handoff, retry, interrupt, surrender and reset guards.\n";
}
