#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t; using int32=int32_t; using InventoryResult=int;
using PrevQuests=std::vector<int32>; using ExclusiveQuestGroups=std::multimap<int32,uint32>;
#define ASSERT assert
enum {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
enum {QUEST_OBJECTIVE_ITEM=1,INVALIDREASON_DONT_HAVE_REQ=1,QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=1,QUEST_SPECIAL_FLAGS_TIMED=2,
 QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS=1,QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR=2,QUEST_OBJECTIVE_FLAG_OPTIONAL=4,QUEST_FLAGS_EX_KEEP_ADDITIONAL_ITEMS=1};
enum {EQUIP_ERR_OK,EQUIP_ERR_ITEM_MAX_COUNT,EQUIP_ERR_LOOT_GONE,EQUIP_ERR_NOT_EQUIPPABLE,EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS,ITEM_LIMIT_CATEGORY_MODE_HAVE};
struct QuestObjective {int Type=1,ObjectID=128380,Amount=50,Flags=0,StorageIndex=0;};
struct Quest {
 uint32 id; int ExclusiveGroup=0; PrevQuests prevQuests; std::vector<QuestObjective> objectives; int FlagsEx=0;
 auto const& GetObjectives() const{return objectives;}
 bool IsWeekly()const{return false;} bool IsSeasonal()const{return false;} bool IsRepeatable()const{return false;}
 bool IsAutoComplete()const{return false;} bool HasSpecialFlag(int)const{return false;}
};
struct QuestStore {std::map<uint32,Quest> quests; ExclusiveQuestGroups mExclusiveQuestGroups;
 Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}
} store;
auto sQuestDataStore=&store;
struct ItemSparseEntry {int MaxCount=30,Stackable=30;};
struct ItemTemplate {ItemSparseEntry sparse; int GetMaxCount()const{return sparse.MaxCount;} uint32 GetId()const{return 128380;} int GetLimitCategory()const{return 0;}} itemTemplate;
struct Item {bool m_lootGenerated=false;};
struct ObjectMgr {ItemTemplate const* GetItemTemplate(uint32 id){return id==128380?&itemTemplate:nullptr;}} objectMgr;
auto sObjectMgr=&objectMgr;
struct ItemLimitCategoryEntry {int Flags=0,Quantity=0;};
struct LimitStore {ItemLimitCategoryEntry const* LookupEntry(int)const{return nullptr;}} sItemLimitCategoryStore;
struct QuestStatusData {int Status=QUEST_STATUS_NONE; uint32 Timer=0;};
struct Player {
 std::set<uint32> m_RewardedQuests; QuestStatusData status; uint32 bags=0,bank=0; int progress=0; unsigned completeCalls=0,reopenCalls=0;
 bool SatisfyQuestPreviousQuest(Quest const*,bool); bool SatisfyQuestRace(Quest const*,bool){return true;}
 int GetQuestStatus(uint32 id){return id==39373?status.Status:QUEST_STATUS_NONE;}
 void SendCanTakeQuestResponse(int,Quest const*,char const*){}
 InventoryResult CanTakeMoreSimilarItems(uint32,uint32,Item*,uint32*,bool)const;
 uint32 GetItemCount(uint32,bool includeBank,Item* =nullptr)const{return bags+(includeBank?bank:0);}
 uint32 GetItemCountWithLimitCategory(int,Item*)const{return 0;}
 bool CanCompleteQuest(uint32); bool IsQuestWeekRewarded(uint32){return false;} bool IsQuestSeasonalRewarded(uint32){return false;}
 bool CanTakeQuest(Quest const*,bool){return false;}
 QuestStatusData* getQuestStatus(uint32){return &status;}
 bool HasQuestObjectiveComplete(Quest const*,QuestObjective const& obj){return progress>=obj.Amount;}
 void SetQuestObjectiveData(Quest const*,QuestObjective const*,uint32 count){progress=int(count);}
 void CompleteQuest(uint32){status.Status=QUEST_STATUS_COMPLETE;++completeCalls;}
 void IncompleteQuest(uint32){status.Status=QUEST_STATUS_INCOMPLETE;++reopenCalls;}
 void DestroyItemCount(int id,int count,bool){assert(id==128380 && bags>=uint32(count));bags-=count;}
 void RewardItems(Quest const* quest){for(auto const& obj:quest->GetObjectives()){switch(obj.Type){
#include "BloodtotemReward.inc"
 }}}
};
#include "BloodtotemCore.inc"
struct PlayerScript {explicit PlayerScript(char const*){} virtual void OnLogin(Player*){}};
#include "BloodtotemLogin.inc"
struct HotfixFixture {bool notified=false;void ApplyHotfix(uint32 id,void(*fixer)(ItemSparseEntry*),bool notify){assert(id==128380);fixer(&itemTemplate.sparse);notified=notify;}};
int main()
{
 store.quests.emplace(38912,Quest{38912,0,{}});
 for(auto id:{39372u,39373u,39873u}){store.quests.emplace(id,Quest{id,-39372,{38912}});store.mExclusiveQuestGroups.emplace(-39372,id);}
 store.quests[39373].objectives.push_back({});
 store.quests.emplace(39374,Quest{39374,0,{39372,39372,39373,39873}});
 Player p;
 for(auto id:{39372u,39373u,39873u})assert(!p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(id),false));
 p.m_RewardedQuests.insert(38912);
 for(auto id:{39372u,39373u,39873u})assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(id),false));
 for(unsigned mask=0;mask<8;++mask){p.m_RewardedQuests={38912};unsigned bit=0;for(auto id:{39372u,39373u,39873u})if(mask&(1u<<bit++))p.m_RewardedQuests.insert(id);assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(39374),false)==(mask==7));}
 // Refuge branches after Rock Troll in a Hard Place use the same actual core gate.
 store.quests.emplace(39381,Quest{39381,0,{}});
 for(auto id:{39391u,39425u,39588u}){store.quests.emplace(id,Quest{id,-39391,{39381}});store.mExclusiveQuestGroups.emplace(-39391,id);}
 store.quests.emplace(39426,Quest{39426,0,{39391,39391,39425,39588}});
 p.m_RewardedQuests.clear();for(auto id:{39391u,39425u,39588u})assert(!p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(id),false));
 p.m_RewardedQuests.insert(39381);for(auto id:{39391u,39425u,39588u})assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(id),false));
 for(unsigned mask=0;mask<8;++mask){p.m_RewardedQuests={39381};unsigned bit=0;for(auto id:{39391u,39425u,39588u})if(mask&(1u<<bit++))p.m_RewardedQuests.insert(id);assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(39426),false)==(mask==7));}
 p={};p.bags=30;assert(p.CanTakeMoreSimilarItems(128380,7,nullptr,nullptr,true)==EQUIP_ERR_ITEM_MAX_COUNT);
 HotfixFixture itemSparseHotfixes;
