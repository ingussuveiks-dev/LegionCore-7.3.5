#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
using uint32 = uint32_t;
enum { SMART_EVENT_JUST_SUMMONED = 54, SMART_EVENT_SPELLHIT = 8, SMART_EVENT_SPELLHIT_TARGET = 31,
    SMART_TARGET_NONE = 0, SMART_TARGET_ACTION_INVOKER = 7, SPELL_CAST_OK = 0, SPELL_FAILED_ERROR = 1 };
using SpellCastResult = int;
using DamageEffectType = int;
uint32 warnings = 0;
#define TC_LOG_WARN(...) (++warnings)
struct SpellInfo { uint32 Id = 201899; } soulSpell;
struct Creature;
struct Unit {
    virtual ~Unit() = default;
    virtual Creature* ToCreature() { return nullptr; }
    bool IsCreature() { return ToCreature() != nullptr; }
};
struct TempSummon { Unit* owner = nullptr; Unit* GetSummoner() { return owner; } };
struct CreatureAI {
    virtual ~CreatureAI() = default;
    virtual void Reset() { }
    virtual void DamageTakenAfterScaling(Unit*, uint32&, DamageEffectType) { }
    virtual void IsSummonedBy(Unit*) { }
    virtual void SpellHit(Unit*, SpellInfo const*) { }
    virtual void SpellHitTarget(Unit*, SpellInfo const*) { }
    virtual void UpdateAI(uint32) { }
};
struct Creature : Unit {
    uint32 entry = 102247, mapId = 1539, instance = 42, phase = 1;
    bool world = true, alive = true, temporary = true, aura = true, IsAIEnabled = true, casting = false;
    float hp = 1.0f, distance = 20.0f;
    int map = 1;
    TempSummon summon;
    CreatureAI* selected = nullptr;
    uint32 attempts = 0;
    Unit* lastTarget = nullptr;
    SpellCastResult result = SPELL_CAST_OK;
    bool deliverHit = true;
    Creature* ToCreature() override { return this; }
    uint32 GetEntry() { return entry; }
    uint32 GetMapId() { return mapId; }
    uint32 GetInstanceId() { return instance; }
    int GetMap() { return map; }
    bool IsInWorld() { return world; }
    bool IsAlive() { return alive; }
    bool InSamePhase(Creature* other) { return phase == other->phase; }
    bool IsWithinDistInMap(Creature* other, float range) { return map == other->map && distance <= range; }
    bool HasAura(uint32 spell) { assert(spell == 61204); return aura; }
    float GetHealthPct() { return hp; }
    TempSummon* ToTempSummon() { return temporary ? &summon : nullptr; }
    CreatureAI* AI() { return selected; }
    bool IsNonMeleeSpellCast(bool delayed) { assert(!delayed); return casting; }
    SpellCastResult CastSpell(Unit*, uint32, bool);
};
struct Scenario {
    uint32 identity = 991, step = 7;
    uint32 GetScenarioId() { return identity; }
    uint32 GetCurrentStep() { return step; }
};
struct ScenarioMgr {
    bool present = true;
    Scenario value;
    Scenario* GetScenario(uint32 instance) { return present && instance == 42 ? &value : nullptr; }
} scenarios;
auto* sScenarioMgr = &scenarios;
using ObjectList = std::vector<Unit*>;
Unit* NativeInvoker(Unit* trigger) {
    ObjectList targets;
    auto* selectedTargets = &targets;
    switch (SMART_TARGET_ACTION_INVOKER) {
#define l selectedTargets
#include "NativeInvoker.inc"
#undef l
    }
    return targets.empty() ? nullptr : targets.front();
}
struct Script {
    Creature* actor;
    uint32 initial = 0, hits = 0, outgoing = 0, updates = 0;
    bool finale = false;
    explicit Script(Creature* source) : actor(source) { }
    void ProcessEventsFor(int event, Unit* unit, uint32 = 0, uint32 = 0, bool = false, SpellInfo const* spell = nullptr) {
        if (event == SMART_EVENT_JUST_SUMMONED) {
            if (initial) return;
            ++initial;
            if (Unit* target = NativeInvoker(unit)) actor->CastSpell(target, 201899, false);
        } else if (event == SMART_EVENT_SPELLHIT_TARGET) {
            ++outgoing;
        } else {
            assert(event == SMART_EVENT_SPELLHIT);
            if (spell && spell->Id == 201899 && !finale) { finale = true; ++hits; }
        }
    }
};
struct SmartAI : CreatureAI {
    Creature* me;
    Script script;
    explicit SmartAI(Creature* actor) : me(actor), script(actor) { }
    Script* GetScript() { return &script; }
    void Reset() override { script.initial = 0; script.finale = false; }
    void IsSummonedBy(Unit*) override;
    void SpellHit(Unit*, SpellInfo const*) override;
    void SpellHitTarget(Unit*, SpellInfo const*) override;
    void UpdateAI(uint32) override { ++script.updates; }
};
#include "NativeIsSummonedBy.inc"
#include "NativeSpellHit.inc"
namespace XalatathFarthing { void Protect(Creature*, Unit*, uint32&) { } }
#include "FarthingAI.inc"
#ifndef BASELINE
#include "SoulAttack.inc"
#else
struct npc_xalatath_soul_attack : SmartAI { using SmartAI::SmartAI; };
#endif
void NativeHit(Creature* m_caster, Unit* spellHitTarget, SpellInfo const* m_spellInfo) {
#include "NativeHitDispatch.inc"
}
SpellCastResult Creature::CastSpell(Unit* target, uint32 spell, bool triggered) {
    assert(spell == 201899 && !triggered);
    ++attempts; lastTarget = target;
    if (result == SPELL_CAST_OK && deliverHit) NativeHit(this, target, &soulSpell);
    return result;
}
struct Encounter {
    Creature owner, soul;
    npc_xalatath_farthing boss;
    npc_xalatath_soul_attack visual;
    Encounter() : boss(&owner), visual(&soul) {
        scenarios = {}; warnings = 0;
        owner.entry = 101148; owner.selected = &boss;
        soul.selected = &visual; soul.summon.owner = &owner;
    }
};
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    {
        Encounter encounter;
        encounter.soul.result = SPELL_FAILED_ERROR;
        encounter.visual.IsSummonedBy(&encounter.owner);
        assert(encounter.soul.attempts == 1 && encounter.boss.script.hits == 0);
        encounter.visual.UpdateAI(4999); assert(encounter.soul.attempts == 1);
        encounter.visual.UpdateAI(1); assert(encounter.soul.attempts == 2 && warnings == 1);
        encounter.soul.result = SPELL_CAST_OK; encounter.soul.deliverHit = false;
        encounter.visual.UpdateAI(5000); assert(encounter.soul.attempts == 3 && encounter.boss.script.hits == 0);
        encounter.soul.casting = true;
        encounter.visual.UpdateAI(5000); assert(encounter.soul.attempts == 3);
        encounter.soul.casting = false; encounter.soul.deliverHit = true;
        encounter.visual.UpdateAI(5000);
        assert(encounter.soul.attempts == 4 && encounter.soul.lastTarget == &encounter.owner && encounter.boss.script.hits == 1);
        for (uint32 repeat = 0; repeat < 20; ++repeat) encounter.visual.UpdateAI(5000);
        assert(encounter.soul.attempts == 4 && encounter.boss.script.hits == 1 && encounter.visual.script.updates == 25);
    }
    {
        Encounter encounter;
        encounter.visual.IsSummonedBy(&encounter.owner);
        encounter.visual.IsSummonedBy(&encounter.owner);
        encounter.visual.UpdateAI(5000);
        assert(encounter.soul.attempts == 1 && encounter.boss.script.hits == 1 && encounter.visual.script.initial == 1);
    }
    for (uint32 invalid = 0; invalid < 24; ++invalid) {
        Encounter encounter; Creature other; CreatureAI foreign; Unit player;
        if (invalid == 0) encounter.soul.entry = 123;
        if (invalid == 1) encounter.soul.world = false;
        if (invalid == 2) encounter.soul.alive = false;
        if (invalid == 3) encounter.soul.mapId = 1;
        if (invalid == 4) encounter.soul.temporary = false;
        if (invalid == 5) encounter.soul.summon.owner = nullptr;
        if (invalid == 6) encounter.soul.summon.owner = &player;
        if (invalid == 7) encounter.owner.entry = 123;
        if (invalid == 8) encounter.owner.world = false;
        if (invalid == 9) encounter.owner.alive = false;
        if (invalid == 10) encounter.owner.map = 2;
        if (invalid == 11) encounter.owner.phase = 2;
        if (invalid == 12) encounter.soul.distance = 30.01f;
        if (invalid == 13) encounter.owner.aura = false;
        if (invalid == 14) encounter.owner.hp = 1.01f;
        if (invalid == 15) encounter.owner.IsAIEnabled = false;
        if (invalid == 16) encounter.owner.selected = &foreign;
        if (invalid == 17) scenarios.present = false;
        if (invalid == 18) scenarios.value.identity = 1037;
        if (invalid == 19) scenarios.value.step = 8;
        if (invalid == 20) encounter.soul.instance = 43;
        if (invalid == 21) scenarios.value.identity = 1092;
        if (invalid == 22) scenarios.value.step = 6;
        if (invalid == 23) scenarios.value.step = 9;
        encounter.visual.IsSummonedBy(&encounter.owner);
        encounter.visual.UpdateAI(5000);
        assert(encounter.soul.attempts == 0 && encounter.visual.script.initial == 0 && encounter.boss.script.hits == 0);
    }
    {
        Encounter encounter; Creature other; other.entry = 101148;
        encounter.visual.IsSummonedBy(&other); assert(encounter.soul.attempts == 0);
        encounter.soul.deliverHit = false;
        encounter.visual.IsSummonedBy(&encounter.owner);
        encounter.visual.SpellHitTarget(&other, &soulSpell);
        SpellInfo unrelated{1}; encounter.visual.SpellHitTarget(&encounter.owner, &unrelated);
        encounter.visual.SpellHitTarget(nullptr, &soulSpell); encounter.visual.SpellHitTarget(&encounter.owner, nullptr);
        encounter.soul.deliverHit = true;
        encounter.visual.UpdateAI(5000);
        assert(encounter.soul.attempts == 2 && encounter.soul.lastTarget == &encounter.owner && encounter.boss.script.hits == 1);
    }
    {
        Encounter encounter;
        encounter.soul.summon.owner = nullptr; encounter.visual.UpdateAI(5000);
        encounter.soul.summon.owner = &encounter.owner;
        encounter.visual.IsSummonedBy(&encounter.owner);
        assert(encounter.soul.attempts == 1 && encounter.boss.script.hits == 1);
    }
    {
        Encounter encounter;
        encounter.soul.result = SPELL_FAILED_ERROR;
        encounter.visual.IsSummonedBy(&encounter.owner);
        encounter.boss.Reset();
        encounter.owner.hp = 100.0f;
        encounter.visual.UpdateAI(5000);
        encounter.owner.hp = 1.0f;
        encounter.soul.result = SPELL_CAST_OK;
        encounter.visual.SpellHitTarget(&encounter.owner, &soulSpell);
        encounter.visual.UpdateAI(5000); encounter.visual.UpdateAI(5000);
        assert(encounter.soul.attempts == 1 && encounter.boss.script.hits == 0);
        Creature replacement; replacement.summon.owner = &encounter.owner;
        npc_xalatath_soul_attack next(&replacement); replacement.selected = &next;
        next.IsSummonedBy(&encounter.owner); next.UpdateAI(5000);
        assert(replacement.attempts == 1 && encounter.boss.script.hits == 1);
    }
    for (uint32 temporaryFailure = 0; temporaryFailure < 3; ++temporaryFailure) {
        Encounter encounter; CreatureAI foreign;
        encounter.soul.result = SPELL_FAILED_ERROR;
        encounter.visual.IsSummonedBy(&encounter.owner);
        if (temporaryFailure == 0) encounter.soul.summon.owner = nullptr;
        if (temporaryFailure == 1) encounter.owner.selected = &foreign;
        if (temporaryFailure == 2) encounter.owner.phase = 2;
        encounter.visual.UpdateAI(5000); assert(encounter.soul.attempts == 1);
        encounter.soul.summon.owner = &encounter.owner;
        encounter.owner.selected = &encounter.boss; encounter.owner.phase = 1;
        encounter.soul.result = SPELL_CAST_OK;
        encounter.visual.UpdateAI(5000);
        assert(encounter.soul.attempts == 2 && encounter.boss.script.hits == 1);
    }
    std::cout << "PASS: native summoner/invoker and target-before-caster hit callbacks; failed cast, accepted-without-hit, busy caster, exact retry boundary, owner-only acknowledgement, inactive/missing/foreign contexts, canceled old-generation retries and a fresh ending after reset. No credit, loot, full reset/re-entry traversal or client visual certification.\n";
}
