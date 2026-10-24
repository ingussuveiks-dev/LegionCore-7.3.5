#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <unordered_map>
#include <vector>
using uint32 = uint32_t;
using ObjectGuid = uint32;
enum { TYPEID_PLAYER = 4, CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1,
    UNIT_FIELD_FLAGS = 0, UNIT_FLAG_IMMUNE_TO_PC = 1, UNIT_FLAG_IMMUNE_TO_NPC = 2, UNIT_FLAG_NOT_ATTACKABLE_1 = 4, REACT_AGGRESSIVE = 2, UNIT_STATE_SIGHTLESS = 8 };
struct Map { } instanceMap, otherMap;
struct Player;
struct Unit
{
    virtual ~Unit() = default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual uint32 GetTypeId() const { return 3; }
    Map* map = &instanceMap;
    bool alive = true, world = true, predelete = false;
    Map* GetMap() const { return map; }
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return world; }
    bool IsPreDelete() const { return predelete; }
    void CreateConversation(uint32);
};
using WorldObject = Unit;
struct Player : Unit
{
    ObjectGuid guid = 1;
    uint32 playerClass = CLASS_WARRIOR, status = QUEST_STATUS_INCOMPLETE, conversations = 0, acceptedConversations = 0;
    bool contact = true, teleporting = false, phase = true, rewarded = false, conversationWorks = true;
    bool flight = false, detectable = true;
    float distance = 1;
    std::function<void()> onConversation;
    Player* ToPlayer() override { return this; }
    uint32 GetTypeId() const override { return TYPEID_PLAYER; }
    bool CanContact() const { return contact; }
    bool IsBeingTeleported() const { return teleporting; }
    bool isInFlight() const { return flight; }
    uint32 getClass() const { return playerClass; }
    bool InSamePhase(Unit*) const { return phase; }
    bool GetQuestRewardStatus(uint32 identity) const { assert(identity == 40043); return rewarded; }
    uint32 GetQuestStatus(uint32 identity) const { assert(identity == 40043); return status; }
    ObjectGuid GetGUID() const { return guid; }
    void CreateConversation(uint32 identity) { assert(identity == 843); ++conversations; if (conversationWorks) { ++acceptedConversations; if (onConversation) onConversation(); } }
};
void Unit::CreateConversation(uint32 identity) { assert(ToPlayer()); ToPlayer()->CreateConversation(identity); }
struct Creature : Unit
{
    uint32 mapId = 1511, instance = 42, auraRemovals = 0, casts = 0;
    Creature* helya = nullptr;
    Player* nearest = nullptr;
    std::list<Player*> nearbyPlayers;
    uint32 playerScans = 0;
    bool castWorks = true;
    bool sightless = false, aggressive = true;
    uint32 acceptedCasts = 0;
    std::vector<std::function<void()>> callbacks;
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    bool HasUnitState(uint32 state) const { assert(state == UNIT_STATE_SIGHTLESS); return sightless; }
    bool HasReactState(uint32 state) const { assert(state == REACT_AGGRESSIVE); return aggressive; }
    bool canSeeOrDetect(Player* player, bool ignoreStealth, bool distanceCheck) { assert(!ignoreStealth && distanceCheck); return player->detectable; }
    float GetExactDist2d(Unit* unit) const { auto* player = unit->ToPlayer(); assert(player); return player->distance; }
    Creature* FindNearestCreature(uint32 identity, float distance, bool aliveOnly) { assert(identity == 99043 && distance == 100 && aliveOnly); return helya; }
    Player* FindNearestPlayer(float distance, bool aliveOnly) { assert(distance == 100 && aliveOnly); return nearest; }
    void GetPlayerListInGrid(std::list<Player*>& players, float distance) { assert(distance == 40); ++playerScans; players = nearbyPlayers; }
    uint32 CastSpell(Unit* target, uint32 identity) { assert(target == this && identity == 195062); ++casts; if (castWorks) ++acceptedCasts; return castWorks ? 0 : 1; }
    void AddDelayedEvent(uint32 delay, std::function<void()> callback) { assert(delay == 8000); callbacks.push_back(std::move(callback)); }
    void RemoveAura(uint32 identity) { assert(identity == 205424 || identity == 195996); if (identity == 205424) ++auraRemovals; }
    void RemoveFlag(uint32, uint32) { }
    void SetReactState(uint32 state) { assert(state == REACT_AGGRESSIVE); }
    void Flush() { auto pending = std::move(callbacks); callbacks.clear(); for (auto& callback : pending) callback(); }
};
struct Scenario { uint32 identity = 944, step = 5; uint32 GetScenarioId() const { return identity; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioManager { Scenario scenario; bool present = true; Scenario* GetScenario(uint32 instance) { return present && instance == 42 ? &scenario : nullptr; } } scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct ObjectAccessor
{
    static std::unordered_map<ObjectGuid, Player*> players;
    template<class Target> static Target* GetObjectInWorld(ObjectGuid guid, Target*) { auto found = players.find(guid); return found == players.end() ? nullptr : found->second; }
#include "ObjectInMap.inc"
    static Player* GetPlayer(WorldObject const&, ObjectGuid);
};
std::unordered_map<ObjectGuid, Player*> ObjectAccessor::players;
#include "GetPlayer.inc"
struct EventMap { void Reset() { } };
struct SummonList { explicit SummonList(Creature*) { } void DespawnAll() { } };
struct ScriptedAI
{
    Creature* me;
    Player* attacked = nullptr;
    uint32 attacks = 0;
    bool seePassive = false;
    explicit ScriptedAI(Creature* actor) : me(actor) { }
    virtual void Reset() { }
    virtual void MoveInLineOfSight(Unit*) { }
    bool CanSeeEvenInPassiveMode() const { return seePassive; }
    void AttackStart(Player* player) { attacked = player; ++attacks; }
};
struct IntroAI : ScriptedAI
{
    explicit IntroAI(Creature* actor) : ScriptedAI(actor), summons(me) { introDone = false; }
#include "Fields.inc"
#include "Reset.inc"
#include "IsIntroTarget.inc"
#include "MoveInLineOfSight.inc"
#include "UpdateIntro.inc"
};
struct Fixture
{
    Player owner, intruder;
    Creature actor, helya;
    IntroAI ai{&actor};
    Fixture() { intruder.guid = 2; intruder.status = 0; actor.helya = &helya; actor.nearest = &intruder; actor.nearbyPlayers = {&intruder, &owner}; scenarioManager = {}; ObjectAccessor::players = {{1, &owner}, {2, &intruder}}; ai.Reset(); }
    ~Fixture() { ObjectAccessor::players.clear(); }
    void Start() { ai.MoveInLineOfSight(&owner); }
};
void Invalidate(Fixture& fixture, uint32 invalid, bool delayed)
{
    if (invalid == 0) scenarioManager.scenario.step = 4;
    if (invalid == 1) scenarioManager.scenario.step = 6;
    if (invalid == 2) scenarioManager.scenario.identity = 991;
    if (invalid == 3) scenarioManager.present = false;
    if (invalid == 4) fixture.actor.instance = 43;
    if (invalid == 5) fixture.actor.mapId = 1220;
    if (invalid == 6) fixture.owner.playerClass = 2;
    if (invalid == 7) fixture.owner.rewarded = true;
    if (invalid == 8) fixture.owner.status = 0;
    if (invalid == 9) fixture.owner.status = 5;
    if (invalid == 10) fixture.owner.alive = false;
    if (invalid == 11) fixture.actor.alive = false;
    if (invalid == 12) fixture.owner.contact = false;
    if (invalid == 13) fixture.owner.world = false;
    if (invalid == 14) fixture.actor.world = false;
    if (invalid == 15) fixture.owner.teleporting = true;
    if (invalid == 16) fixture.owner.map = &otherMap;
    if (invalid == 17) fixture.owner.phase = false;
    if (invalid == 18) fixture.owner.distance = delayed ? 100.1f : 40.1f;
    if (invalid == 19) ObjectAccessor::players.erase(1);
    if (invalid == 20) fixture.owner.predelete = true;
}
int main()
{
#ifdef BASELINE
    {
        Fixture fixture; scenarioManager.scenario.step = 0;
        fixture.Start(); assert(fixture.owner.conversations == 1 && fixture.actor.callbacks.size() == 1);
        fixture.actor.Flush(); assert(fixture.ai.attacked == &fixture.intruder && fixture.actor.auraRemovals == 1);
    }
    {
        Fixture fixture; fixture.Start(); fixture.ai.Reset(); fixture.actor.Flush();
        assert(fixture.ai.attacked == &fixture.intruder && fixture.ai.introDone);
    }
    std::cout << "BASELINE: pinned9dd043ad starts final intro at stage0, attacks an unrelated nearest Player and retains the pending attack across Reset. Controlled map/cast/timer services.\n";
#else
    for (uint32 invalid = 0; invalid < 19; ++invalid)
    {
        Fixture fixture; Invalidate(fixture, invalid, false); fixture.Start();
        assert(!fixture.owner.conversations && !fixture.ai.introDone && fixture.actor.callbacks.empty() && !fixture.helya.casts && !fixture.actor.auraRemovals);
    }
    for (uint32 stage = 0; stage < 8; ++stage) for (uint32 status = 0; status < 7; ++status)
    {
        Fixture fixture; scenarioManager.scenario.step = stage; fixture.owner.status = status; fixture.Start();
        assert(fixture.ai.introDone == (stage == 5 && (status == 1 || status == 3)));
    }
    for (bool missingHelya : {false, true}) for (uint32 status : {1u, 3u}) for (bool presentationWorks : {false, true})
    {
        Fixture fixture; fixture.owner.status = status; fixture.owner.distance = 40;
        if (missingHelya) fixture.actor.helya = nullptr;
        fixture.helya.castWorks = fixture.owner.conversationWorks = presentationWorks;
        fixture.owner.onConversation = [&]() { fixture.Start(); };
        fixture.Start(); fixture.Start();
        assert(fixture.owner.conversations == 1 && fixture.actor.callbacks.size() == 1 && fixture.helya.casts == uint32(!missingHelya));
        assert(fixture.owner.acceptedConversations == uint32(presentationWorks) && fixture.helya.acceptedCasts == uint32(!missingHelya && presentationWorks));
        fixture.owner.distance = 100; fixture.actor.Flush(); fixture.actor.Flush();
        assert(fixture.ai.attacks == 1 && fixture.ai.attacked == &fixture.owner && fixture.actor.auraRemovals == 1 && !fixture.ai.introPending);
        fixture.ai.Reset(); fixture.Start(); assert(fixture.owner.conversations == 1);
    }
    for (uint32 invalid = 0; invalid < 21; ++invalid)
    {
        Fixture fixture; fixture.Start(); Invalidate(fixture, invalid, true); fixture.actor.Flush();
        assert(!fixture.ai.attacks && !fixture.actor.auraRemovals && !fixture.ai.introDone && !fixture.ai.introPending);
        fixture.owner = Player{}; fixture.actor.mapId = 1511; fixture.actor.instance = 42; fixture.actor.alive = fixture.actor.world = true;
        scenarioManager = {}; ObjectAccessor::players[1] = &fixture.owner;
        fixture.Start(); fixture.actor.Flush(); assert(fixture.ai.attacks == 1 && fixture.ai.attacked == &fixture.owner);
    }
    {
        Fixture fixture; fixture.Start(); auto obsolete = std::move(fixture.actor.callbacks.front()); fixture.actor.callbacks.clear();
        fixture.ai.Reset(); fixture.Start(); obsolete(); assert(fixture.ai.introPending && !fixture.ai.attacks && !fixture.actor.auraRemovals);
        fixture.actor.Flush(); assert(fixture.ai.attacks == 1 && fixture.ai.attacked == &fixture.owner);
    }
    {
        Fixture fixture; Unit pet; fixture.ai.MoveInLineOfSight(&pet); fixture.ai.MoveInLineOfSight(nullptr);
        assert(!fixture.ai.introDone && fixture.actor.callbacks.empty());
    }
    {
        Fixture fixture; scenarioManager.scenario.step = 4; fixture.ai.UpdateIntro(1);
        assert(!fixture.actor.playerScans && !fixture.owner.conversations);
        scenarioManager.scenario.step = 5; fixture.ai.UpdateIntro(999);
        assert(!fixture.owner.conversations); fixture.ai.UpdateIntro(1);
        assert(fixture.owner.conversations == 1 && fixture.actor.playerScans == 1 && !fixture.intruder.conversations);
        fixture.actor.Flush(); assert(fixture.ai.attacked == &fixture.owner);
    }
    {
        Fixture fixture; fixture.owner.distance = 1000; fixture.ai.UpdateIntro(1);
        assert(!fixture.ai.introDone && fixture.actor.playerScans == 1);
        fixture.owner.distance = 1; fixture.ai.UpdateIntro(999); assert(!fixture.ai.introDone);
        fixture.ai.UpdateIntro(1); fixture.actor.Flush(); assert(fixture.ai.attacked == &fixture.owner);
    }
    for (uint32 invalid = 0; invalid < 4; ++invalid)
    {
        Fixture fixture;
        if (invalid == 0) fixture.actor.sightless = true;
        if (invalid == 1) fixture.actor.aggressive = false;
        if (invalid == 2) fixture.owner.flight = true;
        if (invalid == 3) fixture.owner.detectable = false;
        fixture.ai.UpdateIntro(1); assert(!fixture.ai.introDone && !fixture.owner.conversations && fixture.actor.callbacks.empty());
    }
    {
        Fixture fixture; fixture.actor.aggressive = false; fixture.ai.seePassive = true;
        fixture.ai.UpdateIntro(1); fixture.actor.Flush(); assert(fixture.ai.attacked == &fixture.owner);
    }
    std::cout << "PASS: actual final intro requires live40043 Warrior/1511/944/5;56 stage/status cases,19 initial and21 delayed invalid contexts, identity-only targeting/retry/reset epochs/reentrant or failed speech/optional or failed Helya/boundary/pet/null and bounded stationary retry checked. Timers, geometry, combat and world/cast services remain controlled; no client visual/audio proof.\n";
#endif
}
