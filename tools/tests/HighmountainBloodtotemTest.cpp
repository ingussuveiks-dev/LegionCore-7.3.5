#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t; using int32=int32_t; using InventoryResult=int;
using PrevQuests=std::vector<int32>; using ExclusiveQuestGroups=std::multimap<int32,uint32>;
#define ASSERT assert
enum {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
enum {MAX_QUEST_LOG_SIZE=25,QUEST_ITEM_COUNT=4,QUEST_SPECIAL_FLAGS_DELIVER=4};
enum {MAX_KILL_CREDIT=2,QUEST_SPECIAL_FLAGS_KILL=8,QUEST_SPECIAL_FLAGS_CAST=16,QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_TALKTO=7,
 CRITERIA_TYPE_KILL_CREATURE_TYPE,CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD,CRITERIA_TIMED_TYPE_CREATURE,CRITERIA_TYPE_KILL_CREATURE};
enum {QUEST_OBJECTIVE_ITEM=1,INVALIDREASON_DONT_HAVE_REQ=1,QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=1,QUEST_SPECIAL_FLAGS_TIMED=2,
 QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS=0x10,QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR=0x40,QUEST_OBJECTIVE_FLAG_OPTIONAL=4,QUEST_FLAGS_EX_KEEP_ADDITIONAL_ITEMS=1};
enum {EQUIP_ERR_OK,EQUIP_ERR_ITEM_MAX_COUNT,EQUIP_ERR_LOOT_GONE,EQUIP_ERR_NOT_EQUIPPABLE,EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS,ITEM_LIMIT_CATEGORY_MODE_HAVE};
struct QuestObjective {int Type=1,ObjectID=128380,Amount=50,Flags=0,StorageIndex=0;};
struct Quest {
 uint32 id; int ExclusiveGroup=0; PrevQuests prevQuests; std::vector<QuestObjective> objectives; int FlagsEx=0;
 uint32 ItemDrop[QUEST_ITEM_COUNT]={},ItemDropQuantity[QUEST_ITEM_COUNT]={};
 auto const& GetObjectives() const{return objectives;}
 bool IsWeekly()const{return false;} bool IsSeasonal()const{return false;} bool IsRepeatable()const{return false;}
 bool IsAutoComplete()const{return false;}
 bool HasSpecialFlag(int flag)const{return !objectives.empty() && ((flag==QUEST_SPECIAL_FLAGS_DELIVER && objectives[0].Type==QUEST_OBJECTIVE_ITEM) || (flag==(QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST) && objectives[0].Type==QUEST_OBJECTIVE_MONSTER));}
 bool IsAllowedInRaid(int)const{return false;}
};
struct QuestStore {std::map<uint32,Quest> quests; ExclusiveQuestGroups mExclusiveQuestGroups;
 Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}
} store;
auto sQuestDataStore=&store;
struct ItemSparseEntry {int MaxCount=30,Stackable=30;};
struct ItemTemplate {ItemSparseEntry sparse; uint32 id=128380; int GetMaxCount()const{return sparse.MaxCount;} uint32 GetId()const{return id;} int GetLimitCategory()const{return 0;} uint32 GetMaxStackSize()const{return sparse.Stackable;}} itemTemplate;
struct Item {bool m_lootGenerated=false;};
struct ObjectMgr {ItemTemplate const* GetItemTemplate(uint32 id){return id==itemTemplate.id?&itemTemplate:nullptr;}} objectMgr;
auto sObjectMgr=&objectMgr;
struct ItemLimitCategoryEntry {int Flags=0,Quantity=0;};
struct LimitStore {ItemLimitCategoryEntry const* LookupEntry(int)const{return nullptr;}} sItemLimitCategoryStore;
struct QuestStatusData {int Status=QUEST_STATUS_NONE; uint32 Timer=0;};
struct ObjectGuid {uint32 value=0;bool IsEmpty()const{return !value;}};
struct CreatureTemplate {uint32 Entry=0,KillCredit[MAX_KILL_CREDIT]={};};
struct Creature {
 uint32 entry=0;uint32 GetEntry()const{return entry;} int GetCreatureType()const{return 1;}
 ObjectGuid GetGUID()const{return {entry};}
} killedCreature;
struct Map {int GetDifficultyID()const{return 0;} Creature* GetCreature(ObjectGuid guid)const{return guid.value==killedCreature.entry?&killedCreature:nullptr;}} testMap;
struct Guild {template<class... T> void UpdateAchievementCriteria(T...) {}};
struct GuildManager {Guild* GetGuildById(uint32){return nullptr;}} guildManager;
auto sGuildMgr=&guildManager;
struct AchievementManager {void StartTimedAchievement(int,uint32){}} achievementManager;
struct Group {bool isRaidGroup()const{return true;}} raidGroup;
struct Player {
 uint32 activeQuest=39373; bool raid=false;
 std::set<uint32> m_RewardedQuests; QuestStatusData status; uint32 bags=0,bank=0; int progress=0; unsigned completeCalls=0,reopenCalls=0;
 bool SatisfyQuestPreviousQuest(Quest const*,bool); bool SatisfyQuestRace(Quest const*,bool){return true;}
 int GetQuestStatus(uint32 id){return id==activeQuest?status.Status:QUEST_STATUS_NONE;}
 uint32 GetQuestSlotQuestId(uint8 slot)const{return slot==0 && status.Status!=QUEST_STATUS_NONE && status.Status!=QUEST_STATUS_REWARDED?activeQuest:0;}
 Map const* GetMap()const{return &testMap;} Group const* GetGroup()const{return raid?&raidGroup:nullptr;}
 AchievementManager* m_achievementMgr=&achievementManager;
 void KilledMonster(CreatureTemplate const*,ObjectGuid);void KilledMonsterCredit(uint32,ObjectGuid={});
 template<class... T> void UpdateAchievementCriteria(T...){}
 uint32 GetGuildId()const{return 0;}void AddKillCreature(uint32,uint16,bool){}void AddTrackingQuestIfNeeded(ObjectGuid){}
 std::map<int,int> nursingProgress;
 unsigned creditPackets=0;
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32){++creditPackets;}
 void SetQuestStatus(uint32,int value){status.Status=value;}
 bool InBattleground()const{return false;}
 int GetQuestObjectiveData(Quest const* quest,int index)const{if(quest->id==39318||quest->id==39321||quest->id==39429){auto it=nursingProgress.find(index);return it==nursingProgress.end()?0:it->second;}return progress;}
 bool HasQuestForItem(uint32)const; void ItemAddedQuestCheck(uint32,uint32); void ItemRemovedQuestCheck(uint32,uint32);
 template<class F> void AddDelayedEvent(int,F callback){callback();} void UpdateForQuestWorldObjects(){}
 void SendCanTakeQuestResponse(int,Quest const*,char const*){}
 InventoryResult CanTakeMoreSimilarItems(uint32,uint32,Item*,uint32*,bool)const;
 uint32 GetItemCount(uint32,bool includeBank,Item* =nullptr)const{return bags+(includeBank?bank:0);}
 uint32 GetItemCountWithLimitCategory(int,Item*)const{return 0;}
 bool CanCompleteQuest(uint32); bool IsQuestWeekRewarded(uint32){return false;} bool IsQuestSeasonalRewarded(uint32){return false;}
 bool CanTakeQuest(Quest const*,bool){return false;}
 QuestStatusData* getQuestStatus(uint32){return &status;}
 bool HasQuestObjectiveComplete(Quest const* quest,QuestObjective const& obj){return quest->id==39456?bool(progress&(1<<obj.StorageIndex)):GetQuestObjectiveData(quest,obj.StorageIndex)>=obj.Amount;}
 void SetQuestObjectiveData(Quest const* quest,QuestObjective const* obj,uint32 count){if(quest->id==39318||quest->id==39321||quest->id==39429)nursingProgress[obj->StorageIndex]=int(count);else progress=int(count);}
 void CompleteQuest(uint32){status.Status=QUEST_STATUS_COMPLETE;++completeCalls;}
 void IncompleteQuest(uint32){status.Status=QUEST_STATUS_INCOMPLETE;++reopenCalls;}
 void DestroyItemCount(int id,int count,bool){assert(uint32(id)==itemTemplate.id && bags>=uint32(count));bags-=count;}
 void RewardItems(Quest const* quest){for(auto const& obj:quest->GetObjectives()){switch(obj.Type){
#include "BloodtotemReward.inc"
 }}}
};
#include "BloodtotemCore.inc"
#include "BloodtotemItems.inc"
#include "BloodtotemKills.inc"
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
 // Native Skyhorn opening, independent of the Bloodtotem branch.
 store.quests.emplace(38913,Quest{38913,0,{}});
 store.quests.emplace(39318,Quest{39318,0,{38913,38913}});
 store.quests.emplace(38910,Quest{38910,0,{39318,39318}});
 for(auto rewarded: {std::set<uint32>{},std::set<uint32>{40229},std::set<uint32>{38913},std::set<uint32>{38913,40229}}){
  p.m_RewardedQuests=rewarded;
  assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(39318),false)==bool(rewarded.count(38913)));
 }
 p.m_RewardedQuests={38913};assert(!p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(38910),false));
 p.m_RewardedQuests.insert(39318);assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(38910),false));
 store.quests[38910].objectives.push_back({QUEST_OBJECTIVE_MONSTER,94991,7,0,0});
 store.quests.emplace(39321,Quest{39321,0,{38910,38910}});
 p={};p.activeQuest=38910;p.status.Status=QUEST_STATUS_INCOMPLETE;
 for(unsigned count=1;count<=9;++count){p.KilledMonsterCredit(94991);assert(p.progress==int(std::min(count,7u)));assert(p.status.Status==(count<7?QUEST_STATUS_INCOMPLETE:QUEST_STATUS_COMPLETE));}
 assert(p.completeCalls==1&&p.creditPackets==7);
 assert(!p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(39321),false));
 p.m_RewardedQuests.insert(38910);assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(39321),false));
 for(int status:{QUEST_STATUS_NONE,QUEST_STATUS_REWARDED}){p={};p.activeQuest=38910;p.status.Status=status;p.KilledMonsterCredit(94991);assert(!p.progress&&!p.creditPackets);}
 p={};p.activeQuest=38910;p.status.Status=QUEST_STATUS_INCOMPLETE;p.KilledMonsterCredit(95261);assert(!p.progress);
 std::cout<<"PASS: Rocs vs Eagles requires exactly seven native credits, caps duplicates, ignores unrelated/absent/rewarded credits; The Three opens only after reward.\n";
 // Haglands is a parallel pair, with both rewards required before The Witchqueen.
 store.quests[39321].ExclusiveGroup=-39321;
 store.quests.emplace(39429,Quest{39429,-39321,{38910}});
 store.mExclusiveQuestGroups.emplace(-39321,39321);store.mExclusiveQuestGroups.emplace(-39321,39429);
 store.quests.emplace(39322,Quest{39322,0,{39321,39321,39429}});
 for(uint32 id:{39321u,39429u}){p={};assert(!p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(id),false));p.m_RewardedQuests={38910};assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(id),false));}
 for(unsigned mask=0;mask<4;++mask){p={};p.m_RewardedQuests={38910};if(mask&1)p.m_RewardedQuests.insert(39321);if(mask&2)p.m_RewardedQuests.insert(39429);assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(39322),false)==(mask==3));}
 store.quests[39321].objectives={{QUEST_OBJECTIVE_MONSTER,95194,1,1,0},{QUEST_OBJECTIVE_MONSTER,95195,1,1,1},{QUEST_OBJECTIVE_MONSTER,95196,1,1,2}};
 for(unsigned mask=0;mask<8;++mask){p={};p.activeQuest=39321;p.status.Status=QUEST_STATUS_INCOMPLETE;for(unsigned index=0;index<3;++index)if(mask&(1u<<index))p.KilledMonsterCredit(95194+index);assert((p.status.Status==QUEST_STATUS_COMPLETE)==(mask==7));}
 std::vector<uint32> order={95194,95195,95196};
 do{p={};p.activeQuest=39321;p.status.Status=QUEST_STATUS_INCOMPLETE;for(uint32 id:order){p.KilledMonsterCredit(id);p.KilledMonsterCredit(id);}assert(p.completeCalls==1&&p.creditPackets==3&&p.nursingProgress.size()==3);}while(std::next_permutation(order.begin(),order.end()));
 for(int state:{QUEST_STATUS_NONE,QUEST_STATUS_REWARDED}){p={};p.activeQuest=39321;p.status.Status=state;for(uint32 id:{95194u,95195u,95196u})p.KilledMonsterCredit(id);assert(p.nursingProgress.empty());}
 std::cout<<"PASS: The Three requires all three distinct kills in every subset/order; duplicate and absent/rewarded credits ignored; both Haglands quests open together and both rewards gate The Witchqueen.\n";
 store.quests[39429].objectives={{QUEST_OBJECTIVE_MONSTER,95266,8,0,0},{QUEST_OBJECTIVE_MONSTER,95277,6,0,1}};
 for(uint32 harpy:{95265u,95266u,105068u}){
  p={};p.activeQuest=39429;p.status.Status=QUEST_STATUS_INCOMPLETE;
  CreatureTemplate enemy{harpy,{harpy==95266?0u:95266u,0}};killedCreature.entry=harpy;
  for(int n=0;n<8;++n)p.KilledMonster(&enemy,{harpy});
  assert(p.nursingProgress[0]==8&&!p.CanCompleteQuest(39429));
  for(int n=0;n<5;++n)p.KilledMonsterCredit(95277);assert(!p.CanCompleteQuest(39429));
  p.KilledMonsterCredit(95277);assert(p.nursingProgress[1]==6&&p.completeCalls==1);
 }
 std::cout<<"PASS: Assaulting the Haglands native harpy aliases count once; eight harpies alone or five loyalists are insufficient; eight plus six completes.\n";
 store.quests[39318].objectives={{QUEST_OBJECTIVE_MONSTER,95261,8,2,0},{QUEST_OBJECTIVE_MONSTER,95403,1,0,1}};
 // Extracted production kill-credit and completion handlers: neither objective alone completes.
 p={};p.activeQuest=39318;p.status.Status=QUEST_STATUS_INCOMPLETE;
 p.KilledMonsterCredit(95403);assert(p.nursingProgress[1]==1 && !p.CanCompleteQuest(39318));
 p.KilledMonsterCredit(95403);assert(p.nursingProgress[1]==1);
 for(int count=1;count<=9;++count){p.KilledMonsterCredit(95261);assert(p.nursingProgress[0]==std::min(count,8));assert(p.status.Status==(count<8?QUEST_STATUS_INCOMPLETE:QUEST_STATUS_COMPLETE));}
 assert(p.completeCalls==1 && p.creditPackets==9);
 p={};p.activeQuest=39318;p.status.Status=QUEST_STATUS_INCOMPLETE;
 for(int count=0;count<8;++count)p.KilledMonsterCredit(95261);
 assert(!p.CanCompleteQuest(39318));p.KilledMonsterCredit(95403);assert(p.completeCalls==1);
 for(int status:{QUEST_STATUS_NONE,QUEST_STATUS_REWARDED}){p={};p.activeQuest=39318;p.status.Status=status;p.KilledMonsterCredit(95261);p.KilledMonsterCredit(95403);assert(p.nursingProgress.empty());}
 p={};
 std::cout<<"PASS: Skyhorn prerequisite independent of Bloodtotem; Julan plus exactly eight fires required, duplicate credits capped, absent/rewarded quests ignored, next quest requires reward.\n";

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
 // Stonedark Focus: actual production loot eligibility and item objective lifecycle.
 store.quests[39425].objectives.push_back({QUEST_OBJECTIVE_ITEM,128343,1,0,0});
 itemTemplate.id=128343;itemTemplate.sparse={0,1};
 p={};p.activeQuest=39425;
 assert(!p.HasQuestForItem(128343));
 p.status.Status=QUEST_STATUS_INCOMPLETE;
 assert(p.HasQuestForItem(128343) && !p.HasQuestForItem(128380) && !p.CanCompleteQuest(39425));
 p.raid=true;assert(!p.HasQuestForItem(128343));p.raid=false;
 assert(p.CanTakeMoreSimilarItems(128343,1,nullptr,nullptr,false)==EQUIP_ERR_OK);
 p.ItemAddedQuestCheck(128380,1);assert(p.progress==0);
 p.bags=1;p.ItemAddedQuestCheck(128343,1);
 assert(p.progress==1 && p.status.Status==QUEST_STATUS_COMPLETE && !p.HasQuestForItem(128343));
 p.ItemAddedQuestCheck(128343,1);assert(p.completeCalls==1);
 // DestroyItemCount invokes the removal hook before changing the stack count.
 p.ItemRemovedQuestCheck(128343,1);p.bags=0;
 assert(p.progress==0 && p.status.Status==QUEST_STATUS_INCOMPLETE && p.HasQuestForItem(128343));
 p.bags=1;p.ItemAddedQuestCheck(128343,1);assert(p.status.Status==QUEST_STATUS_COMPLETE);
 p.bank=1;p.RewardItems(store.GetQuestTemplate(39425));assert(p.bags==0 && p.bank==1);
 p.status.Status=QUEST_STATUS_REWARDED;p.m_RewardedQuests.insert(39425);
 assert(!p.HasQuestForItem(128343) && !p.CanCompleteQuest(39425));
 p={};p.activeQuest=39425;p.ItemAddedQuestCheck(128343,1);assert(p.progress==0);
 std::cout<<"PASS: Stonedark Focus absent/wrong-item/raid/completed/rewarded loot gates, collect -> complete -> discard -> incomplete -> recollect, duplicate notification, reward removes exactly one from bags.\n";
 store.quests[39588].objectives.push_back({QUEST_OBJECTIVE_MONSTER,95290,10,0,0});
 // TDB735 direct target and every alternate KillCredit1 mapping.
 for(uint32 entry:{95290u,95707u,96423u,96615u,100055u,104323u,104328u,105065u,105066u}){
  CreatureTemplate creature{entry,{entry==95290?0u:95290u,0}};killedCreature.entry=entry;
  p={};p.activeQuest=39588;p.status.Status=QUEST_STATUS_INCOMPLETE;
  for(unsigned count=1;count<=12;++count){
   p.KilledMonster(&creature,{entry});
   assert(p.progress==int(std::min(count,10u)) && p.creditPackets==std::min(count,10u));
   assert(p.status.Status==(count<10?QUEST_STATUS_INCOMPLETE:QUEST_STATUS_COMPLETE));
   assert(p.completeCalls==(count<10?0u:1u));
  }
  for(int status:{QUEST_STATUS_NONE,QUEST_STATUS_REWARDED}){
   p={};p.activeQuest=39588;p.status.Status=status;p.KilledMonster(&creature,{entry});assert(p.progress==0 && p.creditPackets==0);
  }
  p={};p.activeQuest=39588;p.status.Status=QUEST_STATUS_INCOMPLETE;p.raid=true;
  p.KilledMonster(&creature,{entry});assert(p.progress==0);
 }
 // The cave Warmonger is a different native template, not an alias inferred from its name.
 CreatureTemplate caveWarmonger{96307,{0,0}};killedCreature.entry=96307;
 p={};p.activeQuest=39588;p.status.Status=QUEST_STATUS_INCOMPLETE;
 p.KilledMonster(&caveWarmonger,{96307});assert(p.progress==0 && p.creditPackets==0);
 std::cout<<"PASS: all nine native Feltotem credit paths count once per kill, complete exactly at 10, cap progress, ignore absent/rewarded/raid quests and exclude cave Warmonger 96307.\n";
 store.quests[39426].objectives.push_back({QUEST_OBJECTIVE_MONSTER,95707,1,1,1});
 p={};p.activeQuest=39426;p.status.Status=QUEST_STATUS_INCOMPLETE;
 assert(!p.CanCompleteQuest(39426)); // Native flag 1 tracks the minimap; it must not bypass the kill.
 CreatureTemplate torok{95707,{95290,0}};killedCreature.entry=95707;
 p.KilledMonster(&torok,{95707});assert(p.progress==1 && p.creditPackets==1 && p.completeCalls==1);
 p.KilledMonster(&torok,{95707});assert(p.progress==1 && p.creditPackets==1 && p.completeCalls==1);
 std::cout<<"PASS: Blood Debt native tracked objective remains incomplete before Torok dies; one kill completes once, alternate Feltotem credit does not double-count.\n";
 store.quests.emplace(40229,Quest{40229,0,{39426}});
 store.quests.emplace(39456,Quest{39456,0,{40229}});
 store.quests[39456].objectives={{3,99619,1,0,0},{0,99619,1,3,1},{0,99624,1,12,2}};
 for(int mask=0;mask<8;++mask){p={};p.activeQuest=39456;p.status.Status=QUEST_STATUS_INCOMPLETE;p.progress=mask;assert(p.CanCompleteQuest(39456)==((mask&3)==3));}
 p={};assert(!p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(39456),false));p.m_RewardedQuests.insert(40229);assert(p.SatisfyQuestPreviousQuest(store.GetQuestTemplate(39456),false));
 std::cout<<"PASS: Unexpected Allies requires both meeting and witness (all eight native objective subsets); optional wait alone cannot complete; Step into the Dark must be rewarded.\n";
 std::cout<<"PASS: extracted prerequisites (all 8 subsets for both branch groups), inventory limit and client hotfix, login reconciliation at 0/30/49/50/56, bank exclusion, idempotence, 50-item reward removal, untouched absent/rewarded quests.\n";
}
