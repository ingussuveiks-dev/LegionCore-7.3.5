#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using uint32 = uint32_t;
constexpr uint32 TYPEID_PLAYER = 4;
#include "CriteriaTypes.inc"
struct AchievementGlobalMgr
{
    bool IsGroupCriteriaType(CriteriaTypes type) const;
} achievementManager;
auto* sAchievementMgr = &achievementManager;
#include "GroupTypes.inc"
struct Scenario
{
    uint32 events = 0;
    void UpdateAchievementCriteria(uint32 identity) { assert(identity == 46987); ++events; }
};
struct ScenarioMgr
{
    Scenario scenario;
    bool available = true;
    Scenario* GetScenario(uint32 instance) { assert(instance == 42); return available ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct Map
{
    uint32 GetInstanceId() const { return 42; }
} instanceMap;
struct Unit;
struct Player;
struct ObjectGuid
{
    Unit* unit = nullptr;
    bool IsPlayer() const;
};
struct Unit
{
    virtual ~Unit() = default;
    Unit* owner = nullptr;
    Unit* charmer = nullptr;
    bool available = true;
    uint32 conversations = 0;
    virtual Player* ToPlayer() { return nullptr; }
    virtual Player const* ToPlayer() const { return nullptr; }
    uint32 GetTypeId() const { return ToPlayer() ? TYPEID_PLAYER : 3; }
    ObjectGuid GetCharmerOrOwnerGUID() const { return {charmer ? charmer : owner}; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
    void CreateConversation(uint32 identity) { assert(identity == 848); ++conversations; }
};
struct Player : Unit
{
    uint32 credits = 0;
    bool alliance = false;
    bool instanced = true;
    bool InInstance() const { return instanced; }
    Player* ToPlayer() override { return this; }
    Player const* ToPlayer() const override { return this; }
    void UpdateAchievementCriteria(CriteriaTypes type, uint32 identity)
    {
        assert(type == CRITERIA_TYPE_SCRIPT_EVENT_2 && identity == 46987);
        ++credits;
        bool ignoreGroup = false;
        Map* map = &instanceMap;
        uint32 referenceCache = identity;
#include "ScenarioBridge.inc"
    }
};
bool ObjectGuid::IsPlayer() const { return unit && unit->ToPlayer(); }
struct ObjectAccessor
{
    static Player* GetPlayer(Unit const&, ObjectGuid guid)
    {
        return guid.unit && guid.unit->available ? guid.unit->ToPlayer() : nullptr;
    }
};
#include "OwnerPlayer.inc"
struct Creature : Unit
{
    uint32 entry = 98602, mapId = 1511;
    bool bubble = true;
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    void RemoveAura(uint32 identity) { assert(identity == 205424); bubble = false; }
};
struct ScriptedAI
{
    Creature actor;
    Creature* me = &actor;
    bool bubbleClearedAtTalk = false;
    virtual ~ScriptedAI() = default;
    virtual void JustDied(Unit*) { }
    uint32 talks = 0;
    void Talk(uint32 identity) { assert(identity == 2); bubbleClearedAtTalk = !me->bubble; ++talks; }
};
struct FinalBoss : ScriptedAI
{
    struct SummonCleanup { void DespawnAll() { } } summons;
    void CancelSpectreRush() { }
#include "JustDied.inc"
};

int main(int argc, char**)
{
    bool baseline = argc > 1;
    for (bool alliance : {false, true})
    {
        Player owner, unrelated, charmer;
        owner.alliance = alliance;
        charmer.alliance = alliance;
        Unit pet, guardian, charmed, npc, unresolved, npcOwned;
        pet.owner = &owner;
        guardian.owner = &owner;
        charmed.owner = &owner;
        charmed.charmer = &charmer;
        unresolved.owner = &unrelated;
        unrelated.available = false;
        npcOwned.owner = &npc;
        for (Unit* killer : std::vector<Unit*>{&owner, &pet, &guardian, &charmed})
        {
            owner.credits = owner.conversations = charmer.credits = charmer.conversations = 0;
            scenarioManager.scenario.events = 0;
            FinalBoss boss;
            boss.JustDied(killer);
            Player* expected = killer == &charmed ? &charmer : &owner;
            assert(boss.talks == 1);
            if (!baseline) assert(!boss.actor.bubble && boss.bubbleClearedAtTalk);
            if (baseline && killer != &owner)
            {
                assert(expected->credits == 0 && expected->conversations == 0);
                assert(scenarioManager.scenario.events == 0);
                std::cout << "BASELINE: non-Player killing blow loses native 46987/848\n";
                continue;
            }
            assert(expected->credits == 1 && expected->conversations == 1);
            assert(scenarioManager.scenario.events == 1);
            assert(killer == expected || (killer->conversations == 0 && !killer->ToPlayer()));
            assert(unrelated.credits == 0 && unrelated.conversations == 0);
            if (killer == &charmed)
                assert(owner.credits == 0 && owner.conversations == 0);
        }
        if (!baseline)
            for (Unit* killer : std::vector<Unit*>{nullptr, &npc, &unresolved, &npcOwned})
            {
                owner.credits = owner.conversations = charmer.credits = charmer.conversations = 0;
                scenarioManager.scenario.events = 0;
                FinalBoss boss;
                boss.JustDied(killer);
                assert(boss.talks == 1);
                assert(!boss.actor.bubble && boss.bubbleClearedAtTalk);
                assert(owner.credits == 0 && charmer.credits == 0 && unrelated.credits == 0);
                assert(owner.conversations == 0 && charmer.conversations == 0 && unrelated.conversations == 0);
                assert(scenarioManager.scenario.events == 0);
            }
        if (!baseline)
        {
            for (bool instanced : {false, true})
                for (bool available : {false, true})
                {
                    owner.instanced = instanced;
                    scenarioManager.available = available;
                    scenarioManager.scenario.events = 0;
                    FinalBoss boss;
                    boss.JustDied(&pet);
                    assert(scenarioManager.scenario.events == uint32(instanced && available));
                }
            scenarioManager.available = true;
        }
    }
    if (!baseline)
        for (uint32 scope = 0; scope < 2; ++scope)
        {
            FinalBoss boss;
            if (scope) boss.actor.mapId = 1220;
            else boss.actor.entry = 98007;
            boss.JustDied(nullptr);
            assert(boss.actor.bubble && boss.talks == 1);
        }
    std::cout << "PASS: actual final death callback and native owner resolver; one event per callback, owner/charmer targeting, both faction labels, no nearby or group blanket award. Scoped205424 is cleared before Talk2 for player/pet/guardian/charmer and unresolved/null killers, not unrelated entry/map. Death dispatch, scenario, conversation and map services are not traversed.\n";
}
