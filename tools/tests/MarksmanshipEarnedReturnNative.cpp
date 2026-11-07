#define MARKSMAN_LOGIN_SUPPORT_ONLY
#include "MarksmanshipLoginRecoveryNative.cpp"
#include "Bag.h"
#include "ConditionMgr.h"
#include "EarnedConditionNative.inc"
struct EarnedItemsTag {using Type=Item* (Player::*)[PLAYER_SLOTS_COUNT];friend Type Access(EarnedItemsTag);};
template struct FixtureAccess<EarnedItemsTag,&Player::m_items>;
struct EarnedBow {
 Player&p;Item*item=nullptr;Bag*bag=nullptr;uint8 slot=EQUIPMENT_SLOT_MAINHAND;
 EarnedBow(Player&v,unsigned mode):p(v){if(mode==14)return;item=Item::CreateItem(mode==18?128808:128826,1,&p);Require(item&&!item->IsInWorld(),"Loading owned item missing");auto&slots=p.*Access(EarnedItemsTag{});if(mode==1||mode==2){auto raw=Item::CreateItem(21841,1,&p);bag=raw?raw->ToBag():nullptr;Require(bag,"Actual loading bag absent");slot=mode==1?INVENTORY_SLOT_BAG_START:BANK_SLOT_BAG_START;slots[slot]=bag;bag->SetSlot(slot);bag->StoreItem(0,item,false);}else{slots[slot]=item;item->SetSlot(slot);}if(mode==15)item->SetGuidValue(ITEM_FIELD_OWNER,ObjectGuid::Create<HighGuid::Player>(999888));if(mode==16)item->SetCount(0);}
 ~EarnedBow(){if(bag)bag->RemoveItem(0,false);(p.*Access(EarnedItemsTag{}))[slot]=nullptr;delete bag;delete item;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 auto quest=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(40419));Require(quest&&quest->GetObjectives().empty(),"Actual40419 bootstrap missing");auto rows=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=40419 ORDER BY StorageIndex");Require(bool(rows),"Native objectives absent");do{quest->LoadQuestObjective(rows->Fetch());}while(rows->NextRow());Require(quest->GetObjectives().size()==6,"Actual objectives changed");
 // Give LoadScriptNames the pending condition name through a private TEMP
 // scope; the production table remains untouched and is not updater input.
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE mm_condition_seed LIKE conditions");WorldDatabase.DirectExecute("INSERT INTO mm_condition_seed SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions LIKE mm_condition_seed");WorldDatabase.DirectExecute("INSERT INTO conditions SELECT * FROM mm_condition_seed");WorldDatabase.DirectExecute("UPDATE conditions SET ScriptName='condition_marksmanship_recovered_arrival' WHERE ConditionTypeOrReference=32 AND ConditionTarget=0 AND ConditionValue1=16");
 sObjectMgr->LoadScriptNames();WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");auto scriptId=sObjectMgr->GetScriptId("condition_marksmanship_recovered_arrival");Require(scriptId,"TEMP native condition name not mapped");sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new condition_marksmanship_recovered_arrival;sScriptMgr->SwapScriptContext(true);
 Condition gate;gate.ConditionType=CONDITION_TYPE_MASK;gate.ConditionValue1=TYPEMASK_PLAYER;gate.ConditionTarget=0;gate.ScriptId=scriptId;
 Condition original=gate;original.ScriptId=0;
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}},{1539,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}},{1539,{}}});
 unsigned cases=0;auto target=sSpellMgr->GetSpellTargetPosition(201674);Require(target,"Native entrance missing");
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)})for(auto status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){
  auto session=Session();MapInstanced parent(1489,300);InstanceMap map(1489,300,9998600+cases,DIFFICULTY_N_SCENARIO,&parent);auto progress=sScenarioMgr->AddScenario(&map,972);
  for(unsigned mode=0;mode<20;++mode){NativePlayer p(session,map,809,{target->target_X,target->target_Y,target->target_Z,target->target_Orientation});p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,mode==7?CLASS_WARLOCK:CLASS_HUNTER);p.setFactionForRace(race);p.SetQuestStatus(40419,status);for(uint32 id:{102128u,103481u,102129u,102131u,103490u})Objective(p,id,1);Objective(p,102548,mode==3?1:0);
   if(mode==5)const_cast<RewardedQuestSet&>(p.getRewardedQuests()).insert(40419);if(mode==6)p.SetGameMaster(true);if(mode==8)p.SetQuestStatus(40419,QUEST_STATUS_NONE);if(mode>=9&&mode<=13)Objective(p,std::vector<uint32>{102128,103481,102129,102131,103490}[mode-9],0);if(mode==17)p.SetQuestStatus(40419,QUEST_STATUS_FAILED);
   EarnedBow item(p,mode);bool earned=mode<=4||mode==19;Require(p.HasEarnedMarksmanshipReturn()==earned,"Native earned receipt/owner scan differs");ConditionSourceInfo info(&p);Require(gate.Meets(info)==!earned&&original.Meets(info),"Native Condition::Meets/ScriptMgr dispatch did not retain original Player mask and suppress only earned arrival");
   (p.*Access(RecoveryResumeTag{}))={1489,972,0,0,target->target_X,target->target_Y,target->target_Z,target->target_Orientation};p.PrepareScenarioLogin(1489,0);if(mode!=7){bool allowed=earned||(mode==13&&status==QUEST_STATUS_INCOMPLETE);Require(p.CanResumeScenarioLogin(972)==allowed,"Real late loading-item recovery admission differs");}
   Require(progress->GetCurrentStep()==0&&p.GetQuestObjectiveData(40419,102548)==(mode==3?1:0),"Proof/admission/condition awarded progression");++cases;
  }sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
 Require(cases==80,"Native earned regression count changed");std::cout<<"PASS earned native80:real40419 six slots/owned loading equipment+inventory-bag+bank-bag/refused owner/missing/zero/wrong item,Human+Orc,incomplete+complete,native seed admission and actual Condition::Meets->ScriptMgr callback;original unbound mask counterfactual still accepts earned intro. No item/return/scenario grant. Quest rows/poses controlled;controller/cast/arrival/client separate"<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
