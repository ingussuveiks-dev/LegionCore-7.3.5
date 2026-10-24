#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32 = uint32_t;
enum { CONFIG_THREAT_RADIUS = 0 };
struct Unit;
using WorldObject = Unit;
struct Creature;
struct Player;
using ObjectGuid = uint32;
struct VisibilityUpdateTask;
struct VisibilityEvents
{
    std::vector<VisibilityUpdateTask*> pending;
    uint32 CalculateTime(uint32 delay) const { return delay; }
    void AddEvent(VisibilityUpdateTask*, uint32);
};
struct MotionMaster
{
    uint32 chases = 0;
    void MoveChase(Unit*, float = 0) { ++chases; }
};
struct Unit
{
    bool inMap = true, accessible = true, inRange = true, world = true, alive = true;
    uint32 visibilityRequests = 0;
    uint32 notifications = 0;
    bool observedVisible = false;
    VisibilityEvents m_Events;
    virtual ~Unit() = default;
    bool IsInWorld() const { return world; }
    float GetVisibilityRange() const { return 100; }
    void UpdateObjectVisibility(bool = true);
#include "BaseVisibility.inc"
    bool IsInMap(Creature const*) const { return inMap; }
    bool isInAccessiblePlaceFor(Creature const*) const { return accessible; }
    bool IsWithinDist(Unit*, float) const { return inRange; }
    bool IsInDist(uint32 const*, float) const { return inRange; }
};
struct CreatureAI
{
    virtual ~CreatureAI() = default;
    virtual bool CanAIAttack(Unit const*) const { return true; }
#include "BaseAIVisibility.inc"
    virtual void AttackStart(Unit*) = 0;
};
struct Creature : Unit
{
    uint32 entry = 98602, mapId = 1511, instance = 42, attacks = 0, m_homePosition = 0;
    ObjectGuid guid = 1;
    Player* nearby = nullptr;
    float m_CombatDistance = 0;
    bool IsAIEnabled = true, valid = true, attackWorks = true, casterPet = false, melee = false;
    CreatureAI* ai = nullptr;
    CreatureAI* i_AI = nullptr;
    CreatureAI* i_disabledAI = nullptr;
    MotionMaster motion;
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    ObjectGuid GetGUID() const { return guid; }
    Player* FindNearestPlayer(float distance, bool liveOnly) { assert(distance == 100 && liveOnly); return nearby; }
    bool IsNeverVisible(WorldObject const* = nullptr) const override;
    bool IsValidAttackTarget(Unit const*) const { return valid; }
    float GetAttackDistance(Unit const*) const { return 10; }
    Unit* GetCharmerOrOwner() const { return nullptr; }
    CreatureAI* AI() const { return ai; }
    bool canCreatureAttack(Unit const*, bool = false) const;
    bool Attack(Unit*, bool) { ++attacks; return attackWorks; }
    bool GetCasterPet() const { return casterPet; }
    bool IsWithinMeleeRange(Unit*, float) const { return melee; }
    float GetAttackDist() const { return 10; }
    MotionMaster* GetMotionMaster() { return &motion; }
};
struct MapEntry { bool dungeon = true; bool IsDungeon() const { return dungeon; } };
struct MapStore { MapEntry entry; MapEntry const* LookupEntry(uint32) const { return &entry; } } sMapStore;
struct World { float getFloatConfig(uint32) const { return 20; } uint32 GetVisibilityAINotifyDelay() const { return 100; } } world;
auto* sWorld = &world;
#include "CreatureAttack.inc"
#include "NativeVisibility.inc"
struct ScriptedAI : CreatureAI
{
    Creature* me;
    bool movementAllowed = true;
    uint32 stationary = 0;
    explicit ScriptedAI(Creature* actor) : me(actor) { actor->ai = actor->i_AI = this; }
    bool IsCombatMovementAllowed() const { return movementAllowed; }
    void DoStartNoMovement(Unit*) { ++stationary; }
    void AttackStartNoMove(Unit*);
    void AttackStart(Unit*) override;
};
#include "NativeAttackStart.inc"
#include "NativeAttackStartNoMove.inc"
struct Scenario { uint32 identity = 944, step = 5; uint32 GetScenarioId() const { return identity; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioMgr
{
    bool available = true;
    Scenario scenario;
    Scenario* GetScenario(uint32 instance) { return available && instance == 42 ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct FinalAI : ScriptedAI
{
    explicit FinalAI(Creature* actor) : ScriptedAI(actor) { }
#include "EncounterReady.inc"
#include "CanAIAttack.inc"
#include "AttackStart.inc"
#include "Visibility.inc"
};
int main()
{
#ifdef BASELINE
    Creature actor; FinalAI ai(&actor); Unit target;
    scenarioManager.scenario.step = 0;
    assert(actor.canCreatureAttack(&target));
    ai.AttackStart(&target);
    assert(actor.attacks == 1 && actor.motion.chases == 1);
    std::cout << "BASELINE: pinned14774c59 final boss admits native target selection/direct scripted engagement at stage0. Controlled combat services, not live client.\n";
#else
    uint32 cases = 0;
    for (uint32 step = 0; step < 9; ++step) for (bool movement : {false, true})
        for (bool attackWorks : {false, true}) for (bool casterPet : {false, true})
    {
        Creature actor; FinalAI ai(&actor); Unit target;
        scenarioManager = {}; scenarioManager.scenario.step = step;
        ai.movementAllowed = movement; actor.attackWorks = attackWorks; actor.casterPet = casterPet;
        assert(ai.CanAIAttack(&target) == (step == 5));
        assert(actor.canCreatureAttack(&target) == (step == 5));
        ai.AttackStart(&target);
        assert(actor.attacks == uint32(step == 5));
        assert(actor.motion.chases == uint32(step == 5 && movement && attackWorks));
        assert(ai.stationary == uint32(step == 5 && !movement && attackWorks));
        ++cases;
    }
    for (uint32 invalid = 0; invalid < 3; ++invalid)
    {
        Creature actor; FinalAI ai(&actor); Unit target;
        scenarioManager = {};
        if (invalid == 0) scenarioManager.available = false;
        if (invalid == 1) actor.instance = 99;
        if (invalid == 2) scenarioManager.scenario.identity = 973;
        assert(!actor.canCreatureAttack(&target)); ai.AttackStart(&target);
        assert(!actor.attacks && !actor.motion.chases);
    }
    for (uint32 context = 0; context < 2; ++context)
    {
        Creature actor; FinalAI ai(&actor); Unit target;
        scenarioManager = {}; scenarioManager.available = false;
        if (context == 0) actor.entry = 98007;
        else actor.mapId = 1220;
        assert(actor.canCreatureAttack(&target)); ai.AttackStart(&target);
        assert(actor.attacks == 1 && actor.motion.chases == 1);
    }
    for (uint32 rejected = 0; rejected < 4; ++rejected)
    {
        Creature actor; FinalAI ai(&actor); Unit target; scenarioManager = {};
        if (rejected == 0) target.inMap = false;
        if (rejected == 1) actor.world = false;
        if (rejected == 2) actor.valid = false;
        if (rejected == 3) target.accessible = false;
        assert(!actor.canCreatureAttack(&target));
    }
    {
        Creature actor; FinalAI ai(&actor); Unit target; scenarioManager = {};
        scenarioManager.scenario.step = 4; ai.AttackStart(&target); assert(!actor.attacks);
        scenarioManager.scenario.step = 5; ai.AttackStart(nullptr); assert(!actor.attacks);
        ai.AttackStart(&target); assert(actor.attacks == 1);
        scenarioManager.scenario.step = 6; ai.AttackStart(&target); assert(actor.attacks == 1);
        scenarioManager.scenario.step = 5; ai.AttackStart(&target); assert(actor.attacks == 2);
    }
    std::cout << "PASS: " << cases << " source-extracted native selection and scripted moving/stationary/caster-pet/failed-attack paths; exact live1511/98602/944/5 admission, missing scenario, transitions/retry, null target and unrelated reuse retained. No visibility/client certification.\n";
#endif
}
