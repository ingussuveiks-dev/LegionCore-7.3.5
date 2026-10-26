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
uint32 expectedQuest = 41993, expectedMap = 1489, expectedScenario = 1085, firstStage = 6, expectedItem = 128825;
uint32 lastStage = 6, eventEntry = 248419, eventSource = 1;
uint32 expectedClassMask = 16;
bool playersOwnArtifactByDefault = false;
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
    bool rewarded = false, owned = playersOwnArtifactByDefault, bank = false;
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
struct ConditionMgr {
    ConditionReferenceContainer ConditionReferenceStore;
    bool IsObjectMeetToConditionList(ConditionSourceInfo&, ConditionList const&) const;
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

int main(int argc,char** argv){
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::vector<Condition> rows(14);
 for(auto& c:rows){uint32 target;assert(input>>c.ElseGroup>>c.ConditionType>>target>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition);assert(target==0);}
 LootItem item;for(auto& c:rows)item.conditions.push_back(&c);CachedLoot cache;
 GameObjectTemplate chest;assert(!chest.GetLootId());chest.chest.chestLoot=249705;assert(chest.GetLootId()==249705);
 uint32 cases=0;
 for(uint32 stage=0;stage<8;++stage)for(uint32 status:{0u,1u,3u,5u,6u})for(uint32 cls:{1u,2u,4u,8u,16u,32u,64u,128u,256u,512u,1024u,2048u}){
  scenarioManager={};scenarioManager.scenario.step=stage;Player p;p.status=status;p.classMask=cls;
  bool valid=stage==6&&(status==1||status==3)&&cls==expectedClassMask;assert(item.AllowedForPlayer(&p)==valid);assert((cache.Read(&p,&item)!=nullptr)==valid);++cases;
 }
 scenarioManager={};
 for(uint32 invalid=0;invalid<10;++invalid){
  Player p;scenarioManager={};worldMap.instanced=true;instanceMap.id=42;
  if(invalid==0)p.alive=false;if(invalid==1)p.owned=true;if(invalid==2)p.bank=true;if(invalid==3){p.status=QUEST_STATUS_COMPLETE;p.rewarded=true;}
  if(invalid==4)p.mapId=1220;if(invalid==5)p.quest=41922;if(invalid==6)scenarioManager.present=false;
  if(invalid==7)scenarioManager.scenario.id=999;if(invalid==8)worldMap.instanced=false;if(invalid==9)instanceMap.id=43;
  assert(!item.AllowedForPlayer(&p)&&!cache.Read(&p,&item));
 }
 worldMap.instanced=true;instanceMap.id=42;scenarioManager={};Player p;assert(cache.Read(&p,&item));p.owned=true;assert(!cache.Read(&p,&item));p.owned=false;p.bank=true;assert(!cache.Read(&p,&item));
 LootItem unrelated;assert(unrelated.AllowedForPlayer(&p));
 std::cout<<"PASS "<<cases<<" actual source4 condition/ElseGroup/LootItem and cached-slot rechecks: native249705 loot binding, Priest1085/6 active quest, alive/unrewarded/unowned-including-bank and invalid instance/map/class/history boundaries. Live character state unchanged.\n";
}
