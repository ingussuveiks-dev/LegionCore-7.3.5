#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using uint32 = unsigned;
using int32 = int;
using PrevQuests = std::vector<int32>;
using ExclusiveQuestGroups = std::multimap<int32, uint32>;

constexpr uint32 QUEST_STATUS_NONE = 0;
constexpr uint32 INVALIDREASON_DONT_HAVE_REQ = 1;
constexpr uint32 INVALIDREASON_DAILY_QUEST_COMPLETED_TODAY = 2;
#define ASSERT assert
#define TC_LOG_ERROR(...) (++loaderErrors)

struct Quest
{
    uint32 ID = 0, Flags = 0;
    int32 PrevQuestID = 0, NextQuestID = 0, ExclusiveGroup = 0;
    PrevQuests prevQuests;

    uint32 GetQuestId() const { return ID; }
    bool IsDaily() const { return (Flags & 0x1000) != 0; }
    bool IsDFQuest() const { return false; }
};

using QuestMap = std::map<uint32, Quest*>;
struct QuestStore
{
    QuestMap _questTemplates;
    ExclusiveQuestGroups mExclusiveQuestGroups;
    unsigned loaderErrors = 0;

    Quest const* GetQuestTemplate(uint32 id) const
    {
        auto found = _questTemplates.find(id);
        return found == _questTemplates.end() ? nullptr : found->second;
    }

    void LoadLinks()
    {
        for (auto const& [id, qinfo] : _questTemplates)
        {
            (void)id;
#include "QuestLinks.inc"
        }
    }
} questStore;
auto* sQuestDataStore = &questStore;

struct Player
{
    std::set<uint32> m_RewardedQuests, m_dailyquests, m_DFQuests;
    std::map<uint32, uint32> status;

    uint32 GetQuestStatus(uint32 id) const
    {
        auto found = status.find(id);
        return found == status.end() ? QUEST_STATUS_NONE : found->second;
    }
    bool SatisfyQuestRace(Quest const*, bool) const { return true; }
    void SendCanTakeQuestResponse(uint32, Quest const*, char const* = nullptr) { assert(false); }
    bool SatisfyQuestPreviousQuest(Quest const*, bool);
    bool SatisfyQuestDay(Quest const*, bool);
};

#include "QuestAdmission.inc"

int main(int argc, char** argv)
{
    assert(argc == 3);
    std::ifstream data(argv[1]);
    bool repaired = std::string(argv[2]) == "repaired";
    std::map<uint32, Quest> quests;
    Quest row;
    while (data >> row.ID >> row.Flags >> row.PrevQuestID >> row.NextQuestID >> row.ExclusiveGroup)
        assert(quests.emplace(row.ID, row).second);
    for (auto& [id, quest] : quests)
        questStore._questTemplates.emplace(id, &quest);
    questStore.LoadLinks();
    assert(quests.size() == 6 && questStore.loaderErrors == 0);
    auto& daily = quests.at(28751);
    assert(daily.IsDaily());
    assert(daily.prevQuests == (repaired ? PrevQuests{28748} : PrevQuests{}));

    for (uint32 state : {0u, 1u, 3u})
    {
        Player player;
        player.status[28748] = state;
        player.m_RewardedQuests = {28733, 28617, 28744, 28747};
        assert(player.SatisfyQuestPreviousQuest(&daily, false) == !repaired);
        player.m_RewardedQuests.insert(28748);
        assert(player.SatisfyQuestPreviousQuest(&daily, false));
        assert(player.SatisfyQuestDay(&daily, false));
        player.m_dailyquests.insert(28751);
        assert(!player.SatisfyQuestDay(&daily, false));
        player.m_dailyquests.clear();
        assert(player.SatisfyQuestPreviousQuest(&daily, false) && player.SatisfyQuestDay(&daily, false));
    }

    Player fresh;
    assert(fresh.SatisfyQuestPreviousQuest(&quests.at(28733), false));
    for (uint32 later : {28617u, 28744u, 28747u, 28748u})
        assert(!fresh.SatisfyQuestPreviousQuest(&quests.at(later), false));
    std::cout << (repaired ? "PASS rewarded finale gate, ordinary daily reset and untouched main chain\n"
                          : "PASS counterfactual: daily prerequisite passes on the original data without the finale reward\n");
}
