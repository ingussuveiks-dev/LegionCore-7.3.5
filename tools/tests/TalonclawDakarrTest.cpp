#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <functional>
#include <vector>
using uint32 = uint32_t;
enum ReactStates { REACT_PASSIVE, REACT_DEFENSIVE, REACT_AGGRESSIVE };
enum DamageEffectType { DIRECT_DAMAGE, SPELL_DIRECT_DAMAGE, DOT };
enum { UNIT_FIELD_FLAGS = 0, UNIT_FLAG_NON_ATTACKABLE = 2, CONDITION_SCENARION_STEP = 44, CLASS_HUNTER = 3,
    QUEST_STATUS_NONE = 0, QUEST_STATUS_COMPLETE = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_FAILED = 5, CRITERIA_TYPE_SCRIPT_EVENT_2 = 92,
    SMART_SCRIPT_TYPE_CREATURE = 0 };
uint32 recoveredLogs = 0, failedLogs = 0;
#define TC_LOG_INFO(...) (++recoveredLogs)
#define TC_LOG_ERROR(...) (++failedLogs)
struct InstanceMap { uint32 id = 42; uint32 GetInstanceId() const { return id; } } instanceMap;
struct Map {
    bool instanced = true;
    uint32 GetInstanceId() const { return 42; }
    InstanceMap* ToInstanceMap() { return instanced ? &instanceMap : nullptr; }
} worldMap;
struct Player;
struct Unit {
    virtual ~Unit() = default;
    Player* creditOwner = nullptr;
    Map* location = &worldMap;
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() { return creditOwner; }
    bool InInstance() const { return location->instanced; }
    Map* GetMap() const { return location; }
};
struct Player : Unit {
    bool inWorld = true, contact = true, gameMaster = false, teleporting = false, alive = true, corpse = false, rewarded = false, phased = true;
    uint32 playerClass = CLASS_HUNTER, quest = QUEST_STATUS_INCOMPLETE, criteria = 0;
    float distance = 0;
    Unit* actualInvoker = nullptr;
    Player() { creditOwner = this; }
    bool IsInWorld() const { return inWorld; }
    bool CanContact() const { return contact; }
    bool isGameMaster() const { return gameMaster; }
    bool IsBeingTeleported() const { return teleporting; }
    bool IsAlive() const { return alive; }
    bool GetCorpse() const { return corpse; }
    uint32 getClass() const { return playerClass; }
    uint32 GetGUIDLow() const { return 123; }
    bool GetQuestRewardStatus(uint32 id) const { assert(id == 40385); return rewarded; }
    uint32 GetQuestStatus(uint32 id) const { assert(id == 40385); return quest; }
    void UpdateAchievementCriteria(uint32, uint32, uint32, uint32, Unit*, bool);
};
struct Creature : Unit {
    uint32 mapId = 1511, instance = 42, flags = 32768, stops = 0;
    ReactStates reaction = REACT_AGGRESSIVE;
    bool combat = false;
    Player* lootRecipient = nullptr;
    bool InSamePhase(Player* player) const { return player->phased; }
    bool IsWithinDistInMap(Player* player, float range) const { return GetMap() == player->GetMap() && player->distance <= range; }
    float GetDistance(Player* player) const { return player->distance; }
    Player* GetLootRecipient() const { return lootRecipient; }
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
struct Scenario {
    uint32 id = 973, step = 0, forwarded = 0;
    bool accepts = true;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
    void UpdateAchievementCriteria(int) { ++forwarded; if (step == 11 && accepts) step = 12; }
};
struct ScenarioManager { Scenario scenario; bool present = true; Scenario* GetScenario(uint32 id) { return present && id == 42 ? &scenario : nullptr; } } scenarioManager;
auto* sScenarioMgr = &scenarioManager;
void Player::UpdateAchievementCriteria(uint32 type, uint32 asset, uint32 second, uint32 third, Unit* invoker, bool ignoreGroup) {
    assert(type == 92 && asset == 47688 && second == 0 && third == 0 && ignoreGroup);
    ++criteria; actualInvoker = invoker;
    Map* map = GetMap();
    int referenceCache = 0;
#include "PlayerScenario.inc"
}
struct NearestPlayerInObjectRangeCheck {
    Creature const* i_obj;
    float i_range;
    NearestPlayerInObjectRangeCheck(Creature const* actor, float range) : i_obj(actor), i_range(range) {}
    bool operator()(Player* player);
};
#include "NearestPlayer.inc"
struct SmartAI {
    Creature* me;
    uint32 initializes = 0, resets = 0, updates = 0, attacks = 0, damages = 0;
    Unit* lastTarget = nullptr;
    DamageEffectType lastType = DIRECT_DAMAGE;
    uint32 deaths = 0;
    std::function<void(Unit*)> deathAction;
    explicit SmartAI(Creature* actor) : me(actor) {}
    virtual ~SmartAI() = default;
    virtual void InitializeAI() { ++initializes; Reset(); }
    virtual void Reset() { ++resets; }
    virtual void UpdateAI(uint32 diff) { assert(diff == 100); ++updates; }
    virtual void AttackStart(Unit* target) { ++attacks; lastTarget = target; }
    virtual void DamageTaken(Unit* attacker, uint32&, DamageEffectType type) { ++damages; lastTarget = attacker; lastType = type; }
    virtual void JustDied(Unit* killer) { ++deaths; lastTarget = killer; if (deathAction) deathAction(killer); }
};
struct ConditionManager {
    bool allowed = true;
    Unit* lastInvoker = nullptr;
    Creature* lastActor = nullptr;
    bool IsObjectMeetingSmartEventConditions(uint32 entry, uint32 eventId, uint32 source, Unit* invoker, Creature* actor) {
        assert(entry == 99480 && eventId == 1 && source == 0);
        lastInvoker = invoker; lastActor = actor; return allowed;
    }
} conditionManager;
auto* sConditionMgr = &conditionManager;
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
    Unit admissionPlayer, admissionPet, admissionGuardian;
    std::vector<Unit*> attackers{&admissionPlayer, &admissionPet, &admissionGuardian, nullptr};
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
        ai.DamageTaken(&admissionPet, damage, SPELL_DIRECT_DAMAGE); ai.AttackStart(&admissionGuardian); ai.UpdateAI(100);
        bool blocked = context >= 2;
        assert(damage == (blocked ? 0u : UINT32_MAX));
        assert(ai.damages == uint32(!blocked) && ai.attacks == uint32(!blocked) && ai.updates == 1);
    }
    {
        scenarioManager = {}; Creature actor; npc_talonclaw_dakarr ai(&actor); ai.InitializeAI();
        scenarioManager.scenario.id = 944; ai.UpdateAI(100);
        assert(actor.flags == 32768 && actor.reaction == REACT_AGGRESSIVE);
        worldMap.instanced = false; assert(!SmartContext{&actor}.Check(&admissionPet, condition)); worldMap.instanced = true;
        scenarioManager.scenario.id = 973; scenarioManager.scenario.step = 11;
        scenarioManager.present = false; assert(!SmartContext{&actor}.Check(nullptr, condition));
    }
    for (uint32 killerType = 0; killerType < 5; ++killerType) {
        scenarioManager = {}; scenarioManager.scenario.step = 11;
        Player owner; owner.alive = false; owner.distance = 49;
        Unit pet, guardian, npc; pet.creditOwner = guardian.creditOwner = &owner;
        Creature actor; npc_talonclaw_dakarr ai(&actor);
        Unit* killer = killerType == 0 ? &owner : killerType == 1 ? &pet : killerType == 2 ? &guardian : killerType == 3 ? &npc : nullptr;
        if (killerType >= 3) actor.lootRecipient = &owner;
        assert(!NearestPlayerInObjectRangeCheck(&actor, 50)(&owner));
        ai.JustDied(killer);
        assert(ai.deaths == 1 && ai.lastTarget == killer && owner.criteria == 1 && owner.actualInvoker == killer);
        assert(conditionManager.lastInvoker == killer && conditionManager.lastActor == &actor);
        assert(scenarioManager.scenario.step == 12 && scenarioManager.scenario.forwarded == 1);
        ai.JustDied(killer);
        assert(owner.criteria == 1 && scenarioManager.scenario.forwarded == 1);
    }
    {
        scenarioManager = {}; scenarioManager.scenario.step = 11;
        Player owner, nearest; Creature actor; npc_talonclaw_dakarr ai(&actor);
        Unit pet; pet.creditOwner = &owner;
        ai.deathAction = [&](Unit* killer) {
            assert(killer == &pet);
            nearest.UpdateAchievementCriteria(92, 47688, 0, 0, killer, true);
        };
        ai.JustDied(&pet);
        assert(nearest.criteria == 1 && owner.criteria == 0 && scenarioManager.scenario.forwarded == 1);
    }
    for (uint32 invalid = 0; invalid < 20; ++invalid) {
        scenarioManager = {}; scenarioManager.scenario.step = 11;
        Player owner; Creature actor; npc_talonclaw_dakarr ai(&actor); Unit pet;
        pet.creditOwner = &owner; owner.alive = false; Map elsewhere;
        if (invalid == 0) owner.inWorld = false;
        if (invalid == 1) owner.teleporting = true;
        if (invalid == 2) owner.playerClass = 1;
        if (invalid == 3) owner.rewarded = true;
        if (invalid == 4) owner.quest = QUEST_STATUS_NONE;
        if (invalid == 5) owner.corpse = true;
        if (invalid == 6) owner.location = &elsewhere;
        if (invalid == 7) owner.phased = false;
        if (invalid == 8) owner.distance = 50.1f;
        if (invalid == 9) pet.creditOwner = nullptr;
        if (invalid == 10) actor.mapId = 1;
        if (invalid == 11) scenarioManager.scenario.id = 944;
        if (invalid == 12) scenarioManager.scenario.step = 10;
        if (invalid == 13) scenarioManager.scenario.step = 12;
        if (invalid == 14) scenarioManager.present = false;
        if (invalid == 15) actor.instance = 43;
        if (invalid == 16) owner.quest = QUEST_STATUS_FAILED;
        if (invalid == 17) owner.gameMaster = true;
        if (invalid == 18) owner.contact = false;
        if (invalid == 19) conditionManager.allowed = false;
        ai.JustDied(&pet);
        assert(ai.deaths == 1 && owner.criteria == 0 && scenarioManager.scenario.forwarded == 0);
        conditionManager.allowed = true;
    }
    {
        scenarioManager = {}; scenarioManager.scenario.step = 11;
        Player owner, looter; owner.alive = looter.alive = false;
        Creature actor; actor.lootRecipient = &looter;
        Unit guardian; guardian.creditOwner = &owner;
        npc_talonclaw_dakarr ai(&actor); ai.JustDied(&guardian);
        assert(looter.criteria == 1 && owner.criteria == 0 && looter.actualInvoker == &guardian);
        scenarioManager.scenario.step = 11; looter.rewarded = true;
        ai.JustDied(&guardian); assert(owner.criteria == 1 && looter.criteria == 1);
        npc_talonclaw_dakarr missing(nullptr); missing.JustDied(&guardian);
        assert(owner.criteria == 1 && looter.criteria == 1 && missing.deaths == 1);
    }
    {
        scenarioManager = {}; scenarioManager.scenario.step = 11; scenarioManager.scenario.accepts = false;
        Player owner; Creature actor; npc_talonclaw_dakarr ai(&actor);
        uint32 previousLogs = recoveredLogs;
        ai.JustDied(&owner);
        assert(owner.criteria == 1 && scenarioManager.scenario.step == 11 && failedLogs == 1 && recoveredLogs == previousLogs);
    }
    for (uint32 quest : {uint32(QUEST_STATUS_INCOMPLETE), uint32(QUEST_STATUS_COMPLETE)}) {
        scenarioManager = {}; scenarioManager.scenario.step = 11;
        Player owner; owner.quest = quest; owner.distance = 50; owner.corpse = true;
        Creature actor; npc_talonclaw_dakarr ai(&actor);
        ai.JustDied(&owner); assert(owner.criteria == 1 && scenarioManager.scenario.step == 12);
    }
    std::cout << "PASS: production Dakarr admission/delegates and native condition44/base-actor context; every stage, lethal/zero/direct/spell/DoT, player/pet/guardian/null, retry/reset, missing scenario and unrelated map/scenario preservation\n";
    std::cout << "PASS: native nearest-player predicate reproduces dead-owner exclusion; post-native death fallback preserves raw killer, loot priority, same-map/phase/50yd/body/quest admission and shared instance forwarding without duplicate or unrelated credit\n";
}
