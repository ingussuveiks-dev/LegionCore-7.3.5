#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;
template<class T>using Optional=std::optional<T>;
#include "DialogEnums.inc"
;
QuestGiverStatus& operator|=(QuestGiverStatus& a,QuestGiverStatus b){a=static_cast<QuestGiverStatus>(uint32(a)|uint32(b));return a;}
constexpr int TYPEID_UNIT=1,TYPEID_GAMEOBJECT=2,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3;
using QuestStatus=int;
constexpr int CONDITION_SOURCE_TYPE_QUEST_ACCEPT=19,CONDITION_SOURCE_TYPE_QUEST_SHOW_MARK=20;
constexpr int QUEST_FLAGS_EX_LEGENDARY_QUEST=1,QUEST_FLAGS_HIDE_REWARD_POI=1,CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF=0;
#define TC_LOG_ERROR(...) ((void)0)
using QuestRelations=std::multimap<uint32,uint32>;
using QuestRelationBounds=std::pair<QuestRelations::iterator,QuestRelations::iterator>;
struct Object {int type=TYPEID_UNIT;uint32 entry=100453;int GetTypeId(){return type;}uint32 GetEntry(){return entry;}Object* ToCreature(){return this;}Object* ToGameObject(){return this;}};
struct Quest {
 uint32 id=40518;bool legendary=false,hide=false,autoComplete=false,repeatable=false,daily=false;
 uint32 GetQuestId()const{return id;}bool HasFlagEx(int)const{return legendary;}bool HasFlag(int)const{return hide;}
 bool IsAutoComplete()const{return autoComplete;}bool IsRepeatable()const{return repeatable;}bool IsDailyOrWeekly()const{return daily;}bool IsDaily()const{return daily;}
};
struct Store {
 QuestRelations starts,ends;std::map<uint32,Quest> quests;
 Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}
 QuestRelationBounds GetCreatureQuestRelationBounds(uint32 id){return starts.equal_range(id);}
 QuestRelationBounds GetCreatureQuestInvolvedRelationBounds(uint32 id){return ends.equal_range(id);}
 QuestRelationBounds GetGOQuestRelationBounds(uint32 id){return starts.equal_range(id);}
 QuestRelationBounds GetGOQuestInvolvedRelationBounds(uint32 id){return ends.equal_range(id);}
} store;auto sQuestDataStore=&store;
using ConditionList=std::vector<bool>;
struct Player;
struct Conditions {
 std::map<uint32,bool> accept,show;unsigned acceptReads=0;
 ConditionList GetConditionsForNotGroupedEntry(int source,uint32 id){if(source==19)++acceptReads;auto& data=source==19?accept:show;auto it=data.find(id);return {it==data.end()||it->second};}
 bool IsObjectMeetToConditions(Player*,ConditionList const& values){for(bool allowed:values)if(!allowed)return false;return true;}
} conditions;auto sConditionMgr=&conditions;
struct Scripts {Optional<QuestGiverStatus> status;Optional<QuestGiverStatus> GetDialogStatus(Player*,Object*){return status;}} scripts;auto sScriptMgr=&scripts;
struct World {int getIntConfig(int){return 5;}} world;auto sWorld=&world;
struct Player {
 std::map<uint32,int> states;bool canTake=false,canSee=true,levelAllowed=true;
 int GetQuestStatus(uint32 id){auto i=states.find(id);return i==states.end()?0:i->second;}
 bool CanTakeQuest(Quest const*,bool){return canTake;}bool CanSeeStartQuest(Quest const*){return canSee;}
 bool SatisfyQuestLevel(Quest const*,bool){return levelAllowed;}int getLevel(){return 102;}int GetQuestLevel(Quest const*){return 100;}
 std::string GetName(){return "fixture";}struct Guid{std::string ToString(){return "fixture";}};Guid GetGUID(){return {};}
 QuestGiverStatus GetQuestDialogStatus(Object*);
};
#include "Dialog.inc"
void Reset(){store={};conditions={};scripts={};store.quests.emplace(40518,Quest{});store.ends.emplace(100453,40518);}
int main()
{
 Object giver;Player player;Reset();player.states[40518]=QUEST_STATUS_COMPLETE;conditions.accept[40518]=false;
 #ifdef EXPECT_BASELINE
 assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::None);
 std::cout<<"PASS baseline reproduction: completed boost quest loses Eitrigg turn-in marker when skipped preparation fails acceptance conditions.\n";
 return 0;
 #else
 assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::RewardCompletePOI);assert(conditions.acceptReads==0);
 for(int status:{0,5,6}){player.states[40518]=status;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::None);}
 player.states[40518]=QUEST_STATUS_COMPLETE;Object wrongGiver;wrongGiver.entry=100866;assert(player.GetQuestDialogStatus(&wrongGiver)==QuestGiverStatus::None);
 player.states[40518]=QUEST_STATUS_INCOMPLETE;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::Incomplete);
 player.states[40518]=QUEST_STATUS_COMPLETE;conditions.show[40518]=false;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::None);
 conditions.show[40518]=true;store.quests[40518].hide=true;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::RewardCompleteNoPOI);
 store.quests[40518].legendary=true;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::LegendaryRewardCompleteNoPOI);
 store.quests[40518].hide=false;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::LegendaryRewardCompletePOI);
 giver.type=TYPEID_GAMEOBJECT;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::LegendaryRewardCompletePOI);giver.type=TYPEID_UNIT;
 Reset();player.states.clear();conditions.accept[40518]=false;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::None);
 store.starts.emplace(100453,40518);assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::None);
 conditions.accept[40518]=true;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::Quest);
 conditions.show[40518]=false;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::None);
 conditions.show[40518]=true;player.canSee=false;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::None);player.canSee=true;
 player.levelAllowed=false;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::Future);player.levelAllowed=true;
 Reset();store.quests[40518].autoComplete=true;store.quests[40518].repeatable=true;player.canTake=true;conditions.accept[40518]=false;
 assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::None);
 conditions.accept[40518]=true;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::RepeatableTurnin);
 Reset();player.states[40518]=QUEST_STATUS_COMPLETE;conditions.accept[40518]=false;
 Quest next;next.id=40522;store.quests.emplace(40522,next);store.starts.emplace(100453,40522);conditions.accept[40522]=false;
 assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::RewardCompletePOI);
 assert(player.states.at(40518)==QUEST_STATUS_COMPLETE&&player.states.count(40522)==0);
 scripts.status=QuestGiverStatus::None;assert(player.GetQuestDialogStatus(&giver)==QuestGiverStatus::None);
 std::cout<<"PASS: native NPC/GO turn-in markers survive changed acceptance; explicit visibility, start/next-quest admission, repeatable auto quests, level and script overrides remain enforced.\n";
 #endif
}
