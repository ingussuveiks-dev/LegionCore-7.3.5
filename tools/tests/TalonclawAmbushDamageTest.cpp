#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
using uint32 = uint32_t;
using uint64 = uint64_t;
using DamageEffectType = uint32;
enum { SMART_EVENT_DAMAGED = 9, SMART_EVENT_HEALTH_PCT = 2, SMART_EVENT_FLAG_ALLOW_EVENT_IN_COMBAT = 32 };
#define TC_LOG_INFO(...) ((void)0)
struct Creature;
struct CreatureAI {
    virtual ~CreatureAI() = default;
    virtual void DamageTaken(struct Unit*, uint32&, DamageEffectType) { }
    virtual void DamageTakenAfterScaling(struct Unit*, uint32&, DamageEffectType) { }
};
struct Unit { uint32 kind = 0; uint32 Deal(Creature*, uint32); };
struct InstanceScript { void OnCreatureDamageTaken(Creature*, Unit*, uint32&) { } };
struct Creature {
    uint32 entry = 100376, map = 1511, instance = 42;
    uint64 health = 100000, maximum = 100000, targetMaximum = 1000;
    float multiplier = 0.001f;
    bool alive = true, combat = true, IsAIEnabled = true;
    CreatureAI* primary = nullptr;
    CreatureAI* i_disabledAI = nullptr;
    CreatureAI* AI() { return primary; }
    Creature* ToCreature() { return this; }
    InstanceScript* GetInstanceScript() { return nullptr; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return map; }
    uint32 GetInstanceId() const { return instance; }
    bool IsAlive() const { return alive; }
    bool isInCombat() const { return combat; }
    uint64 GetHealth() const { return health; }
    uint64 GetHealth(Unit*) const { return std::max<uint64>(1, targetMaximum * health / maximum); }
    uint64 GetMaxHealth() const { return maximum; }
    uint64 CountPctFromMaxHealth(uint32 percent, Unit* attacker = nullptr) const { return (attacker ? targetMaximum : maximum) * percent / 100; }
    float GetHealthPct() const { return 100.0f * health / maximum; }
    float GetHealthMultiplierForTarget(Unit*) const { return multiplier; }
    void SetHealth(uint64 value) { health = value; }
};
struct Scenario {
    uint32 identity = 973, step = 9;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioMgr {
    bool present = true;
    Scenario scenario;
    Scenario* GetScenario(uint32 instance) { return present && instance == 42 ? &scenario : nullptr; }
} scenarios;
auto* sScenarioMgr = &scenarios;
struct SmartAI : CreatureAI {
    Creature* me;
    uint32 mInvincibilityHpLevel = 0, damagedEvents = 0;
    explicit SmartAI(Creature* actor) : me(actor) { }
    SmartAI* GetScript() { return this; }
    void ProcessEventsFor(uint32 event, Unit*, uint32) { assert(event == SMART_EVENT_DAMAGED); ++damagedEvents; }
    void DamageTaken(Unit*, uint32&, DamageEffectType) override;
};
#include "SmartDamage.inc"
#include "Protection.inc"
#ifdef BASELINE
using npc_talonclaw_ambush_dakarr = SmartAI;
#else
#include "Wrapper.inc"
#endif
uint32 Unit::Deal(Creature* victim, uint32 damage) {
    DamageEffectType damagetype = 0;
    if (victim->IsAIEnabled && victim->AI()) victim->AI()->DamageTaken(this, damage, damagetype);
    if (!damage) return 0;
    uint64 health = victim->GetHealth(this);
#include "ScaledHook.inc"
    if (health <= damage) victim->alive = false;
    if (damage && victim->alive) victim->health -= std::min<uint64>(victim->health - 1, uint64(damage) * victim->maximum / victim->targetMaximum);
    return damage;
}
struct HealthEvent {
    struct Event {
        uint32 event_flags = 1;
        struct Range { uint32 min = 0, max = 75, repeatMin = 0, repeatMax = 0; } minMaxRepeat;
    } event;
    bool runOnce = false;
};
struct HealthProbe {
    Creature* me;
    uint32 step, starts = 0;
    HealthEvent event;
    void RecalcTimer(HealthEvent&, uint32, uint32) { }
    void ProcessAction(HealthEvent& holder) { holder.runOnce = true; ++starts; ++scenarios.scenario.step; }
    void Tick() {
        if (event.runOnce || scenarios.scenario.step != step) return;
        auto& nativeEvent = event;
        switch (SMART_EVENT_HEALTH_PCT) {
#include "HealthEvent.inc"
        }
    }
};
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    for (uint32 stage : {9u, 10u}) for (uint32 kind : {0u, 1u, 2u, 3u})
    for (float scale : {0.001f, 0.01f, 1.0f, 100.0f}) for (bool charmed : {false, true}) {
        scenarios = {}; scenarios.scenario.step = stage;
        Creature actor; actor.multiplier = scale;
        Unit attacker; attacker.kind = kind;
        npc_talonclaw_ambush_dakarr content(&actor); CreatureAI charm;
        if (stage == 9) content.mInvincibilityHpLevel = 70000;
        actor.primary = charmed ? &charm : &content; actor.i_disabledAI = charmed ? &content : nullptr;
        HealthProbe first{&actor, 9}, second{&actor, 10}; second.event.event.minMaxRepeat.max = 60;
        first.Tick(); second.Tick(); assert(!first.starts && !second.starts);
        uint32 raw = uint32(2000 * scale);
        assert(attacker.Deal(&actor, raw) == 0 && actor.alive);
        assert(actor.health == (stage == 9 ? 70000 : 60000));
        first.Tick(); second.Tick(); first.Tick(); second.Tick();
        assert(scenarios.scenario.step == stage + 1);
        assert(first.starts == (stage == 9 ? 1u : 0u) && second.starts == (stage == 10 ? 1u : 0u));
        actor.IsAIEnabled = false;
        assert(attacker.Deal(&actor, raw) == 0 && actor.health == (stage == 9 ? 70000 : 60000) && actor.alive);
        if (stage == 9) { second.Tick(); assert(!second.starts); }
        assert(content.damagedEvents == (charmed ? 0u : 1u));
    }
    for (uint32 stage : {9u, 10u}) {
        scenarios = {}; scenarios.scenario.step = stage;
        Creature actor; actor.multiplier = 1; actor.targetMaximum = actor.maximum;
        Unit attacker; npc_talonclaw_ambush_dakarr content(&actor); actor.primary = &content;
        assert(attacker.Deal(&actor, 10) == 10 && actor.health == 99990);
        assert(attacker.Deal(&actor, 99999) == 0 && actor.alive);
        uint64 floor = stage == 9 ? 70000 : 60000;
        assert(actor.health == floor);
        for (uint32 repeat = 0; repeat < 10; ++repeat) assert(attacker.Deal(&actor, 99999) == 0 && actor.health == floor);
        actor.combat = false; HealthProbe retry{&actor, stage}; retry.event.event.minMaxRepeat.max = stage == 9 ? 75 : 60;
        retry.Tick(); assert(!retry.starts); actor.combat = true; retry.Tick(); retry.Tick(); assert(retry.starts == 1);
    }
    {
        scenarios = {}; Creature actor; actor.targetMaximum = actor.maximum; actor.multiplier = 1;
        Unit attacker; npc_talonclaw_ambush_dakarr content(&actor); actor.primary = &content;
        content.mInvincibilityHpLevel = 70000;
        assert(attacker.Deal(&actor, 50000) == 0 && actor.health == 70000);
        scenarios.scenario.step = 10; content.mInvincibilityHpLevel = 0;
        assert(attacker.Deal(&actor, 50000) == 0 && actor.health == 70000 && actor.alive);
    }
    for (uint32 failure = 0; failure < 9; ++failure) {
        scenarios = {}; Creature actor; Unit attacker; uint32 damage = 2000, encounter = 0;
        switch (failure) {
            case 0: actor.entry = 99480; break;
            case 1: actor.map = 1495; break;
            case 2: actor.alive = false; break;
            case 3: scenarios.present = false; break;
            case 4: scenarios.scenario.identity = 1099; break;
            case 5: actor.instance = 43; break;
            case 6: damage = 0; break;
            case 7: scenarios.scenario.step = 8; break;
            case 8: scenarios.scenario.step = 11; break;
        }
        uint32 before = damage; TalonclawAmbush::Protect(&actor, &attacker, damage, encounter);
        assert(damage == before && actor.health == 100000 && encounter == 0);
    }
    scenarios = {}; Creature actor; Unit attacker; uint32 damage = 2000, encounter = 0;
    TalonclawAmbush::Protect(nullptr, &attacker, damage, encounter);
    TalonclawAmbush::Protect(&actor, nullptr, damage, encounter); assert(damage == 2000 && encounter == 0);
    actor.health = 50000; TalonclawAmbush::Protect(&actor, &attacker, damage, encounter);
    assert(damage == 0 && actor.health == 50000);
    std::cout << "PASS: extracted SmartAI/Unit post-scaling bridge and scoped ambush wrapper preserve separate native75/60 health events; player/pet/guardian/NPC, scales, suspended AI, one-hit/nonlethal/repeated hits, combat retry and stale first actor covered. Scenario advancement/health services are controlled, not client traversal or group-credit execution.\n";
}
