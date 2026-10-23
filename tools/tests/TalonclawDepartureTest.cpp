#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
using uint32 = uint32_t;
using ObjectGuid = uint64_t;
enum { CLASS_HUNTER = 3, QUEST_STATUS_INCOMPLETE = 3, TEMPSUMMON_MANUAL_DESPAWN = 8, GAMEOBJECT_FIELD_FLAGS = 1, GO_FLAG_IN_USE = 1, GO_FLAG_NOT_SELECTABLE = 2 };
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED };
int warnings = 0;
template<class... Arguments> void RecordWarning(char const*, char const*, Arguments...) { ++warnings; }
#define TC_LOG_WARN(...) RecordWarning(__VA_ARGS__)
struct Position { float x, y, z, orientation; };
struct Player;
struct Map;
struct ActorAI {
    std::vector<uint32> lines;
    ObjectGuid recipient = 0;
    void Talk(uint32 line, ObjectGuid owner) { lines.push_back(line); recipient = owner; }
};
struct Creature {
    ObjectGuid guid = 0;
    bool IsAIEnabled = true, expired = false, alive = true, phase = true;
    ActorAI ai;
    ActorAI* AI() { return &ai; }
    bool IsAlive() const { return alive; }
    bool IsQuestObjectExpired() const { return expired; }
    void DespawnOrUnsummon() { expired = true; }
    ObjectGuid GetGUID() const { return guid; }
    float GetOrientation() const { return 1.89629f; }
    Position GetNearPosition(float distance, float angle) const {
        assert(distance == 2.0f);
        return {3878.35f + distance * std::cos(GetOrientation() + angle), 1636.92f + distance * std::sin(GetOrientation() + angle), -0.0612079f, GetOrientation()};
    }
};
struct GameObject {
    ObjectGuid guid = 0, owner = 1;
    Map* map = nullptr;
    bool expired = false, spawned = true, deleted = false;
    uint32 entry = 9000575, flags = 0;
    ObjectGuid GetGUID() const { return guid; }
    ObjectGuid GetOwnerGUID() const { return owner; }
    uint32 GetEntry() const { return entry; }
    Map* GetMap() const { return map; }
    bool IsQuestObjectExpired() const { return expired; }
    bool isSpawned() const { return spawned; }
    bool HasFlag(int, uint32 mask) const { return (flags & mask) != 0; }
    void Delete() { deleted = true; }
};
struct Map {
    ObjectGuid next = 10;
    std::map<ObjectGuid, std::unique_ptr<Creature>> actors;
    std::map<ObjectGuid, std::unique_ptr<GameObject>> objects;
    std::map<ObjectGuid, Player*> players;
    Creature* GetCreature(ObjectGuid guid) { auto found = actors.find(guid); return found == actors.end() || found->second->expired ? nullptr : found->second.get(); }
    GameObject* GetGameObject(ObjectGuid guid) { auto found = objects.find(guid); return found == objects.end() || found->second->deleted ? nullptr : found->second.get(); }
};
struct Scenario { uint32 id = 973, step = 13; uint32 GetScenarioId() const { return id; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioManager { Scenario scenario; bool present = true; Scenario* GetScenario(uint32) { return present ? &scenario : nullptr; } } scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct GameObjectTemplate { uint32 ScriptId = 42; };
struct ObjectManager {
    GameObjectTemplate object; bool present = true;
    GameObjectTemplate const* GetGameObjectTemplate(uint32 entry) { assert(entry == 9000575); return present ? &object : nullptr; }
    uint32 GetScriptId(char const* name) { assert(std::string(name) == "go_talonclaw_return_portal"); return 42; }
} objectManager;
auto* sObjectMgr = &objectManager;
struct CreatureTextEntry { uint32 BroadcastTextID = 106397; };
using CreatureTextGroup = std::vector<CreatureTextEntry>;
struct TextManager {
    CreatureTextGroup group{{}};
    CreatureTextGroup const* GetTextGroup(uint32 actor, uint32 id) { assert(actor == 100005 && id == 1); return &group; }
} textManager;
auto* sCreatureTextMgr = &textManager;
struct Player {
    Map worldMap;
    bool world = true, alive = true, teleporting = false, rewarded = false, owned = true, scene = false;
    bool phase = true, visible = true, los = true, failActor = false, failPortal = false;
    uint32 playerClass = 3, mapId = 1511, instance = 42, status = 3, flight = 1, claim = 1, returned = 0;
    uint32 summons = 0, portals = 0, casts = 0, cancels = 0;
    ObjectGuid guid = 1;
    float distance = 0, clickDistance = 2;
    SpellCastResult castResult = SPELL_CAST_OK;
    Player() { worldMap.players[guid] = this; scenarioManager = {}; objectManager = {}; textManager = {}; }
    bool IsInWorld() const { return world; }
    bool IsAlive() const { return alive; }
    bool IsBeingTeleported() const { return teleporting; }
    uint32 getClass() const { return playerClass; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    bool GetQuestRewardStatus(uint32 id) const { assert(id == 40385); return rewarded; }
    uint32 GetQuestStatus(uint32 id) const { assert(id == 40385); return status; }
    uint32 GetQuestObjectiveData(uint32 id, uint32 objective) const { assert(id == 40385); return objective == 100111 ? flight : objective == 100112 ? claim : returned; }
    bool HasItemCount(uint32 item, uint32 count, bool bank) const { assert(item == 128808 && count == 1 && bank); return owned; }
    bool HasAura(uint32 spell) const { assert(spell == 205589); return scene; }
    float GetExactDist(float, float, float) const { return distance; }
    float GetExactDist(GameObject*) const { return clickDistance; }
    ObjectGuid GetGUID() const { return guid; }
    uint32 GetGUIDLow() const { return uint32(guid); }
    Map* GetMap() { return &worldMap; }
    Creature* SummonCreature(uint32 entry, float x, float y, float z, float orientation, int type, uint32 duration, ObjectGuid viewer) {
        assert(entry == 100005 && std::abs(x - 3878.35f) < 0.01f && std::abs(y - 1636.92f) < 0.01f && std::abs(z + 0.0612079f) < 0.001f);
        assert(orientation == 1.89629f && type == 8 && duration == 0 && viewer == guid);
        ++summons; if (failActor) return nullptr;
        auto actor = std::make_unique<Creature>(); actor->guid = worldMap.next++;
        actor->ai.lines.push_back(0);
        auto* result = actor.get(); worldMap.actors[actor->guid] = std::move(actor); return result;
    }
    GameObject* SummonGameObject(uint32 entry, Position position, float, float first, float second, float third, float fourth, uint32 duration, ObjectGuid viewer) {
        assert(entry == 9000575 && first == 0 && second == 0 && std::abs(third * third + fourth * fourth - 1) < 0.0001f);
        assert(duration == 300 && viewer == guid && std::abs(position.z + 0.0612079f) < 0.001f);
        ++portals; if (failPortal) return nullptr;
        auto object = std::make_unique<GameObject>(); object->guid = worldMap.next++; object->owner = guid; object->map = &worldMap;
        auto* result = object.get(); worldMap.objects[object->guid] = std::move(object); return result;
    }
    void BindQuestObject(uint32 quest, Creature* actor) { assert(quest == 40385 && actor); }
    void BindQuestObject(uint32 quest, GameObject* object) { assert(quest == 40385 && object); }
    void CancelQuestObjects(uint32 quest) {
        assert(quest == 40385); ++cancels;
        for (auto& actor : worldMap.actors) actor.second->expired = true;
        for (auto& object : worldMap.objects) object.second->expired = object.second->deleted = true;
    }
    bool InSamePhase(GameObject*) const { return phase; }
    bool InSamePhase(Creature* actor) const { return actor->phase; }
    bool canSeeOrDetect(GameObject*) const { return visible; }
    bool IsWithinLOSInMap(GameObject*) const { return los; }
    SpellCastResult CastSpell(Player* target, uint32 spell, bool triggered) {
        assert(target == this && spell == 190598 && triggered); ++casts;
        if (castResult == SPELL_CAST_OK) teleporting = true;
        return castResult;
    }
};
namespace ObjectAccessor { Player* GetPlayer(GameObject& object, ObjectGuid guid) { auto found = object.map->players.find(guid); return found == object.map->players.end() ? nullptr : found->second; } }
struct PlayerScript {
    explicit PlayerScript(char const*) {}
    virtual void OnUpdate(Player*, uint32) {}
    virtual void OnLogout(Player*) {}
    virtual void OnMapChanged(Player*) {}
};
struct GameObjectAI {
    GameObject* go;
    explicit GameObjectAI(GameObject* object) : go(object) {}
    virtual ~GameObjectAI() = default;
    virtual void UpdateAI(uint32) {}
};
struct GameObjectScript {
    explicit GameObjectScript(char const*) {}
    virtual bool OnGossipHello(Player*, GameObject*) { return false; }
    virtual GameObjectAI* GetAI(GameObject*) const { return nullptr; }
};
#include "Departure.inc"
int main() {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    {
        Player player; player_talonclaw_departure script;
        player.scene = true; script.OnUpdate(&player, 100000); assert(player.summons == 0);
        player.scene = false; script.OnUpdate(&player, 1); assert(player.summons == 1 && player.portals == 0);
        Creature* actor = player.worldMap.actors.begin()->second.get();
        script.OnUpdate(&player, 9999); assert(actor->ai.lines.size() == 1);
        script.OnUpdate(&player, 1); assert(actor->ai.lines == std::vector<uint32>({0, 1}));
        assert(actor->ai.recipient == player.guid && player.portals == 0);
        script.OnUpdate(&player, 5999); assert(player.portals == 0);
        script.OnUpdate(&player, 1); assert(player.portals == 1);
        script.OnUpdate(&player, UINT32_MAX); assert(player.portals == 1 && actor->ai.lines.size() == 2);
        GameObject* portal = player.worldMap.objects.begin()->second.get();
        go_talonclaw_return_portal handler;
        player.castResult = SPELL_FAILED; handler.OnGossipHello(&player, portal);
        assert(player.casts == 1 && !portal->deleted && player.returned == 0);
        player.castResult = SPELL_CAST_OK; handler.OnGossipHello(&player, portal);
        handler.OnGossipHello(&player, portal); assert(player.casts == 2 && player.teleporting && player.returned == 0);
        std::unique_ptr<GameObjectAI> ai(handler.GetAI(portal)); ai->UpdateAI(1); assert(portal->deleted);
        script.OnMapChanged(&player); script.OnLogout(&player); assert(player.cancels == 1 && actor->expired);
    }
    for (int gate = 0; gate < 18; ++gate) {
        Player player; player_talonclaw_departure script;
        switch (gate) {
            case 0: player.world = false; break;
            case 1: player.alive = false; break;
            case 2: player.teleporting = true; break;
            case 3: player.playerClass = 1; break;
            case 4: player.mapId = 1220; break;
            case 5: player.rewarded = true; break;
            case 6: player.status = 0; break;
            case 7: player.flight = 0; break;
            case 8: player.claim = 0; break;
            case 9: player.returned = 1; break;
            case 10: player.owned = false; break;
            case 11: player.scene = true; break;
            case 12: player.distance = 61; break;
            case 13: scenarioManager.present = false; break;
            case 14: scenarioManager.scenario.id = 1068; break;
            case 15: scenarioManager.scenario.step = 11; break;
            case 16: objectManager.object.ScriptId = 999; break;
            case 17: textManager.group.front().BroadcastTextID = 999; break;
        }
        script.OnUpdate(&player, UINT32_MAX); assert(player.summons == 0);
    }
    {
        Player player; player_talonclaw_departure script; player.failActor = true;
        int before = warnings;
        for (int attempt = 0; attempt < 35; ++attempt) script.OnUpdate(&player, 1000);
        assert(player.summons == 30 && warnings == before + 2);
        script.OnLogout(&player); player.failActor = false; script.OnUpdate(&player, 1);
        assert(player.summons == 31);
        player.claim = 0; script.OnUpdate(&player, 1); assert(player.cancels == 2);
    }
    {
        Player player; player_talonclaw_departure script; script.OnUpdate(&player, 1);
        script.OnUpdate(&player, 10000); player.failPortal = true; script.OnUpdate(&player, 6000);
        script.OnUpdate(&player, 999); assert(player.portals == 1);
        player.failPortal = false; script.OnUpdate(&player, 1); assert(player.portals == 2);
        player.worldMap.objects.begin()->second->deleted = true;
        script.OnUpdate(&player, 1); assert(player.portals == 3);
        player.worldMap.actors.begin()->second->expired = true;
        script.OnUpdate(&player, 1); assert(player.summons == 2);
        player.instance = 43; script.OnUpdate(&player, 1); assert(player.cancels == 1 && player.summons == 3);
        player.worldMap.actors.rbegin()->second->alive = false;
        script.OnUpdate(&player, 10000); assert(player.summons == 4);
        player.worldMap.actors.rbegin()->second->phase = false;
        script.OnUpdate(&player, 10000); assert(player.summons == 5);
    }
    for (int gate = 0; gate < 8; ++gate) {
        Player player; GameObject object; object.map = &player.worldMap;
        switch (gate) {
            case 0: object.owner = 2; break;
            case 1: object.expired = true; break;
            case 2: object.spawned = false; break;
            case 3: player.phase = false; break;
            case 4: player.visible = false; break;
            case 5: player.los = false; break;
            case 6: player.clickDistance = 6; break;
            case 7: object.flags = GO_FLAG_NOT_SELECTABLE; break;
        }
        assert(TalonclawDeparture::Use(&player, &object) && player.casts == 0);
    }
    assert(TalonclawDeparture::Use(nullptr, nullptr));
    std::cout << "PASS: extracted departure controller and portal scripts; post-scene personal actors, delayed native speech, real-click teleport/no credit, failed cast and summon retry, admission, expiry, instance/re-entry/logout cleanup\n";
}
