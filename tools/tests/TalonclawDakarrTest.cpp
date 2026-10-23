#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
using uint32 = uint32_t;
enum ReactStates { REACT_PASSIVE, REACT_DEFENSIVE, REACT_AGGRESSIVE };
enum DamageEffectType { DIRECT_DAMAGE, SPELL_DIRECT_DAMAGE, DOT };
enum { UNIT_FIELD_FLAGS = 0, UNIT_FLAG_NON_ATTACKABLE = 2, CONDITION_SCENARION_STEP = 44 };
struct InstanceMap { uint32 id = 42; uint32 GetInstanceId() const { return id; } } instanceMap;
struct Map {
    bool instanced = true;
    InstanceMap* ToInstanceMap() { return instanced ? &instanceMap : nullptr; }
} map;
struct Unit {
    virtual ~Unit() = default;
    bool InInstance() const { return map.instanced; }
    Map* GetMap() const { return &map; }
};
struct Creature : Unit {
    uint32 mapId = 1511, instance = 42, flags = 32768, stops = 0;
    ReactStates reaction = REACT_AGGRESSIVE;
    bool combat = false;
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    bool HasFlag(uint32 field, uint32 mask) const { assert(field == 0); return (flags & mask) != 0; }
    void SetFlag(uint32 field, uint32 mask) { assert(field == 0); flags |= mask; }
    void RemoveFlag(uint32 field, uint32 mask) { assert(field == 0); flags &= ~mask; }
    ReactStates GetReactState() const { return reaction; }
    void SetReactState(ReactStates value) { reaction = value; }
    bool isInCombat() const { return combat; }
    void CombatStop(bool interrupt) { assert(interrupt); combat = false; ++stops; }
};
struct Scenario { uint32 id = 973, step = 0; uint32 GetScenarioId() const { return id; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioManager { Scenario scenario; bool present = true; Scenario* GetScenario(uint32 id) { return present && id == 42 ? &scenario : nullptr; } } scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct SmartAI {
    Creature* me;
    uint32 initializes = 0, resets = 0, updates = 0, attacks = 0, damages = 0;
    Unit* lastTarget = nullptr;
    DamageEffectType lastType = DIRECT_DAMAGE;
    explicit SmartAI(Creature* actor) : me(actor) {}
    virtual ~SmartAI() = default;
    virtual void InitializeAI() { ++initializes; Reset(); }
    virtual void Reset() { ++resets; }
    virtual void UpdateAI(uint32 diff) { assert(diff == 100); ++updates; }
    virtual void AttackStart(Unit* target) { ++attacks; lastTarget = target; }
    virtual void DamageTaken(Unit* attacker, uint32&, DamageEffectType type) { ++damages; lastTarget = attacker; lastType = type; }
};
#include "Dakarr.inc"
struct ConditionSourceInfo {
    Unit* targets[2];
    ConditionSourceInfo(Unit* invoker, Unit* base) : targets{invoker, base} {}
};
struct Condition {
    uint32 ConditionTypeOrReference = 44, ConditionTarget = 1, ConditionValue1 = 973, ConditionValue2 = 11;
    bool Check(Unit* object) const {
        if (!object) return false;
        bool condMeets = false;
        switch (ConditionTypeOrReference) {
#include "Condition.inc"
            default: assert(false);
        }
        return condMeets;
    }
};
struct SmartContext {
    Creature* base;
    Unit* GetBaseObject() const { return base; }
    bool Check(Unit* unit, Condition const& condition) const {
#include "SourceInfo.inc"
        return condition.Check(info.targets[condition.ConditionTarget]);
    }
};
int main(int argc, char** argv) {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 2);
    std::ifstream input(argv[1]); Condition condition;
    assert(input >> condition.ConditionTypeOrReference >> condition.ConditionTarget >> condition.ConditionValue1 >> condition.ConditionValue2);
    Unit player, pet, guardian;
    std::vector<Unit*> attackers{&player, &pet, &guardian, nullptr};
    {
        TalonclawDakarr::Gate gate; assert(gate.Update(nullptr));
    }
    for (uint32 stage = 0; stage <= 13; ++stage) {
        scenarioManager = {}; scenarioManager.scenario.step = stage;
        Creature actor; npc_talonclaw_dakarr ai(&actor); ai.InitializeAI();
        assert(ai.initializes == 1 && ai.resets == 1);
        bool blocked = stage != 11;
        assert(actor.HasFlag(0, 2) == blocked && actor.reaction == (blocked ? REACT_PASSIVE : REACT_AGGRESSIVE));
        ai.UpdateAI(100); assert(ai.updates == 1);
        for (Unit* attacker : attackers) {
            ai.AttackStart(attacker);
            for (auto type : {DIRECT_DAMAGE, SPELL_DIRECT_DAMAGE, DOT})
                for (uint32 incoming : {0u, 1u, 100u, UINT32_MAX}) {
                    uint32 damage = incoming; uint32 previous = ai.damages;
                    ai.DamageTaken(attacker, damage, type);
                    assert(damage == (blocked ? 0u : incoming));
                    assert(ai.damages == previous + uint32(!blocked));
                    if (!blocked) assert(ai.lastTarget == attacker && ai.lastType == type);
                }
            SmartContext context{&actor}; assert(context.Check(attacker, condition) == !blocked);
        }
        assert(ai.attacks == (blocked ? 0u : 4u));
    }
    {
        scenarioManager = {}; Creature actor; actor.reaction = REACT_DEFENSIVE; actor.combat = true;
        npc_talonclaw_dakarr ai(&actor); ai.InitializeAI(); ai.Reset(); ai.UpdateAI(100);
        assert(actor.stops == 1 && actor.reaction == REACT_PASSIVE && ai.updates == 1);
        actor.flags |= 1024;
        scenarioManager.scenario.step = 11; ai.UpdateAI(100);
        assert(actor.flags == (32768u | 1024u) && actor.reaction == REACT_DEFENSIVE && ai.updates == 2);
        actor.combat = true; scenarioManager.scenario.step = 12; ai.UpdateAI(100);
        assert(actor.stops == 2 && actor.reaction == REACT_PASSIVE && ai.updates == 3);
        scenarioManager.scenario.step = 11; ai.Reset(); ai.UpdateAI(100);
        assert(actor.flags == (32768u | 1024u) && actor.reaction == REACT_DEFENSIVE && ai.updates == 4);
    }
    {
        scenarioManager = {}; Creature actor; actor.flags |= 2;
        npc_talonclaw_dakarr ai(&actor); ai.InitializeAI(); scenarioManager.scenario.step = 11; ai.UpdateAI(100);
        assert(actor.HasFlag(0, 2) && actor.reaction == REACT_AGGRESSIVE);
    }
    for (uint32 context = 0; context < 4; ++context) {
        scenarioManager = {}; Creature actor;
        if (context == 0) actor.mapId = 1;
        if (context == 1) scenarioManager.scenario.id = 944;
        if (context == 2) scenarioManager.present = false;
        if (context == 3) actor.instance = 43;
        npc_talonclaw_dakarr ai(&actor); ai.InitializeAI(); uint32 damage = UINT32_MAX;
        ai.DamageTaken(&pet, damage, SPELL_DIRECT_DAMAGE); ai.AttackStart(&guardian); ai.UpdateAI(100);
        bool blocked = context >= 2;
        assert(damage == (blocked ? 0u : UINT32_MAX));
        assert(ai.damages == uint32(!blocked) && ai.attacks == uint32(!blocked) && ai.updates == 1);
    }
    {
        scenarioManager = {}; Creature actor; npc_talonclaw_dakarr ai(&actor); ai.InitializeAI();
        scenarioManager.scenario.id = 944; ai.UpdateAI(100);
        assert(actor.flags == 32768 && actor.reaction == REACT_AGGRESSIVE);
        map.instanced = false; assert(!SmartContext{&actor}.Check(&pet, condition)); map.instanced = true;
        scenarioManager.scenario.id = 973; scenarioManager.scenario.step = 11;
        scenarioManager.present = false; assert(!SmartContext{&actor}.Check(nullptr, condition));
    }
    std::cout << "PASS: production Dakarr admission/delegates and native condition44/base-actor context; every stage, lethal/zero/direct/spell/DoT, player/pet/guardian/null, retry/reset, missing scenario and unrelated map/scenario preservation\n";
}
