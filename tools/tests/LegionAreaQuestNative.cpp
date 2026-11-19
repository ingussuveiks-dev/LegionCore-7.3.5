// Matching26972 task/area/filter inventory;no character or rotation mutation.
#define HARBARON_NATIVE_SUPPORT_ONLY
#include "HarbaronFragmentNative.cpp"
#include "DisableMgr.h"
static std::string CellText(std::string value){for(char& c:value)if(c=='\t'||c=='\r'||c=='\n')c=' ';return value;}
static std::string IDs(std::vector<uint32> const& values){std::string out;for(auto id:values){if(!out.empty())out+=',';out+=std::to_string(id);}return out;}
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);DisableMgr::LoadDisables();sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sQuestDataStore->LoadQuests();
 // Dungeon bootstrap deliberately registers dungeon maps only. This fixture
 // also instantiates the real outdoor Legion map,as production does at startup.
 std::unordered_map<uint32,std::vector<uint32>> outdoor{{1220,{}}};
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(outdoor);MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(outdoor);
#if defined(AREA_ADMISSION_BASELINE) || defined(AREA_ADMISSION_REPAIRED)
 auto quest=sQuestDataStore->GetQuestTemplate(38321);Require(quest&&quest->Type==QUEST_TYPE_TASK&&quest->AreaGroupID==4416&&!quest->IsWorld(),"Actual restricted area task changed");
 Require(!quest->GetObjectives().empty()&&!quest->SourceItemId,"Baseline needs real objectives and no item mutation");for(auto const& obj:quest->GetObjectives()){std::cout<<"OBJECTIVE id="<<obj.ID<<" type="<<unsigned(obj.Type)<<" object="<<obj.ObjectID<<" count="<<obj.Amount<<" bugged="<<obj.Bugged<<'\n';Require(!obj.Bugged&&obj.Amount>0,"Baseline has an auto-granted or empty objective");}
#ifdef AREA_ADMISSION_BASELINE
 {
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,RACE_HUMAN);player.SetLevel(1);
  bool eligible=player.CanTakeQuest(quest,false);bool disabled=DisableMgr::IsDisabledFor(DISABLE_TYPE_QUEST,38321,&player);
  player.UpdateAreaQuestTasks(7603,0);bool present=player.FindQuestSlot(38321)<MAX_QUEST_LOG_SIZE;
  std::cout<<"COUNTERFACTUAL task=38321 area=7603 race=Human level=1 CanTake="<<eligible<<" disabled="<<disabled<<" actualAreaAdded="<<present<<" status="<<unsigned(player.GetQuestStatus(38321))<<'\n';
  Require(!eligible&&present,"Expected pre-fix area admission bypass changed");
 }
 std::cout<<"COUNTERFACTUAL actual area task38321 added despite native CanTakeQuest refusal;expected broken,not a passing ordinary run\n";
