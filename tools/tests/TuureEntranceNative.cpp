// Real native Recorder/Prepare/LFG/Scenario; earlier quest/pose and final events
// are controlled. No persistent character data or production progress grants.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "MapManager.h"
#include "DB2Store.h"
#include "DB2LoadInfo.h"
#include <iomanip>
struct TuureEntryTag{using Type=ScenarioResume Player::*;friend Type Access(TuureEntryTag);};
template struct FixtureAccess<TuureEntryTag,&Player::m_scenarioResume>;
static void Priest(Player& p,unsigned race){p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);p.setFactionForRace(race);p.SetQuestStatus(41993,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,41993);}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 DB2Storage<CharBaseInfoEntry> pairs("CharBaseInfo.db2",CharBaseInfoLoadInfo::Instance());Require(pairs.Load(argv[1],0),"Matching Legion race/class data missing");
 auto safe=sWorldSafeLocsStore.LookupEntry(5480);Require(safe&&safe->MapID==1489&&std::string(safe->Name->Str[0])=="7.0 Artifacts - Priest - Holy - Niskara Entrance Loc - LWB","Real Holy Priest entry missing");
 Require(std::fabs(safe->Loc.X+43.1440964f)<0.001f&&std::fabs(safe->Loc.Y-1160.33679f)<0.001f&&std::fabs(safe->Loc.Z+74.353569f)<0.001f,"Native5480 coordinates changed");
 Position entry{safe->Loc.X,safe->Loc.Y,safe->Loc.Z,safe->Loc.O*float(M_PI)/180.0f};
 Require(std::fabs(entry.GetOrientation()-0.483893174f)<0.001f&&sScenarioMgr->GetScenarioSteps(1085)&&sScenarioMgr->GetScenarioSteps(1085)->size()==7,"Native Tuure route/facing changed");
#ifndef TUURE_ENTRANCE_LIVE
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("lfg_entrances","1");
#ifdef TUURE_ENTRANCE_BASELINE
 WorldDatabase.DirectExecute("DELETE FROM lfg_entrances WHERE dungeonId=1243");WorldDatabase.DirectExecute("INSERT INTO lfg_entrances VALUES(1243,NULL,-828.85,4372,738.64,1.86768)");
#else
 std::string migration=
#include "TuureEntranceMigration.inc"
 ;std::string statement;bool quote=false;for(char c:migration){if(c=='\'')quote=!quote;if(c==';'&&!quote){WorldDatabase.DirectExecute(statement.c_str());statement.clear();}else statement+=c;}
 auto applied=WorldDatabase.Query("SELECT COALESCE(@tuure_resume_safe,0)");Require(applied&&applied->Fetch()[0].GetBool(),"Native migration guard did not accept the original graph");
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
#endif
 sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();
 auto dungeon=sLFGMgr->GetLFGDungeon(1243);Require(dungeon&&dungeon->dbc&&dungeon->map==1489&&dungeon->dbc->ScenarioID==1085&&dungeon->difficulty==DIFFICULTY_N_SCENARIO&&(dungeon->dbc->Flags&LFG_FLAG_USER_TELEPORT_NOT_ALLOWED),"Native1243 route changed");
#ifdef TUURE_ENTRANCE_BASELINE
 Require(std::fabs(dungeon->x+828.85f)<0.001f&&std::fabs(dungeon->z-738.64f)<0.001f,"Legacy Dalaran-coordinate baseline changed");
