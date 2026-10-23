#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32 = uint32_t;
using QuestStatus = uint32;
uint32 expectedQuest = 40385, expectedMap = 1511, expectedScenario = 973, firstStage = 12, expectedItem = 128808;
enum { CONDITION_ITEM = 2, CONDITION_CLASS = 15, CONDITION_MAPID = 22, CONDITION_SCENARION_STEP = 44,
    CONDITION_QUESTTAKEN = 9, CONDITION_QUEST_COMPLETE = 28, CONDITION_QUESTREWARDED = 8,
    QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1, QUEST_STATUS_REWARDED = 6 };
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(value) assert(value)
struct Player;
struct InstanceMap { uint32 id = 42; uint32 GetInstanceId() const { return id; } } instanceMap;
struct Map { bool instanced = true; InstanceMap* ToInstanceMap() { return instanced ? &instanceMap : nullptr; } } worldMap;
struct Unit {
    virtual ~Unit() = default;
    uint32 mapId = expectedMap, classMask = 4;
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
struct ConditionSourceInfo { Unit* object; explicit ConditionSourceInfo(Unit* value) : object(value) {} };
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
struct ConditionMgr {
    ConditionReferenceContainer ConditionReferenceStore;
    bool IsObjectMeetToConditionList(ConditionSourceInfo&, ConditionList const&) const;
    bool IsObjectMeetToConditions(Unit* object, ConditionList const& conditions) const {
        if (conditions.empty()) return true;
        ConditionSourceInfo sourceInfo(object);
        return IsObjectMeetToConditionList(sourceInfo, conditions);
    }
} conditionManager;
auto* sConditionMgr = &conditionManager;
#include "ElseGroups.inc"
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
    assert(argc == 2 || argc == 3);
    if (argc == 3) { expectedQuest = 40419; expectedMap = 1489; expectedScenario = 972; firstStage = 7; expectedItem = 128826; scenarioManager = {}; }
    std::ifstream input(argv[1]); std::vector<Condition> rows(argc == 3 ? 24 : 20);
    for (Condition& condition : rows) {
        uint32 target = 0;
        assert(input >> condition.ElseGroup >> condition.ConditionType >> target >> condition.ConditionValue1 >> condition.ConditionValue2 >> condition.ConditionValue3 >> condition.NegativeCondition);
        assert(target == 0);
    }
    LootItem item; for (Condition& condition : rows) item.conditions.push_back(&condition);
    CachedLoot cache;
    {
        Player player; LootItem unguarded; scenarioManager.scenario.step = firstStage - 1;
        assert(unguarded.AllowedForPlayer(&player) && !item.AllowedForPlayer(&player));
    }
    for (uint32 stage = 0; stage <= 14; ++stage) for (uint32 status : {0u, 1u, 3u, 5u, 6u}) {
        scenarioManager = {}; scenarioManager.scenario.step = stage;
        Player player; player.status = status;
        bool expected = (stage == firstStage || stage == firstStage + 1) && (status == 1 || status == 3);
        assert(item.AllowedForPlayer(&player) == expected);
        assert((cache.Read(&player, &item) != nullptr) == expected);
    }
    for (uint32 invalid = 0; invalid < 8; ++invalid) {
        scenarioManager = {}; worldMap.instanced = true; instanceMap.id = 42;
        Player player;
        assert(item.AllowedForPlayer(&player));
        if (invalid == 0) scenarioManager.scenario.step = firstStage - 1;
        if (invalid == 1) scenarioManager.scenario.id = 944;
        if (invalid == 2) scenarioManager.present = false;
        if (invalid == 3) player.mapId = 1;
        if (invalid == 4) player.classMask = 1;
        if (invalid == 5) player.quest = 41542;
        if (invalid == 6) worldMap.instanced = false;
        if (invalid == 7) instanceMap.id = 43;
        assert(!item.AllowedForPlayer(&player) && !cache.Read(&player, &item));
    }
    worldMap.instanced = true; instanceMap.id = 42; scenarioManager = {};
    Player player; player.status = QUEST_STATUS_COMPLETE; player.rewarded = true;
    assert(!item.AllowedForPlayer(&player));
    LootItem unrelated; assert(unrelated.AllowedForPlayer(&player));
    item.loot_restricted = true; assert(!item.AllowedForPlayer(&player));
    assert(!cache.Read(&player, nullptr));
    if (argc == 3) {
        player.rewarded = false; player.owned = false; player.bank = true; item.loot_restricted = false;
        Condition original; original.ConditionType = CONDITION_ITEM; original.ConditionValue1 = expectedItem; original.ConditionValue2 = 1;
        ConditionSourceInfo originalContext(&player); assert(!original.Meets(originalContext));
        assert(item.AllowedForPlayer(&player));
        player.bank = false; assert(!item.AllowedForPlayer(&player));
    }
    std::cout << "PASS: actual SQL rows/native condition cases/ElseGroup evaluator and loot condition bridge reject early/wrong/abandoned/cached requests, retaining intended final/completed stages and incomplete/complete quests without changing other loot\n";
}
