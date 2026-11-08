// Actual Recorder/Prepare/LFG/Scenario with isolated TEMP entrance data;
// earlier quest/stage/pose controlled, no character DB writes.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "MapManager.h"
struct BloodEntryTag{using Type=ScenarioResume Player::*;friend Type Access(BloodEntryTag);};
template struct FixtureAccess<BloodEntryTag,&Player::m_scenarioResume>;
static void BloodPlayer(Player& p,unsigned race){p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_DEATH_KNIGHT);p.setFactionForRace(race);p.SetQuestStatus(40740,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40740);}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sScenarioMgr->GetScenarioSteps(940)&&sScenarioMgr->GetScenarioSteps(940)->size()==11,"Actual940 has eleven steps0..10;11 is completion sentinel");
#ifndef BLOOD_ENTRANCE_LIVE
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("lfg_entrances","1");
#ifdef BLOOD_ENTRANCE_BASELINE
 WorldDatabase.DirectExecute("DELETE FROM lfg_entrances WHERE dungeonId IN(1059)");
#else
 std::string migration=
#include "BloodEntranceMigration.inc"
 ;std::string statement;bool quote=false;for(char c:migration){if(c=='\'')quote=!quote;if(c==';'&&!quote){WorldDatabase.DirectExecute(statement.c_str());statement.clear();}else statement+=c;}
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#endif
 sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 auto target=sSpellMgr->GetSpellTargetPosition(201678);auto spell=sSpellMgr->GetSpellInfo(201678);Require(target&&target->target_mapId==1489&&spell&&spell->GetEffect(EFFECT_0)->Effect==227,"Matching201678 native teleport changed");
 Require(std::fabs(target->target_X-240.9f)<0.001f&&std::fabs(target->target_Y-1953.44f)<0.001f&&std::fabs(target->target_Z+55.29f)<0.001f&&std::fabs(target->target_Orientation-0.384845f)<0.001f,"Native201678 entry coordinates changed");
 for(unsigned id:{1059u}){auto d=sLFGMgr->GetLFGDungeon(id);Require(d&&d->dbc&&d->map==1489&&d->dbc->ScenarioID==940&&d->difficulty==DIFFICULTY_N_SCENARIO&&(d->dbc->Flags&LFG_FLAG_USER_TELEPORT_NOT_ALLOWED),"Matching native Blood LFG route/disabled teleport changed");
#ifdef BLOOD_ENTRANCE_BASELINE
 Require(!d->x&&!d->y&&!d->z,"Missing native entrance baseline no longer zero");
#else
 Require(std::fabs(d->x-target->target_X)<0.001f&&std::fabs(d->y-target->target_Y)<0.001f&&std::fabs(d->z-target->target_Z)<0.001f&&std::fabs(d->o-target->target_Orientation)<0.001f,"Native loader did not consume exact Blood source into the row");
#endif
 }
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  auto session=Session();unsigned instanceId=9998800+race;ScenarioResume saved;Position late{62.6788f,1215.55f,-38.6585f,0};
  {
   MapInstanced parent(1489,300);InstanceMap map(1489,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,940);
   map.LoadGrid(target->target_X,target->target_Y);Require(std::fabs(map.GetHeight(target->target_X,target->target_Y,target->target_Z+5)-target->target_Z)<0.1f,"Real native Blood entry floor unsupported");
   for(unsigned step=1;step<11;++step){scenario->SetCurrentStep(step);NativePlayer p(session,map,701,late);BloodPlayer(p,race);auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();p.RecordScenarioEntrance();auto receipt=p.*Access(BloodEntryTag{});
#ifdef BLOOD_ENTRANCE_BASELINE
    Require(!receipt.scenario,"Absent native entries failed to reproduce missing late checkpoint");
#else
    Require(receipt.map==1489&&receipt.scenario==940&&receipt.instance==instanceId&&receipt.boot&&std::fabs(receipt.x-target->target_X)<0.001f&&std::fabs(receipt.y-target->target_Y)<0.001f&&std::fabs(receipt.z-target->target_Z)<0.001f,"Native late Recorder failed actual Blood entrance");if(step==1)saved=receipt;
#endif
    Require(p.GetExactDist(late)<0.001f&&scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&!p.HasItemCount(128402,1,true),"Record granted movement/progression/artifact");++cases;
   }
   scenario->SetCurrentStep(0);{NativePlayer p(session,map,701,late);BloodPlayer(p,race);p.RecordScenarioEntrance();auto receipt=p.*Access(BloodEntryTag{});Require(receipt.scenario==940&&std::fabs(receipt.x-late.GetPositionX())<0.001f,"Ordinary first entry no longer records actual pose");++cases;scenario->SetCurrentStep(3);p.RecordScenarioEntrance();Require((p.*Access(BloodEntryTag{})).x==receipt.x&&p.GetExactDist(late)<0.001f,"Existing legitimate record overwritten by restored LFG");++cases;}
#ifndef BLOOD_ENTRANCE_BASELINE
   for(unsigned mode=0;mode<3;++mode){NativePlayer p(session,map,701,late);BloodPlayer(p,race);Require((p.*Access(BloodEntryTag{})).Read(saved.Write()),"Actual receipt roundtrip failed");if(mode==1)(p.*Access(BloodEntryTag{})).boot=saved.boot+1;p.PrepareScenarioLogin(1489,instanceId+(mode==2));if(!mode)Require(p.GetScenarioLoginInstance()==instanceId&&p.GetExactDist(late)<0.001f,"Live native Blood rejoin lost its real pose");else Require(!p.GetScenarioLoginInstance()&&p.GetScenarioId()==940&&p.GetExactDist(target->target_X,target->target_Y,target->target_Z)<0.001f,"Fresh native Blood restart lost entrance/selector");Require(scenario->GetCurrentStep()==3,"Prepare changed old stage");p.Relocate(late);++cases;}
#endif
   scenario->SetCurrentStep(10);{NativePlayer p(session,map,703,late);BloodPlayer(p,race);(p.*Access(BloodEntryTag{}))={1489,940,instanceId,uint64(GameTime::GetStartTime()),11,22,33,0};p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,48857);Require(scenario->GetCurrentStep()==11&&scenario->IsCompleted(false),"Real native final48857 did not reach completed sentinel11");p.RecordScenarioEntrance();Require(!(p.*Access(BloodEntryTag{})).scenario&&!p.HasItemCount(128402,1,true),"Completed acquisition retained a fresh replay checkpoint");++cases;}
   sScenarioMgr->RemoveScenario(instanceId);scenario=sScenarioMgr->AddScenario(&map,972);scenario->SetCurrentStep(3);{NativePlayer p(session,map,702,late);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.RecordScenarioEntrance();auto receipt=p.*Access(BloodEntryTag{});Require(receipt.scenario==972&&scenario->GetCurrentStep()==3&&p.GetExactDist(late)<0.001f,"Unrelated Hunter972 entrance changed");++cases;}sScenarioMgr->RemoveScenario(instanceId);
  }