#else
 // SQL arithmetic checks stored FLOAT values directly. MariaDB's ordinary
 // text result formats this Y as1160.34, so the real Field::GetFloat loader
 // differs from DB2 by3.174mm. Keep that bounded geometry delta visible;
 // receipts/restarts below must match the actual native loader within1mm.
 auto stored=WorldDatabase.Query("SELECT ABS(position_x-(-43.1440964))<0.001 AND ABS(position_y-1160.33679)<0.001 AND ABS(position_z-(-74.353569))<0.001 AND ABS(orientation-0.483893174)<0.001 FROM lfg_entrances WHERE dungeonId=1243");Require(stored&&stored->Fetch()[0].GetBool(),"Stored coordinates differ from native5480");
 std::cout<<std::setprecision(9)<<"CONTROL native5480 "<<entry.GetPositionX()<<' '<<entry.GetPositionY()<<' '<<entry.GetPositionZ()<<' '<<entry.GetOrientation()<<" loaded1243 "<<dungeon->x<<' '<<dungeon->y<<' '<<dungeon->z<<' '<<dungeon->o<<" distance "<<entry.GetExactDist(dungeon->x,dungeon->y,dungeon->z)<<std::endl;
 Require(entry.GetExactDist(dungeon->x,dungeon->y,dungeon->z)<0.01f&&std::fabs(dungeon->o-entry.GetOrientation())<0.001f,"Native loader exceeded1cm geometry precision");
#endif
 Position resume{dungeon->x,dungeon->y,dungeon->z,dungeon->o};
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_UNDEAD_PLAYER)}){
  bool eligible=false;for(auto pair:pairs)if(pair->RaceID==race&&pair->ClassID==CLASS_PRIEST)eligible=true;Require(eligible,"Fixture must use an actual26972 Priest race/class pair");
  auto session=Session();unsigned instance=9998900+race;ScenarioResume saved;Position late{71.9653f,1218.51f,-74.2702f,0};
  {
   MapInstanced parent(1489,300);InstanceMap map(1489,300,instance,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,1085);
   map.LoadGrid(entry.GetPositionX(),entry.GetPositionY());Require(std::fabs(map.GetHeight(entry.GetPositionX(),entry.GetPositionY(),entry.GetPositionZ()+5)-entry.GetPositionZ())<0.1f,"Native Holy entry floor unsupported");
   for(unsigned step=1;step<7;++step){scenario->SetCurrentStep(step);NativePlayer p(session,map,801,late);Priest(p,race);auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();p.RecordScenarioEntrance();auto receipt=p.*Access(TuureEntryTag{});Require(receipt.scenario==1085&&receipt.map==1489&&receipt.instance==instance&&receipt.boot,"Native late receipt missing");
#ifdef TUURE_ENTRANCE_BASELINE
    Require(std::fabs(receipt.x+828.85f)<0.001f&&std::fabs(receipt.z-738.64f)<0.001f&&entry.GetExactDist(receipt.x,receipt.y,receipt.z)>3000,"Legacy bad checkpoint not reproduced");
#else
    Require(resume.GetExactDist(receipt.x,receipt.y,receipt.z)<0.001f&&std::fabs(receipt.o-resume.GetOrientation())<0.001f,"Native late receipt lost loaded Holy entrance");
#endif
    if(step==1)saved=receipt;Require(p.GetExactDist(late)<0.001f&&scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&!p.HasItemCount(128825,1,true),"Recorder granted movement/progression/item");++cases;
   }
   scenario->SetCurrentStep(0);{NativePlayer p(session,map,801,late);Priest(p,race);p.RecordScenarioEntrance();auto receipt=p.*Access(TuureEntryTag{});Require(receipt.scenario==1085&&std::fabs(receipt.x-late.GetPositionX())<0.001f,"Ordinary entry changed");++cases;scenario->SetCurrentStep(3);p.RecordScenarioEntrance();Require((p.*Access(TuureEntryTag{})).x==receipt.x&&p.GetExactDist(late)<0.001f,"Existing legitimate pose overwritten");++cases;}
#ifdef TUURE_ENTRANCE_BASELINE
   {NativePlayer p(session,map,801,late);Priest(p,race);Require((p.*Access(TuureEntryTag{})).Read(saved.Write()),"Bad receipt read failed");(p.*Access(TuureEntryTag{})).boot++;p.PrepareScenarioLogin(1489,instance);Require(std::fabs(p.GetPositionX()+828.85f)<0.001f&&entry.GetExactDist(&p)>3000,"Bad fresh restart not reproduced");p.Relocate(late);++cases;}
#else
   for(unsigned mode=0;mode<3;++mode){NativePlayer p(session,map,801,late);Priest(p,race);Require((p.*Access(TuureEntryTag{})).Read(saved.Write()),"Receipt roundtrip failed");if(mode==1)(p.*Access(TuureEntryTag{})).boot++;p.PrepareScenarioLogin(1489,instance+(mode==2));if(!mode)Require(p.GetScenarioLoginInstance()==instance&&p.GetExactDist(late)<0.001f,"Exact live rejoin lost pose");else Require(!p.GetScenarioLoginInstance()&&p.GetScenarioId()==1085&&p.GetExactDist(resume)<0.001f,"Fresh Holy restart lost native entrance");Require(scenario->GetCurrentStep()==3,"Prepare changed old stage");p.Relocate(late);++cases;}
#endif
   scenario->SetCurrentStep(6);{NativePlayer p(session,map,803,late);Priest(p,race);(p.*Access(TuureEntryTag{}))=saved;p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50679);Require(scenario->GetCurrentStep()==6&&!scenario->IsCompleted(false),"Pickup alone completed native ALL-root");p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50702);Require(scenario->GetCurrentStep()==7&&scenario->IsCompleted(false),"Real final return did not reach sentinel7");p.RecordScenarioEntrance();Require(!(p.*Access(TuureEntryTag{})).scenario&&!p.HasItemCount(128825,1,true),"Completed scenario retained replay checkpoint");++cases;}
   sScenarioMgr->RemoveScenario(instance);scenario=sScenarioMgr->AddScenario(&map,972);scenario->SetCurrentStep(3);{NativePlayer p(session,map,802,late);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.RecordScenarioEntrance();auto receipt=p.*Access(TuureEntryTag{});auto other=sLFGMgr->GetLFGDungeon(1162);Require(other&&receipt.scenario==972&&std::fabs(receipt.x-other->x)<0.001f&&p.GetExactDist(late)<0.001f&&scenario->GetCurrentStep()==3,"Shared Hunter route changed");++cases;}sScenarioMgr->RemoveScenario(instance);
  }
