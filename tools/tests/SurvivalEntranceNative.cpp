// Actual Recorder/Prepare/LFG/Scenario with isolated TEMP entrance data;
// earlier quest/stage/pose controlled, no character DB writes.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "MapManager.h"
struct SurvivalEntryTag{using Type=ScenarioResume Player::*;friend Type Access(SurvivalEntryTag);};
template struct FixtureAccess<SurvivalEntryTag,&Player::m_scenarioResume>;
static void Hunter(Player& p,unsigned race){p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.setFactionForRace(race);p.SetQuestStatus(40385,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40385);}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
#ifndef SURVIVAL_ENTRANCE_LIVE
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("lfg_entrances","1");
#ifdef SURVIVAL_ENTRANCE_BASELINE
 WorldDatabase.DirectExecute("DELETE FROM lfg_entrances WHERE dungeonId IN(1178,1384)");
#else
 std::string migration=
#include "SurvivalEntranceMigration.inc"
 ;std::string statement;bool quote=false;for(char c:migration){if(c=='\'')quote=!quote;if(c==';'&&!quote){WorldDatabase.DirectExecute(statement.c_str());statement.clear();}else statement+=c;}
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#endif
 sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 auto target=sSpellMgr->GetSpellTargetPosition(198950);auto spell=sSpellMgr->GetSpellInfo(198950);Require(target&&target->target_mapId==1511&&spell&&spell->GetEffect(EFFECT_0)->Effect==227,"Matching198950 native teleport changed");
 Require(std::fabs(target->target_X-3364.77f)<0.001f&&std::fabs(target->target_Y-1919.54f)<0.001f&&std::fabs(target->target_Z-0.092994f)<0.001f&&std::fabs(target->target_Orientation-5.69981f)<0.001f,"Native198950 entry coordinates changed");
 for(unsigned id:{1178u,1384u}){auto d=sLFGMgr->GetLFGDungeon(id);Require(d&&d->dbc&&d->map==1511&&d->dbc->ScenarioID==973&&d->difficulty==DIFFICULTY_N_SCENARIO&&(d->dbc->Flags&LFG_FLAG_USER_TELEPORT_NOT_ALLOWED),"Matching native Hunter LFG route/disabled teleport changed");
#ifdef SURVIVAL_ENTRANCE_BASELINE
 Require(!d->x&&!d->y&&!d->z,"Missing native entrance baseline no longer zero");
#else
 Require(std::fabs(d->x-target->target_X)<0.001f&&std::fabs(d->y-target->target_Y)<0.001f&&std::fabs(d->z-target->target_Z)<0.001f&&std::fabs(d->o-target->target_Orientation)<0.001f,"Native loader did not consume exact Hunter source into both rows");
#endif
 }
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  auto session=Session();unsigned instanceId=9998800+race;ScenarioResume saved;Position late{3601.95f,1782.54f,1.86404f,0};
  {
   MapInstanced parent(1511,300);InstanceMap map(1511,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,973);
   map.LoadGrid(target->target_X,target->target_Y);Require(std::fabs(map.GetHeight(target->target_X,target->target_Y,target->target_Z+5)-target->target_Z)<0.1f,"Real native Hunter entry floor unsupported");
   for(unsigned step=1;step<13;++step){scenario->SetCurrentStep(step);NativePlayer p(session,map,701,late);Hunter(p,race);auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();p.RecordScenarioEntrance();auto receipt=p.*Access(SurvivalEntryTag{});
#ifdef SURVIVAL_ENTRANCE_BASELINE
    Require(!receipt.scenario,"Absent native entries failed to reproduce missing late checkpoint");
#else
    Require(receipt.map==1511&&receipt.scenario==973&&receipt.instance==instanceId&&receipt.boot&&std::fabs(receipt.x-target->target_X)<0.001f&&std::fabs(receipt.y-target->target_Y)<0.001f&&std::fabs(receipt.z-target->target_Z)<0.001f,"Native late Recorder failed actual Hunter entrance");if(step==1)saved=receipt;
#endif
    Require(p.GetExactDist(late)<0.001f&&scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&!p.HasItemCount(128808,1,true),"Record granted movement/progression/artifact");++cases;
   }
   scenario->SetCurrentStep(0);{NativePlayer p(session,map,701,late);Hunter(p,race);p.RecordScenarioEntrance();auto receipt=p.*Access(SurvivalEntryTag{});Require(receipt.scenario==973&&std::fabs(receipt.x-late.GetPositionX())<0.001f,"Ordinary first entry no longer records actual pose");++cases;scenario->SetCurrentStep(3);p.RecordScenarioEntrance();Require((p.*Access(SurvivalEntryTag{})).x==receipt.x&&p.GetExactDist(late)<0.001f,"Existing legitimate record overwritten by restored LFG");++cases;}
#ifndef SURVIVAL_ENTRANCE_BASELINE
   for(unsigned mode=0;mode<3;++mode){NativePlayer p(session,map,701,late);Hunter(p,race);Require((p.*Access(SurvivalEntryTag{})).Read(saved.Write()),"Actual receipt roundtrip failed");if(mode==1)(p.*Access(SurvivalEntryTag{})).boot=saved.boot+1;p.PrepareScenarioLogin(1511,instanceId+(mode==2));if(!mode)Require(p.GetScenarioLoginInstance()==instanceId&&p.GetExactDist(late)<0.001f,"Live native Hunter rejoin lost its real pose");else Require(!p.GetScenarioLoginInstance()&&p.GetScenarioId()==973&&p.GetExactDist(target->target_X,target->target_Y,target->target_Z)<0.001f,"Fresh native Hunter restart lost entrance/selector");Require(scenario->GetCurrentStep()==3,"Prepare changed old stage");p.Relocate(late);++cases;}
#endif
   sScenarioMgr->RemoveScenario(instanceId);scenario=sScenarioMgr->AddScenario(&map,944);scenario->SetCurrentStep(3);{NativePlayer p(session,map,701,late);p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);p.SetQuestStatus(40043,QUEST_STATUS_INCOMPLETE);auto fury=sSpellMgr->GetSpellTargetPosition(193335);p.RecordScenarioEntrance();auto receipt=p.*Access(SurvivalEntryTag{});Require(fury&&receipt.scenario==944&&std::fabs(receipt.x-fury->target_X)<0.001f&&std::fabs(receipt.y-fury->target_Y)<0.001f&&std::fabs(receipt.z-fury->target_Z)<0.001f&&p.GetExactDist(late)<0.001f,"Shared restored Hunter rows overrode actual Fury source");++cases;}sScenarioMgr->RemoveScenario(instanceId);
  }
