#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>
using uint32 = uint32_t;
struct ObjectGuid
{
    uint32 value = 0;
    bool IsEmpty() const { return value == 0; }
    void Clear() { value = 0; }
    bool operator==(ObjectGuid other) const { return value == other.value; }
};
constexpr uint32 CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1;
constexpr uint32 SCENE_LAUNCH = 1, SCENE_COMPLETE = 3;
constexpr uint32 REACT_PASSIVE = 0, UNIT_FIELD_FLAGS = 0, UNIT_FLAG_NOT_SELECTABLE = 1;
constexpr uint32 UNIT_FLAG_IMMUNE_TO_PC = 2, UNIT_FLAG_IMMUNE_TO_NPC = 4, UNIT_FLAG_NOT_ATTACKABLE_1 = 8;
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_ERROR };
uint32 warnings = 0;
#define TC_LOG_WARN(...) (++warnings)
struct Player;
struct Creature;
struct InstanceScript
{
    bool introCreditSent = false;
    uint32 generation = 1;
    uint32 GetData(uint32 identity) const { if (identity == 1060) return generation; if (identity != 1059) std::abort(); return introCreditSent; }
};
struct PlayerReference { Player* player; Player* getSource() const { return player; } };
struct Map
{
    uint32 identity = 1495, instance = 42;
    std::vector<PlayerReference> players;
    auto const& GetPlayers() const { return players; }
};
struct Player
{
    ObjectGuid guid{100};
    Map* map = nullptr;
    bool world = true, contact = true, alive = true, gm = false, teleport = false, rewarded = false;
    uint32 classId = 1, instanceId = 42, phase = 6999, quest = 3, sceneStatus = 0, casts = 0, packets = 0, removals = 0;
    uint32 faction = 0;
    float position = 10;
    bool aura = false, applyAura = true, sendScene = true;
    SpellCastResult castResult = SPELL_CAST_OK;
    std::function<void()> castCallback;
    ObjectGuid GetGUID() const { return guid; }
    bool IsInWorld() const { return world; }
    bool CanContact() const { return contact; }
    bool IsAlive() const { return alive; }
    bool isGameMaster() const { return gm; }
    bool IsBeingTeleported() const { return teleport; }
    uint32 getClass() const { return classId; }
    Map* GetMap() const { return map; }
    uint32 GetInstanceId() const { return instanceId; }
    bool InSamePhase(Creature*) const;
    bool IsWithinDistInMap(Creature*, float) const;
    bool GetQuestRewardStatus(uint32 identity) const { if (identity != 39191) std::abort(); return rewarded; }
    uint32 GetQuestStatus(uint32 identity) const { if (identity != 39191) std::abort(); return quest; }
    bool HasSceneStatus(uint32 identity, uint32 status) const { if (identity != 1059) std::abort(); return sceneStatus >= status; }
    bool HasAura(uint32 identity) const { if (identity != 189721) std::abort(); return aura; }
    void RemoveAurasDueToSpell(uint32 identity) { if (identity != 189721) std::abort(); if (aura) ++removals; aura = false; }
    SpellCastResult CastSpell(Player* target, uint32 identity, bool triggered)
    {
        if (target != this || identity != 189721 || !triggered) std::abort();
        ++casts;
        if (castResult == SPELL_CAST_OK)
        {
            aura = applyAura;
            if (sendScene && applyAura) { ++packets; sceneStatus = SCENE_LAUNCH; }
        }
        if (castCallback) castCallback();
        return castResult;
    }
};
struct Motion
{
    uint32 moves = 0;
    void MovePoint(uint32 identity, float coordinateX, float coordinateY, float coordinateZ)
    {
        if (identity != 6 || coordinateX != 4763.73f || coordinateY != 340.87f || coordinateZ != -37.19f) std::abort();
        ++moves;
    }
};
struct Creature
{
    Map* map = nullptr;
    InstanceScript* instanceScript = nullptr;
    bool world = true, alive = true, despawn = false, IsAIEnabled = true;
    uint32 entry = 96468, phase = 6999, stops = 0;
    void* ai = nullptr;
    Motion motion;
    Map* GetMap() const { return map; }
    bool IsInWorld() const { return world; }
    bool IsAlive() const { return alive; }
    bool IsDespawn() const { return despawn; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return map ? map->identity : 0; }
    uint32 GetInstanceId() const { return map ? map->instance : 0; }
    void* AI() const { return ai; }
    float GetDistance(Player* player) const { return std::abs(player->position); }
    void StopAttack() { ++stops; }
    void SetReactState(uint32) { }
    void SetFlag(uint32, uint32) { }
    Motion* GetMotionMaster() { return &motion; }
    InstanceScript* GetInstanceScript() const { return instanceScript; }
};
bool Player::InSamePhase(Creature* actor) const { return phase == actor->phase; }
bool Player::IsWithinDistInMap(Creature* actor, float distance) const { return map == actor->map && std::abs(position) <= distance; }
namespace ObjectAccessor
{
    Player* GetPlayer(Creature const& actor, ObjectGuid guid)
    {
        if (!actor.map) return nullptr;
        for (auto reference : actor.map->players)
            if (reference.player && reference.player->guid == guid && reference.player->map == actor.map && reference.player->world)
                return reference.player;
        return nullptr;
    }
}
struct Scenario { uint32 identity = 909, step = 2; uint32 GetScenarioId() const { return identity; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioManager { Scenario* scenario = nullptr; Scenario* GetScenario(uint32 instance) const { return instance == 42 ? scenario : nullptr; } } manager;
auto* sScenarioMgr = &manager;
struct SpellScene { uint32 SceneScriptPackageID = 1458, PlaybackFlags = 16, CustomDuration = 0, scriptID = 0; };
struct SpellManager
{
    SpellScene scene;
    bool present = true, events = false;
    SpellScene const* GetSpellScene(uint32 identity) const { if (identity != 1059) std::abort(); return present ? &scene : nullptr; }
    void* GetSceneTriggerEvent(uint32 identity) { if (identity != 1059) std::abort(); return events ? this : nullptr; }
} spellManager;
auto* sSpellMgr = &spellManager;
struct IntroAI
{
    Creature* me;
    std::shared_ptr<bool> magnarOfferToken, magnarStartToken;
    bool magnarArrivalPending = false;
    uint32 magnarStartInstance = 0;
    uint32 talks = 0;
    void Talk(uint32 identity) { if (identity != 14) std::abort(); ++talks; }
#include "Fields.inc"
#include "Context.inc"
#include "Intro.inc"
#include "Action.inc"
};
void Require(bool condition, char const* message) { if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); } }
struct Fixture
{
    Map map;
    Creature actor;
    Scenario scenario;
    InstanceScript instance;
    Player player, other;
    IntroAI script{&actor};
    Fixture()
    {
        actor.map = &map; actor.ai = &script; actor.instanceScript = &instance;
        player.map = other.map = &map; other.guid = {101}; other.classId = 3;
        map.players = {{&other}, {&player}};
        manager.scenario = &scenario; spellManager.present = true; spellManager.events = false; spellManager.scene = {}; warnings = 0;
    }
    void Start() { script.StartIntro(); script.UpdateMagnarIntro(1); }
    void Complete()
    {
        player.sceneStatus = SCENE_COMPLETE; player.aura = false;
        script.UpdateMagnarIntro(500);
        Require(script.GetData(1059) == 1 && script.talks == 0 && actor.motion.moves == 0 && !script.magnarIntroPending && scenario.step == 2, "scene completion publishes readiness without advancing or moving itself");
        script.StartIntro(); Require(script.talks == 0 && actor.motion.moves == 0, "ready intro at stage2 cannot begin chamber early");
        instance.introCreditSent = true; scenario.step = 3; script.StartIntro();
        Require(script.talks == 1 && actor.motion.moves == 1 && script.magnarChamberStarted, "only observed shared-instance credit plus stage3 releases chamber speech/movement");
        script.StartIntro(); script.UpdateMagnarIntro(500);
        Require(script.talks == 1 && actor.motion.moves == 1 && player.casts == 1, "successful intro cannot replay from duplicate action2");
    }
};
int main()
{
#ifdef BASELINE
    Fixture fixture; fixture.script.StartIntro();
    Require(fixture.script.talks == 1 && fixture.actor.motion.moves == 1 && fixture.player.casts == 0, "pinned native action2 walks/talks without any reveal scene");
    std::cout << "PASS: pinned missing camera/animation/sound launch and immediate chamber handoff reproduced\n";
#else
    for (uint32 faction : {0u, 1u})
        for (uint32 status : {QUEST_STATUS_INCOMPLETE, QUEST_STATUS_COMPLETE})
        {
            Fixture fixture; fixture.player.faction = faction; fixture.player.quest = status;
            fixture.script.UpdateMagnarIntro(500); Require(fixture.player.casts == 0, "no launch before actual guard action2");
            fixture.Start(); Require(fixture.player.casts == 1 && fixture.script.talks == 0 && fixture.actor.motion.moves == 0 && fixture.other.casts == 0, "launch native189721 only on eligible local owner");
            fixture.script.StartIntro(); fixture.script.UpdateMagnarIntro(499); fixture.script.UpdateMagnarIntro(1);
            Require(fixture.player.casts == 1 && fixture.actor.motion.moves == 0, "active scene waits at500ms cadence without refresh or early handoff");
            fixture.Complete();
        }
    for (uint32 invalid = 0; invalid < 21; ++invalid)
    {
        Fixture fixture;
        switch (invalid)
        {
            case 0: fixture.player.world = false; break;
            case 1: fixture.player.contact = false; break;
            case 2: fixture.player.alive = false; break;
            case 3: fixture.player.gm = true; break;
            case 4: fixture.player.teleport = true; break;
            case 5: fixture.player.classId = 3; break;
            case 6: fixture.player.map = nullptr; break;
            case 7: fixture.player.instanceId = 43; break;
            case 8: fixture.player.phase = 6985; break;
            case 9: fixture.player.position = 151; break;
            case 10: fixture.player.rewarded = true; break;
            case 11: fixture.player.quest = 0; break;
            case 12: fixture.player.quest = 5; break;
            case 13: spellManager.present = false; break;
            case 14: spellManager.scene.SceneScriptPackageID = 1; break;
            case 15: fixture.player.castResult = SPELL_FAILED_ERROR; break;
            case 16: fixture.player.applyAura = false; break;
            case 17: spellManager.scene.PlaybackFlags = 21; break;
            case 18: spellManager.scene.CustomDuration = 1; break;
            case 19: spellManager.scene.scriptID = 1; break;
            case 20: spellManager.events = true; break;
        }
        fixture.Start(); fixture.script.UpdateMagnarIntro(500);
        Require(fixture.actor.motion.moves == 0 && fixture.script.talks == 0 && fixture.script.magnarIntroPending, "invalid player/missing binding/failed cast retains native retry without chamber motion");
        Require(warnings <= 1, "retained error is diagnosed once, not every500ms");
        fixture.player.world = fixture.player.contact = fixture.player.alive = fixture.player.applyAura = true;
        fixture.player.gm = fixture.player.teleport = fixture.player.rewarded = false;
        fixture.player.classId = 1; fixture.player.map = &fixture.map; fixture.player.instanceId = 42; fixture.player.phase = 6999; fixture.player.position = 10; fixture.player.quest = 3;
        fixture.player.castResult = SPELL_CAST_OK; spellManager.present = true; spellManager.events = false; spellManager.scene = {};
        fixture.script.UpdateMagnarIntro(500);
        Require(fixture.player.aura && fixture.actor.motion.moves == 0, "restored services actually launch, not fabricate completion");
        fixture.player.sceneStatus = SCENE_COMPLETE; fixture.player.aura = false; fixture.script.UpdateMagnarIntro(500);
        Require(fixture.script.GetData(1059) == 1 && fixture.actor.motion.moves == 0, "failed start recovers readiness after real scene completion, still waits for native criterion");
    }
    for (uint32 scenario : {1068u, 1082u})
    {
        Fixture fixture; fixture.scenario.identity = scenario; fixture.Start();
        Require(fixture.player.casts == 0 && !fixture.script.magnarIntroPending, "shared Hunter/Paladin map cannot start Warrior scene");
    }
    for (uint32 invalid = 0; invalid < 9; ++invalid)
    {
        Fixture fixture;
        switch (invalid)
        {
            case 0: fixture.actor.world = false; break;
            case 1: fixture.actor.alive = false; break;
            case 2: fixture.actor.despawn = true; break;
            case 3: fixture.actor.IsAIEnabled = false; break;
            case 4: fixture.actor.ai = nullptr; break;
            case 5: fixture.actor.entry = 1; break;
            case 6: fixture.map.identity = 1; break;
            case 7: fixture.map.instance = 43; break;
            case 8: manager.scenario = nullptr; break;
        }
        fixture.Start(); Require(fixture.player.casts == 0 && fixture.actor.motion.moves == 0 && !fixture.script.magnarIntroPending, "missing/replaced/wrong actor context cannot queue scene");
    }
    for (uint32 stage : {0u, 1u, 3u, 4u, 5u, 6u})
    {
        Fixture fixture; fixture.scenario.step = stage; fixture.Start();
        Require(fixture.player.casts == 0 && !fixture.script.magnarIntroPending, "scene cannot start too early/late");
    }
    {
        Fixture fixture; fixture.scenario.step = 3; fixture.instance.introCreditSent = true;
        fixture.script.StartIntro();
        Require(fixture.player.casts == 0 && fixture.actor.motion.moves == 1 && fixture.script.magnarChamberStarted, "recreated actor after acknowledged shared transition resumes chamber without replaying or requiring lost local scene state");
        fixture.script.StartIntro(); Require(fixture.actor.motion.moves == 1, "recovered chamber claim is once-only");
    }
    {
        Fixture fixture; fixture.Start(); fixture.player.sceneStatus = SCENE_COMPLETE; fixture.player.aura = false;
        fixture.script.UpdateMagnarIntro(500); Require(fixture.script.GetData(1059) == 1, "ready before reset");
        fixture.script.CancelMagnarIntro(); Require(fixture.script.GetData(1059) == 0 && fixture.script.magnarIntroPending, "Reset revokes completed but not credited readiness and permits a fresh scene");
        fixture.script.UpdateMagnarIntro(500); Require(fixture.player.casts == 2 && fixture.actor.motion.moves == 0, "revoked ready attempt cannot move chamber");
    }
    {
        Fixture fixture; fixture.Start(); fixture.player.sceneStatus = SCENE_COMPLETE; fixture.player.aura = false;
        fixture.script.UpdateMagnarIntro(500); fixture.player.quest = 0;
        Require(fixture.script.GetData(1059) == 0, "late abandonment revokes readiness even before the next AI poll");
        fixture.script.UpdateMagnarIntro(500); fixture.player.quest = 3; fixture.script.UpdateMagnarIntro(500);
        Require(fixture.player.casts == 2 && fixture.script.GetData(1059) == 0, "abandoned ready scene must launch fresh before shared credit");
    }
    for (bool completed : {false, true})
    {
        Fixture fixture; fixture.Start();
        if (completed) { fixture.player.sceneStatus = SCENE_COMPLETE; fixture.player.aura = false; fixture.script.UpdateMagnarIntro(500); }
        ++fixture.instance.generation;
        Require(fixture.script.GetData(1059) == 0, "instance Initialize epoch immediately revokes pending or completed old-scene readiness");
        fixture.script.UpdateMagnarIntro(1);
        Require(!fixture.script.magnarIntroPending && !fixture.script.magnarIntroFinished && !fixture.player.aura && fixture.actor.motion.moves == 0, "old attempt cancels without credit or movement");
        fixture.Start(); Require(fixture.player.casts == 2 && fixture.script.GetData(1059) == 0, "fresh guarded attempt must replay camera rather than reuse old COMPLETE");
    }
    {
        Fixture fixture; fixture.Start(); fixture.Complete();
        fixture.script.magnarOfferToken = std::make_shared<bool>(false);
        fixture.script.magnarStartToken = std::make_shared<bool>(false);
        std::weak_ptr<bool> offer = fixture.script.magnarOfferToken, start = fixture.script.magnarStartToken;
        fixture.script.magnarArrivalPending = true; fixture.script.magnarStartInstance = 42;
        ++fixture.instance.generation; fixture.instance.introCreditSent = false; fixture.scenario.step = 2;
        fixture.Start(); Require(fixture.player.casts == 2 && fixture.actor.motion.moves == 1 && !fixture.script.magnarChamberStarted && fixture.script.GetData(1059) == 0, "new instance epoch also retires a previous chamber claim on the same AI, without replaying its movement before a fresh scene");
        Require(offer.expired() && start.expired() && !fixture.script.magnarArrivalPending && fixture.script.magnarStartInstance == 0, "actual scene producer retires old offer/start owners and arrival snapshot before a new epoch");
    }
    {
        Fixture fixture; fixture.player.sceneStatus = SCENE_COMPLETE; fixture.Start();
        Require(fixture.player.sceneStatus == SCENE_LAUNCH && fixture.actor.motion.moves == 0, "stale completion cannot acknowledge new cast"); fixture.Complete();
    }
    {
        Fixture fixture; fixture.player.aura = true; fixture.player.sceneStatus = SCENE_COMPLETE; fixture.Start();
        Require(fixture.player.casts == 0 && fixture.player.removals == 0 && fixture.actor.motion.moves == 0, "unowned preexisting aura/status is not claimed or canceled");
    }
    {
        Fixture fixture; fixture.Start(); fixture.player.aura = false; fixture.script.UpdateMagnarIntro(500);
        Require(fixture.player.casts == 2 && fixture.actor.motion.moves == 0, "native aura expiry without callback retries scene, not fake progress");
    }
    {
        Fixture fixture; fixture.Start(); fixture.player.quest = 0; fixture.script.UpdateMagnarIntro(500);
        Require(!fixture.player.aura && fixture.player.removals == 1 && fixture.actor.motion.moves == 0, "abandon cancels owned local scene and retains admission");
        fixture.player.quest = 3; fixture.script.UpdateMagnarIntro(500); Require(fixture.player.casts == 2, "reaccept can replay native scene");
    }
    {
        Fixture fixture; fixture.Start(); fixture.script.CancelMagnarIntro();
        Require(!fixture.player.aura && fixture.player.removals == 1 && fixture.script.magnarIntroPlayer.IsEmpty(), "Reset/destructor cleanup revokes owned local aura");
        fixture.script.UpdateMagnarIntro(500); Require(fixture.player.casts == 2, "pending guard handoff survives ordinary Reset");
    }
    {
        Fixture fixture; fixture.Start(); Map destination; fixture.player.map = &destination;
        fixture.script.UpdateMagnarIntro(500);
        Require(fixture.player.aura && fixture.player.removals == 0 && fixture.script.magnarIntroPlayer.IsEmpty() && fixture.actor.motion.moves == 0, "map-local cleanup never removes a possibly newer scene in another map; bounded old aura needs native transfer/expiry integration check");
        fixture.player.map = &fixture.map; fixture.player.aura = false; fixture.player.sceneStatus = SCENE_COMPLETE;
        fixture.script.UpdateMagnarIntro(500);
        Require(fixture.player.casts == 2 && fixture.actor.motion.moves == 0 && fixture.player.sceneStatus == SCENE_LAUNCH, "reentry after aura cleanup starts fresh rather than accepting stale complete");
    }
    {
        Fixture fixture; fixture.Start(); fixture.player.contact = false;
        fixture.script.UpdateMagnarIntro(500);
        Require(!fixture.player.aura && fixture.player.removals == 1 && fixture.actor.motion.moves == 0, "disconnected but map-local owner is cleaned without completion");
    }
    {
        Fixture fixture; fixture.player.castCallback = [&] { fixture.script.UpdateMagnarIntro(500); };
        fixture.Start(); Require(fixture.player.casts == 1 && fixture.actor.motion.moves == 0, "cast reentry cannot see stale completion or cast recursively");
    }
    {
        Fixture fixture; fixture.player.castCallback = [&] { fixture.script.CancelMagnarIntro(); };
        fixture.Start(); Require(!fixture.player.aura && fixture.script.magnarIntroPlayer.IsEmpty() && fixture.actor.motion.moves == 0, "reentrant Reset revokes old launch without retaining unowned aura");
    }
    {
        Fixture fixture; fixture.Start(); fixture.scenario.step = 4; fixture.script.UpdateMagnarIntro(1);
        Require(!fixture.player.aura && !fixture.script.magnarIntroPending && fixture.actor.motion.moves == 0, "scenario loss cancels before timer/old completion");
    }
    {
        Fixture fixture; fixture.player.sendScene = false; fixture.Start();
        Require(!fixture.player.aura && fixture.script.magnarIntroPlayer.IsEmpty() && fixture.actor.motion.moves == 0, "accepted aura without launch status is cleaned and retried");
    }
    std::cout << "PASS: extracted native intro queue/player admission/500ms retry/scene acknowledgement/cleanup;4 faction-status cases,21 invalid/recovered cases,9 invalid actors, shared/early/late/stale/reentrant/abandon/map-local transfer branches\n";
#endif
}