#else
 Require(!sQuestV2CliTaskStore.LookupEntry(38321),"Review changed client task fallback");
 auto gates=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=38321");
 Require(gates&&!gates->Fetch()[0].GetUInt32()&&quest->prevQuests.empty(),"Load new task prerequisites before running this fixture");
 unsigned cases=0;
 for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint8 level:{uint8(1),uint8(97),uint8(98),uint8(110)}){
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,race);player.SetLevel(level);
  bool expected=race==RACE_ORC&&level>=98;
  Require(player.CanTakeQuest(quest,false)==expected,"Native ordinary admission differs from matching task metadata");
  player.UpdateAreaQuestTasks(7603,0);
  Require((player.FindQuestSlot(38321)<MAX_QUEST_LOG_SIZE)==expected,"Area task bypassed native race/level admission");
  if(expected){
   auto objective=std::find_if(quest->Objectives.begin(),quest->Objectives.end(),[](QuestObjective const& obj){return obj.Type==QUEST_OBJECTIVE_MONSTER&&obj.Amount>1;});Require(objective!=quest->Objectives.end(),"Native counted objective missing");
   player.QuestObjectiveSatisfy(objective->ObjectID,1,QUEST_OBJECTIVE_MONSTER);
   Require(player.GetQuestObjectiveData(quest,objective->StorageIndex)==1,"Native objective credit missing");
   for(unsigned visit=0;visit<3;++visit){
    player.UpdateAreaQuestTasks(7603,7603);unsigned slots=0;for(unsigned slot=0;slot<MAX_QUEST_LOG_SIZE;++slot)if(player.GetQuestSlotQuestId(slot)==38321)++slots;
    Require(slots==1,"Repeated area entry duplicated the task");
    player.UpdateAreaQuestTasks(0,7603);Require(player.FindQuestSlot(38321)==MAX_QUEST_LOG_SIZE&&player.GetQuestStatus(38321)==QUEST_STATUS_INCOMPLETE,"Leaving area lost accepted task or kept its log slot");
    Require(!player.CanTakeQuest(quest,false),"Paused task unexpectedly counts as a new acceptance");
    player.UpdateAreaQuestTasks(7603,0);Require(player.FindQuestSlot(38321)<MAX_QUEST_LOG_SIZE&&player.GetQuestObjectiveData(quest,objective->StorageIndex)==1,"Area re-entry blocked or reset real objective progress");
   }
   player.UpdateAreaQuestTasks(0,7603);player.SetQuestStatus(38321,QUEST_STATUS_COMPLETE);player.UpdateAreaQuestTasks(7603,0);
   Require(player.GetQuestStatus(38321)==QUEST_STATUS_COMPLETE&&(player.GetQuestSlotState(player.FindQuestSlot(38321))&QUEST_STATE_COMPLETE),"Re-entry reset an already completed task");
   player.UpdateAreaQuestTasks(0,7603);player.SetQuestStatus(38321,QUEST_STATUS_FAILED);player.UpdateAreaQuestTasks(7603,0);Require(player.FindQuestSlot(38321)==MAX_QUEST_LOG_SIZE,"Failed task restarted without abandonment");
   player.RemoveActiveQuest(38321);player.UpdateAreaQuestTasks(7603,0);Require(player.FindQuestSlot(38321)<MAX_QUEST_LOG_SIZE&&player.GetQuestStatus(38321)==QUEST_STATUS_INCOMPLETE&&player.GetQuestObjectiveData(quest,objective->StorageIndex)==0,"Abandoned task did not restart through native admission with fresh progress");
   player.UpdateAreaQuestTasks(0,7603);player.RemoveActiveQuest(38321);player.SetRewardedQuest(38321);player.UpdateAreaQuestTasks(7603,0);Require(player.FindQuestSlot(38321)==MAX_QUEST_LOG_SIZE,"Rewarded task was offered again");
  }else{
   // Legacy/persisted acceptance must not bypass the resume identity guards.
   player.SetQuestStatus(38321,QUEST_STATUS_INCOMPLETE);player.UpdateAreaQuestTasks(7603,0);
   Require(player.FindQuestSlot(38321)==MAX_QUEST_LOG_SIZE,"Ineligible saved task bypassed resume race/level guards");
  }
  std::cout<<"PASS task38321 race="<<unsigned(race)<<" level="<<unsigned(level)<<" admitted="<<expected<<";eligible entry/repeat/leave/resume/progress/complete/failed/rewarded\n";++cases;
 }
 {
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,RACE_ORC);player.SetLevel(98);
  for(unsigned slot=0;slot<MAX_QUEST_LOG_SIZE;++slot)player.SetQuestSlot(slot,1);
  player.UpdateAreaQuestTasks(7603,0);Require(player.FindQuestSlot(38321)==MAX_QUEST_LOG_SIZE&&player.GetQuestStatus(38321)==QUEST_STATUS_NONE,"Full log partially accepted area task");
  for(unsigned slot=0;slot<MAX_QUEST_LOG_SIZE;++slot)player.SetQuestSlot(slot,0);
  player.UpdateAreaQuestTasks(7603,0);Require(player.FindQuestSlot(38321)<MAX_QUEST_LOG_SIZE,"Failed admission did not retry after log space became available");++cases;
 }
 // Controlled metadata variants exercise class filtering through the same
 // native Player method without changing any database template.
 Require(!quest->AllowableClasses,"Review original task class metadata");
 for(int32 mask:{int32((1 << (CLASS_HUNTER-1))),-int32((1 << (CLASS_HUNTER-1)))})for(uint8 cls:{uint8(CLASS_HUNTER),uint8(CLASS_MAGE)}){
  auto mutableQuest=const_cast<Quest*>(quest);mutableQuest->AllowableClasses=mask;
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,RACE_ORC);player.SetLevel(98);player.SetByteValue(UNIT_FIELD_BYTES_0,1,cls);
  bool expected=(cls==CLASS_HUNTER)==(mask>0);player.UpdateAreaQuestTasks(7603,0);Require((player.FindQuestSlot(38321)<MAX_QUEST_LOG_SIZE)==expected,"Native signed class admission differs");
  if(expected)player.UpdateAreaQuestTasks(0,7603);else player.SetQuestStatus(38321,QUEST_STATUS_INCOMPLETE);
  player.UpdateAreaQuestTasks(7603,0);Require((player.FindQuestSlot(38321)<MAX_QUEST_LOG_SIZE)==expected,"Native resume class admission differs");
  mutableQuest->AllowableClasses=0;++cases;
 }
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE area_seed_disables AS SELECT * FROM disables");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE disables LIKE area_seed_disables");WorldDatabase.DirectExecute("INSERT INTO disables SELECT * FROM area_seed_disables");
 WorldDatabase.DirectExecute("INSERT INTO disables (sourceType,entry,flags,params_0,params_1,comment) VALUES (1,38321,0,'','','Isolated native task admission fixture')");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");DisableMgr::LoadDisables();
 {
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,RACE_ORC);player.SetLevel(98);Require(DisableMgr::IsDisabledFor(DISABLE_TYPE_QUEST,38321,&player),"Temporary native disable not loaded");
  player.UpdateAreaQuestTasks(7603,0);Require(player.FindQuestSlot(38321)==MAX_QUEST_LOG_SIZE,"Disabled fresh task admitted");player.SetQuestStatus(38321,QUEST_STATUS_INCOMPLETE);player.UpdateAreaQuestTasks(7603,0);Require(player.FindQuestSlot(38321)==MAX_QUEST_LOG_SIZE,"Disabled paused task resumed");++cases;
 }
 std::cout<<"PASS "<<cases<<" native area-admission fixtures;actual Player method/quest38321/area7603,retained progress,signed class variants,temporary disable,no reward or character save;client dialogue/traversal separate\n";
