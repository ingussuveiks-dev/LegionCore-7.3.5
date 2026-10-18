#include <cstdint>
#include <map>
#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
namespace Gate {
using uint32=uint32_t;
struct Quest {uint32 id=0,QuestSortID=0;uint32 GetQuestId()const{return id;}};
struct WorldQuest{};
namespace Trinity{namespace Containers{template<class K,class V>V const* MapGetValuePtr(std::map<K,V> const& m,K k){auto i=m.find(k);return i==m.end()?nullptr:&i->second;}}}
struct QuestDataStoreMgr{
 std::map<uint32,Quest> templates;std::map<uint32,std::map<uint32,WorldQuest>> _worldQuest;
 Quest const* GetQuestTemplate(uint32 id){auto i=templates.find(id);return i==templates.end()?nullptr:&i->second;}
 WorldQuest const* GetWorldQuest(Quest const*);
}store;auto sQuestDataStore=&store;
#include "NativeWorldQuestLookup.inc"
constexpr int CONDITION_WORLD_QUEST=60;
struct Condition{uint32 SourceGroup=0,SourceEntry=142487,ConditionValue1=0;bool isLoaded()const{return true;}bool Meets()const{bool condMeets=false;switch(CONDITION_WORLD_QUEST){
#include "NativeWorldQuestCondition.inc"
 }return condMeets;}};
}
#include "LostArmyFixture.inc"
#include "NativeAttachLootCondition.inc"
struct DatabaseFixture{unsigned deletes=0;template<class... T>void PExecute(T...){++deletes;}}WorldDatabase;
#include "NativeConditionLootLoader.inc"
#include "NativeLootItemCondition.inc"
int main(int argc,char** argv){
 assert(argc==2);auto& q=store.quests[45088];q.id=45088;q.special=QUEST_SPECIAL_FLAGS_DELIVER;q.objectives={{QUEST_OBJECTIVE_ITEM,142487,1000,0,0}};
 for(uint32 id:{44737u,44923u,44746u})Gate::store.templates[id]={id,1};
 std::ifstream data(argv[1]);uint32 entry,item,quest,mode,group,min,max,wq;float chance;unsigned rows=0;
 while(data>>entry>>item>>chance>>quest>>mode>>group>>min>>max>>wq){
  assert(item==142487&&quest==1&&!group&&min&&max>=min);++rows;
  LootStoreItem row;row.itemid=item;row.chance=chance;row.needs_quest=quest;row.lootmode=mode;row.groupid=group;row.mincount=min;row.maxcount=max;
  LootTemplate loot;loot.Entries={row};Condition condition;condition.SourceGroup=entry;condition.ConditionValue1=wq;
  if(wq)assert(conditionMgr.addToLootTemplate(&condition,&loot));
  for(uint32 team:{ALLIANCE,HORDE}){
   Player p;p.team=team;p.Accept(45088);Loot result;result.owner=&p;rolled=0;loot.Process(result,true);assert(result.generated.size()==1);
   if(wq){Gate::store._worldQuest.clear();result.Take();assert(!p.bags[item]);Gate::store._worldQuest[1][wq==44737?44923:44737]={};result.Take();assert(!p.bags[item]);Gate::store._worldQuest[1][wq]={};}
   result.Take();assert(p.bags[item]==min&&p.GetQuestObjectiveData(45088,item)==int32(min));
   if(wq){Gate::store._worldQuest.clear();uint32 old=p.bags[item];result.Take();assert(p.bags[item]==old);Gate::store._worldQuest[1][wq]={};}
   p.bags[item]=999;p.AdjustQuestReqItemCount(&q);assert(!p.CanRewardQuest(&q,false));result.Take();assert(p.GetQuestObjectiveData(45088,item)==1000&&p.CanRewardQuest(&q,false)&&!p.HasQuestForItem(item));
   uint32 extra=p.bags[item]-1000;Consume(p,&q);assert(p.bags[item]==extra);p.quests.erase(45088);p.m_RewardedQuests.insert(45088);assert(!p.HasQuestForItem(item));
  }
  Player none;Loot noQuest;noQuest.owner=&none;rolled=0;loot.Process(noQuest,true);noQuest.Take();assert(!none.bags[item]);
 }
 assert(rows==10&&!WorldDatabase.deletes);
 {Player p;p.Accept(45088);p.bags[142487]=700;p.AdjustQuestReqItemCount(&q);p.quests.erase(45088);assert(!p.HasQuestForItem(142487));p.Accept(45088);p.AdjustQuestReqItemCount(&q);assert(p.GetQuestObjectiveData(45088,142487)==700);p.bank[142487]=1000;p.bags[142487]=0;p.AdjustQuestReqItemCount(&q);p.CompleteQuest(45088);assert(!p.CanRewardQuest(&q,false));}
 {Player p;p.Accept(45088);p.bags[142487]=1000;p.AdjustQuestReqItemCount(&q);p.CompleteQuest(45088);p.ItemRemovedQuestCheck(142487,1);--p.bags[142487];assert(p.GetQuestStatus(45088)==QUEST_STATUS_INCOMPLETE&&p.HasQuestForItem(142487));++p.bags[142487];p.ItemAddedQuestCheck(142487,1);assert(p.CanRewardQuest(&q,false));}
 std::cout<<"PASS: actual loot Process/Roll, source1 condition attachment, native world-quest lookup/type60 and LootItem visibility, quest-item progress999/1000, consumption, retained/banked items and both factions. Rotation/inventory/count RNG/storage services are fixtures; live group/client checks pending.\n";
}
