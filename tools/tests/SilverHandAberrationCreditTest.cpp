#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>

using uint32 = uint32_t;
enum { CLASS_PALADIN = 2, QUEST_STATUS_INCOMPLETE = 3 };
#include "CriteriaTypes.inc"
struct AchievementGlobalMgr
{
    bool IsGroupCriteriaType(CriteriaTypes type) const;
} achievements;
auto* sAchievementMgr = &achievements;
#include "GroupTypes.inc"
struct Player;
struct PlayerReference
{
    Player* player;
    Player* getSource() const { return player; }
};
struct Map
{
    std::vector<PlayerReference> players;
    uint32 GetInstanceId() const { return 42; }
    auto const& GetPlayers() const { return players; }
} liveMap, otherMap;
struct Scenario
{
    uint32 id = 1092, step = 5, events = 0;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
    void UpdateAchievementCriteria(uint32 identity) { assert(identity == 50896); ++events; }
};
struct ScenarioManager
{
    bool present = true;
    Scenario scenario;
    Scenario* GetScenario(uint32 instance) { return present && instance == 42 ? &scenario : nullptr; }
} scenarios;
auto* sScenarioMgr = &scenarios;
struct Unit;
struct ObjectGuid
{
    Unit* unit = nullptr;
    bool IsPlayer() const;
    explicit operator bool() const { return unit != nullptr; }
};
struct Unit
{
    virtual ~Unit() = default;
    Map* contextMap = &liveMap;
    Unit* owner = nullptr;
    Unit* charmer = nullptr;
    bool world = true, available = true, preDelete = false;
    Map* GetMap() const { return contextMap; }
    bool IsInWorld() const { return world; }
    bool IsPreDelete() const { return preDelete; }
    virtual Player* ToPlayer() { return nullptr; }
    virtual Player const* ToPlayer() const { return nullptr; }
    bool IsPlayer() const { return ToPlayer() != nullptr; }
    ObjectGuid GetCharmerOrOwnerGUID() const { return {charmer ? charmer : owner}; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
};
struct Creature;
struct Player : Unit
{
    bool contact = true, gm = false, teleport = false, phase = true, rewarded = false, alive = true;
    uint32 klass = 2, status = 3, instance = 42, credits = 0, faction = 0;
    float distance = 1;
    std::function<void()> onCredit;
    Player* ToPlayer() override { return this; }
    Player const* ToPlayer() const override { return this; }
    bool CanContact() const { return contact; }
    bool isGameMaster() const { return gm; }
    bool IsBeingTeleported() const { return teleport; }
    uint32 getClass() const { return klass; }
    uint32 GetInstanceId() const { return instance; }
    bool InSamePhase(Creature*) const { return phase; }
    bool IsWithinDistInMap(Creature*, float radius) const { return distance <= radius; }
    bool GetQuestRewardStatus(uint32 quest) const { assert(quest == 42120); return rewarded; }
    uint32 GetQuestStatus(uint32 quest) const { assert(quest == 42120); return status; }
    bool InInstance() const { return contextMap == &liveMap; }
    void UpdateAchievementCriteria(CriteriaTypes type, uint32 identity)
    {
        assert(type == CRITERIA_TYPE_SCRIPT_EVENT_2 && identity == 50896);
        ++credits;
        if (onCredit) onCredit();
        bool ignoreGroup = false;
        uint32 referenceCache = identity;
        Map* map = GetMap();
#include "ScenarioBridge.inc"
    }
};
bool ObjectGuid::IsPlayer() const { return unit && unit->IsPlayer(); }
struct ObjectAccessor
{
    static Player* GetPlayer(Unit const& source, ObjectGuid guid)
    {
        return guid.unit && guid.unit->world && !guid.unit->preDelete && guid.unit->available && guid.unit->GetMap() == source.GetMap() ? guid.unit->ToPlayer() : nullptr;
    }
};
#include "OwnerPlayer.inc"
struct Creature : Unit
{
    bool alive = false;
    uint32 entry = 106669, mapId = 1539, instance = 42;
    Player* nearest = nullptr;
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    bool IsAlive() const { return alive; }
    Player* FindNearestPlayer(float radius) const { assert(radius == 100); return nearest; }
};
struct EventMap { uint32 resets = 0; void Reset() { ++resets; } };
#include "CreditPolicy.inc"
struct AberrationAI
{
    Creature* me;
    EventMap events;
    bool outroDone = false;
    bool _deathCreditDone = false;
    uint32 _outroTimer = 0;
    bool _outroFinished = false;
    std::uint64_t _outroGeneration = 0;
#include "Reset.inc"
#include "JustDied.inc"
};
struct Fixture
{
    Player owner, neighbor, charmer, groupMember;
    Creature actor;
    Unit pet, guardian, charmed, npc;
    AberrationAI ai{&actor};
    Fixture()
    {
        scenarios = {};
        liveMap.players = {{&neighbor}, {nullptr}, {&owner}, {&groupMember}};
        neighbor.status = 0;
        actor.nearest = &neighbor;
        pet.owner = guardian.owner = charmed.owner = &owner;
        charmed.charmer = &charmer;
    }
};
void Invalidate(uint32 invalid, Fixture& fixture)
{
    auto& player = fixture.owner;
    auto& actor = fixture.actor;
    switch (invalid)
    {
        case 0: player.world = false; break;
        case 1: player.contact = false; break;
        case 2: player.gm = true; break;
        case 3: player.teleport = true; break;
        case 4: player.klass = 1; break;
        case 5: actor.world = false; break;
        case 6: actor.alive = true; break;
        case 7: actor.entry = 106676; break;
        case 8: actor.mapId = 1220; break;
        case 9: player.contextMap = nullptr; break;
        case 10: player.contextMap = &otherMap; break;
        case 11: player.instance = 43; break;
        case 12: player.phase = false; break;
        case 13: player.distance = 100.01f; break;
        case 14: player.rewarded = true; break;
        case 15: player.status = 0; break;
        case 16: player.status = 1; break;
        case 17: scenarios.present = false; break;
        case 18: scenarios.scenario.id = 988; break;
        case 19: scenarios.scenario.step = 4; break;
        case 20: scenarios.scenario.step = 6; break;
        case 21: actor.contextMap = nullptr; break;
        case 22: player.preDelete = true; break;
        case 23: actor.preDelete = true; break;
        default: assert(false);
    }
}
int main()
{
#ifdef BASELINE
    for (uint32 killer = 0; killer < 3; ++killer)
    {
        Fixture fixture;
        Unit* source = killer == 0 ? static_cast<Unit*>(&fixture.owner) : killer == 1 ? &fixture.pet : &fixture.npc;
        fixture.ai.JustDied(source);
        fixture.ai.JustDied(source);
        assert(fixture.owner.credits == 0 && fixture.neighbor.credits == 2);
    }
    std::cout << "PASS baseline: actual fc0bc298 callback attempts50896 twice on ineligible nearest neighbor, not the player/pet owner or eligible shared participant. Not persisted native criteria acceptance.\n";
#else
    assert(!sAchievementMgr->IsGroupCriteriaType(CRITERIA_TYPE_SCRIPT_EVENT_2));
    {
        Fixture fixture;
        assert(!SilverHandAberration::CanCredit(nullptr, &fixture.actor));
        assert(!SilverHandAberration::CanCredit(&fixture.owner, nullptr));
    }
    for (uint32 faction : {0U, 1U})
        for (uint32 killer = 0; killer < 4; ++killer)
            for (bool alive : {false, true})
            {
                Fixture fixture;
                fixture.owner.faction = fixture.charmer.faction = faction;
                fixture.owner.alive = fixture.charmer.alive = alive;
                Unit* source = killer == 0 ? static_cast<Unit*>(&fixture.owner) : killer == 1 ? &fixture.pet : killer == 2 ? &fixture.guardian : &fixture.charmed;
                Player* expected = killer == 3 ? &fixture.charmer : &fixture.owner;
                expected->onCredit = [&] { fixture.ai.JustDied(source); };
                fixture.ai.JustDied(source);
                fixture.ai.JustDied(source);
                assert(expected->credits == 1 && fixture.neighbor.credits == 0 && fixture.groupMember.credits == 0);
                assert(scenarios.scenario.events == 1 && fixture.ai._deathCreditDone);
                fixture.ai.Reset();
                assert(!fixture.ai._deathCreditDone && !fixture.ai.outroDone && fixture.ai.events.resets == 1);
                fixture.ai.JustDied(source);
                assert(expected->credits == 2 && scenarios.scenario.events == 2);
            }
    for (uint32 invalid = 0; invalid < 24; ++invalid)
        for (uint32 killer = 0; killer < 3; ++killer)
        {
            Fixture fixture;
            fixture.groupMember.status = 0;
            Invalidate(invalid, fixture);
            Unit* source = killer == 0 ? static_cast<Unit*>(&fixture.owner) : killer == 1 ? &fixture.guardian : &fixture.npc;
            fixture.ai.JustDied(source);
            assert(fixture.owner.credits == 0 && fixture.neighbor.credits == 0 && fixture.groupMember.credits == 0);
            assert(scenarios.scenario.events == 0 && !fixture.ai._deathCreditDone);
        }
    for (uint32 invalid = 0; invalid < 4; ++invalid)
    {
        Fixture fixture;
        if (invalid == 0) { fixture.owner.available = false; fixture.owner.contact = false; }
        if (invalid == 1) fixture.owner.world = false;
        if (invalid == 2) fixture.owner.contextMap = &otherMap;
        if (invalid == 3) fixture.guardian.owner = &fixture.npc;
        fixture.ai.JustDied(&fixture.guardian);
        assert(fixture.groupMember.credits == uint32(invalid != 3));
        assert(fixture.owner.credits == uint32(invalid == 3));
        assert(fixture.ai._deathCreditDone && scenarios.scenario.events == 1);
    }
    for (bool ownerEligible : {false, true})
    {
        Fixture fixture;
        fixture.owner.status = ownerEligible ? 3 : 0;
        fixture.ai.JustDied(&fixture.npc);
        fixture.ai.JustDied(&fixture.npc);
        assert(fixture.owner.credits == uint32(ownerEligible));
        assert(fixture.groupMember.credits == uint32(!ownerEligible));
        assert(fixture.neighbor.credits == 0 && scenarios.scenario.events == 1);
    }
    for (uint32 invalid = 0; invalid < 3; ++invalid)
    {
        Fixture fixture;
        if (invalid == 1) fixture.npc.world = false;
        if (invalid == 2) fixture.npc.contextMap = &otherMap;
        fixture.ai.JustDied(invalid ? &fixture.npc : nullptr);
        assert(!fixture.ai._deathCreditDone && scenarios.scenario.events == 0);
    }
    for (uint32 invalid : {4U, 13U, 15U, 22U})
    {
        Fixture shared;
        Invalidate(invalid, shared);
        shared.ai.JustDied(&shared.guardian);
        assert(shared.owner.credits == 0 && shared.neighbor.credits == 0 && shared.groupMember.credits == 1);
        assert(scenarios.scenario.events == 1);
    }
    Fixture fixture;
    fixture.groupMember.status = 0;
    fixture.owner.status = 0;
    fixture.ai.JustDied(&fixture.owner);
    assert(!fixture.ai._deathCreditDone && fixture.groupMember.credits == 0);
    fixture.owner.status = 3;
    fixture.owner.distance = 100;
    fixture.ai.JustDied(&fixture.pet);
    assert(fixture.owner.credits == 1 && scenarios.scenario.events == 1);
    std::cout << "PASS: production death policy/callback/reset, native owner resolver and Player-to-shared-scenario bridge;24 invalid contexts, both faction labels, dead/live owner, player/pet/guardian/charmer, NPC/missing-owner/foreign-class/out-of-range-owner shared participant fallback, no ineligible neighbor, reentrant/repeat/reset and100yd boundary. Geometry, list/accessor services and scenario counter are controlled; not actual combat or client traversal.\n";
#endif
}
