#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <vector>

using uint8 = uint8_t;
using uint32 = uint32_t;
enum QuestStatus { QUEST_STATUS_NONE=0, QUEST_STATUS_COMPLETE=1, QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_FAILED=5, QUEST_STATUS_REWARDED=6 };
constexpr uint32 HORDE=67, ALLIANCE=469, CLASS_DEMON_HUNTER=12;
constexpr uint8 MAX_QUEST_LOG_SIZE=25;
#define TC_LOG_INFO(...) ((void)0)

struct Player
{
    uint32 team=HORDE, playerClass=9;
    std::map<uint32,QuestStatus> active;
    std::set<uint32> rewarded;
    std::array<uint32,MAX_QUEST_LOG_SIZE> slots{};
    std::vector<uint32> removed, sourceItems;
    uint32 GetTeam() const { return team; }
    uint32 getClass() const { return playerClass; }
    bool GetQuestRewardStatus(uint32 id) const { return rewarded.count(id)!=0; }
    QuestStatus GetQuestStatus(uint32 id) const
    {
        if (rewarded.count(id)) return QUEST_STATUS_REWARDED;
        auto it=active.find(id);
        return it==active.end()?QUEST_STATUS_NONE:it->second;
    }
    uint32 GetQuestSlotQuestId(uint8 slot) const { return slots[slot]; }
    void SetQuestSlot(uint8 slot,uint32 quest) { slots[slot]=quest; }
    void TakeQuestSourceItem(uint32 id,bool) { sourceItems.push_back(id); }
    void RemoveActiveQuest(uint32 id) { active.erase(id); removed.push_back(id); }
    void Add(uint32 id,QuestStatus state=QUEST_STATUS_INCOMPLETE)
    {
        active[id]=state;
        for (auto& slot:slots) if(!slot) { slot=id; return; }
        assert(false);
    }
};

#include "LegionIntroSequence.inc"

int main()
{
    Player maiko;
    maiko.rewarded={40518,40522,40760,40607,40658};
    maiko.Add(43926,QUEST_STATUS_COMPLETE);
    maiko.Add(44663,QUEST_STATUS_COMPLETE);
    auto const rewards=maiko.rewarded;
    LegionIntroSequence::RepairQuestLog(&maiko);
    assert(maiko.GetQuestStatus(43926)==QUEST_STATUS_NONE);
    assert(maiko.GetQuestStatus(44663)==QUEST_STATUS_NONE);
    assert(maiko.rewarded==rewards);
    for(auto slot:maiko.slots) assert(slot!=43926&&slot!=44663);
    assert(maiko.sourceItems.size()==2);
    auto const removed=maiko.removed;
    LegionIntroSequence::RepairQuestLog(&maiko);
    assert(maiko.removed==removed);

    Player boost;
    boost.Add(43926,QUEST_STATUS_COMPLETE);
    boost.Add(40518);
    LegionIntroSequence::RepairQuestLog(&boost);
    assert(boost.GetQuestStatus(40518)==QUEST_STATUS_INCOMPLETE);
    assert(boost.GetQuestStatus(43926)==QUEST_STATUS_NONE);
    assert(boost.rewarded.empty());

    Player regular;
    regular.rewarded={43926};
    regular.Add(44281);
    LegionIntroSequence::RepairQuestLog(&regular);
    assert(regular.removed.empty());

    for(uint32 team:{HORDE,ALLIANCE})
        for(uint32 playerClass:{9u,CLASS_DEMON_HUNTER})
        {
            Player other;
            other.team=team;
            other.playerClass=playerClass;
            other.Add(44663,QUEST_STATUS_COMPLETE);
            LegionIntroSequence::RepairQuestLog(&other);
            assert(other.removed.empty()==(team==ALLIANCE||playerClass==CLASS_DEMON_HUNTER));
        }

    Player legacyFinished;
    legacyFinished.rewarded={44663};
    legacyFinished.Add(40658);
    LegionIntroSequence::RepairQuestLog(&legacyFinished);
    assert(legacyFinished.GetQuestStatus(44663)==QUEST_STATUS_REWARDED);
    assert(legacyFinished.GetQuestStatus(40658)==QUEST_STATUS_INCOMPLETE);

    uint32 const ordered[]={40518,40522,40760,40607,40605,44663};
    for(uint8 i=1;i<6;++i)
    {
        Player valid;
        valid.rewarded.insert(ordered[i-1]);
        valid.Add(ordered[i],QUEST_STATUS_COMPLETE);
        LegionIntroSequence::RepairQuestLog(&valid);
        assert(valid.GetQuestStatus(ordered[i])==QUEST_STATUS_COMPLETE);
        assert(valid.removed.empty());
        Player early;
        early.Add(ordered[i-1],QUEST_STATUS_COMPLETE);
        early.Add(ordered[i]);
        LegionIntroSequence::RepairQuestLog(&early);
        assert(early.GetQuestStatus(ordered[i])==QUEST_STATUS_NONE);
        assert(early.rewarded.empty());
    }
    puts("PASS: production intro repair preserves rewards and valid quests, removes premature active quests and stale boost preparation, and leaves Alliance/DH routes intact.");
}
