// Native26972 quest-task prerequisite routing,without character saves.
#define HARBARON_NATIVE_SUPPORT_ONLY
#include "HarbaronFragmentNative.cpp"
#include "DisableMgr.h"
#include "QuestTaskPrerequisites.h"
#ifndef TASK_PREREQUISITE_BASELINE
struct TaskWorldAreasTag { using Type=QuestAreaTaskMap QuestDataStoreMgr::*;friend Type Access(TaskWorldAreasTag); };
template struct FixtureAccess<TaskWorldAreasTag,&QuestDataStoreMgr::_worldQuestAreaTaskStore>;
#endif
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);DisableMgr::LoadDisables();sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sQuestDataStore->LoadQuests();
 std::cout<<"LAYOUT completed="<<offsetof(QuestV2CliTaskEntry,FiltCompletedQuest)<<" logic="<<offsetof(QuestV2CliTaskEntry,FiltCompletedQuestLogic)<<std::endl;
 std::unordered_map<uint32,std::vector<uint32>> outdoor{{1220,{}}};dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(outdoor);MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(outdoor);
 for(auto task:sQuestV2CliTaskStore){unsigned count=0;for(auto id:task->FiltCompletedQuest)if(id)++count;if(count<2)continue;
  std::cout<<"TASK id="<<task->ID<<" logic="<<unsigned(task->FiltCompletedQuestLogic)<<" prerequisites="<<task->FiltCompletedQuest[0]<<','<<task->FiltCompletedQuest[1]<<','<<task->FiltCompletedQuest[2]<<" condition="<<task->ConditionID<<" title="<<task->QuestTitle->Str[0]<<'\n';
 }
 auto quest=sQuestDataStore->GetQuestTemplate(36811);auto cli=sQuestV2CliTaskStore.LookupEntry(36811);Require(quest&&cli&&cli->FiltCompletedQuestLogic==2&&cli->FiltCompletedQuest[0]==36920&&cli->FiltCompletedQuest[1]==40815&&!cli->FiltCompletedQuest[2],"Actual alternative prerequisite row changed");
 auto condition=sPlayerConditionStore.LookupEntry(cli->ConditionID);
 std::cout<<"CONDITION id="<<cli->ConditionID<<" present="<<bool(condition)<<";actual ConditionMgr admission is exercised below;missing row keeps its existing native semantics"<<std::endl;
 Require(!quest->SourceItemId&&!quest->GetObjectives().empty(),"Task fixture needs objectives and no item grants");for(auto const& obj:quest->GetObjectives()){std::cout<<"OBJECTIVE id="<<obj.ID<<" type="<<unsigned(obj.Type)<<" count="<<obj.Amount<<" bugged="<<obj.Bugged<<std::endl;Require(!obj.Bugged&&obj.Amount>0,"Review automatic objective completion before native fixture");}
 unsigned cases=0;
 for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(unsigned mask=0;mask<4;++mask){
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,race);player.SetLevel(100);if(mask&1)player.SetRewardedQuest(36920);if(mask&2)player.SetRewardedQuest(40815);
  bool other=sConditionMgr->IsPlayerMeetingCondition(&player,cli->ConditionID);player.UpdateAreaQuestTasks(7349,0);bool added=player.FindQuestSlot(36811)<MAX_QUEST_LOG_SIZE;
  std::cout<<"OBSERVE race="<<unsigned(race)<<" rewarded-mask="<<mask<<" native-condition="<<other<<" added="<<added<<std::endl;
  Require(other,"Native alternative prerequisite fixture has another unmet gate");
#ifdef TASK_PREREQUISITE_BASELINE
  Require(added==(mask==3),"Expected original all-AND task admission differs");
#else
  Require(added==(mask!=0),"Native task ignored authored OR prerequisite");
#endif
  if(added){
   auto obj=std::find_if(quest->Objectives.begin(),quest->Objectives.end(),[](QuestObjective const& objective){return objective.Type==QUEST_OBJECTIVE_MONSTER&&objective.Amount>1;});Require(obj!=quest->Objectives.end(),"Actual counted objective missing");player.QuestObjectiveSatisfy(obj->ObjectID,1,QUEST_OBJECTIVE_MONSTER);
   player.UpdateAreaQuestTasks(7349,7349);unsigned slots=0;for(unsigned slot=0;slot<MAX_QUEST_LOG_SIZE;++slot)if(player.GetQuestSlotQuestId(slot)==36811)++slots;Require(slots==1,"Prerequisite admission duplicated a task");player.UpdateAreaQuestTasks(0,7349);player.UpdateAreaQuestTasks(7349,0);Require(player.FindQuestSlot(36811)<MAX_QUEST_LOG_SIZE&&player.GetQuestObjectiveData(quest,obj->StorageIndex)==1,"Alternative route or progress was lost on area re-entry");
   player.UpdateAreaQuestTasks(0,7349);player.RemoveActiveQuest(36811);player.UpdateAreaQuestTasks(7349,0);Require(player.FindQuestSlot(36811)<MAX_QUEST_LOG_SIZE&&player.GetQuestObjectiveData(quest,obj->StorageIndex)==0,"Alternative-route abandonment/retry failed");
   player.UpdateAreaQuestTasks(0,7349);player.RemoveActiveQuest(36811);player.SetRewardedQuest(36811);player.UpdateAreaQuestTasks(7349,0);Require(player.FindQuestSlot(36811)==MAX_QUEST_LOG_SIZE,"Rewarded bonus task was reoffered");
  }
  ++cases;
 }