#endif
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
#endif
 unsigned tasks=0,world=0,legion=0;std::set<uint32> areas;
 for(auto const& pair:sQuestDataStore->GetQuestTemplates()){
  auto q=pair.second;if(q->Type!=QUEST_TYPE_TASK)continue;auto members=sDB2Manager.GetAreasForGroup(q->AreaGroupID);bool isLegion=q->Expansion==6;
  for(auto id:members){areas.insert(id);if(auto area=sAreaTableStore.LookupEntry(id))if(auto map=sMapStore.LookupEntry(area->ContinentID))if(map->ExpansionID==6)isLegion=true;}
  auto cli=sQuestV2CliTaskStore.LookupEntry(q->GetQuestId());
  std::cout<<"TASK\t"<<q->GetQuestId()<<'\t'<<q->Expansion<<'\t'<<q->IsWorld()<<'\t'<<isLegion<<'\t'<<q->AreaGroupID<<'\t'<<q->MinLevel<<'\t'<<q->MaxLevel<<'\t'<<q->AllowableClasses<<'\t'<<q->AllowableRaces<<'\t'<<bool(cli)<<'\t'<<(cli?cli->ConditionID:0)<<'\t'<<(cli?cli->FiltClasses:0)<<'\t'<<(cli?cli->FiltRaces:0)<<'\t'<<(cli?int(cli->FiltMinLevel):-1)<<'\t'<<(cli?int(cli->FiltMaxLevel):-1)<<'\t'<<(cli?cli->WorldStateExpressionID:0)<<'\t'<<(cli?unsigned(cli->FiltCompletedQuestLogic):0)<<'\t'<<(cli?cli->FiltCompletedQuest[0]:0)<<','<<(cli?cli->FiltCompletedQuest[1]:0)<<','<<(cli?cli->FiltCompletedQuest[2]:0)<<'\t'<<q->PrevQuestID<<'\t'<<IDs(members)<<'\t'<<CellText(cli?cli->QuestTitle->Str[0]:q->LogTitle)<<'\n';++tasks;if(q->IsWorld())++world;if(isLegion)++legion;
 }
 for(auto id:areas)if(auto area=sAreaTableStore.LookupEntry(id)){auto map=sMapStore.LookupEntry(area->ContinentID);std::cout<<"AREA\t"<<id<<'\t'<<area->ContinentID<<'\t'<<area->ParentAreaID<<'\t'<<(map?int(map->ExpansionID):-1)<<'\t'<<CellText(area->AreaName->Str[0])<<'\n';}
 auto updates=WorldDatabase.Query("SELECT QuestID,Timer,VariableID,Value,EventID,AreaID FROM world_quest_update ORDER BY QuestID");if(updates)do{auto f=updates->Fetch();std::cout<<"UPDATE\t"<<f[0].GetUInt32()<<'\t'<<f[1].GetUInt32()<<'\t'<<f[2].GetUInt32()<<'\t'<<f[3].GetUInt32()<<'\t'<<f[4].GetUInt32()<<'\t'<<f[5].GetString()<<'\n';}while(updates->NextRow());
 std::cout<<"PASS native area/task catalog tasks="<<tasks<<" world="<<world<<" legion="<<legion<<" areas="<<areas.size()<<";static inventory only,no gameplay or active-rotation mutation\n";
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