#ifndef TUURE_ENTRANCE_BASELINE
  {MapInstanced parent(1489,300);InstanceMap fresh(1489,300,instance,DIFFICULTY_N_SCENARIO,&parent);{NativePlayer p(session,fresh,801,late);Priest(p,race);Require((p.*Access(TuureEntryTag{})).Read(saved.Write()),"Destroyed-map receipt read failed");p.PrepareScenarioLogin(1489,instance);Require(!p.GetScenarioLoginInstance()&&p.GetScenarioId()==1085&&p.GetExactDist(resume)<0.001f,"Recycled ID recovered stale pose");auto scenario=sScenarioMgr->AddScenario(&fresh,dungeon,&p,false);Require(scenario&&scenario->GetScenarioId()==1085&&!scenario->GetCurrentStep()&&!p.GetScenarioId()&&p.CanResumeScenarioLogin(1085)&&!p.HasItemCount(128825,1,true),"Native1243 constructor failed fresh Holy step0");p.Relocate(late);++cases;}sScenarioMgr->RemoveScenario(instance);}
#endif
  session->SetPlayer(nullptr);delete session;
 }
#ifdef TUURE_ENTRANCE_BASELINE
 Require(cases==22,"Baseline count changed");std::cout<<"COUNTERFACTUAL native Tuure entrance22:12 wrong Dalaran-coordinate late receipts and2 wrong fresh restarts;8 ordinary/existing/shared972/completed controls retained;TEMP legacy1243,not ordinary successful gameplay"<<std::endl;
#else
 Require(cases==28,"Corrected count changed");std::cout<<"PASS native Tuure entrance28:12 late native5480 receipts,ordinary/existing/live/boot/numeric mismatch,destroyed Map/recycledID/native1243 constructor1085 step0,real floor and degree-facing conversion,actual two-event ALL-root cleanup and shared972;no persistent character writes or movement/item/progression grant by recorder. Quest/pose/stage/final events controlled;full client traversal separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