#include "BloodtotemHotfix.inc"
 assert(itemSparseHotfixes.notified && itemTemplate.sparse.MaxCount==50 && itemTemplate.sparse.Stackable==30);
 assert(p.CanTakeMoreSimilarItems(128380,7,nullptr,nullptr,true)==EQUIP_ERR_OK);
 p.bags=49;assert(p.CanTakeMoreSimilarItems(128380,1,nullptr,nullptr,false)==EQUIP_ERR_OK);
 assert(p.CanTakeMoreSimilarItems(128380,2,nullptr,nullptr,false)==EQUIP_ERR_ITEM_MAX_COUNT);
 p.bags=50;assert(p.CanTakeMoreSimilarItems(128380,3,nullptr,nullptr,true)==EQUIP_ERR_ITEM_MAX_COUNT);
 p.bags=30;p.bank=20;assert(p.CanTakeMoreSimilarItems(128380,1,nullptr,nullptr,true)==EQUIP_ERR_ITEM_MAX_COUNT);
 player_bloodtotem_objectives login;
 for(int status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE})for(uint32 bags:{0u,30u,49u,50u,56u}){
  p={};p.status.Status=status;p.bags=bags;p.bank=50;p.progress=30;
  login.OnLogin(&p);assert(p.progress==int(std::min(bags,50u)));assert(p.status.Status==(bags>=50?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE));
  auto completes=p.completeCalls,reopens=p.reopenCalls;login.OnLogin(&p);assert(p.completeCalls==completes && p.reopenCalls==reopens);
  if(bags>=50){p.RewardItems(store.GetQuestTemplate(39373));assert(p.bags==bags-50 && p.bank==50);}
 }
 for(int status:{QUEST_STATUS_NONE,QUEST_STATUS_REWARDED}){p={};p.status.Status=status;p.progress=30;login.OnLogin(&p);assert(p.status.Status==status && p.progress==30);}
 std::cout<<"PASS: extracted prerequisites (all 8 subsets for both branch groups), inventory limit and client hotfix, login reconciliation at 0/30/49/50/56, bank exclusion, idempotence, 50-item reward removal, untouched absent/rewarded quests.\n";
}
