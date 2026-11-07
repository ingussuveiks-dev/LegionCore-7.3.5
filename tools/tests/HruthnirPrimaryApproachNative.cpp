// Original DoAction2 -> actual point6 movement, not the cold entrance route.
// Earlier guard/scene receipt and following unsaved Player are controlled.
#define HRUTHNIR_CURSOR_SUPPORT_ONLY
#include "HruthnirCursorNative.cpp"
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sWaypointMgr->Load();
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("creature_text","CreatureID=96468");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sCreatureTextMgr->LoadCreatureTexts();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});Require(DisableMgr::IsPathfindingEnabled(1495),"Native pathfinding disabled");
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(auto status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE})for(uint32 mode=0;mode<9;++mode){
#ifdef HRUTHNIR_PRIMARY_BASELINE
  if(mode||status!=QUEST_STATUS_INCOMPLETE)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9998100+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   Position start=mode==3?Position{4763.73f,340.87f,-37.19f,0}:Position{4822.31f,328.56f,-30.14f,0};map.LoadGrid(start.GetPositionX(),start.GetPositionY());map.LoadGrid(4763.73f,340.87f);
   NativePlayer player(session,map,901,start);CursorPlayerLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,status);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},false);
   HruthnirActor actor(&map,96468,15202,start);actor.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&actor)),"Companion admission failed");actor.SetPhaseId({6999},false);script->OnCreatureCreate(&actor);script->magnarGuardCreditSent=true;script->magnarGuardsDelivered=mode!=7;
   if(mode==4)actor.AddUnitState(UNIT_STATE_ROOT);
   actor.Brain().DoAction(2);Require(actor.Brain().magnarChamberStarted,"Original delivered chamber action not accepted");
   auto tick=[&]{actor.Unit::Update(100);map.MoveAllCreaturesInMoveList();player.Relocate(&actor);actor.Brain().UpdateAI(100);};
   std::unique_ptr<MagnarActor> boss;auto addBoss=[&]{boss=std::make_unique<MagnarActor>(&map,96034,15201,Position{4714.82f,349.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(boss.get())),"Boss admission failed");boss->SetPhaseId({6999},false);};if(mode==0)addBoss();
   auto replace=[&]{auto pose=actor.GetPosition();auto guid=actor.GetGUID();Require(actor.AIM_Initialize(new HruthnirAI(&actor)),"Native AI replacement failed");Require(actor.GetGUID()==guid&&actor.GetExactDist(pose)<0.01f,"AI replacement moved/recreated actor");actor.Brain().UpdateAI(0);};
   if(mode==0||mode==5||mode==6||mode==8){for(uint32 t=0;t<2000&&actor.GetExactDist(start)<5;t+=100)tick();Require(actor.GetExactDist(start)>5&&actor.GetExactDist(4714.82f,349.11f,-37.77f)>60&&!actor.movespline->Finalized(),"Real middle original point6 spline not reached");}
#ifdef HRUTHNIR_PRIMARY_BASELINE
   Require(!script->GetData(MagnarStart::RouteCursor),"Pinned original approach unexpectedly persisted route intent");replace();actor.StopMoving();actor.GetMotionMaster()->MovementExpired(false);Require(actor.GetExactDist(boss.get())>60,"Interrupted baseline is not outside actual live boss range");for(uint32 t=0;t<10000;t+=100)tick();Require(actor.GetExactDist(boss.get())>60&&actor.Brain().HasMagnarChamberRequest()&&!actor.Brain().magnarRoutePoint&&actor.movespline->Finalized()&&!actor.Brain().magnarOfferToken&&!CursorCredit(scenario)&&!StartStories(map),"Pinned original approach dead end not reproduced");
   std::cout<<"COUNTERFACTUAL original point6 race="<<race<<": native DoAction2 motion interrupted after same-GUID AI replacement loses unrecorded intent,retains earned receipt but remains idle10s outside actual live boss60yd; expected broken,not successful gameplay\n";
