// Native loot/quest handlers and SmartScript SpellHit/action33; world, timers,
// cast scheduling, loot ownership and inventory writes are explicit fixtures.
#include "GreywatchLootFixture.inc"
#include "StormheimTorchTargets.h"
enum {SMART_EVENT_SPELLHIT_TARGET=31,SMART_EVENT_SPELLHIT=8,SMART_ACTION_CALL_KILLEDMONSTER=33};
using ObjectList=std::list<WorldObject*>;
struct TorchEvent {
 struct {struct {uint32 spell=186745,school=0,cooldownMin=10000,cooldownMax=10000;}spellHit;}event;
 struct {struct {uint32 creature=0;}killedMonster;}action;
};
struct TorchAI {
 Creature* me;WorldObject* trigger=nullptr;uint32 cooldown=0;
 WorldObject* GetBaseObject(){return me;}
 bool IsUnit(WorldObject* o){return o&&o->ToUnit();}bool IsPlayer(WorldObject* o){return o&&o->ToPlayer();}
 ObjectList* GetTargets(TorchEvent const&,Unit* u){return new ObjectList{u};}
 void RecalcTimer(TorchEvent&,uint32 a,uint32 b){assert(a==b);cooldown=a;}
 void ProcessAction(TorchEvent& e,Unit* unit,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr){switch(SMART_ACTION_CALL_KILLEDMONSTER){
 #include "TorchAction.inc"
 }}
 void Hit(TorchEvent& e,Unit* unit,SpellInfo const* spell){if(cooldown)return;bool bvar=false;switch(SMART_EVENT_SPELLHIT){
 #include "TorchSpellHit.inc"
 }}
};
int main(int argc,char**argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 TorchTargets::Verify();
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;std::map<uint32,LootStoreItem> orders;itemStore.items[127285].stack=10;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;}
  if(kind=="objective"){uint32 id,objective;int index;QuestObjective o;in>>id>>objective>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[id].objectives.push_back(o);}
  if(kind=="starter"){uint32 entry,id;in>>entry>>id;store.starters.emplace(entry,id);}
  if(kind=="loot"){uint32 entry,required;in>>entry;auto& d=orders[entry];in>>d.itemid>>d.chance>>required>>d.lootmode>>d.groupid>>d.mincount>>d.maxcount>>d.reference>>d.currencyid;d.needs_quest=required;assert(d.IsValid(lootStore,entry));}
 }
 auto& burn=store.quests[39060];auto& collect=store.quests[39061];auto& next=store.quests[39062];burn.special|=QUEST_SPECIAL_FLAGS_KILL;collect.special|=QUEST_SPECIAL_FLAGS_DELIVER;
 assert(burn.objectives.size()==3&&collect.objectives.size()==1&&orders.size()==4);itemStore.items[127285].stack=10;
 QuestMap _questTemplates;for(auto& r:store.quests)_questTemplates[r.first]=&r.second;
 for(auto const& r:_questTemplates){auto qinfo=r.second;
 #include "QuestPrevLoader.inc"
 }
 assert(next.prevQuests.size()==4); // Explicit39472 plus the three incoming links; native OR semantics.
 Creature giver;giver.entry=94318;giver.guid={338673};creatures[giver.guid]=&giver;SmartAI giverAI(&giver);
 Player horde;horde.team=HORDE;horde.race=2;horde.m_RewardedQuests.insert(39059);assert(!horde.CanTakeQuest(&burn,false)&&!horde.CanTakeQuest(&collect,false));
 Player gated;gated.race=1;assert(!gated.CanTakeQuest(&burn,false)&&!gated.CanTakeQuest(&collect,false));gated.Accept(39059);gated.CompleteQuest(39059);assert(!gated.CanTakeQuest(&burn,false));
 for(uint32 predecessor:{39060,39061,39472}){Player p;p.race=1;assert(!p.CanTakeQuest(&next,false));p.Accept(predecessor);p.CompleteQuest(predecessor);assert(!p.CanTakeQuest(&next,false));p.quests.clear();p.m_RewardedQuests.insert(predecessor);assert(p.CanTakeQuest(&next,false));}
 std::array<uint32,3> order{94340,94341,94342};unsigned permutations=0;
 do{Player p;p.race=1;p.m_RewardedQuests.insert(39059);assert(p.CanTakeQuest(&burn,false));p.AddQuestAndCheckCompletion(&burn,&giver);assert(!p.CanRewardQuest(&burn,false));
  for(auto id:order){Creature bunny;bunny.entry=id;TorchAI ai{&bunny};TorchEvent e;e.action.killedMonster.creature=id;SpellInfo spell;spell.Id=1;ai.Hit(e,&p,&spell);assert(!ai.cooldown);spell.Id=186745;
   // A cast attempt/interruption never calls the hit handler. Native hit is the credit boundary.
   auto before=p.packets;ai.Hit(e,&p,&spell);assert(p.packets==before+1&&p.GetQuestObjectiveData(39060,id)==1);ai.Hit(e,&p,&spell);assert(p.packets==before+1);ai.cooldown=0;ai.Hit(e,&p,&spell);assert(p.packets==before+1);
  }assert(p.completions==1&&p.CanRewardQuest(&burn,false));p.quests.clear();p.m_RewardedQuests.insert(39060);RewardNext(p,giver,&burn);assert(p.menu.detailed.back()==39062);++permutations;
 }while(std::next_permutation(order.begin(),order.end()));assert(permutations==6);
 {Creature bunny;TorchAI ai{&bunny};TorchEvent e;e.action.killedMonster.creature=94340;SpellInfo spell;spell.Id=186745;Player a,b,far;a.Accept(39060);b.Accept(39060);far.Accept(39060);far.rewardRange=false;
  Unit pet;pet.ownerPlayer=&a;ai.Hit(e,&pet,&spell);assert(!a.packets);ai.cooldown=0;
  GroupReference c{&far,nullptr},d{&b,&c},f{&a,&d};Group group;group.first=&f;a.group=&group;ai.Hit(e,&a,&spell);assert(a.packets==1&&b.packets==1&&!far.packets);
  Player second;second.Accept(39060);ai.Hit(e,&second,&spell);assert(!second.packets);ai.cooldown=0;ai.Hit(e,&second,&spell);assert(second.packets==1); // Shared10s cooldown is retriable.
  a.group=nullptr;a.quests.clear();a.Accept(39060);assert(!a.GetQuestObjectiveData(39060,94340));ai.cooldown=0;ai.Hit(e,&a,&spell);assert(a.GetQuestObjectiveData(39060,94340)==1);
 }
 for(auto const& row:orders){Player p;p.race=1;p.m_RewardedQuests.insert(39059);Loot empty;empty.owner=&p;assert(!empty.AllowedForPlayer(&p,127285,0,LOOT_ITEM_TYPE_ITEM,true));p.AddQuestAndCheckCompletion(&collect,&giver);
  p.KilledMonsterCredit(row.first);assert(!p.CanCompleteQuest(39061));LootTemplate table;table.Entries={row.second};Loot loot;loot.owner=&p;rolled=100;table.Process(loot,true);assert(loot.generated.empty());rolled=0;table.Process(loot,true);assert(loot.generated.size()==1);Take(p,loot);assert(p.completions==1&&p.CanRewardQuest(&collect,false));
  assert(!p.HasQuestForItem(127285)&&!empty.AllowedForPlayer(&p,127285,0,LOOT_ITEM_TYPE_ITEM,true));p.bags[127285]=0;p.bank[127285]=1;assert(!p.CanRewardQuest(&collect,false));p.bank[127285]=0;p.bags[127285]=1;
  p.ItemRemovedQuestCheck(127285,1);p.bags[127285]=0;assert(!p.CanCompleteQuest(39061));p.bags[127285]=1;p.ItemAddedQuestCheck(127285,1);Consume(p,&collect);assert(!p.bags[127285]);p.quests.clear();p.m_RewardedQuests.insert(39061);RewardNext(p,giver,&collect);assert(p.menu.detailed.back()==39062);
 }
 {Player p;p.Accept(39061);p.bags[127285]=1;p.AdjustQuestReqItemCount(&collect);assert(p.CanCompleteQuest(39061));p.CompleteQuest(39061);assert(p.CanRewardQuest(&collect,false));p.quests.clear();p.Accept(39061);p.AdjustQuestReqItemCount(&collect);assert(p.CanCompleteQuest(39061));p.CompleteQuest(39061);assert(p.CanRewardQuest(&collect,false));} // Retained quest item on reconstructed acceptance; Adjust alone does not change quest status.
 std::cout<<"PASS: native SmartAI SpellHit/action33/group credit and actual quest counters; all6 cache orders, wrong spell, repeated hit, shared10s retry, pet-invoker rejection and independent player/group progress; four native quest-only loot rolls, miss/pickup/remove/bank/consume/reaccept; Alliance prerequisite and actual39062 OR/handoff. Cast targets, cooldown scheduling, world/inventory/packets remain fixture boundaries.\n";
}
