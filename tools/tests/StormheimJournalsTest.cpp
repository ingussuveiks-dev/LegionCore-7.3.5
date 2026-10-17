#include "JournalsFixture.inc"
using Object=WorldObject;
constexpr uint32 TYPEMASK_UNIT=1,TYPEMASK_GAMEOBJECT=2,CONDITION_QUEST_OBJECTIVE_DONE=41;
#include "JournalRelations.inc"
#include "QuestMenu.inc"
#include "QuestSource.inc"
Item* Player::StoreNewItem(ItemPosCountVec const&,uint32 id,bool){bags[id]+=storeCount;static Item item;return &item;}
bool WorldObject::hasInvolvedQuest(uint32 id){if(auto go=dynamic_cast<GameObject*>(this))return go->hasInvolvedQuest(id);auto bounds=store.enders.equal_range(GetEntry());for(auto i=bounds.first;i!=bounds.second;++i)if(i->second==id)return true;return false;}
namespace ObjectAccessor{Object* GetObjectByTypeMask(Player& player,ObjectGuid guid,uint32){if(guid.high==HighGuid::GameObject)return GetGameObject(player,guid);return GetCreature(player,guid);}}
namespace WorldPackets{namespace Quest{struct QuestGiverRequestReward{ObjectGuid QuestGiverGUID;uint32 QuestID;};}}
struct WorldSession {Player* _player;void HandleQuestGiverRequestReward(WorldPackets::Quest::QuestGiverRequestReward&);};
#include "JournalRequest.inc"
struct PlayerConditionEntry{};
struct ConditionStore{PlayerConditionEntry const* LookupEntry(int32){return nullptr;}} sPlayerConditionStore;
namespace WorldPackets{namespace Character{struct FailedPlayerCondition{explicit FailedPlayerCondition(int32){}int Write(){return 0;}};}}
struct ConditionMgr{bool IsPlayerMeetingCondition(Unit*,int32,bool);bool IsPlayerMeetingCondition(Unit*,PlayerConditionEntry const*){assert(false);return false;}};
#include "JournalConditionLookup.inc"
bool HaviCondition(Player* object){uint32 ConditionValue1=39984,ConditionValue2=97990,ConditionValue3=0;bool condMeets=false;switch(CONDITION_QUEST_OBJECTIVE_DONE){
 #include "JournalCondition.inc"
 }return !condMeets; // Live row is negative: no duplicate final marker.
}
using ObjectList=std::vector<WorldObject*>;
constexpr uint32 SMART_ACTION_CALL_KILLEDMONSTER=33;
struct CreditDispatcher {
 Creature* me;bool trigger=false;WorldObject* GetBaseObject(){return me;}
 bool IsUnit(WorldObject* o){return o&&o->ToUnit();}bool IsPlayer(WorldObject* o){return o&&o->ToPlayer();}
 ObjectList* GetTargets(int,Unit* unit){return new ObjectList{unit};}
 void Run(Unit* unit){struct {struct {struct {uint32 creature=97990;} killedMonster;} action;operator int(){return 0;}} e;
  switch(SMART_ACTION_CALL_KILLEDMONSTER){
   #include "JournalSmartCredit.inc"
  }
 }
};
int main(int argc,char** argv){
 _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;LootStoreItem verse;
 ConditionMgr conditions;assert(conditions.IsPlayerMeetingCondition(nullptr,0,false)&&conditions.IsPlayerMeetingCondition(nullptr,36152,false));
 itemStore.items[126946].stack=8;
 while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
  if(kind=="quest"){uint32 id;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.FlagsEx>>q.AllowableRaces>>q.SourceItemId>>q.SourceItemIdCount>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.special;}
  if(kind=="objective"){uint32 id,objective;int index;QuestObjective o;in>>id>>objective>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[id].objectives.push_back(o);}
  if(kind=="starter"||kind=="go-ender"||kind=="npc-ender"){uint32 actor,q;in>>actor>>q;(kind=="starter"?store.starters:store.enders).emplace(actor,q);}
  if(kind=="loot"){uint32 entry,required;in>>entry>>verse.itemid>>verse.chance>>required>>verse.lootmode>>verse.groupid>>verse.mincount>>verse.maxcount>>verse.reference>>verse.currencyid;verse.needs_quest=required;assert(entry==94856&&verse.itemid==126946&&verse.IsValid(lootStore,entry));}
 }
 auto& story=store.quests.at(39405);auto& remnants=store.quests.at(39984);story.special|=QUEST_SPECIAL_FLAGS_DELIVER;remnants.special|=QUEST_SPECIAL_FLAGS_CAST|QUEST_SPECIAL_FLAGS_KILL;
 assert(story.flags&QUEST_FLAGS_AUTOCOMPLETE);assert(story.SourceItemId==126946&&story.SourceItemIdCount==1&&story.objectives.size()==1&&story.objectives[0].Amount==8);assert(remnants.objectives.size()==5&&remnants.objectives[4].Flags==2);
 GameObject poem,journal;poem.entry=251571;poem.guid={115386};poem.guid.high=HighGuid::GameObject;journal.entry=244708;journal.guid={115008};journal.guid.high=HighGuid::GameObject;objects[poem.guid]=&poem;objects[journal.guid]=&journal;
 Creature havi;havi.entry=97973;havi.guid={369621};creatures[havi.guid]=&havi;
 for(auto team:{ALLIANCE,HORDE}){
  Player p;p.team=team;p.race=team==ALLIANCE?1:2;
  // The installed original GO relations already work. A simulated missing
  // relation demonstrates the actual menu/hasQuest dependency, not a live bug.
  auto saved=store.starters;store.starters.clear();p.PrepareQuestMenu(poem.guid);assert(p.menu.quests.items.empty()&&!poem.hasQuest(39405));store.starters=saved;
  for(auto go:{&poem,&journal}){p.PrepareQuestMenu(go->guid);uint32 id=go==&poem?39405:39984;assert(go->hasQuest(id)&&p.menu.quests.items.size()==1&&p.menu.quests.items.at(id)==2);}
  assert(!poem.hasQuest(39984)&&!journal.hasQuest(39405)&&!poem.hasInvolvedQuest(39405));
  Loot empty;empty.owner=&p;assert(!empty.AllowedForPlayer(&p,126946,0,LOOT_ITEM_TYPE_ITEM,true));
  assert(p.CanTakeQuest(&story,false));p.AddQuestAndCheckCompletion(&story,&poem);assert(p.GiveQuestSourceItem(&story));p.AdjustQuestReqItemCount(&story);assert(p.bags[126946]==1&&p.GetQuestObjectiveData(39405,126946)==1&&p.sourcePackets==1&&!p.CanCompleteQuest(39405));
  p.KilledMonsterCredit(94856);assert(p.bags[126946]==1);
  WorldSession session{&p};WorldPackets::Quest::QuestGiverRequestReward request{p.GetGUID(),39405};session.HandleQuestGiverRequestReward(request);assert(p.menu.offers.empty());
  for(int n=2;n<=8;++n){LootTemplate table;table.Entries={verse};Loot loot;loot.owner=&p;rolled=100;table.Process(loot,true);assert(loot.generated.empty());rolled=0;table.Process(loot,true);Take(p,loot);assert(p.GetQuestObjectiveData(39405,126946)==n&&p.completions==(n==8?1:0));}
  assert(p.CanRewardQuest(&story,false)&&!p.HasQuestForItem(126946));session.HandleQuestGiverRequestReward(request);assert(p.menu.offers.size()==1&&p.menu.offers[0]==39405); // Native auto-submit UI flag bypasses an NPC/GO ender.
  story.flags&=~QUEST_FLAGS_AUTOCOMPLETE;session.HandleQuestGiverRequestReward(request);assert(p.menu.offers.size()==1);story.flags|=QUEST_FLAGS_AUTOCOMPLETE;
  p.bags[126946]=7;p.bank[126946]=1;assert(!p.CanRewardQuest(&story,false));p.bank[126946]=0;p.bags[126946]=8;p.ItemRemovedQuestCheck(126946,1);p.bags[126946]=7;assert(p.HasQuestForItem(126946)&&!p.CanCompleteQuest(39405));p.bags[126946]=8;p.ItemAddedQuestCheck(126946,1);assert(p.CanRewardQuest(&story,false));Consume(p,&story);assert(!p.bags[126946]);p.quests.erase(39405);p.m_RewardedQuests.insert(39405);assert(!p.CanTakeQuest(&story,false));
  Player noSpace;noSpace.Accept(39405);noSpace.sourceResult=3;assert(!noSpace.GiveQuestSourceItem(&story)&&!noSpace.bags[126946]&&!noSpace.sourcePackets); // Full inventory source-grant failure remains a failure.
  Player restored;restored.Accept(39405);restored.bags[126946]=7;restored.AdjustQuestReqItemCount(&story);assert(restored.GetQuestObjectiveData(39405,126946)==7&&!restored.CanCompleteQuest(39405));restored.bags[126946]=8;restored.ItemAddedQuestCheck(126946,1);assert(restored.CanRewardQuest(&story,false));
  Player owner;owner.Accept(39984);Unit pet;pet.ownerPlayer=&owner;CreditDispatcher petTarget{&havi};petTarget.Run(&pet);assert(!owner.GetQuestObjectiveData(39984,97990));petTarget.Run(&owner);assert(owner.GetQuestObjectiveData(39984,97990)==1&&!owner.CanCompleteQuest(39984));
  std::array<uint32,4> books{244717,244718,244719,244720};unsigned orders=0;
  do{for(bool early:{false,true}){
   Player reader;reader.team=team;reader.race=p.race;assert(reader.CanTakeQuest(&remnants,false));reader.AddQuestAndCheckCompletion(&remnants,&journal);
   CreditDispatcher dispatcher{&havi};auto meet=[&]{if(HaviCondition(&reader))dispatcher.Run(&reader);};
   if(early){meet();assert(reader.GetQuestObjectiveData(39984,97990)==1&&!reader.CanCompleteQuest(39984));}
   for(unsigned n=0;n<books.size();++n){reader.KillCreditGO(books[n],{100+n});reader.KillCreditGO(books[n],{100+n});assert(reader.GetQuestObjectiveData(39984,books[n])==1);if(n<3||!early)assert(!reader.completions);}
   meet();meet();assert(reader.GetQuestObjectiveData(39984,97990)==1&&reader.completions==1&&reader.CanRewardQuest(&remnants,false));
   WorldSession reward{&reader};WorldPackets::Quest::QuestGiverRequestReward wrong{journal.guid,39984},right{havi.guid,39984};reward.HandleQuestGiverRequestReward(wrong);assert(reader.menu.offers.empty());reward.HandleQuestGiverRequestReward(right);assert(reader.menu.offers.size()==1&&reader.menu.offers[0]==39984);
  }++orders;}while(std::next_permutation(books.begin(),books.end()));assert(orders==24);
 }
 std::cout<<"PASS: native GO hasQuest/hasInvolvedQuest and PrepareQuestMenu for both original starters, GiveQuestSourceItem first verse plus7 real loot drops, failure/cap/bank/removal/consumption, native auto-submit RequestReward without invented ender. Remnants:24 book orders, both factions, repeated clicks, early/late Havi native condition and group credit, no completion before all5 objectives, correct NPC reward admission. World/session acceptance, inventory storage, AI LOS and packet services remain fixtures; client pending.\n";
}
