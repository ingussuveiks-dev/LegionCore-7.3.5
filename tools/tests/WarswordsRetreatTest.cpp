#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <list>
#include <vector>

using uint8 = uint8_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int32 = int32_t;
using DamageEffectType = uint32;
#include "CriteriaTypes.inc"
#include "CalculatePct.inc"
enum { CLASS_WARRIOR = 1, POINT_MOTION_TYPE = 8, REACT_PASSIVE = 0, REACT_AGGRESSIVE = 2,
    UNIT_FIELD_FLAGS = 0, UNIT_FLAG_IMMUNE_TO_PC = 0x100, UNIT_FLAG_IMMUNE_TO_NPC = 0x200, UNIT_FLAG_NOT_ATTACKABLE_1 = 0x80,
    QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1, QUEST_STATUS_REWARDED = 6,
    UNIT_MOD_HEALTH = 0, TOTAL_PCT = 0 };
struct Map { } instanceMap, unrelatedMap;
struct Unit;
using WorldObject = Unit;
struct Creature;
struct Player;
struct ObjectGuid { Unit* unit = nullptr; bool IsPlayer() const; };
struct Unit
{
    virtual ~Unit() = default;
    Unit* owner = nullptr;
    Unit* charmer = nullptr;
    bool available = true, alive = true;
    uint64 health = 100000, maximum = 100000;
    virtual Player* ToPlayer() { return nullptr; }
    virtual Player const* ToPlayer() const { return nullptr; }
    virtual Creature* ToCreature() { return nullptr; }
    virtual Creature const* ToCreature() const { return nullptr; }
    ObjectGuid GetCharmerOrOwnerGUID() const { return {charmer ? charmer : owner}; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
    uint64 GetHealth() const { return health; }
    uint64 GetHealth(Unit* victim) const;
    uint64 GetMaxHealth() const { return maximum; }
    uint64 GetMaxHealth(Unit* victim) const;
    float GetHealthPct() const { return maximum ? 100.f * health / maximum : 0.f; }
    bool HealthBelowPct(int32 percent) const { return GetHealth() < CountPctFromMaxHealth(percent); }
    uint64 CountPctFromMaxHealth(int32 percent, Unit* victim = nullptr) const { return CalculatePct(GetMaxHealth(victim), percent); }
    bool IsAlive() const { return alive; }
    uint8 GetLevelForTarget(Unit const*) const { return 100; }
    float GetModifierValue(uint32, uint32) const { return 1.f; }
    void SetHealth(uint64 value, uint32 = 0) { health = value; }
    void SetHealthScal(uint64 value, Unit* victim, uint32 spell = 0);
    uint32 Deal(Creature* victim, uint32 damage);
};
struct Scenario
{
    uint32 identity = 944, step = 4;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioMgr
{
    Scenario scenario;
    bool available = true;
    Scenario* GetScenario(uint32 identity) { return available && identity == 42 ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct Player : Unit
{
    uint32 credits = 0, status = QUEST_STATUS_INCOMPLETE, playerClass = CLASS_WARRIOR;
    bool contact = true, phase = true, rewarded = false, alliance = false;
    float distance = 1.f;
    Map* map = &instanceMap;
    Player* ToPlayer() override { return this; }
    Player const* ToPlayer() const override { return this; }
    bool CanContact() { return contact; }
    uint32 getClass() const { return playerClass; }
    Map* GetMap() const { return map; }
    bool InSamePhase(Creature*) const { return phase; }
    bool IsWithinDistInMap(Creature*, float range) const { assert(range == 100.f); return distance <= range; }
    bool GetQuestRewardStatus(uint32 identity) const { assert(identity == 40043); return rewarded; }
    uint32 GetQuestStatus(uint32 identity) const { assert(identity == 40043); return status; }
    void UpdateAchievementCriteria(CriteriaTypes type, uint32 identity)
    {
        assert(type == CRITERIA_TYPE_SCRIPT_EVENT_2 && identity == 46982);
        ++credits;
        scenarioManager.scenario.step = 5;
    }
};
bool ObjectGuid::IsPlayer() const { return unit && unit->ToPlayer(); }
struct ObjectAccessor
{
    static Player* GetPlayer(Unit const&, ObjectGuid guid) { return guid.unit && guid.unit->available ? guid.unit->ToPlayer() : nullptr; }
};
#include "OwnerPlayer.inc"
struct CreatureAI
{
    virtual ~CreatureAI() = default;
    virtual void Reset() { }
    virtual void DamageTaken(Unit*, uint32&, DamageEffectType) { }
    virtual void DamageTakenAfterScaling(Unit*, uint32&, DamageEffectType) { }
    virtual void MovementInform(uint32, uint32) { }
    Unit* attacked = nullptr;
    void AttackStart(Unit* target) { attacked = target; }
};
struct EventMap { uint32 resets = 0; void Reset() { ++resets; } };
struct SummonList { uint32 resets = 0; void DespawnAll() { ++resets; } };
struct MotionMaster
{
    std::vector<uint32> points;
    void MovePoint(uint32 identity, float positionX, float positionY, float positionZ)
    {
        if (identity == 1)
            assert(std::abs(positionX - 3444.66f) < .001f && std::abs(positionY - 1703.69f) < .001f && std::abs(positionZ - .43f) < .001f);
        else
            assert(identity == 2 && std::abs(positionX - 3514.32f) < .001f && std::abs(positionY - 1800.86f) < .001f && std::abs(positionZ - .6f) < .001f);
        points.push_back(identity);
    }
};
struct CreatureLevelStat { uint64 healthMax = 1000; };
struct InstanceScript { void OnCreatureDamageTaken(Creature*, Unit*, uint32&) { } };
struct Creature : Unit
{
    uint32 entry = 98007, mapId = 1511, instance = 42, flags = 0, react = REACT_AGGRESSIVE, stops = 0, despawns = 0, casts = 0, removed = 0;
    bool scalable = true, statsAvailable = true, castWorks = true;
    CreatureLevelStat stat;
    CreatureAI* active = nullptr;
    CreatureAI* i_disabledAI = nullptr;
    MotionMaster motion;
    Player* nearby = nullptr;
    Creature* ToCreature() override { return this; }
    Creature const* ToCreature() const override { return this; }
    bool HasScalableLevels() const { return scalable; }
    CreatureLevelStat const* GetScaleLevelStat(uint8) { return statsAvailable ? &stat : nullptr; }
    uint64 GetMaxHealthByLevel(uint8) const { return stat.healthMax; }
    uint64 GetCreateHealth() const { return maximum; }
    uint8 getLevel() const { return 110; }
    float GetHealthMultiplierForTarget(Unit const* target) const;
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    Map* GetMap() const { return &instanceMap; }
    CreatureAI* AI() const { return active; }
    InstanceScript* GetInstanceScript() { return nullptr; }
    MotionMaster* GetMotionMaster() { return &motion; }
    void SetFlag(uint32, uint32 mask) { flags |= mask; }
    void RemoveFlag(uint32, uint32 mask) { flags &= ~mask; }
    void SetReactState(uint32 state) { react = state; }
    void StopAttack() { ++stops; }
    void RemoveAura(uint32 identity) { assert(identity == 128264 || identity == 195996); if (identity == 128264) ++removed; }
    void DespawnOrUnsummon(uint32 delay) { assert(delay == 7000); ++despawns; }
    Player* FindNearestPlayer(float range) { assert(range == 100.f); return nearby; }
};
#include "Health.inc"
#include "MaxHealth.inc"
#include "Multiplier.inc"
#include "SetHealthScal.inc"
std::vector<Creature*> ambushers;
void GetCreatureListWithEntryInGrid(std::list<Creature*>& result, Creature*, uint32 identity, float range)
{
    assert(identity == 98795 && range == 30.f);
    result.assign(ambushers.begin(), ambushers.end());
}
struct FirstBoss : CreatureAI
{
    Creature* me;
    explicit FirstBoss(Creature* actor) : me(actor) { }
    bool phase = false, check_first_rephase = false;
    bool secondRetreatStarted = false;
    uint32 count_adds = 0;
    EventMap events;
    SummonList summons;
#include "SpectreState.inc"
    std::vector<uint32> texts;
    void Talk(uint32 identity) { texts.push_back(identity); }
    void DoCast(uint32 identity)
    {
        assert(identity == 128264);
        if (me->castWorks)
            ++me->casts;
        MovementInform(POINT_MOTION_TYPE, 1);
    }
#include "Reset.inc"
#include "DamageTaken.inc"
#include "AfterScaling.inc"
#include "MovementInform.inc"
};
uint32 Unit::Deal(Creature* victim, uint32 damage)
{
    DamageEffectType damagetype = 0;
    if (victim->AI())
        victim->AI()->DamageTaken(this, damage, damagetype);
    uint64 health = victim->GetHealth(this);
#include "NativeScaledHook.inc"
    if (health <= damage)
        victim->alive = false;
    else
        victim->SetHealthScal(health - damage, this);
    return damage;
}

int main()
{
    for (bool alliance : {false, true})
        for (uint32 kind : {0u, 1u, 2u, 3u})
            for (bool suspended : {false, true})
                for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE), uint32(QUEST_STATUS_COMPLETE)})
                    for (bool castWorks : {false, true})
            {
                scenarioManager = {};
                Player player, unrelated;
                player.alliance = alliance;
                player.status = status;
                Unit pet, guardian, charmed;
                pet.owner = guardian.owner = &player;
                charmed.owner = &unrelated;
                charmed.charmer = &player;
                Unit* attacker = kind == 0 ? static_cast<Unit*>(&player) : kind == 1 ? &pet : kind == 2 ? &guardian : &charmed;
                Creature actor, add;
                CreatureAI temporary, addAI;
                add.active = &addAI;
                ambushers = {&add};
                actor.nearby = &unrelated;
                actor.castWorks = castWorks;
                FirstBoss boss(&actor);
                boss.Reset();
#ifndef BASELINE
                boss.spectreActive = true;
                boss.spectreDefeated[0] = true;
                boss.spectres[0] = {&add};
                boss.count_adds = 1;
                uint32 generation = boss.spectreGeneration;
#endif
                actor.active = suspended ? &temporary : &boss;
                actor.i_disabledAI = suspended ? &boss : nullptr;
                uint32 damage = attacker->Deal(&actor, 200000);
#ifdef BASELINE
                assert(!actor.alive && !boss.phase && player.credits == 0 && boss.texts.empty());
                continue;
#else
                assert(damage == 0 && actor.alive && actor.health == 50000 && boss.phase && player.credits == 1 && unrelated.credits == 0);
                assert(!boss.spectreActive && boss.spectreGeneration > generation && boss.count_adds == 0);
                assert(!boss.spectres[0].unit && !boss.spectres[1].unit && !boss.spectreDefeated[0] && !boss.spectreDefeated[1]);
                assert(boss.summons.resets == 2);
                assert(boss.texts == std::vector<uint32>{5} && actor.motion.points == std::vector<uint32>{1});
                assert(actor.react == REACT_PASSIVE && actor.stops == 1 && boss.events.resets == 2);
                assert((actor.flags & (UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC)) != 0);
                for (uint32 repeat = 0; repeat < 8; ++repeat)
                    assert(attacker->Deal(&actor, 200000) == 0 && actor.alive && player.credits == 1);
                boss.MovementInform(POINT_MOTION_TYPE + 1, 1);
                assert(!boss.check_first_rephase && actor.casts == 0);
                boss.MovementInform(POINT_MOTION_TYPE, 1);
                assert(boss.check_first_rephase && actor.casts == uint32(castWorks));
                boss.MovementInform(POINT_MOTION_TYPE, 1);
                assert(actor.casts == uint32(castWorks));
                actor.active = &boss;
                actor.i_disabledAI = nullptr;
                assert(attacker->Deal(&actor, 200000) == 0 && actor.alive && actor.health == 50000);
                assert(boss.texts == (std::vector<uint32>{5, 7}) && actor.motion.points == (std::vector<uint32>{1, 2}));
                assert(actor.removed == 1 && addAI.attacked == attacker && actor.despawns == 1 && player.credits == 1);
                boss.MovementInform(POINT_MOTION_TYPE, 1);
                attacker->Deal(&actor, 200000);
                assert(actor.despawns == 1 && actor.motion.points.size() == 2);
                boss.Reset();
                assert(!boss.phase && !boss.check_first_rephase && !boss.secondRetreatStarted && actor.react == REACT_AGGRESSIVE && !actor.flags);
                boss.MovementInform(POINT_MOTION_TYPE, 1);
                assert(!boss.check_first_rephase);
#endif
            }
#ifndef BASELINE
    for (uint32 damage : {499u, 500u, 501u})
    {
        scenarioManager = {};
        Player player;
        Creature actor;
        FirstBoss boss(&actor);
        actor.active = &boss;
        uint32 supplied = damage;
        boss.DamageTakenAfterScaling(&player, damage, 0);
        assert(boss.phase == (supplied > 500) && player.credits == uint32(supplied > 500));
        assert(damage == (supplied > 500 ? 0 : supplied));
        assert(actor.health == (supplied > 500 ? 50000 : 100000));
    }
    for (uint64 targetMaximum : {100ull, 1000ull, 100000ull, 1000000ull})
        for (uint32 damage : {1u, 2u, 49u, 50u, 51u, 500u, 501u, 100000u, 200000u})
        {
            scenarioManager = {};
            Player player;
            Creature actor;
            actor.stat.healthMax = targetMaximum;
            FirstBoss boss(&actor);
            actor.active = &boss;
            uint32 finalDamage = uint32(damage / actor.GetHealthMultiplierForTarget(&player));
            bool retreat = finalDamage && (finalDamage >= targetMaximum || targetMaximum - finalDamage < targetMaximum / 2);
            uint32 dealt = player.Deal(&actor, damage);
            assert(actor.alive && boss.phase == retreat && player.credits == uint32(retreat));
            assert(dealt == (retreat ? 0 : finalDamage));
        }
    for (uint32 invalid = 0; invalid < 12; ++invalid)
    {
        scenarioManager = {};
        Player player;
        Unit pet;
        pet.owner = &player;
        Creature actor;
        FirstBoss boss(&actor);
        actor.active = &boss;
        if (invalid == 0) pet.owner = nullptr;
        if (invalid == 1) player.available = false;
        if (invalid == 2) player.alive = false;
        if (invalid == 3) player.contact = false;
        if (invalid == 4) player.playerClass = 3;
        if (invalid == 5) player.map = &unrelatedMap;
        if (invalid == 6) player.phase = false;
        if (invalid == 7) player.distance = 100.1f;
        if (invalid == 8) player.status = 0;
        if (invalid == 9) player.status = 5;
        if (invalid == 10) player.rewarded = true;
        if (invalid == 11) player.status = QUEST_STATUS_REWARDED;
        assert(pet.Deal(&actor, 200000) == 0 && actor.alive && !boss.phase && player.credits == 0 && actor.motion.points.empty());
        assert(actor.flags == 0 && actor.react == REACT_AGGRESSIVE && boss.texts.empty());
        pet.owner = &player;
        player.available = player.alive = player.contact = player.phase = true;
        player.playerClass = CLASS_WARRIOR;
        player.map = &instanceMap;
        player.distance = 1.f;
        player.status = QUEST_STATUS_INCOMPLETE;
        player.rewarded = false;
        assert(pet.Deal(&actor, 200000) == 0 && boss.phase && player.credits == 1);
    }
    for (uint32 invalid = 0; invalid < 8; ++invalid)
    {
        scenarioManager = {};
        Player player;
        Creature actor;
        FirstBoss boss(&actor);
        actor.active = &boss;
        if (invalid == 0) actor.entry = 98602;
        if (invalid == 1) actor.mapId = 1220;
        if (invalid == 2) actor.instance = 43;
        if (invalid == 3) scenarioManager.available = false;
        if (invalid == 4) scenarioManager.scenario.identity = 973;
        if (invalid == 5) scenarioManager.scenario.step = 3;
        if (invalid == 6) scenarioManager.scenario.step = 5;
        if (invalid == 7) scenarioManager.scenario.step = 6;
        uint32 damage = 2000;
        boss.DamageTakenAfterScaling(&player, damage, 0);
        assert(damage == 2000 && !boss.phase && actor.health == 100000 && player.credits == 0);
    }
    scenarioManager = {};
    Player player;
    Creature actor;
    actor.health = 40000;
    FirstBoss boss(&actor);
    actor.active = &boss;
    assert(player.Deal(&actor, 200000) == 0 && actor.health == 40000 && player.credits == 1);
    ambushers.clear();
    boss.MovementInform(POINT_MOTION_TYPE, 1);
    assert(player.Deal(&actor, 200000) == 0 && actor.despawns == 1 && player.credits == 1);
    boss.Reset();
    scenarioManager.scenario.step = 4;
    uint32 damage = 0;
    boss.DamageTakenAfterScaling(&player, damage, 0);
    assert(!boss.phase && damage == 0);
    damage = 1000;
    boss.DamageTakenAfterScaling(nullptr, damage, 0);
    assert(!boss.phase && damage == 1000);
    actor.alive = false;
    boss.DamageTakenAfterScaling(&player, damage, 0);
    assert(!boss.phase && damage == 1000);
#endif
#ifdef BASELINE
    std::cout << "BASELINE: pinned first Vigfus dies to full-health scaled lethal blows without retreat or46982 for Player/pet/guardian/charmer and current/suspended AI hosts.\n";
#else
    std::cout << "PASS: production retreat/departure/reset, native scaled dispatch/health arithmetic, player/pet/guardian owner admission, no healing/early/duplicate credit. Native pathing/cast/criteria/world services remain controlled, not client traversal.\n";
#endif
}