#ifndef SURVIVAL_ENTRANCE_BASELINE
  // Original whole Map and parent are gone; both native queue IDs construct
  // actual973 step0, even when exactly the old numeric instance ID is reused.
  for(unsigned id:{1178u,1384u}){MapInstanced parent(1511,300);InstanceMap fresh(1511,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);auto d=sLFGMgr->GetLFGDungeon(id);{NativePlayer p(session,fresh,701,late);Hunter(p,race);Require((p.*Access(SurvivalEntryTag{})).Read(saved.Write()),"Fresh persisted entrance read failed");p.PrepareScenarioLogin(1511,instanceId);Require(!p.GetScenarioLoginInstance()&&p.GetScenarioId()==973&&p.GetExactDist(target->target_X,target->target_Y,target->target_Z)<0.001f,"Destroyed map recovered stale instance or pose");auto scenario=sScenarioMgr->AddScenario(&fresh,d,&p,false);Require(scenario&&scenario->GetScenarioId()==973&&scenario->GetCurrentStep()==0&&!p.GetScenarioId()&&p.CanResumeScenarioLogin(973)&&!p.HasItemCount(128808,1,true),"Actual native fresh constructor failed Hunter step0/no grant");p.Relocate(late);++cases;}sScenarioMgr->RemoveScenario(instanceId);}
#endif
 }
#ifdef SURVIVAL_ENTRANCE_BASELINE
 Require(cases==30,"Native baseline count changed");std::cout<<"COUNTERFACTUAL native Hunter entrance30:24 missing late checkpoints,ordinary/existing/Fury6 controls retained;TEMP absent1178/1384,not ordinary success"<<std::endl;
#else
 Require(cases==40,"Native corrected count changed");std::cout<<"PASS native Hunter entrance40:24 actual late records,ordinary/existing poses,live/boot/numeric mismatch,whole Map destruction/recycledID and both native queue constructors973/step0,real entry floor,shared Fury retained;no character/credit/item writes. Earlier quest/stage/pose controlled;full saved-character/client/group traversal separate"<<std::endl;
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
