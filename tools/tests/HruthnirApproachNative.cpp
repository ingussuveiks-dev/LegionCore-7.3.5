// Native finalized-short PointMovement rejection and scoped re-request.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
using ApproachRoute=WaypointMovementGenerator<Creature>;
struct ApproachArrivedTag{using Type=bool ApproachRoute::*;friend Type Access(ApproachArrivedTag);};
template struct FixtureAccess<ApproachArrivedTag,&ApproachRoute::_isArrivalDone>;
struct ApproachRepeatTag{using Type=bool ApproachRoute::*;friend Type Access(ApproachRepeatTag);};
template struct FixtureAccess<ApproachRepeatTag,&ApproachRoute::_repeating>;
using ApproachBase=PathMovementBase<Creature,WaypointPath const*>;
struct ApproachNodeTag{using Type=uint32 ApproachBase::*;friend Type Access(ApproachNodeTag);};
template struct FixtureAccess<ApproachNodeTag,&ApproachBase::_currentNode>;
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 if(!sWaypointMgr->GetPathScript(9859004))sWaypointMgr->Load();auto path=sWaypointMgr->GetPathScript(9859004);Require(path&&path->size()==7,"Actual earlier seven-node route missing");
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<12;++mode){
#ifdef HRUTHNIR_APPROACH_BASELINE
  if(mode)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9990200+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   auto owner=std::make_unique<NativePlayer>(session,map,901,Position{4730.8f,346,-37.71f,0});
   auto prepare=[&]{owner->SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner->setFactionForRace(race);owner->SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);owner->SetQuestSlot(0,39191);};prepare();
   HruthnirActor companion(&map,96468,12302,{4730.8f,347.91f,-37.71f,0});Require(map.AddToMap(static_cast<Creature*>(&companion)),"Native Hruthnir admission failed");
   MagnarActor boss(&map,96034,12301,{4714.82f,349.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native Magnar admission failed");
   auto& brain=companion.Brain();script->magnarGuardCreditSent=true;script->magnarCompanionGUID=companion.GetGUID();
   // Earlier actual accepted gossip/quest receipt endpoints are controlled.
   brain.magnarChamberStarted=true;brain.magnarIntroInstanceGeneration=script->GetData(1060);brain.magnarStartInstance=map.GetInstanceId();brain.magnarStartOwner=owner->GetGUID();brain.magnarStartAttempt=owner->GetQuestCancellationReceipt(39191);brain.magnarStartToken=std::make_shared<bool>(false);brain.magnarArrivalPending=true;auto token=brain.magnarStartToken;
   auto motion=companion.GetMotionMaster();ApproachRoute* route=nullptr;
   if(mode>=9){motion->MovePath(9859004,false);companion.StopMoving();route=dynamic_cast<ApproachRoute*>(motion->GetMotionSlot(MOTION_SLOT_IDLE));Require(route&&route->GetPathId()==9859004,"Native original route not in idle slot");route->*Access(ApproachNodeTag{})=uint32(path->size()-1);route->*Access(ApproachArrivedTag{})=mode!=10;route->*Access(ApproachRepeatTag{})=mode==11;Require(route->IsPathComplete()==(mode==9),"Actual route completion query accepted an unfinished/repeating route");}
#ifndef HRUTHNIR_APPROACH_BASELINE
   brain.ScheduleMagnarApproach();
#endif
   motion->MovePoint(7,4720.80f,347.91f,-37.71f);Require(!companion.movespline->Finalized(),"Native initial point spline did not launch");
   if(mode==1){auto id=companion.movespline->GetId();companion.Tick(1000);Require(companion.movespline->GetId()==id&&!companion.movespline->Finalized()&&!StartStories(map),"Approach monitor restarted active native spline");}
   companion.StopMoving();motion->MovementExpired(false);
   Require(companion.movespline->Finalized()&&motion->GetCurrentMovementGeneratorType()==(mode>=9?WAYPOINT_MOTION_TYPE:IDLE_MOTION_TYPE)&&!brain.magnarStartReached&&!StartStories(map),"Native finalized-short PointMovement guard did not refuse arrival");
#ifdef HRUTHNIR_APPROACH_BASELINE
   companion.Tick(60000);Require(motion->GetCurrentMovementGeneratorType()==IDLE_MOTION_TYPE&&!brain.magnarStartReached&&!brain.magnarStartPresented&&!*token&&!StartStories(map)&&!boss.Brain().encounterStarted,"Pinned permanently stopped approach no longer reproduces");
   std::cout<<"COUNTERFACTUAL native approach race="<<race<<": real MovePoint/StopMoving/MovementExpired rejects short arrival but remains idle without retry after60s; actual original6s never starts, earlier accepted menu controlled\n";
#else
   std::unique_ptr<PausedAI> pause;
   if(mode==2)companion.AddUnitState(UNIT_STATE_ROOT);
   if(mode==3)owner->Relocate(5000,346,-37.71f,0);
   if(mode==4)pause=std::make_unique<PausedAI>(companion,false);
   if(mode==5){auto receipt=brain.magnarStartAttempt;owner.reset();session->SetPlayer(nullptr);companion.Tick(1000);Require(companion.movespline->Finalized()&&!StartStories(map),"Missing native owner consumed movement");owner=std::make_unique<NativePlayer>(session,map,901,Position{4730.8f,346,-37.71f,0});prepare();script->OnPlayerEnter(owner.get());Require(owner->GetQuestCancellationReceipt(39191)==receipt,"Warm native approach did not adopt original quest receipt");}
   if(mode==6){owner->SetQuestStatus(39191,QUEST_STATUS_NONE);owner->SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);}
   if(mode==7)script->Initialize();
   if(mode==8)owner->Unit::setDeathState(CORPSE);
   if(mode==10||mode==11){companion.Tick(1000);Require(companion.movespline->Finalized()&&!StartStories(map),"Unfinished/repeating native route was overridden");route->*Access(ApproachArrivedTag{})=true;route->*Access(ApproachRepeatTag{})=false;Require(route->IsPathComplete(),"Actual earlier route completion endpoint not restored");}
   if(mode==2||mode==3||mode==4||mode==8){companion.Tick(1000);Require(companion.movespline->Finalized()&&!StartStories(map)&&!brain.magnarStartReached,"Unavailable native actor/player overrode approach");companion.ClearUnitState(UNIT_STATE_ROOT);if(pause)pause->Restore();owner->Relocate(4730.8f,346,-37.71f,0);if(mode==8)owner->Unit::setDeathState(ALIVE);}
   if(mode==6||mode==7){companion.Tick(1000);Require(!brain.magnarStartToken&&companion.movespline->Finalized()&&!StartStories(map)&&!boss.Brain().encounterStarted,"Canceled native approach replayed its stale request");companion.Tick(60000);std::cout<<"PASS native approach cancellation race="<<race<<" mode="<<mode<<": original receipt/epoch cancellation, no move/story/combat\n";}
   else {
   companion.Tick(999);Require(companion.movespline->Finalized(),"Native approach retry ran early");companion.Tick(1);float x,y,z;Require(motion->GetCurrentMovementGeneratorType()==POINT_MOTION_TYPE&&motion->GetDestination(x,y,z)&&fabs(x-4720.80f)<0.01f&&fabs(y-347.91f)<0.01f&&fabs(z+37.71f)<0.01f&&!StartStories(map)&&!brain.magnarStartReached,"Native approach did not reissue original path without fabricated arrival");
   // Endpoint pose is controlled, then actual Stop/Point Finalize notifies AI.
   companion.StopMoving();companion.Relocate(4720.80f,347.91f,-37.71f,0);motion->MovementExpired(false);
   Require(brain.magnarStartReached&&brain.magnarStartPresented&&StartStories(map)==1&&!*token,"Actual finalization at endpoint did not publish497 once");
   companion.Tick(5999);Require(!boss.Brain().encounterStarted&&!*token,"Recovered native approach shortened original6s");companion.Tick(1);Require(boss.Brain().encounterStarted&&*token&&!WaveCredit(scenario),"Recovered native approach did not reach real Magnar acknowledgement");
   boss.Brain().Reset();map.RemoveAllObjectsInRemoveList();std::cout<<"PASS native approach race="<<race<<" mode="<<mode<<": actual stopped-point rejection/1s re-request/original destination, native endpoint callback/497/full6s/Magnar receipt; earlier path endpoint controlled, no client movement claim\n";
   }
   pause.reset();
#endif
   brain.Reset();CleanStories(map);
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native Hruthnir approach contexts="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