#ifndef BLOOD_ENTRANCE_BASELINE
  // Original whole Map and parent are gone; the native Blood queue constructs
  // actual940 step0, even when exactly the old numeric instance ID is reused.
  for(unsigned id:{1059u}){MapInstanced parent(1489,300);InstanceMap fresh(1489,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);auto d=sLFGMgr->GetLFGDungeon(id);{NativePlayer p(session,fresh,701,late);BloodPlayer(p,race);Require((p.*Access(BloodEntryTag{})).Read(saved.Write()),"Fresh persisted entrance read failed");p.PrepareScenarioLogin(1489,instanceId);Require(!p.GetScenarioLoginInstance()&&p.GetScenarioId()==940&&p.GetExactDist(target->target_X,target->target_Y,target->target_Z)<0.001f,"Destroyed map recovered stale instance or pose");auto scenario=sScenarioMgr->AddScenario(&fresh,d,&p,false);Require(scenario&&scenario->GetScenarioId()==940&&scenario->GetCurrentStep()==0&&!p.GetScenarioId()&&p.CanResumeScenarioLogin(940)&&!p.HasItemCount(128402,1,true),"Actual native fresh constructor failed Blood step0/no grant");p.Relocate(late);++cases;}sScenarioMgr->RemoveScenario(instanceId);}
#endif
  session->SetPlayer(nullptr);delete session;
 }
#ifdef BLOOD_ENTRANCE_BASELINE
 Require(cases==28,"Native baseline count changed");std::cout<<"COUNTERFACTUAL native Blood entrance28:20 missing late checkpoints,ordinary/existing/shared972 six controls and completed cleanup2 retained;TEMP absent1059,not ordinary success"<<std::endl;
#else
 Require(cases==36,"Native corrected count changed");std::cout<<"PASS native Blood entrance36:20 actual late records,ordinary/existing poses,live/boot/numeric mismatch,whole Map destruction/recycledID and native queue constructor940/step0,real entry floor,completed sentinel cleanup and shared972 retained;no persistent character/item writes or grant by recorder/prepare. Earlier quest/stage/pose and final48857 input controlled;full saved-character/client/group traversal separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
