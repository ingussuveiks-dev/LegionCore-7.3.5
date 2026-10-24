#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <memory>
using uint32 = uint32_t;
using uint64 = uint64_t;
using Seconds = uint32;
using LootState = uint32;
#ifdef TITANSTRIKE
constexpr uint32 PickupEntry = 249812, PickupMap = 1609, PickupQuest = 42185, PickupItem = 128861;
constexpr uint32 PickupActor = 106745, PickupSource = 103154, PickupScenario = 1099, PickupStep = 4;
constexpr float PickupRadius = 6.0f;
#else
constexpr uint32 PickupEntry = 248834, PickupMap = 1511, PickupQuest = 40385, PickupItem = 128808;
constexpr uint32 PickupActor = 100310, PickupSource = 99480, PickupScenario = 973, PickupStep = 12;
constexpr float PickupRadius = 3.0f;
#endif
enum { CLASS_HUNTER = 3, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1, GO_JUST_DEACTIVATED = 3, GO_STATE_READY = 1, SMART_EVENT_GO_STATE_CHANGED = 70 };
struct ObjectGuid {
    uint32 entry = 0, counter = 0, kind = 0;
    static const ObjectGuid Empty;
    bool IsEmpty() const { return !counter; }
    bool IsCreatureOrVehicle() const { return kind == 1 || kind == 2; }
    uint32 GetEntry() const { return entry; }
    bool operator==(ObjectGuid other) const { return entry == other.entry && counter == other.counter && kind == other.kind; }
};
const ObjectGuid ObjectGuid::Empty{};
using GuidUnorderedSet = std::vector<ObjectGuid>;
struct GameObject;
struct Creature;
struct TempSummon;
std::vector<std::string> order;
struct BasicEvent { virtual ~BasicEvent() = default; virtual bool Execute(uint64, uint32) = 0; };
#include "DelayEvent.inc"
struct EventProcessor {
    std::unique_ptr<BasicEvent> event;
    uint64 now = 0, when = 0;
    uint32 schedules = 0;
    void KillAllEvents(bool force) { assert(!force); event.reset(); }
    uint64 CalculateTime(uint32 delay) const { return now + delay; }
    void AddEvent(BasicEvent* value, uint64 time) { assert(!event); ++schedules; event.reset(value); when = time; order.push_back("schedule"); }
    void Update(uint32 diff) { now += diff; if (event && now >= when) { auto current = std::move(event); assert(current->Execute(now, diff)); } }
};
struct Unit;
struct Player;
struct GameObjectTemplate {};
struct Position {
    float positionX, positionY, positionZ, orientation;
    Position(float initialX, float initialY, float initialZ, float initialOrientation) : positionX(initialX), positionY(initialY), positionZ(initialZ), orientation(initialOrientation) {}
};
namespace G3D { struct Quat { Quat(float, float, float, float) {} }; }
enum class HighGuid { GameObject };
struct Map {
    std::vector<GameObject*> objects;
    ObjectGuid expected;
    void AddToMap(GameObject*);
};
struct WorldObject {
    ObjectGuid identity{PickupSource, 10, 1};
    Map* worldMap = nullptr;
    uint32 mapId = PickupMap;
    bool world = true, phase = true, scope = true, playerKind = false, creatureKind = true;
    ObjectGuid GetGUID() const { return identity; }
    bool IsInWorld() const { return world; }
    Map* GetMap() const { return worldMap; }
    uint32 GetMapId() const { return mapId; }
    bool IsPlayer() const { return playerKind; }
    bool IsCreature() const { return creatureKind; }
    Unit* ToUnit();
    uint32 GetPhaseMask() const { return 1; }
    int GetPhases() const { return 1; }
    int GetTransport() const { return 0; }
    bool InSamePhase(WorldObject const* other) const { return phase && other->phase; }
    GameObject* SummonGameObject(uint32, float, float, float, float, float, float, float, float, uint32, ObjectGuid, GuidUnorderedSet*, bool = true);
};
struct Unit : WorldObject {
    virtual ~Unit() = default;
    virtual Player* ToPlayer() { return nullptr; }
    uint32 ownedObjects = 0;
    void AddGameObject(GameObject*);
};
Unit* WorldObject::ToUnit() { return static_cast<Unit*>(this); }
struct Player : Unit {
    bool alive = true, teleporting = false, rewarded = false, owned = true;
    uint32 playerClass = CLASS_HUNTER, quest = QUEST_STATUS_INCOMPLETE, instance = 42;
    Player* ToPlayer() override { return this; }
    bool IsAlive() const { return alive; }
    bool IsBeingTeleported() const { return teleporting; }
    uint32 getClass() const { return playerClass; }
    uint32 GetInstanceId() const { return instance; }
    bool GetQuestRewardStatus(uint32 id) const { assert(id == PickupQuest); return rewarded; }
    uint32 GetQuestStatus(uint32 id) const { assert(id == PickupQuest); return quest; }
    bool HasItemCount(uint32 item, uint32 count, bool bank) const { assert(item == PickupItem && count == 1 && bank); return owned; }
};
struct Creature : Unit {
    uint32 entry = PickupActor, removals = 0;
    float distance = PickupRadius - 0.1f;
    bool visible = true, m_despawn = false;
    virtual TempSummon* ToTempSummon() { return nullptr; }
    uint32 GetEntry() const { return entry; }
    bool IsVisible() const { return visible; }
    bool IsDespawn() const { return m_despawn; }
    float GetExactDist(GameObject const*) const { return distance; }
    void SetVisible(bool value) { assert(!value); visible = value; order.push_back("hide"); }
    void DespawnOrUnsummon(uint32 = 0, Seconds const& = 0);
    void ForcedDespawn(uint32, Seconds const&) { assert(false); }
};
struct TempSummon : Creature {
    ObjectGuid summoner{PickupSource, 10, 1};
    bool onUnload = false;
    EventProcessor m_Events;
    TempSummon* ToTempSummon() override { return this; }
    ObjectGuid GetSummonerGUID() const { return summoner; }
    void UnSummon(uint32 msTime = 0);
    void CompleteUnSummon() { onUnload = true; visible = false; world = false; ++removals; order.push_back("despawn"); }
};
#include "Despawn.inc"
#include "UnSummonDelay.inc"
#include "DelayExecute.inc"
struct Scenario { uint32 id = PickupScenario, step = PickupStep; uint32 GetScenarioId() const { return id; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioManager { Scenario scenario; bool present = true; Scenario* GetScenario(uint32 instance) { return present && instance == 42 ? &scenario : nullptr; } } scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct NativeScript {
    uint32 calls = 0;
    void ProcessEventsFor(uint32 event, Unit*, uint32 state) {
        assert(event == 70); ++calls; order.push_back("smart");
        if (state == GO_JUST_DEACTIVATED && scenarioManager.scenario.step == PickupStep) scenarioManager.scenario.step = PickupStep + 1;
    }
};
struct SmartGameObjectAI { NativeScript script; NativeScript* GetScript() { return &script; } void OnStateChanged(uint32, Unit*); };
#include "OnStateChanged.inc"
struct GameObject : WorldObject {
    ObjectGuid owner;
    uint32 entry = PickupEntry, m_lootState = 0;
    bool spawnedByDefault = true;
    std::list<Creature*> candidates;
    std::vector<ObjectGuid> viewers;
    SmartGameObjectAI ai;
    GameObject() { creatureKind = false; }
    virtual ~GameObject() = default;
#include "Summoner.inc"
    uint32 GetEntry() const { return entry; }
    uint32 GetGoType() const { return 3; }
    uint32 getLootState() const { return m_lootState; }
    SmartGameObjectAI* AI() { return &ai; }
    void SetLootState(LootState, Unit*);
    bool Create(uint32, uint32 id, Map* target, uint32, Position, G3D::Quat, uint32, uint32) { entry = id; worldMap = target; return true; }
    bool InheritQuestObjectScope(WorldObject const* source) { return source->scope; }
    void SetPhaseId(int, bool) {}
    void SetTratsport(int) {}
    void SetRespawnTime(uint32) {}
    void SetSpawnedByDefault(bool value) { spawnedByDefault = value; }
    void AddPlayerInPersonnalVisibilityList(ObjectGuid viewer) { viewers.push_back(viewer); }
    void AddPlayersInPersonnalVisibilityList(GuidUnorderedSet const& list) { viewers.insert(viewers.end(), list.begin(), list.end()); }
    void GetCreatureListWithEntryInGrid(std::list<Creature*>& output, uint32 id, float range) const { assert(id == PickupActor && range == PickupRadius); output = candidates; }
};
struct StaticTransport : GameObject {};
void Unit::AddGameObject(GameObject* object) { ++ownedObjects; object->owner = identity; object->spawnedByDefault = false; }
void Map::AddToMap(GameObject* object) { assert(object->GetSummonerGUID() == expected); objects.push_back(object); }
struct Generator { uint32 Generate() const { return 1; } } generator;
struct ObjectManager {
    bool present = true, transport = false;
    GameObjectTemplate definition;
    GameObjectTemplate const* GetGameObjectTemplate(uint32) const { return present ? &definition : nullptr; }
    bool IsStaticTransport(uint32) const { return transport; }
    template<HighGuid> Generator* GetGenerator() const { return &generator; }
} objectManager;
auto* sObjectMgr = &objectManager;
#define TC_LOG_ERROR(...) ((void)0)
#include "SummonGameObject.inc"
namespace ArtifactAcquisition {
#include "Active.inc"
bool Use(Player*, GameObject*) { return false; }
}
#include "Retire.inc"
#include "TitanstrikeRetire.inc"
struct GameObjectScript {
    explicit GameObjectScript(char const*) {}
    virtual bool OnGossipHello(Player*, GameObject*) { return false; }
    virtual void OnLootStateChanged(GameObject*, uint32, Unit*) {}
};
#include "Binding.inc"
go_artifact_acquisition binding;
struct ScriptMgr { void OnGameObjectLootStateChanged(GameObject*, uint32, Unit*); } scriptManager;
auto* sScriptMgr = &scriptManager;
#define ASSERT(value) assert(value)
#define GET_SCRIPT(kind,id,name) kind* name = &binding
#include "ScriptDispatch.inc"
#include "SetLootState.inc"
struct Loot { bool empty = false; bool isLooted() const { return empty; } };
enum { GAMEOBJECT_TYPE_FISHINGNODE = 17 };
bool NativeCompletedLoot(Loot* loot, GameObject* go) {
#include "CompletedLootPredicate.inc"
}
struct Pickup {
    Map map;
    Player player;
    TempSummon actor;
    GameObject object;
    Pickup() {
        scenarioManager = {}; order.clear();
        object.worldMap = player.worldMap = actor.worldMap = &map;
        object.SetSummonerGUID(actor.summoner);
        object.candidates.push_back(&actor);
    }
    void Finish() { object.SetLootState(GO_JUST_DEACTIVATED, &player); }
};
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    {
        Map map; Unit source; source.worldMap = &map;
        for (bool player : {false, true}) for (uint32 duration : {0u, 3600u}) for (bool creator : {false, true}) {
            source.playerKind = player; source.creatureKind = !player; source.ownedObjects = 0;
            source.identity = {player ? 0u : PickupSource, 10, player ? 4u : 1u}; map.expected = source.identity;
            auto object = source.SummonGameObject(PickupEntry, 0, 0, 0, 0, 0, 0, 0, 0, duration, {}, nullptr, creator);
            assert(object && object->GetSummonerGUID() == source.identity && !object->spawnedByDefault);
            bool owned = creator && (player || !duration);
            assert(source.ownedObjects == uint32(owned) && (object->owner == source.identity) == owned);
            delete object;
        }
        WorldObject objectSource; objectSource.creatureKind = false; objectSource.worldMap = &map; objectSource.identity = {248834, 20, 3}; map.expected = objectSource.identity;
        objectManager.transport = true;
        auto transport = objectSource.SummonGameObject(123, 0, 0, 0, 0, 0, 0, 0, 0, 30, {0, 5, 4}, nullptr);
        assert(dynamic_cast<StaticTransport*>(transport) && transport->owner.IsEmpty() && transport->viewers.size() == 1); delete transport;
        objectManager.transport = false; objectSource.scope = false;
        assert(!objectSource.SummonGameObject(123, 0, 0, 0, 0, 0, 0, 0, 0, 30, {}, nullptr));
        GameObject staticObject; assert(staticObject.GetSummonerGUID().IsEmpty());
    }
    {
        Pickup pickup; Loot loot;
        assert(!NativeCompletedLoot(&loot, &pickup.object));
        pickup.object.SetLootState(1, &pickup.player);
        assert(pickup.actor.visible && !pickup.actor.removals);
        loot.empty = true; assert(NativeCompletedLoot(&loot, &pickup.object));
        pickup.Finish(); assert(pickup.actor.IsDespawn() && pickup.actor.visible && pickup.actor.removals == 0);
        assert((order == std::vector<std::string>{"smart", "smart", "schedule"}));
        assert(scenarioManager.scenario.step == PickupStep + 1);
        pickup.Finish(); assert(pickup.actor.m_Events.schedules == 1);
        pickup.actor.m_Events.Update(999); assert(pickup.actor.visible && pickup.actor.removals == 0);
        pickup.actor.m_Events.Update(1); assert(!pickup.actor.visible && pickup.actor.removals == 1);
        pickup.Finish(); assert(pickup.actor.removals == 1);
    }
    for (uint32 failure = 0; failure < 17; ++failure) {
        Pickup pickup; Map other;
        switch (failure) {
            case 0: pickup.object.entry = 248419; break;
            case 1: pickup.object.mapId = 1495; break;
            case 2: pickup.player.world = false; break;
            case 3: pickup.player.alive = false; break;
            case 4: pickup.player.teleporting = true; break;
            case 5: pickup.player.playerClass = 1; break;
            case 6: pickup.player.worldMap = &other; break;
            case 7: pickup.player.phase = false; break;
            case 8: pickup.player.quest = 0; break;
            case 9: pickup.player.rewarded = true; break;
            case 10: pickup.player.owned = false; break;
            case 11: scenarioManager.present = false; break;
            case 12: scenarioManager.scenario.id = 972; break;
            case 13: scenarioManager.scenario.step = PickupStep - 1; break;
            case 14: pickup.object.SetSummonerGUID({}); break;
            case 15: pickup.object.SetSummonerGUID({99999, 10, 1}); break;
            case 16: pickup.object.SetSummonerGUID({PickupSource, 10, 4}); break;
        }
        pickup.Finish(); assert(pickup.actor.visible && !pickup.actor.removals && !pickup.actor.m_Events.schedules);
    }
    {
        Pickup pickup;
        binding.OnLootStateChanged(nullptr, 3, &pickup.player);
        binding.OnLootStateChanged(&pickup.object, 3, &pickup.player);
        pickup.object.SetLootState(3, nullptr);
        Unit pet; pickup.object.SetLootState(3, &pet);
        assert(pickup.actor.visible && !pickup.actor.removals && !pickup.actor.m_Events.schedules);
    }
    {
        Pickup pickup; Map other;
        TempSummon actors[7]; Creature staticActor; staticActor.worldMap = &pickup.map;
        pickup.object.candidates.push_back(nullptr);
        pickup.object.candidates.push_back(&staticActor);
        for (auto& actor : actors) { actor.worldMap = &pickup.map; pickup.object.candidates.push_back(&actor); }
        actors[0].summoner.counter = 11;
        actors[1].phase = false;
        actors[2].worldMap = &other;
        actors[3].distance = PickupRadius + 0.1f;
        actors[4].world = false;
        actors[5].m_despawn = true;
        actors[6].entry = 100005;
        pickup.Finish(); pickup.actor.m_Events.Update(1000); assert(pickup.actor.removals == 1);
        for (auto const& actor : actors) assert(actor.removals == 0 && actor.m_Events.schedules == 0);
        assert(staticActor.removals == 0);
    }
    {
        Pickup pickup;
        pickup.object.SetSummonerGUID({PickupSource, 11, 1});
        pickup.Finish(); assert(pickup.actor.removals == 0 && pickup.actor.m_Events.schedules == 0);
        pickup.actor.summoner.counter = 11;
        pickup.player.quest = QUEST_STATUS_COMPLETE;
        pickup.Finish(); pickup.actor.m_Events.Update(1000); assert(pickup.actor.removals == 1);
    }
    {
        Pickup pickup; pickup.object.candidates.clear(); pickup.Finish(); assert(pickup.actor.removals == 0 && pickup.actor.m_Events.schedules == 0);
        pickup.object.candidates.push_back(&pickup.actor); pickup.player.quest = 0;
        pickup.Finish(); assert(pickup.actor.removals == 0 && pickup.actor.m_Events.schedules == 0);
        pickup.player.quest = QUEST_STATUS_INCOMPLETE;
        pickup.Finish(); pickup.actor.m_Events.Update(1000); assert(pickup.actor.removals == 1);
    }
#ifdef TITANSTRIKE
    {
        Pickup pickup;
        scenarioManager.scenario.step = 6;
        pickup.Finish(); assert(!pickup.actor.m_Events.schedules);
        scenarioManager.scenario.step = 4;
        pickup.actor.distance = 4.82f;
        pickup.Finish(); assert(pickup.actor.m_Events.schedules == 1);
    }
#endif
    std::cout << "PASS: native summon provenance and completed-loot dispatch preserve SmartAI; scoped prop " << PickupActor
        << " retires once after1000ms, with invalid/replay/abandon/re-entry/pet/null and shared-object isolation\n";
}
