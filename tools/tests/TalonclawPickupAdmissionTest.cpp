#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
using uint32 = uint32_t;
using int64 = int64_t;
using QuestStatus = uint32;
uint32 expectedQuest = 40385, expectedMap = 1511, expectedScenario = 973, firstStage = 12, expectedItem = 128808;
uint32 lastStage = 13, eventEntry = 248419, eventSource = 1;
uint32 expectedClassMask = 4;
enum { GAMEOBJECT_TYPE_CHEST = 3, GAMEOBJECT_TYPE_FISHINGHOLE = 25,
    GAMEOBJECT_TYPE_GATHERING_NODE = 50, GAMEOBJECT_TYPE_CHALLENGE_MODE_REWARD = 51 };
struct GameObjectTemplate {
    uint32 type = GAMEOBJECT_TYPE_CHEST;
    struct { uint32 chestLoot = 0; } chest, fishingHole, gatheringNode, challengeModeReward;
    uint32 GetLootId() const;
};
#include "GetLootId.inc"
enum { CONDITION_ITEM = 2, CONDITION_CLASS = 15, CONDITION_MAPID = 22, CONDITION_SCENARION_STEP = 44,
    CONDITION_QUESTTAKEN = 9, CONDITION_QUEST_COMPLETE = 28, CONDITION_QUESTREWARDED = 8, CONDITION_ALIVE = 36,
    QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1, QUEST_STATUS_REWARDED = 6 };
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(value) assert(value)
struct Player;
struct InstanceMap { uint32 id = 42; uint32 GetInstanceId() const { return id; } } instanceMap;
struct Map { bool instanced = true; InstanceMap* ToInstanceMap() { return instanced ? &instanceMap : nullptr; } } worldMap;
struct Unit {
    virtual ~Unit() = default;
    uint32 mapId = expectedMap, classMask = expectedClassMask;
    bool alive = true;
    bool IsAlive() const { return alive; }
    bool IsQuestObjectExpired() const { return false; }
    virtual Player* ToPlayer() { return nullptr; }
    Unit* ToUnit() { return this; }
    bool IsCreature() const { return false; }
    bool isPet() const { return false; }
    Unit* GetOwner() const { return nullptr; }
    struct Threat { Unit* getHostilTarget() const { return nullptr; } } threat;
    Threat& getThreatManager() { return threat; }
    uint32 getClassMask() const { return classMask; }
    uint32 GetMapId() const { return mapId; }
    bool InInstance() const { return worldMap.instanced; }
    Map* GetMap() const { return &worldMap; }
};
struct Player : Unit {
    uint32 quest = expectedQuest, status = 3;
    bool rewarded = false, owned = true, bank = false;
    Player* ToPlayer() override { return this; }
    uint32 GetGUID() const { return 7; }
    QuestStatus GetQuestStatus(uint32 id) const { return quest == id ? status : 0; }
    bool GetQuestRewardStatus(uint32 id) const { return id == quest && rewarded; }
    bool HasItemCount(uint32 item, uint32 amount, bool includeBank) const { assert(item == expectedItem && amount == 1); return owned || (includeBank && bank); }
};
struct Scenario { uint32 id = expectedScenario, step = firstStage; uint32 GetScenarioId() const { return id; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioManager { Scenario scenario; bool present = true; Scenario* GetScenario(uint32 id) { return present && id == 42 ? &scenario : nullptr; } } scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct Condition;
using WorldObject = Unit;
struct ConditionSourceInfo { Unit* object; explicit ConditionSourceInfo(Unit* value, Unit* = nullptr) : object(value) {} };
struct Condition {
    uint32 ElseGroup = 0, ConditionType = 0, ConditionValue1 = 0, ConditionValue2 = 0, ConditionValue3 = 0, ReferenceId = 0;
    bool NegativeCondition = false;
    bool isLoaded() const { return true; }
    bool Meets(ConditionSourceInfo& sourceInfo) {
        Unit* object = sourceInfo.object;
        if (!object) return false;
        bool condMeets = false;
        switch (ConditionType) {
#include "Conditions.inc"
            default: assert(false);
        }
        return NegativeCondition ? !condMeets : condMeets;
    }
};
using ConditionList = std::vector<Condition*>;
using ConditionReferenceContainer = std::map<uint32, ConditionList>;
using ConditionTypeContainer = std::map<uint32, ConditionList>;
using SmartEventConditionContainer = std::map<std::pair<int32_t, uint32>, ConditionTypeContainer>;
struct ConditionMgr {
    ConditionReferenceContainer ConditionReferenceStore;
    SmartEventConditionContainer SmartEventConditionStore;
    bool IsObjectMeetToConditionList(ConditionSourceInfo&, ConditionList const&) const;
    bool IsObjectMeetingSmartEventConditions(int64, uint32, uint32, Unit*, WorldObject*) const;
    bool IsObjectMeetToConditions(ConditionSourceInfo& sourceInfo, ConditionList const& conditions) const {
        return conditions.empty() || IsObjectMeetToConditionList(sourceInfo, conditions);
    }
    bool IsObjectMeetToConditions(Unit* object, ConditionList const& conditions) const {
        if (conditions.empty()) return true;
        ConditionSourceInfo sourceInfo(object);
        return IsObjectMeetToConditionList(sourceInfo, conditions);
    }
} conditionManager;
auto* sConditionMgr = &conditionManager;
#include "ElseGroups.inc"
#include "SmartEventConditions.inc"
enum SMART_EVENT { SMART_EVENT_LINK = 61, SMART_EVENT_GOSSIP_HELLO = 64, SMART_EVENT_GO_STATE_CHANGED = 70 };
struct SpellInfo {};
struct GameObject {};
struct SmartScriptHolder {
    int64 entryOrGuid = 247690;
    uint32 event_id = 0, source_type = 1;
    SMART_EVENT eventType = SMART_EVENT_GO_STATE_CHANGED;
    SMART_EVENT GetEventType() const { return eventType; }
};
struct SmartScript {
    std::vector<SmartScriptHolder> mEvents;
    WorldObject* base = nullptr;
    uint32 actions = 0;
    WorldObject* GetBaseObject() const { return base; }
    void ProcessEvent(SmartScriptHolder&, Unit*, uint32, uint32, bool, SpellInfo const*, GameObject*) { ++actions; }
    void ProcessEventsFor(SMART_EVENT, Unit*, uint32 = 0, uint32 = 0, bool = false, SpellInfo const* = nullptr, GameObject* = nullptr);
};
#include "RootDispatch.inc"
struct LootItem {
    bool loot_restricted = false, needs_quest = false;
    std::set<uint32> allowedGUIDs;
    ConditionList conditions;
    struct { uint32 ItemID = expectedItem, CurrencyID = 0; } item;
    uint32 type = 0;
    bool AllowedForPlayer(Player const*) const;
};
#include "LootCondition.inc"
struct CachedLoot {
    bool AllowedForPlayer(Player const* player, uint32, uint32, uint32, bool, LootItem* item) const { return item->AllowedForPlayer(player); }
    LootItem* Read(Player const* player, LootItem* item) const {
#include "SlotRecheck.inc"
    }
};
int main(int argc, char** argv) {
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 2 || argc == 3 || argc == 4);
    if (argc == 3) { expectedQuest = 40419; expectedMap = 1489; expectedScenario = 972; firstStage = 7; lastStage = 8; expectedItem = 128826; scenarioManager = {}; }
    bool titanstrike = argc == 3 && std::string(argv[2]).find("titanstrike-") == 0;
    bool castAdmission = argc == 3 && std::string(argv[2]) == "titanstrike-cast";
    if (titanstrike) { expectedQuest = 42185; expectedMap = 1609; expectedScenario = 1099; firstStage = 4; lastStage = 5; expectedItem = 128861; eventEntry = 249812; scenarioManager = {}; }
    bool xalatath = argc == 4 && std::string(argv[2]).find("xalatath-") == 0;
    bool xalatathRoot = xalatath && std::string(argv[2]) != "xalatath-loot";
    bool xalatathGossip = xalatathRoot && std::string(argv[2]).find("xalatath-gossip") == 0;
    bool xalatathBaseline = xalatathRoot && std::string(argv[2]).find("-baseline") != std::string::npos;
    if (xalatath) {
        expectedQuest = 40710; expectedMap = 1539; expectedScenario = 991; firstStage = 8; lastStage = 9;
        expectedItem = 128827; expectedClassMask = 16; scenarioManager = {};
        GameObjectTemplate chest;
        assert(chest.GetLootId() == 0);
        chest.chest.chestLoot = static_cast<uint32>(std::stoul(argv[3]));
        assert(chest.GetLootId() == 247690);
        if (xalatathRoot) { castAdmission = true; eventEntry = 247690; }
    }
    bool stromkar = argc == 4 && std::string(argv[2]).find("stromkar-") == 0;
    bool stromkarRoot = stromkar && std::string(argv[2]) == "stromkar-root";
    if (stromkar) {
        expectedQuest = 41105; expectedMap = 1539; expectedScenario = 1037; firstStage = lastStage = 9;
        expectedItem = 128910; expectedClassMask = 1; scenarioManager = {};
        GameObjectTemplate chest;
        assert(chest.GetLootId() == 0);
        chest.chest.chestLoot = static_cast<uint32>(std::stoul(argv[3]));
        assert(chest.GetLootId() == 247877);
        if (stromkarRoot) { castAdmission = true; eventEntry = 247877; }
    }
    bool ownedReplay = xalatathRoot || stromkarRoot || (argc == 3 && std::string(argv[2]) != "marksmanship-loot" && std::string(argv[2]) != "titanstrike-loot");
    uint32 eventId = argc == 3 && std::string(argv[2]) == "marksmanship-root" ? 0 : 2;
    if (titanstrike) eventId = 0;
    if (xalatathRoot) eventId = xalatathGossip ? 2 : 0;
    if (stromkarRoot) eventId = 0;
    bool dialogueEvent = argc == 3 && std::string(argv[2]) == "marksmanship-dialogue";
    bool returnEvent = argc == 3 && (std::string(argv[2]) == "marksmanship-return" || dialogueEvent);
    if (returnEvent) { firstStage = lastStage = 8; eventEntry = 100397; eventSource = 0; eventId = dialogueEvent ? 52 : 59; scenarioManager = {}; }
    std::ifstream input(argv[1]); std::vector<Condition> rows(stromkar ? (stromkarRoot ? 14 : 10) : castAdmission ? 28 : returnEvent ? 12 : ownedReplay ? 24 : 20);
    for (Condition& condition : rows) {
        uint32 target = 0;
        assert(input >> condition.ElseGroup >> condition.ConditionType >> target >> condition.ConditionValue1 >> condition.ConditionValue2 >> condition.ConditionValue3 >> condition.NegativeCondition);
        assert(target == 0);
    }
    LootItem item; for (Condition& condition : rows) item.conditions.push_back(&condition);
    if (ownedReplay && !xalatathBaseline) conditionManager.SmartEventConditionStore[{eventEntry, eventSource}][eventId + 1] = item.conditions;
    if (xalatathBaseline && xalatathGossip) conditionManager.SmartEventConditionStore[{eventEntry, eventSource}][eventId + 1] = {&rows.front()};
    if (xalatathBaseline) {
        Player early; early.owned = xalatathGossip; scenarioManager.scenario.step = 7;
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &early, &early));
    }
    if (xalatathRoot) {
        Player player;
        SMART_EVENT eventType = xalatathGossip ? SMART_EVENT_GOSSIP_HELLO : SMART_EVENT_GO_STATE_CHANGED;
        SmartScript script; script.base = &player; script.mEvents = {{247690, eventId, 1, eventType}};
        scenarioManager.scenario.step = 7;
        script.ProcessEventsFor(eventType, &player, 3); assert(script.actions == 0);
        scenarioManager.scenario.step = 8; player.owned = false;
        script.ProcessEventsFor(eventType, &player, 3); assert(script.actions == 0);
        player.owned = true;
        script.ProcessEventsFor(eventType, &player, 3); assert(script.actions == 1);
    }
    if (stromkarRoot) {
        Player player;
        SmartScript script; script.base = &player; script.mEvents = {{247877, 0, 1, SMART_EVENT_GO_STATE_CHANGED}};
        scenarioManager.scenario.step = 8;
        script.ProcessEventsFor(SMART_EVENT_GO_STATE_CHANGED, &player, 3); assert(script.actions == 0);
        scenarioManager.scenario.step = 9; player.owned = false;
        script.ProcessEventsFor(SMART_EVENT_GO_STATE_CHANGED, &player, 3); assert(script.actions == 0);
        player.owned = true;
        script.ProcessEventsFor(SMART_EVENT_GO_STATE_CHANGED, &player, 3); assert(script.actions == 1);
    }
    if (returnEvent) {
        Player owner; scenarioManager.scenario.step = 7;
        ConditionMgr original;
        assert(original.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &owner, &owner));
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &owner, &owner));
    }
    if (titanstrike) {
        Player owner; scenarioManager.scenario.step = 3;
        ConditionMgr original; LootItem unguarded;
        assert(unguarded.AllowedForPlayer(&owner) && !item.AllowedForPlayer(&owner));
        if (ownedReplay) {
            assert(original.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &owner, &owner));
            assert(!conditionManager.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &owner, &owner));
        }
        std::cout << "BASELINE: missing native conditions admitted early Titanstrike loot/root; supplied source4/22 rows reject the same1099/3 owner\n";
    }
    CachedLoot cache;
    {
        Player player; LootItem unguarded; scenarioManager.scenario.step = firstStage - 1;
        assert(unguarded.AllowedForPlayer(&player) && !item.AllowedForPlayer(&player));
    }
    for (uint32 stage = 0; stage <= 14; ++stage) for (uint32 status : {0u, 1u, 3u, 5u, 6u}) {
        scenarioManager = {}; scenarioManager.scenario.step = stage;
        Player player; player.status = status;
        bool expected = stage >= firstStage && stage <= lastStage && (status == 1 || status == 3);
        assert(item.AllowedForPlayer(&player) == expected);
        assert((cache.Read(&player, &item) != nullptr) == expected);
        if (ownedReplay) assert(conditionManager.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &player, &player) == expected);
    }
    for (uint32 invalid = 0; invalid < 8; ++invalid) {
        scenarioManager = {}; worldMap.instanced = true; instanceMap.id = 42;
        Player player;
        assert(item.AllowedForPlayer(&player));
        if (invalid == 0) scenarioManager.scenario.step = firstStage - 1;
        if (invalid == 1) scenarioManager.scenario.id = 944;
        if (invalid == 2) scenarioManager.present = false;
        if (invalid == 3) player.mapId = 1;
        if (invalid == 4) player.classMask = expectedClassMask == 1 ? 16 : 1;
        if (invalid == 5) player.quest = 41542;
        if (invalid == 6) worldMap.instanced = false;
        if (invalid == 7) instanceMap.id = 43;
        assert(!item.AllowedForPlayer(&player) && !cache.Read(&player, &item));
    }
    worldMap.instanced = true; instanceMap.id = 42; scenarioManager = {};
    if (castAdmission) {
        Player dead; dead.alive = false;
        assert(!item.AllowedForPlayer(&dead) && !cache.Read(&dead, &item));
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &dead, &dead));
    }
    if (stromkar) {
        for (uint32 sharedMapScenario : {991u, 1092u}) {
            Player owner; scenarioManager.scenario.id = sharedMapScenario;
            assert(!item.AllowedForPlayer(&owner) && !cache.Read(&owner, &item));
            if (ownedReplay) assert(!conditionManager.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &owner, &owner));
        }
        scenarioManager = {};
    }
    Player player; player.status = QUEST_STATUS_COMPLETE; player.rewarded = true;
    assert(!item.AllowedForPlayer(&player));
    LootItem unrelated; assert(unrelated.AllowedForPlayer(&player));
    item.loot_restricted = true; assert(!item.AllowedForPlayer(&player));
    assert(!cache.Read(&player, nullptr));
    if (ownedReplay) {
        player.rewarded = false; player.owned = false; player.bank = true; item.loot_restricted = false;
        Condition original; original.ConditionType = CONDITION_ITEM; original.ConditionValue1 = expectedItem; original.ConditionValue2 = 1;
        ConditionSourceInfo originalContext(&player); assert(!original.Meets(originalContext));
        assert(item.AllowedForPlayer(&player) == !(xalatathRoot || stromkarRoot));
        player.bank = false; assert(!item.AllowedForPlayer(&player));
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &player, &player));
        Unit pet;
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, &pet, &pet));
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(eventEntry, eventId, eventSource, nullptr, nullptr));
    }
    else {
        player.rewarded = false; player.owned = false; player.bank = false; item.loot_restricted = false;
        assert(item.AllowedForPlayer(&player) && cache.Read(&player, &item));
    }
    std::cout << "PASS: actual SQL rows/native condition cases/ElseGroup evaluator and loot condition bridge reject early/wrong/abandoned/cached requests, retaining intended final/completed stages and incomplete/complete quests without changing other loot\n";
}
