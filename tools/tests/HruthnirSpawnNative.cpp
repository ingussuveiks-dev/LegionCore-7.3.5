// Actual Creature::LoadCreatureFromDB and native AI/Motion initialization.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
#include <fstream>
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 // Inspect this scoped spawn at WARN visibility without editing user config.
 Require(sLog->SetLogLevel("sql.sql","4"),"Scoped SQL warning visibility unavailable");
 auto addon=WorldDatabase.Query("SELECT (SELECT COUNT(*) FROM creature_addon WHERE guid=361306)+(SELECT COUNT(*) FROM creature_template_addon WHERE entry=96468)");Require(addon&&!addon->Fetch()[0].GetUInt32(),"Actual Hruthnir unexpectedly has addon/path metadata; review fixture");
 auto live=WorldDatabase.Query("SELECT MovementType FROM creature WHERE guid=361306 AND id=96468 AND map=1495 AND PhaseId='6999'");Require(bool(live),"Native source identity missing");if(auto verify=std::getenv("LEGION_SPAWN_VERIFY"))if(std::string(verify)=="1")Require(live->Fetch()[0].GetUInt32()==0,"Live spawn repair not deployed");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("creature","guid=361306");Shadow("creature_addon","guid=361306");
#ifdef HRUTHNIR_SPAWN_BASELINE
 WorldDatabase.DirectExecute("UPDATE creature SET MovementType=2 WHERE guid=361306");
#else
 auto file=std::getenv("LEGION_SPAWN_SQL");Require(file,"Actual migration path missing");std::ifstream input(file);std::string sql((std::istreambuf_iterator<char>(input)),{});Require(input.eof()||input.good(),"Migration read failed");Require(sql.size()>100,"Actual migration empty");
 WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE hruthnir_other_spawn LIKE creature");WorldDatabase.DirectExecute("INSERT INTO hruthnir_other_spawn SELECT * FROM creature");WorldDatabase.DirectExecute("UPDATE hruthnir_other_spawn SET guid=900000001,MovementType=2");WorldDatabase.DirectExecute("INSERT INTO creature SELECT * FROM hruthnir_other_spawn");
 WorldDatabase.DirectExecute("UPDATE creature SET MovementType=2 WHERE guid=361306");WorldDatabase.DirectExecute("INSERT INTO creature_addon(guid,path_id) VALUES(361306,9859004)");WorldDatabase.DirectExecute(sql.c_str());auto retained=WorldDatabase.Query("SELECT MovementType FROM creature WHERE guid=361306");Require(retained&&retained->Fetch()[0].GetUInt32()==2,"Migration overwrote custom addon route");WorldDatabase.DirectExecute("DELETE FROM creature_addon WHERE guid=361306");
 for(uint32 repeat=0;repeat<2;++repeat){WorldDatabase.DirectExecute(sql.c_str());auto rows=WorldDatabase.Query("SELECT guid,MovementType FROM creature ORDER BY guid");Require(rows&&rows->GetRowCount()==2,"Migration changed spawn population");Require(rows->Fetch()[0].GetUInt32()==361306&&rows->Fetch()[1].GetUInt32()==0,"Actual target repair failed");rows->NextRow();Require(rows->Fetch()[0].GetUInt32()==900000001&&rows->Fetch()[1].GetUInt32()==2,"Migration affected unrelated spawn");}
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sObjectMgr->LoadCreatures();sWaypointMgr->Load();
 auto data=sObjectMgr->GetCreatureData(361306);Require(data&&data->id==96468&&data->mapid==1495&&data->spawndist==0,"Actual Hruthnir DB source identity differs");auto path=sWaypointMgr->GetPathScript(9859004);Require(path&&path->size()==7,"Original scripted7-node path changed");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new npc_hrutnir;new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 uint32 cases=0;
 for(uint32 stage:{0u,3u}){
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9991200+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(stage);
  auto actor=std::make_unique<Creature>();Require(actor->LoadCreatureFromDB(361306,&map,false),"Actual native DB load failed");
  Require(actor->GetDBTableGUIDLow()==361306&&actor->GetEntry()==96468&&!actor->GetWaypointPath(),"Actual source/path0 provenance differs");
  Require(actor->AIM_Initialize()&&dynamic_cast<HruthnirAI*>(actor->AI()),"Actual ScriptName factory did not install Hruthnir AI");
#ifdef HRUTHNIR_SPAWN_BASELINE
  Require(actor->GetDefaultMovementType()==WAYPOINT_MOTION_TYPE&&actor->GetMotionMaster()->GetCurrentMovementGeneratorType()==WAYPOINT_MOTION_TYPE,"Pinned MovementType2 no longer reproduces");
  std::cout<<"COUNTERFACTUAL real DB load stage="<<stage<<": actual spawn361306 MovementType2, no addon path, native Motion_Initialize creates path0 and logs missing path; not an ordinary passing runtime\n";
#else
  Require(actor->GetDefaultMovementType()==IDLE_MOTION_TYPE&&actor->GetMotionMaster()->GetCurrentMovementGeneratorType()==IDLE_MOTION_TYPE,"Repaired native DB spawn did not initialize idle");
#endif
  actor->GetMotionMaster()->MovePath(9859004,false);auto route=dynamic_cast<WaypointMovementGenerator<Creature>*>(actor->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE));Require(route&&route->GetPathId()==9859004&&!route->IsPathComplete()&&scenario->GetCurrentStep()==stage&&!StartStories(map)&&!WaveCredit(scenario),"Default repair removed original native route or fabricated progression");
  actor->StopMoving();Require(actor->AIM_Destroy(),"Native DB actor cleanup failed");actor->ResetMap();actor.reset();sScenarioMgr->RemoveScenario(map.GetInstanceId());
#ifndef HRUTHNIR_SPAWN_BASELINE
  std::cout<<"PASS real DB spawn stage="<<stage<<": native LoadCreatures/LoadCreatureFromDB/ScriptName/AI/Motion initialize idle, explicit original9859004 still launches, no criterion/story/quest grant; map publication/client traversal separate\n";
#endif
 }
 std::cout<<"CASE real DB spawn contexts="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
