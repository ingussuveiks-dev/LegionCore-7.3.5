#include "BjornhartaLootFixture.inc"

int main(int argc,char** argv)
{
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;LootStoreItem heart;
 itemStore.items[124503].stack=10;
 for(auto id:{39796u,38810u,38817u})store.quests[id].id=id;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id,start,package;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>start>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special>>package;assert(!start&&!q.ExclusiveGroup&&!q.special);}
  if(kind=="objective"){uint32 id,objective;int index;QuestObjective o;in>>id>>objective>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[id].objectives.push_back(o);}
  if(kind=="starter"){uint32 entry,id;in>>entry>>id;store.starters.emplace(entry,id);}
  if(kind=="loot"){uint32 entry,required;in>>entry>>heart.itemid>>heart.chance>>required>>heart.lootmode>>heart.groupid>>heart.mincount>>heart.maxcount>>heart.reference>>heart.currencyid;heart.needs_quest=required;assert(entry==93095&&heart.itemid==124503&&required==1&&heart.mincount==1&&heart.maxcount==1&&!heart.lootmode&&!heart.groupid&&!heart.reference&&!heart.currencyid&&heart.IsValid(lootStore,entry));}
 }
 auto& q=store.quests[38808];auto& next=store.quests[38811];assert(q.objectives.size()==1&&q.objectives[0].ObjectID==124503&&q.objectives[0].Amount==4&&q.PrevQuestID==39796);q.special|=QUEST_SPECIAL_FLAGS_DELIVER;
 QuestMap _questTemplates;for(auto& row:store.quests)_questTemplates[row.first]=&row.second;
 for(auto const& row:_questTemplates){auto qinfo=row.second;
  #include "QuestPrevLoader.inc"
 }
 Creature vydhar;vydhar.entry=93231;vydhar.guid={292106};SmartAI ai(&vydhar);creatures[vydhar.guid]=&vydhar;
 for(auto team:{ALLIANCE,HORDE}){
  Player p;p.team=team;p.race=team==ALLIANCE?1:2;Loot empty;empty.owner=&p;
  assert(!p.CanTakeQuest(&q,false)&&!p.CanTakeQuest(&next,false)&&!empty.AllowedForPlayer(&p,124503,0,LOOT_ITEM_TYPE_ITEM,true));
  p.Accept(39796);p.quests[39796].Status=QUEST_STATUS_COMPLETE;assert(!p.CanTakeQuest(&q,false));p.quests.erase(39796);p.m_RewardedQuests.insert(39796);
  p.canAdd=false;assert(!p.CanAddQuest(&q,false));p.canAdd=true;assert(p.CanTakeQuest(&q,false));p.AddQuestAndCheckCompletion(&q,&vydhar);
  p.KilledMonsterCredit(93095);assert(!p.GetQuestObjectiveData(q.id,124503)&&!p.CanCompleteQuest(q.id));
  for(unsigned n=1;n<=4;++n){LootTemplate table;table.Entries={heart};Loot loot;loot.owner=&p;
   rolled=100;table.Process(loot,true);assert(loot.generated.empty());rolled=0;table.Process(loot,true);assert(loot.generated.size()==1);
   Take(p,loot);assert(p.GetQuestObjectiveData(q.id,124503)==int32(n)&&p.completions==(n==4?1:0));
  }
  assert(p.CanRewardQuest(&q,false)&&!p.CanTakeQuest(&next,false)&&!p.HasQuestForItem(124503)&&!empty.AllowedForPlayer(&p,124503,0,LOOT_ITEM_TYPE_ITEM,true));
  p.bags[124503]=3;p.bank[124503]=1;assert(!p.CanRewardQuest(&q,false));p.bank[124503]=0;p.bags[124503]=4;
  p.ItemRemovedQuestCheck(124503,1);p.bags[124503]=3;assert(!p.CanCompleteQuest(q.id)&&p.HasQuestForItem(124503));p.bags[124503]=4;p.ItemAddedQuestCheck(124503,1);assert(p.CanRewardQuest(&q,false));
  Consume(p,&q);assert(!p.bags[124503]);p.quests.erase(q.id);p.m_RewardedQuests.insert(q.id);assert(!p.CanTakeQuest(&q,false)&&p.CanTakeQuest(&next,false));
  // Native loader treats incoming positive NextQuestID links as alternatives.
  // Bjornharta and Dreaming Fungus can each unlock the existing next quest.
  RewardNext(p,vydhar,&q);assert(p.menu.detailed.size()==1&&p.menu.detailed[0]==38811);
  Player alternate;alternate.m_RewardedQuests.insert(38810);assert(alternate.CanTakeQuest(&next,false));
  Player retry;retry.team=team;retry.race=p.race;retry.m_RewardedQuests.insert(39796);retry.AddQuestAndCheckCompletion(&q,&vydhar);retry.bags[124503]=3;retry.AdjustQuestReqItemCount(&q);assert(retry.GetQuestObjectiveData(q.id,124503)==3&&!retry.CanCompleteQuest(q.id));
  retry.quests.clear();retry.data.clear();assert(!retry.HasQuestForItem(124503));retry.AddQuestAndCheckCompletion(&q,&vydhar);retry.AdjustQuestReqItemCount(&q);assert(!retry.CanCompleteQuest(q.id));retry.bags[124503]++;retry.ItemAddedQuestCheck(124503,1);assert(retry.CanRewardQuest(&q,false));
  Player relog;relog.team=team;relog.race=p.race;relog.m_RewardedQuests=p.m_RewardedQuests;assert(!relog.CanTakeQuest(&q,false)&&relog.CanTakeQuest(&next,false));
 }
 std::cout<<"PASS: both factions, rewarded39796 prerequisite, actual4-heart loot lifecycle, native loot validation/Process/Roll/AllowedForPlayer, missed rolls and kills without loot, bank/removal/retry/reconstructed history/consumption; native loader preserves Bjornharta OR Dreaming Fungus admission and Vydhar38811 details after reward. Inventory storage, corpse ownership, non-quest admission and packets remain fixtures; client pending.\n";
}
