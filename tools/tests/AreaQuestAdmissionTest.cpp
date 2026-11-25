#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>

using uint32 = std::uint32_t;
constexpr int NULL_BAG = 0;
constexpr int NULL_SLOT = 0;
enum InventoryResult { EQUIP_ERR_OK, EQUIP_ERR_ITEM_MAX_COUNT, EQUIP_ERR_INVENTORY_FULL };
using ItemPosCountVec = std::vector<int>;

struct Quest
{
    uint32 Id;
    uint32 SourceItemId = 0;
    uint32 SourceItemIdCount = 1;
    bool Repeatable = false;

    bool IsAutoComplete() const { return Repeatable; }
    bool IsRepeatable() const { return Repeatable; }
    bool IsDailyOrWeekly() const { return false; }
};

using QuestRelations = std::multimap<uint32, uint32>;
using QuestRelationBounds = std::pair<QuestRelations::const_iterator, QuestRelations::const_iterator>;

struct Database
{
    QuestRelations relations;
    std::map<uint32, Quest> quests;
    QuestRelationBounds GetAreaQuestRelationBounds(uint32 area) const { return relations.equal_range(area); }
    Quest const* GetQuestTemplate(uint32 id) const
    {
        auto itr = quests.find(id);
        return itr == quests.end() ? nullptr : &itr->second;
    }
};

Database storage;
Database* sQuestDataStore = &storage;

struct Menu
{
    int details = 0;
    int rewardPrompt = 0;
    void SendQuestGiverQuestDetails(Quest const*, int, bool, bool) { ++details; }
    void SendQuestGiverRequestItems(Quest const*, int, bool, bool) { ++rewardPrompt; }
};

struct Player
{
    Menu menu;
    Menu* PlayerTalkClass = &menu;
    bool levelEligible = true;
    bool logSlot = true;
    bool acceptSucceeds = true;
    InventoryResult inventory = EQUIP_ERR_OK;
    std::set<uint32> active;
    int accepts = 0;
    int completes = 0;
    int equipErrors = 0;

    void PrepareAreaQuest(uint32 area);
    bool CanAddQuest(Quest const* quest, bool msg);
    bool CanTakeQuest(Quest const* quest, bool) const { return levelEligible && !IsActiveQuest(quest->Id); }
    bool SatisfyQuestLog(bool) const { return logSlot; }
    InventoryResult CanStoreNewItem(int, int, ItemPosCountVec&, uint32, uint32) const { return inventory; }
    void SendEquipError(InventoryResult, void*, void*, uint32) { ++equipErrors; }
    bool IsActiveQuest(uint32 id) const { return active.count(id) > 0; }
    void AddQuest(Quest const* quest, Player*)
    {
        ++accepts;
        if (logSlot && acceptSucceeds)
            active.insert(quest->Id);
    }
    bool CanCompleteQuest(uint32 id) const { return IsActiveQuest(id); }
    bool CanCompleteRepeatableQuest(Quest const*) const { return true; }
    void CompleteQuest(uint32) { ++completes; }
    int GetGUID() const { return 1; }
};

#include "AreaQuestMethods.inc"

int main()
{
    constexpr uint32 Area = 10;
    storage.relations = {{Area, 1}, {Area, 2}, {Area, 3}, {Area + 1, 4}};
    storage.quests = {{1, {1, 100}}, {2, {2, 0, 1, true}}, {4, {4}}};

    Player fullLog;
    fullLog.logSlot = false;
    fullLog.PrepareAreaQuest(Area);
    if (fullLog.accepts || fullLog.completes || fullLog.menu.details || fullLog.menu.rewardPrompt)
    {
        std::cerr << "A full quest log must prevent acceptance, completion and quest panels\n";
        return 1;
    }

    Player fullBags;
    fullBags.inventory = EQUIP_ERR_INVENTORY_FULL;
    fullBags.PrepareAreaQuest(Area);
    if (fullBags.IsActiveQuest(1) || !fullBags.IsActiveQuest(2) || fullBags.IsActiveQuest(3) ||
        fullBags.accepts != 1 || fullBags.completes != 1 || fullBags.equipErrors != 1 ||
        fullBags.menu.details || fullBags.menu.rewardPrompt != 1)
        return 2;
    fullBags.inventory = EQUIP_ERR_ITEM_MAX_COUNT;
    fullBags.PrepareAreaQuest(Area);
    if (!fullBags.IsActiveQuest(1) || fullBags.accepts != 2 || fullBags.completes != 2 ||
        fullBags.menu.details != 1 || fullBags.menu.rewardPrompt != 1)
        return 3;

    Player failedAddition;
    failedAddition.acceptSucceeds = false;
    failedAddition.PrepareAreaQuest(Area);
    if (failedAddition.accepts != 2 || failedAddition.completes ||
        failedAddition.menu.details || failedAddition.menu.rewardPrompt)
        return 4;

    Player ineligible;
    ineligible.levelEligible = false;
    ineligible.PrepareAreaQuest(Area);
    if (ineligible.accepts || ineligible.completes || ineligible.menu.details || ineligible.menu.rewardPrompt)
        return 5;

    std::cout << "PASS: area quest admission, inventory, retry, and packet delivery\n";
}
