#include "CallFixture.inc"
constexpr unsigned INVALIDREASON_QUEST_FAILED_LOW_LEVEL = 99;
#include "CallLevel.inc"
#include "CallNextChain.inc"
#include "CallPrevChain.inc"
void Player::AddQuestAndCheckCompletion(Quest const* quest, WorldObject* questGiver)
{
    // Actual acceptance completion block; quest storage and subsequent AI callbacks are separate boundaries.
#include "CallAccept.inc"
}

int main(int argc, char** argv)
{
    assert(argc == 2);
    std::ifstream input(argv[1]); assert(input);
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream in(line); std::string kind; in >> kind;
        if (kind == "quest")
        {
            uint32 id, type, info; in >> id; auto& q = store.quests[id]; q.id = id;
            in >> q.flags >> q.FlagsEx >> q.AllowableRaces >> q.SourceItemId >> q.SourceItemIdCount
               >> q.NextQuestIdChain >> q.PrevQuestID >> q.NextQuestID >> q.special >> q.MinLevel >> q.MaxLevel >> type >> info;
            if (id == 44720) assert(type == 2 && info == 0 && q.MinLevel == 110 && !q.MaxLevel && q.PrevQuestID == 40072 && q.NextQuestID == 44771 && q.NextQuestIdChain == 44771);
        }
        if (kind == "objective")
        {
            uint32 id, objective; int index; QuestObjective o;
            in >> id >> objective >> o.Type >> o.ObjectID >> o.Amount >> index >> o.Flags;
            o.StorageIndex = int8(index); store.quests[id].objectives.push_back(o);
        }
        if (kind == "starter" || kind == "ender")
        {
            uint32 actor, quest; in >> actor >> quest;
            (kind == "starter" ? store.starters : store.enders).emplace(actor, quest);
        }
    }
    assert(input.eof());
    auto& q = store.quests.at(44720); auto& next = store.quests.at(44771);
    assert(q.objectives.empty() && next.objectives.size() == 1 && next.objectives[0].ObjectID == 114962);
    assert(!(q.flags & QUEST_FLAGS_AUTOCOMPLETE));
    // These upstream/downstream records are context only; their gameplay is not exercised.
    store.quests[40005].id = 40005; store.quests[44721].id = 44721;
    QuestMap _questTemplates; for (auto& row : store.quests) _questTemplates[row.first] = &row.second;
    for (auto& row : _questTemplates)
    {
        auto qinfo = row.second;
#include "QuestPrevLoader.inc"
#include "CallChainLoader.inc"
    }
    Creature muninn, havi;
    muninn.entry = 114946; muninn.guid = {14208136}; muninn.map = 1220;
    havi.entry = 92539; havi.guid = {291280}; havi.map = 1220;
    creatures[muninn.guid] = &muninn; creatures[havi.guid] = &havi;
    for (auto team : {ALLIANCE, HORDE})
    {
        for (unsigned level : {109u, 110u}) for (unsigned previous : {0u, 1u, 3u, 6u})
        {
            Player p; p.team = team; p.race = team == ALLIANCE ? 1 : 2; p.level = level;
            if (previous == 6) p.m_RewardedQuests.insert(40072);
            else if (previous) { p.Accept(40072); p.quests[40072].Status = previous; }
            p.PrepareQuestMenu(muninn.guid);
            bool eligible = level >= 110 && previous == 6;
            assert(p.CanTakeQuest(&q, false) == eligible && bool(p.menu.quests.items.count(44720)) == eligible);
        }
        Player p; p.team = team; p.race = team == ALLIANCE ? 1 : 2; p.level = 110; p.m_RewardedQuests.insert(40072);
        assert(!p.CanCompleteQuest(44720) && !p.CanRewardQuest(&q, false) && !p.CanTakeQuest(&next, false));
        p.AddQuestAndCheckCompletion(&q, &muninn);
        assert(p.GetQuestStatus(44720) == QUEST_STATUS_COMPLETE && p.CanRewardQuest(&q, false));
        assert(!p.CanTakeQuest(&q, false) && !p.CanTakeQuest(&next, false));
        p.PrepareQuestMenu(havi.guid); assert(p.menu.quests.items.at(44720) == 4);
        WorldSession session{&p}; WorldPackets::Quest::QuestGiverRequestReward wrong{muninn.guid, 44720}, right{havi.guid, 44720};
        session.HandleQuestGiverRequestReward(wrong); assert(p.menu.offers.empty());
        p.phase = false; session.HandleQuestGiverRequestReward(right); assert(p.menu.offers.empty()); p.phase = true;
        session.HandleQuestGiverRequestReward(right); assert(p.menu.offers.size() == 1);
        // RewardQuest inventory/money/transaction work remains an adapter, explicitly outside this test.
        p.quests.erase(44720); p.m_RewardedQuests.insert(44720);
        assert(!p.CanTakeQuest(&q, false) && !p.CanCompleteQuest(44720) && !p.CanRewardQuest(&q, false));
        assert(p.CanTakeQuest(&next, false) && p.GetNextQuest(havi.guid, &q) == &next && !p.GetNextQuest(muninn.guid, &q));
        p.PrepareQuestMenu(havi.guid); assert(p.menu.quests.items.at(44771) == 2);
        p.Accept(44771); assert(!p.CanCompleteQuest(44771)); // Do not auto-complete the next quest's listening objective.
        Player retry; retry.level = 110; retry.m_RewardedQuests.insert(40072);
        retry.AddQuestAndCheckCompletion(&q, &muninn); retry.quests.erase(44720); // abandonment adapter
        assert(retry.CanTakeQuest(&q, false)); retry.AddQuestAndCheckCompletion(&q, &muninn); assert(retry.CanRewardQuest(&q, false));
        for (unsigned status : {3u, 1u, 6u})
        {
            Player ahead; ahead.level = 110; ahead.m_RewardedQuests.insert(40072);
            if (status == 6) ahead.m_RewardedQuests.insert(44771);
            else { ahead.Accept(44771); ahead.quests[44771].Status = status; }
            assert(!ahead.CanTakeQuest(&q, false)); // Native next-chain gate, independent of44720's own status.
        }
    }
    std::cout << "PASS:44720 native level/status/race/prerequisite/chain gates, both factions, Muninn quest menu, objective-free completion, Havi-only reward request and44771 offer; abandonment/retry and no replay. Acceptance storage, interaction/world services and reward persistence remain adapters; client pending.\n";
}
