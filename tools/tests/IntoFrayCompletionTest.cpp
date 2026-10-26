#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t;constexpr uint32 MAX_QUEST_LOG_SIZE=25;
enum {QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1};
enum {QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=2,QUEST_SPECIAL_FLAGS_TIMED=4,
      QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS=1,QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR=2,QUEST_OBJECTIVE_FLAG_OPTIONAL=4};
struct QuestObjective{uint32 Flags=0;bool complete=false;};
struct Quest
{
    bool weekly=false,seasonal=false,repeatable=false,autoComplete=false;
    uint32 flags=2;std::vector<QuestObjective> objectives;
    bool IsWeekly()const{return weekly;}bool IsSeasonal()const{return seasonal;}
    bool IsRepeatable()const{return repeatable;}bool IsAutoComplete()const{return autoComplete;}
    bool HasSpecialFlag(uint32 value)const{return flags&value;}
    auto const& GetObjectives()const{return objectives;}
};
struct QuestStore{Quest* quest=nullptr;Quest const* GetQuestTemplate(uint32 id){return id==38834?quest:nullptr;}} store;
auto sQuestDataStore=&store;
struct QuestStatusData{uint32 Status=QUEST_STATUS_INCOMPLETE,Timer=1;};
struct Player
{
    std::set<uint32> m_RewardedQuests;QuestStatusData status;bool hasQuest=true;uint32 completions=0,legacyCalls=0;
    bool IsQuestWeekRewarded(uint32)const{return false;}bool IsQuestSeasonalRewarded(uint32)const{return false;}
    bool CanTakeQuest(Quest const*,bool)const{return true;}
    QuestStatusData* getQuestStatus(uint32){return hasQuest?&status:nullptr;}
    bool HasQuestObjectiveComplete(Quest const*,QuestObjective const& objective){return objective.complete;}
    uint32 FindQuestSlot(uint32)const{return hasQuest?0:MAX_QUEST_LOG_SIZE;}
    uint32 GetQuestStatus(uint32)const{return status.Status;}
    void CompleteQuest(uint32){++completions;status.Status=QUEST_STATUS_COMPLETE;}
    void AreaExploredOrEventHappens(uint32){++legacyCalls;}
    bool CanCompleteQuest(uint32 quest_id);
};
#include "CanCompleteQuest.inc"
#include "ScriptedQuestEvent.inc"
int main()
{
    Quest quest;store.quest=&quest;Player player;
    assert(!player.CanCompleteQuest(38834)); // The flag is required by native189764 and must remain.
    CompleteScriptedQuestEvent(&player,38834);assert(player.completions==1&&!player.legacyCalls&&player.status.Status==QUEST_STATUS_COMPLETE);
    CompleteScriptedQuestEvent(&player,38834);assert(player.completions==1);
    player.status.Status=QUEST_STATUS_INCOMPLETE;player.hasQuest=false;CompleteScriptedQuestEvent(&player,38834);assert(player.completions==1);player.hasQuest=true;
    CompleteScriptedQuestEvent(&player,999);assert(player.completions==1);
    quest.flags=0;assert(player.CanCompleteQuest(38834));
    player.m_RewardedQuests.insert(38834);assert(!player.CanCompleteQuest(38834));player.m_RewardedQuests.clear();
    quest.objectives.push_back({0,false});assert(!player.CanCompleteQuest(38834));
    CompleteScriptedQuestEvent(&player,38834);assert(player.legacyCalls==1&&player.completions==1);
    quest.objectives.back().complete=true;assert(player.CanCompleteQuest(38834));
    quest.flags|=QUEST_SPECIAL_FLAGS_TIMED;player.status.Timer=0;assert(!player.CanCompleteQuest(38834));
    assert(!player.CanCompleteQuest(0)&&!player.CanCompleteQuest(37653));
    std::cout<<"PASS native scripted quest-event helper/CanCompleteQuest: required189764 event flag retained; active objective-free event uses CompleteQuest once; missing/complete/inactive rejected; objective-bearing legacy path and absent/rewarded/objective/timer guards preserved. Range/alive checks remain in caller; not real turn-in/rewards/client proof\n";
}