#else
   if(mode==7){Require(!script->GetData(MagnarStart::RouteCursor),"Undelivered receipt invented a route");replace();Require(!actor.Brain().magnarChamberStarted,"Undelivered original request restored");}
   else if(mode==6){auto epoch=script->GetData(1060);script->Initialize();replace();script->SetData(MagnarStart::RouteCursor,epoch);Require(!actor.Brain().magnarChamberStarted&&!script->GetData(MagnarStart::RouteCursor),"Old epoch original approach restored");}
   else {
    Require(script->GetData(MagnarStart::RouteCursor)==12,"Original point6 did not retain actual leg12 intent");
    if(mode==8){auto spline=actor.movespline->GetId();actor.Brain().DoAction(2);Require(actor.movespline->GetId()==spline&&script->GetData(MagnarStart::RouteCursor)==12,"Repeated original chamber action restarted motion");}
    if(mode==2||mode==3){for(uint32 t=0;t<30000&&!actor.Brain().magnarOfferToken;t+=100)tick();Require(actor.Brain().magnarOfferToken&&script->GetData(MagnarStart::RouteCursor)==13,"Actual original point6 arrival did not persist physical checkpoint13");}
    if(mode==5)player.SetQuestStatus(39191,QUEST_STATUS_NONE);
    replace();Require(actor.Brain().magnarRoutePoint==((mode==2||mode==3)?13:12),"Fresh AI lost original approach checkpoint");
    if(mode==0){actor.StopMoving();actor.GetMotionMaster()->MovementExpired(false);Require(actor.GetExactDist(boss.get())>60,"Actual original interruption missed middle leg");}
    if(mode==4||mode==5){
     if(mode==5)Require(player.GetQuestStatus(39191)==QUEST_STATUS_NONE&&!actor.Brain().IsMagnarIntroPlayer(&player),"Missing-quest participant control not established");
     auto spline=actor.movespline->GetId();for(uint32 t=0;t<3000;t+=100)tick();
     // Motion_Initialize clears the generator; native Unit spline may still
     // finish an already issued move. Refuse a new route while ineligible.
     Require((mode==5||actor.movespline->Finalized())&&actor.movespline->GetId()==spline&&!actor.Brain().magnarOfferToken&&actor.Brain().magnarRoutePoint==12,"Original replacement issued new motion/offer while root or participant deferred");actor.ClearUnitState(UNIT_STATE_ROOT);player.SetQuestStatus(39191,status);
    }
    for(uint32 t=0;t<30000&&actor.Brain().magnarRoutePoint!=13;t+=100)tick();Require(actor.Brain().magnarRoutePoint==13&&actor.Brain().IsMagnarOfferPosition()&&!CursorCredit(scenario)&&!StartStories(map),"Original approach recovery failed real physical endpoint or replayed credit");
    if(!boss)addBoss();for(uint32 t=0;t<3000&&!actor.Brain().magnarOfferToken;t+=100)tick();Require(bool(actor.Brain().magnarOfferToken),"Real point6 offer missing");actor.Tick(12999);Require(!actor.Brain().GetData(1061),"Original recovery skipped13s");actor.Tick(1);Require(actor.Brain().GetData(1061),"Original offer not ready");auto& menu=player.PlayerTalkClass->GetGossipMenu();menu.SetMenuId(18743);menu.AddMenuItem(0,GossipOptionNpc::None,"",0,0,"",0);actor.Brain().sGossipSelect(&player,18743,0);for(uint32 t=0;t<30000&&!actor.Brain().magnarStartPresented;t+=100)tick();Require(actor.Brain().magnarStartPresented&&StartStories(map)==1&&!boss->Brain().encounterStarted,"Actual point7/497 missing");actor.Tick(5999);Require(!boss->Brain().encounterStarted,"Original recovery skipped6s");actor.Tick(1);Require(boss->Brain().GetData(MagnarStart::Accepted)&&boss->Brain().reserveGUIDs[11]&&!CursorCredit(scenario),"Original recovery failed native reserve/no-credit endpoint");boss->Brain().Reset();map.RemoveAllObjectsInRemoveList();
   }
   std::cout<<"PASS original approach race="<<race<<" status="<<status<<" mode="<<mode<<": actual DoAction2/point6/same-GUID AI lifecycle or admission refusal; original13s/menu/point7/497/6s/reserve; no old criteria; earlier receipt/player follow controlled,client traversal separate\n";
#endif
   actor.Brain().Reset();actor.StopMoving();CleanStories(map);
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native original approach cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
