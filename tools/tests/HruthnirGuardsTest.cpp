#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <vector>

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
struct ObjectGuid
{
    uint32 value = 0;
    void Clear() { value = 0; }
    bool operator==(ObjectGuid other) const { return value == other.value; }
};
constexpr uint32 CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1;
constexpr uint32 CRITERIA_TYPE_SCRIPT_EVENT_2 = 92;
constexpr uint32 NPC_INVISIBLE_MAN = 1, NPC_ORIK_TRUEHEART_PET = 2, NPC_CATO_PET = 3;
constexpr uint32 GO_FIRST_DOOR = 1, GO_EVENT_DOOR = 2;
struct Player;
struct Creature;
struct GameObject;
struct Map;
uint32 CurrentScenarioStep();
struct Unit
{
    ObjectGuid guid;
    uint32 entry = 96455, phase = 6999;
    bool world = true, alive = true;
    float position = 0, coordinateX = 4794;
    Map* map = nullptr;
    uint32 GetEntry() const { return entry; }
    ObjectGuid GetGUID() const { return guid; }
    bool IsInWorld() const { return world; }
    bool IsAlive() const { return alive; }
    Map* GetMap() const { return map; }
    float GetPositionX() const { return coordinateX; }
    Player* FindNearestPlayer(float);
    Creature* FindNearestCreature(uint32, float, bool);
    GameObject* FindNearestGameObject(uint32, float);
};
struct ActorAI
{
    uint32 actions = 0;
    uint32 introRequests = 0;
    bool sceneReady = true;
    std::function<void()> callback;
    std::function<void()> introCallback;
    uint32 GetData(uint32 identity) const { if (identity != 1059) std::abort(); return sceneReady; }
    void DoAction(uint32 action)
    {
        if (action != 2) std::abort();
#ifdef BASELINE
        ++actions; if (callback) callback();
#else
        if (CurrentScenarioStep() == 2) { ++introRequests; if (introCallback) introCallback(); }
        else { ++actions; if (callback) callback(); }
#endif
    }
};
using CreatureAI = ActorAI;
struct Creature : Unit
{
    uint32 spawn = 0;
    bool despawn = false, IsAIEnabled = true;
    ActorAI* ai = nullptr;
    uint32 GetDBTableGUIDLow() const { return spawn; }
    bool IsDespawn() const { return despawn; }
    ActorAI* AI() const { return ai; }
    float GetDistance(Player*) const;
};
struct GameObject
{
    ObjectGuid guid{134170};
    uint32 entry = 243897, spawn = 134170, phase = 6999, changes = 0;
    bool world = true, samePhase = true;
    uint32 GetEntry() const { return entry; }
    uint32 GetDBTableGUIDLow() const { return spawn; }
    ObjectGuid GetGUID() const { return guid; }
    bool IsInWorld() const { return world; }
    bool InSamePhase(Creature*) const { return samePhase; }
    void SetPhaseMask(uint32 mask, bool update) { if (mask != 4 || !update) std::abort(); ++changes; }
};
struct Player : Unit
{
    uint32 classId = CLASS_WARRIOR, status = QUEST_STATUS_INCOMPLETE, instanceId = 42, credits = 0;
    bool contact = true, gm = false, teleport = false, rewarded = false;
    uint32 faction = 0, group = 0;
    std::function<void()> creditCallback;
    bool CanContact() const { return contact; }
    bool isGameMaster() const { return gm; }
    bool IsBeingTeleported() const { return teleport; }
    uint32 getClass() const { return classId; }
    uint32 GetInstanceId() const { return instanceId; }
    bool InSamePhase(Creature* actor) const { return phase == actor->phase; }
    bool IsWithinDistInMap(Creature* actor, float distance) const { return map == actor->map && std::abs(position - actor->position) <= distance; }
    bool GetQuestRewardStatus(uint32 quest) const { if (quest != 39191) std::abort(); return rewarded; }
    uint32 GetQuestStatus(uint32 quest) const { if (quest != 39191) std::abort(); return status; }
    void UpdateAchievementCriteria(uint32 type, uint32 asset) { if (type != 92 || asset != 46358) std::abort(); ++credits; if (creditCallback) creditCallback(); }
};
float Creature::GetDistance(Player* player) const { return std::abs(position - player->position); }
struct PlayerReference { Player* player; Player* getSource() const { return player; } };
struct Map
{
    uint32 instanceId = 42;
    std::map<uint32, Creature*> creatures;
    std::map<uint32, GameObject*> objects;
    std::vector<PlayerReference> players;
    uint32 GetInstanceId() const { return instanceId; }
    Creature* GetCreature(ObjectGuid guid) const { auto found = creatures.find(guid.value); return found == creatures.end() ? nullptr : found->second; }
    GameObject* GetGameObject(ObjectGuid guid) const { auto found = objects.find(guid.value); return found == objects.end() ? nullptr : found->second; }
    auto const& GetPlayers() const { return players; }
};
Player* Unit::FindNearestPlayer(float range)
{
    Player* candidate = nullptr;
    for (auto reference : map->players)
        if (reference.player && std::abs(position - reference.player->position) <= range &&
            (!candidate || std::abs(position - reference.player->position) < std::abs(position - candidate->position))) candidate = reference.player;
    return candidate;
}
Creature* Unit::FindNearestCreature(uint32 identity, float, bool)
{
    for (auto pair : map->creatures) if (pair.second->entry == identity) return pair.second;
    return nullptr;
}
GameObject* Unit::FindNearestGameObject(uint32 identity, float)
{
    for (auto pair : map->objects) if (pair.second->entry == identity && pair.second->phase == phase) return pair.second;
    return nullptr;
}
struct Scenario
{
    uint32 identity = 909, step = 2;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioManager
{
    Scenario* scenario = nullptr;
    Scenario* GetScenario(uint32 instanceId) const { if (instanceId != 42) std::abort(); return scenario; }
} manager;
ScenarioManager* sScenarioMgr = &manager;
uint32 CurrentScenarioStep() { return manager.scenario ? manager.scenario->step : 0; }
struct Instance
{
    Map* instance;
    explicit Instance(Map* map) : instance(map) { Initialize(); }
#include "Fields.inc"
#include "Initialize.inc"
#include "Create.inc"
#include "Object.inc"
#include "Death.inc"
#include "Advance.inc"
};
void Require(bool condition, char const* message) { if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); } }
struct Fixture
{
    Map map;
    Scenario scenario;
    ActorAI ai;
    Creature first, second, companion, unrelated;
    GameObject door, hunterDoor;
    Player owner, stranger;
    Instance script{&map};
    Fixture()
    {
        manager.scenario = &scenario;
        first.guid = {101}; first.spawn = 361412;
        second.guid = {102}; second.spawn = 361413;
        companion.guid = {103}; companion.spawn = 361306; companion.entry = 96468; companion.ai = &ai;
        unrelated.guid = {104}; unrelated.spawn = 361392;
        for (auto actor : {&first, &second, &companion, &unrelated})
        {
            actor->map = &map;
            map.creatures[actor->guid.value] = actor;
            script.OnCreatureCreate(actor);
        }
        hunterDoor.guid = {140833}; hunterDoor.entry = 248777; hunterDoor.spawn = 140833; hunterDoor.phase = 6985;
        for (auto object : {&door, &hunterDoor}) { map.objects[object->guid.value] = object; script.OnGameObjectCreate(object); }
        owner.map = &map; owner.position = 10;
        stranger.map = &map; stranger.classId = 3;
        map.players = {{&stranger}, {&owner}};
        owner.creditCallback = [&] { scenario.step = 3; };
    }
    void Death(Creature& actor) { actor.alive = false; script.OnUnitDeath(&actor); }
    void Both() { Death(first); Death(second); }
    void Completed()
    {
        Require(owner.credits == 1 && stranger.credits == 0 && ai.actions == 1 && door.changes == 1 && hunterDoor.changes == 0 && scenario.step == 3, "one native transition, eligible reference, companion and Warrior-only door");
    }
};
int main()
{
#ifdef SCENE_BASELINE
    Fixture fixture; fixture.ai.sceneReady = false;
    fixture.Both();
    Require(fixture.owner.credits == 1 && fixture.scenario.step == 3 && fixture.door.changes == 1 && fixture.ai.actions == 1 && fixture.ai.introRequests == 0, "pinned08e49c9d changes stage/barrier before asking for any ready scene");
    std::cout << "PASS: pinned scene-end ordering regression reproduced:46358/stage3/barrier precede native intro readiness\n";
#elif defined(BASELINE)
    {
        Fixture fixture; fixture.map.players = {{&fixture.owner}};
        fixture.Both(); Require(fixture.owner.credits == 1 && fixture.ai.actions == 1, "pinned ordinary distinct-guard handoff");
        fixture.unrelated.coordinateX = 4829; fixture.Death(fixture.unrelated);
        Require(fixture.owner.credits == 2 && fixture.ai.actions == 2, "pinned legitimate later corridor death replays an already advanced handoff");
    }
    {
        Fixture fixture;
        fixture.Death(fixture.first);
        fixture.Death(fixture.first);
        Require(fixture.stranger.credits == 1 && fixture.ai.actions == 1 && fixture.hunterDoor.changes == 0, "pinned duplicate death consumes both counts and credits nearest unrelated player without inventing a cross-phase door lookup");
        fixture.unrelated.coordinateX = 4829;
        fixture.Death(fixture.unrelated);
        Require(fixture.stranger.credits == 2 && fixture.ai.actions == 2, "pinned unscoped death replays threshold outside the X filter");
    }
    std::cout << "PASS: pinned duplicate/wrong-player/unrelated-death replay reproduced\n";
#else
    {
        Fixture fixture; fixture.ai.sceneReady = false;
        fixture.Both();
        for (uint32 tick = 0; tick < 5; ++tick) fixture.script.TryAdvanceMagnarGuards();
        Require(fixture.owner.credits == 0 && fixture.scenario.step == 2 && fixture.ai.actions == 0 && fixture.door.changes == 0 && fixture.ai.introRequests == 6, "pending/failed/missing scene cannot release criterion, door or chamber; idempotent intro is polled");
        fixture.ai.sceneReady = true; fixture.script.TryAdvanceMagnarGuards(); fixture.Completed();
    }
    {
        Fixture fixture; fixture.ai.introCallback = [&] { fixture.script.Initialize(); };
        fixture.Both(); Require(fixture.owner.credits == 0 && fixture.door.changes == 0 && fixture.ai.actions == 0, "intro callback Reset revokes old guard epoch before credit");
    }
    {
        Fixture fixture; fixture.ai.introCallback = [&] { fixture.scenario.step = 4; };
        fixture.Both(); Require(fixture.owner.credits == 0 && fixture.door.changes == 0 && fixture.ai.actions == 0, "intro callback changing scenario cannot release old stage2 event");
    }
    {
        Fixture fixture; ActorAI replacement; replacement.sceneReady = false;
        fixture.ai.introCallback = [&] { fixture.companion.ai = &replacement; };
        fixture.Both(); Require(fixture.owner.credits == 0 && fixture.door.changes == 0, "replaced AI cannot borrow old ready return");
        replacement.sceneReady = true; fixture.script.TryAdvanceMagnarGuards();
        Require(fixture.owner.credits == 1 && replacement.actions == 1 && fixture.door.changes == 1, "replacement must provide its own ready callback");
    }
    for (bool alive : {true, false})
        for (uint32 faction = 0; faction < 2; ++faction)
            for (uint32 status : {QUEST_STATUS_INCOMPLETE, QUEST_STATUS_COMPLETE})
            {
                Fixture fixture;
                fixture.owner.faction = faction; fixture.owner.status = status;
                fixture.owner.alive = alive;
                fixture.Both(); fixture.Completed();
                fixture.Death(fixture.first); fixture.Death(fixture.second); fixture.Death(fixture.unrelated);
                for (uint32 tick = 0; tick < 4; ++tick) fixture.script.TryAdvanceMagnarGuards();
                fixture.Completed();
            }
    {
        Fixture fixture;
        fixture.Death(fixture.first); fixture.Death(fixture.first);
        fixture.Death(fixture.unrelated);
        Require(fixture.ai.actions == 0 && fixture.owner.credits == 0, "distinct native spawn identities, not repeated callbacks/X coordinates");
        fixture.Death(fixture.second); fixture.Completed();
    }
    for (uint32 invalid = 0; invalid < 19; ++invalid)
    {
        Fixture fixture;
        switch (invalid)
        {
            case 0: fixture.owner.world = false; break;
            case 1: fixture.owner.contact = false; break;
            case 2: fixture.owner.gm = true; break;
            case 3: fixture.owner.teleport = true; break;
            case 4: fixture.owner.classId = 3; break;
            case 5: fixture.owner.map = nullptr; break;
            case 6: fixture.owner.instanceId = 43; break;
            case 7: fixture.owner.phase = 6985; break;
            case 8: fixture.owner.position = 151; break;
            case 9: fixture.owner.rewarded = true; break;
            case 10: fixture.owner.status = 0; break;
            case 11: fixture.owner.status = 5; break;
            case 12: fixture.companion.world = false; break;
            case 13: fixture.companion.alive = false; break;
            case 14: fixture.companion.despawn = true; break;
            case 15: fixture.companion.IsAIEnabled = false; break;
            case 16: fixture.companion.ai = nullptr; break;
            case 17: fixture.door.world = false; break;
            case 18: fixture.door.samePhase = false; break;
        }
        fixture.Both(); fixture.script.TryAdvanceMagnarGuards();
        Require(fixture.owner.credits == 0 && fixture.ai.actions == 0 && fixture.door.changes == 0, "invalid admission must retain retry without early effects");
        fixture.owner.world = fixture.owner.contact = true; fixture.owner.gm = fixture.owner.teleport = fixture.owner.rewarded = false;
        fixture.owner.classId = CLASS_WARRIOR; fixture.owner.map = &fixture.map; fixture.owner.instanceId = 42;
        fixture.owner.phase = 6999; fixture.owner.position = 10; fixture.owner.status = QUEST_STATUS_INCOMPLETE;
        fixture.companion.world = fixture.companion.alive = fixture.companion.IsAIEnabled = true; fixture.companion.despawn = false; fixture.companion.ai = &fixture.ai;
        fixture.door.world = fixture.door.samePhase = true;
        fixture.script.TryAdvanceMagnarGuards(); fixture.Completed();
    }
    for (uint32 identity : {1068u, 1082u, 0u})
    {
        Fixture fixture; fixture.scenario.identity = identity;
        fixture.Both(); fixture.script.TryAdvanceMagnarGuards();
        Require(fixture.owner.credits == 0 && fixture.ai.actions == 0 && fixture.door.changes == 0 && fixture.hunterDoor.changes == 0, "shared Hunter/Paladin scenario untouched");
        fixture.scenario.identity = 909; fixture.script.TryAdvanceMagnarGuards();
        Require(fixture.owner.credits == 0, "wrong-scenario deaths are not banked");
    }
    {
        Fixture fixture; fixture.scenario.step = 1; fixture.Both();
        Require(fixture.owner.credits == 0 && fixture.ai.actions == 0, "early deaths do not advance out of order");
        fixture.scenario.step = 2; fixture.script.TryAdvanceMagnarGuards(); fixture.Completed();
    }
    {
        Fixture fixture; fixture.scenario.step = 3; fixture.Both();
        Require(fixture.ai.actions == 0, "late deaths do not reopen confrontation");
    }
    {
        Fixture fixture; manager.scenario = nullptr; fixture.Both();
        manager.scenario = &fixture.scenario; fixture.script.TryAdvanceMagnarGuards();
        Require(fixture.owner.credits == 0, "missing-scenario deaths are not banked");
    }
    {
        Fixture fixture; fixture.first.alive = true; fixture.script.OnUnitDeath(&fixture.first); fixture.Death(fixture.second);
        Require(fixture.owner.credits == 0, "live notifications cannot stand in for a guard death");
        fixture.Death(fixture.first); fixture.Completed();
    }
    {
        Fixture fixture; Creature replacement = fixture.first; replacement.guid = {201};
        fixture.Death(fixture.first); fixture.script.OnCreatureCreate(&replacement);
        fixture.map.creatures[201] = &replacement; fixture.Death(fixture.first); fixture.Death(fixture.second);
        Require(fixture.owner.credits == 0, "stale GUID from recreated native spawn is rejected");
        fixture.Death(replacement); fixture.Completed();
    }
    {
        Fixture fixture; fixture.owner.creditCallback = [] {};
        fixture.Both(); Require(fixture.scenario.step == 2 && fixture.ai.actions == 0 && fixture.door.changes == 0, "failed criterion update does not fabricate an acknowledged stage");
        fixture.owner.creditCallback = [&] { fixture.scenario.step = 3; };
        fixture.script.TryAdvanceMagnarGuards();
        Require(fixture.owner.credits == 2 && fixture.ai.actions == 1 && fixture.door.changes == 1, "unacknowledged criteria remain retryable, handoff is still once only");
    }
    {
        Fixture fixture; fixture.stranger.classId = CLASS_WARRIOR;
        fixture.owner.group = fixture.stranger.group = 7;
        fixture.stranger.creditCallback = [&] { fixture.scenario.step = 3; };
        fixture.Both();
        Require(fixture.owner.credits == 0 && fixture.stranger.credits == 1 && fixture.ai.actions == 1 && fixture.scenario.step == 3, "one eligible local reference advances the shared scenario, not one event per group member");
    }
    {
        Fixture fixture; fixture.Death(fixture.first); fixture.first.alive = true; fixture.Death(fixture.second);
        Require(fixture.ai.actions == 0, "respawned first guard cancels its pending death");
        fixture.Death(fixture.first); fixture.Completed();
    }
    {
        Fixture fixture; fixture.Death(fixture.first); fixture.script.OnCreatureCreate(&fixture.first);
        fixture.Death(fixture.second); fixture.Completed();
    }
    {
        Fixture fixture; fixture.Death(fixture.first);
        Creature reloaded = fixture.first; reloaded.guid = {201};
        fixture.map.creatures.erase(fixture.first.guid.value); fixture.map.creatures[201] = &reloaded;
        fixture.script.OnCreatureCreate(&reloaded); fixture.Death(fixture.first); fixture.Death(fixture.second); fixture.Completed();
    }
    {
        Fixture fixture; fixture.first.alive = false; fixture.script.OnCreatureCreate(&fixture.first);
        fixture.Death(fixture.second); Require(fixture.owner.credits == 0, "loading an unrecorded corpse is not an observed death");
    }
    {
        Fixture fixture; fixture.map.creatures.erase(fixture.companion.guid.value); fixture.Both();
        Require(fixture.owner.credits == 0, "missing actor preserves progression");
        fixture.map.creatures[fixture.companion.guid.value] = &fixture.companion;
        fixture.script.TryAdvanceMagnarGuards(); fixture.Completed();
    }
    {
        Fixture fixture;
        fixture.owner.creditCallback = [&] { fixture.script.TryAdvanceMagnarGuards(); fixture.scenario.step = 3; fixture.map.creatures.erase(fixture.companion.guid.value); };
        fixture.Both(); Require(fixture.owner.credits == 1 && fixture.ai.actions == 0, "missing actor after observed credit remains retryable");
        fixture.first.alive = fixture.second.alive = true;
        fixture.script.OnCreatureCreate(&fixture.first); fixture.script.OnCreatureCreate(&fixture.second);
        fixture.map.creatures[fixture.companion.guid.value] = &fixture.companion;
        fixture.script.TryAdvanceMagnarGuards(); fixture.Completed();
    }
    {
        Fixture fixture; fixture.owner.creditCallback = [&] { fixture.script.Initialize(); };
        fixture.Both(); fixture.script.TryAdvanceMagnarGuards();
        Require(fixture.ai.actions == 0 && fixture.door.changes == 0, "reentrant instance reset revokes old transition");
    }
    {
        Fixture fixture; fixture.ai.callback = [&] { fixture.script.TryAdvanceMagnarGuards(); fixture.Death(fixture.unrelated); };
        fixture.Both(); fixture.Completed();
    }
    {
        Fixture fixture; fixture.Death(fixture.first); fixture.Death(fixture.second);
        fixture.script.Initialize(); fixture.scenario.step = 2; fixture.owner.credits = fixture.ai.actions = fixture.door.changes = 0;
        fixture.first.alive = fixture.second.alive = true;
        fixture.script.OnCreatureCreate(&fixture.first); fixture.script.OnCreatureCreate(&fixture.second); fixture.script.OnCreatureCreate(&fixture.companion); fixture.script.OnGameObjectCreate(&fixture.door);
        fixture.script.TryAdvanceMagnarGuards(); Require(fixture.owner.credits == 0, "fresh instance requires fresh deaths");
        fixture.Both(); fixture.Completed();
    }
    std::cout << "PASS: extracted native guard identity/death/transition/500ms retry admission;8 recipient contexts,19 rejected/recovered contexts, shared scenarios, early/late/reset/respawn/reentrant cases\n";
#endif
}
