// Execute native condition dispatch and loot attachment using real migration
// rows. Inventory, condition-list AND aggregation and rotation are fixtures.
#include <cstdint>
#include <map>
#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
struct Player;
#include "LandingWorldQuestFixture.inc"
constexpr int CONDITION_WORLD_QUEST=60, CONDITION_QUESTTAKEN=9, CONDITION_QUEST_OBJECTIVE_DONE=41, CONDITION_IN_RAID_OR_GROUP=58;
struct Condition {
 uint32 SourceGroup=0,SourceEntry=142487,ConditionType=60,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0;
 bool NegativeCondition=false;
 bool isLoaded()const{return true;}
 bool Meets(Player*)const;
};
}
#include "LostArmyFixture.inc"
#include "NativeAttachLootCondition.inc"
struct DatabaseFixture{unsigned deletes=0;template<class...T>void PExecute(T...){++deletes;}}WorldDatabase;
#include "NativeConditionLootLoader.inc"
#include "NativeLootItemCondition.inc"
#include "LandingObjectiveLookup.inc"
bool Gate::Condition::Meets(Player* player)const
{
 WorldObject* object=player;bool condMeets=false;
 switch(ConditionType){
#include "NativeWorldQuestCondition.inc"
#include "LandingQuestTaken.inc"
#include "LandingObjectiveDone.inc"
#include "LandingGroup.inc"
 }
#include "LandingNegation.inc"
 return condMeets;
}

int main(int argc,char**argv)
{
 assert(argc==2);
 for(uint32 id:{44737u,44923u}){Gate::store.templates[id]={id,1};store.quests[id].id=id;}
 auto& v=store.quests[44923];v.objectives={{QUEST_OBJECTIVE_MONSTER,115845,10,0,0},{QUEST_OBJECTIVE_MONSTER,115846,1,0,1}};
 auto& fragments=store.quests[45088];fragments.id=45088;fragments.special=QUEST_SPECIAL_FLAGS_DELIVER;
 fragments.objectives={{QUEST_OBJECTIVE_ITEM,142487,1000,0,0}};
 std::map<uint32,std::vector<Condition>> byEntry;
 std::ifstream file(argv[1]);Condition c;unsigned rows=0;
 while(file>>c.SourceGroup>>c.ConditionType>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition){byEntry[c.SourceGroup].push_back(c);++rows;}
 assert(rows==25&&byEntry.size()==7);
 for(auto& [entry,conditions]:byEntry)
 {
  bool vrekt=entry==115846||entry==116180||entry==116600||entry==116601;
  uint32 wq=vrekt?44923:44737;
  LootStoreItem drop;drop.itemid=142487;drop.needs_quest=true;drop.mincount=drop.maxcount=1;drop.chance=100;drop.lootmode=0;
  LootTemplate loot;loot.Entries={drop};
  for(auto& condition:conditions)assert(conditionMgr.addToLootTemplate(&condition,&loot));
  LootItem visible;visible.conditions=loot.Entries[0].conditions;
  LootItem oldRotation,withoutQuota;
  for(auto condition:visible.conditions)
  {
   if(condition->ConditionType==60)oldRotation.conditions.push_back(condition);
   if(condition->ConditionType!=41)withoutQuota.conditions.push_back(condition);
  }
  for(uint32 team:{ALLIANCE,HORDE})
  {
   Gate::store._worldQuest.clear();Gate::store._worldQuest[1][wq]={};
   Player p;p.team=team;p.Accept(45088);
   assert(oldRotation.AllowedForPlayer(&p)); // reproduce migration02's gap
   assert(!visible.AllowedForPlayer(&p)); // rotation alone was the old bug
   p.Accept(wq);assert(visible.AllowedForPlayer(&p));
   Group party;p.group=&party;assert(visible.AllowedForPlayer(&p));
   party.raid=true;assert(!visible.AllowedForPlayer(&p));
   party.raid=false;assert(visible.AllowedForPlayer(&p));p.group=nullptr;
   Gate::store._worldQuest.clear();assert(!visible.AllowedForPlayer(&p));
   Gate::store._worldQuest[1][wq==44923?44737:44923]={};assert(!visible.AllowedForPlayer(&p));
   Gate::store._worldQuest[1][wq]={};
   Loot result;result.owner=&p;rolled=0;loot.Process(result,true);assert(result.generated.size()==1);
   result.Take();assert(p.bags[142487]==1);
   if(vrekt)
   {
    p.SetQuestObjectiveData(&v,&v.objectives[0],9);assert(visible.AllowedForPlayer(&p));
    // Killing Vaettfang first must leave the farm available below ten.
    p.SetQuestObjectiveData(&v,&v.objectives[1],1);assert(visible.AllowedForPlayer(&p));
    p.SetQuestObjectiveData(&v,&v.objectives[1],0);
    p.SetQuestObjectiveData(&v,&v.objectives[0],10);
    assert(withoutQuota.AllowedForPlayer(&p)); // incomplete elite alone cannot gate it
    assert(p.GetQuestStatus(wq)==QUEST_STATUS_INCOMPLETE&&!visible.AllowedForPlayer(&p));
    result.Take();assert(p.bags[142487]==1); // even stale generated loot
    Player other;other.team=team;other.Accept(45088);other.Accept(wq);assert(visible.AllowedForPlayer(&other));
    p.SetQuestObjectiveData(&v,&v.objectives[0],11);assert(!visible.AllowedForPlayer(&p));
    p.SetQuestObjectiveData(&v,&v.objectives[0],0);
   }
   p.CompleteQuest(wq);assert(!visible.AllowedForPlayer(&p));
   p.quests.erase(wq);p.m_RewardedQuests.insert(wq);assert(!visible.AllowedForPlayer(&p));
   p.m_RewardedQuests.erase(wq);p.Accept(wq);assert(visible.AllowedForPlayer(&p));
   p.quests.erase(wq);assert(!visible.AllowedForPlayer(&p));
   p.Accept(wq);p.quests.erase(45088);result.Take();assert(p.bags[142487]==1);
  }
 }
 assert(!WorldDatabase.deletes);
 std::cout<<"PASS: native condition9/41/58/60, negative quota, actual source1 loader/attachment and LootItem visibility; both factions, solo/party/raid, 0/9/10/11 kills, elite first, observers, stale loot, completed/rewarded/absent/reaccepted quests. World, inventory and AND aggregation remain fixtures.\n";
}