#ifdef TASK_PREREQUISITE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" actual native quest36811 admission cases;each legitimate single-prerequisite route rejected by original all-AND loop;expected broken,not passing gameplay\n";
#else
 for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint8 level:{uint8(97),uint8(98),uint8(109),uint8(110)}){
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,race);player.SetLevel(level);player.SetRewardedQuest(40815);player.UpdateAreaQuestTasks(7349,0);Require((player.FindQuestSlot(36811)<MAX_QUEST_LOG_SIZE)==(level>=98&&level<=109),"Alternative route bypassed native client-task level range");++cases;
 }
 {
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,RACE_HUMAN);player.SetLevel(100);player.SetRewardedQuest(36920);for(unsigned slot=0;slot<MAX_QUEST_LOG_SIZE;++slot)player.SetQuestSlot(slot,1);
  player.UpdateAreaQuestTasks(7349,0);Require(player.FindQuestSlot(36811)==MAX_QUEST_LOG_SIZE&&player.GetQuestStatus(36811)==QUEST_STATUS_NONE,"Alternative route partially admitted a full-log task");for(unsigned slot=0;slot<MAX_QUEST_LOG_SIZE;++slot)player.SetQuestSlot(slot,0);player.UpdateAreaQuestTasks(7349,0);Require(player.FindQuestSlot(36811)<MAX_QUEST_LOG_SIZE,"Alternative route did not retry after log space was freed");++cases;
 }
 unsigned expressions=0,rows=0;
 {
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,RACE_ORC);
  for(auto const& row:sQuestDataStore->GetQuestTemplates()){
   auto q=row.second;if(q->Type!=QUEST_TYPE_TASK)continue;auto task=sQuestV2CliTaskStore.LookupEntry(row.first);if(!task)continue;
   unsigned count=0;for(auto id:task->FiltCompletedQuest)if(id)++count;
   Require((count<=1&&task->FiltCompletedQuestLogic==0)||(count==2&&task->FiltCompletedQuestLogic==2)||(count==3&&task->FiltCompletedQuestLogic==5),"Review new client prerequisite expression shape");
   Require(count==0||task->FiltCompletedQuest[0],"Review sparse client prerequisite list");
   for(auto id:task->FiltCompletedQuest)if(id)Require(sQuestDataStore->GetQuestTemplate(id),"Native prerequisite quest absent");
   for(unsigned mask=0;mask<(1u<<count);++mask){
    // Explicit native quest states isolate the expression,including metadata
    // that references repeatables. Never call RemoveRewardedQuest here:that
    // maintenance API executes a persistent DELETE as well as changing memory.
    for(unsigned i=0;i<count;++i)player.SetQuestStatus(task->FiltCompletedQuest[i],mask&(1u<<i)?QUEST_STATUS_REWARDED:QUEST_STATUS_NONE);
    bool expected=count==0||(count==1?mask==1:count==2?mask!=0:mask==7);
    Require(MeetsQuestTaskPrerequisites(player,*task)==expected,"Actual client prerequisite expression truth table differs");++expressions;
   }
   for(auto id:task->FiltCompletedQuest)if(id)player.SetQuestStatus(id,QUEST_STATUS_NONE);++rows;
  }
 }
 // Actual WorldQuest/area controller with a connection-free controlled active
 // rotation entry. No scheduler/character rows are loaded or changed.
 auto worldQuest=sQuestDataStore->GetQuestTemplate(46134);auto worldCli=sQuestV2CliTaskStore.LookupEntry(46134);Require(worldQuest&&worldQuest->IsWorld()&&worldCli&&worldCli->FiltCompletedQuestLogic==5,"Native three-prerequisite world task changed");
 if(auto pc=sPlayerConditionStore.LookupEntry(worldCli->ConditionID)){
  std::cout<<"WORLD_CONDITION id="<<pc->ID<<" level="<<pc->MinLevel<<','<<pc->MaxLevel<<" race="<<pc->RaceMask<<" class="<<pc->ClassMask<<" modifier="<<pc->ModifierTreeID<<" prevLogic="<<pc->PrevQuestLogic<<" currLogic="<<pc->CurrQuestLogic<<" expression="<<pc->WorldStateExpressionID<<std::endl;
  for(unsigned i=0;i<4;++i)std::cout<<"WORLD_FILTER slot="<<i<<" skill="<<pc->SkillID[i]<<','<<pc->MinSkill[i]<<','<<pc->MaxSkill[i]<<" prev="<<pc->PrevQuestID[i]<<" current="<<pc->CurrQuestID[i]<<" completed="<<pc->CurrentCompletedQuestID[i]<<" spell="<<pc->SpellID[i]<<" item="<<pc->ItemID[i]<<','<<pc->ItemCount[i]<<" area="<<pc->AreaID[i]<<std::endl;
 }
 for(auto const& obj:worldQuest->GetObjectives())Require(!obj.Bugged&&obj.Amount>0,"Review automatic world-task objectives before fixture");Require(!worldQuest->SourceItemId,"Review world-task source item");
 WorldQuestTemplate rewardTemplate;WorldQuest rotation;rotation.QuestID=46134;rotation.quest=worldQuest;rotation.worldQuest=&rewardTemplate;rotation.ResetTime=uint32(GameTime::GetGameTime()+600);
 auto rotations=const_cast<WorldQuestMap*>(sQuestDataStore->GetWorldQuestMap());Require(rotations&&rotations->empty(),"Native fixture must not load persistent active rotation");(*rotations)[worldQuest->QuestSortID][46134]=rotation;
 auto& areas=sQuestDataStore->*Access(TaskWorldAreasTag{});areas[7349].insert(worldQuest);
 Require(sWorld->getBoolConfig(CONFIG_WORLD_QUEST),"World quests disabled in fixture configuration");
 for(unsigned mask=0;mask<8;++mask){
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,RACE_HUMAN);for(unsigned i=0;i<3;++i)if(mask&(1u<<i))player.SetRewardedQuest(worldCli->FiltCompletedQuest[i]);
  if(worldCli->FiltMinSkillID)player.SetSkill(worldCli->FiltMinSkillID,1,worldCli->FiltMinSkillValue,worldCli->FiltMinSkillValue);
  Require(!sConditionMgr->IsPlayerMeetingCondition(&player,worldCli->ConditionID),"World task should retain its native Isles unlock gate");player.UpdateAreaQuestTasks(7349,0);Require(player.FindQuestSlot(46134)==MAX_QUEST_LOG_SIZE,"World task offered before native Isles unlock");
  auto pc=sPlayerConditionStore.LookupEntry(worldCli->ConditionID);Require(pc&&pc->PrevQuestID[0]==41694,"Native world task unlock changed");player.SetRewardedQuest(41694);auto bit=sDB2Manager.GetQuestUniqueBitFlag(41694);Require(bit,"Actual native Isles unlock bit unavailable");player.SetQuestCompletedBit(bit,true);
  Require(sConditionMgr->IsPlayerMeetingCondition(&player,worldCli->ConditionID),"Native Isles reward bit did not unlock world tasks");
  player.UpdateAreaQuestTasks(7349,0);Require((player.FindQuestSlot(46134)<MAX_QUEST_LOG_SIZE)==(mask==7),"Native world-task AND admission changed");
  if(mask==7){player.UpdateAreaQuestTasks(0,7349);player.UpdateAreaQuestTasks(7349,0);Require(player.FindQuestSlot(46134)<MAX_QUEST_LOG_SIZE,"Native world-task AND re-entry failed");}++cases;
 }
 // Neither a missing rotation nor an expired entry may be started by the
 // expression repair,even with every genuine prerequisite/unlock present.
 for(bool expired:{true,false}){
  if(expired)(*rotations)[worldQuest->QuestSortID][46134].ResetTime=uint32(GameTime::GetGameTime());else (*rotations)[worldQuest->QuestSortID].erase(46134);
  Map map(1220,300,0,DIFFICULTY_NONE);DungeonPlayer player(map,RACE_HUMAN);for(auto id:worldCli->FiltCompletedQuest)player.SetRewardedQuest(id);player.SetRewardedQuest(41694);player.SetQuestCompletedBit(sDB2Manager.GetQuestUniqueBitFlag(41694),true);if(worldCli->FiltMinSkillID)player.SetSkill(worldCli->FiltMinSkillID,1,worldCli->FiltMinSkillValue,worldCli->FiltMinSkillValue);
  player.UpdateAreaQuestTasks(7349,0);Require(player.FindQuestSlot(46134)==MAX_QUEST_LOG_SIZE,"Inactive or expired world task was offered");++cases;
 }
 areas.clear();rotations->clear();
 std::cout<<"PASS "<<expressions<<" native completed-quest expression cases across"<<rows<<" actual task rows;0/single/OR/AND metadata retained\n";
 std::cout<<"PASS "<<cases<<" native area/controller cases:Retake Faronaar alternatives,level/log/progress/retry/reward guards;Gemcutter AND/Isles unlock/active/expired rotation;client offer/traversal separate\n";
#endif
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
