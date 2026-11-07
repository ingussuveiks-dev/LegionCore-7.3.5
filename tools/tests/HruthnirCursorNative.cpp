// Real AI replacement and native corridor motion. Earlier guard/scene receipt
// and the following unsaved Player are controlled; no character writes.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
#include "DisableMgr.h"
struct CursorPlayerLink { Player& p; CursorPlayerLink(Player& a,Map& m):p(a){p.GetMapRef().link(&m,&p);} ~CursorPlayerLink(){p.GetMapRef().unlink();} };
static uint64 CursorCredit(Scenario* s) { uint64 n=0; for(auto const& row:*s->GetAchievementMgr().GetCriteriaProgressMap()){auto t=sCriteriaTreeStore.LookupEntry(row.first);auto c=t?sCriteriaStore.LookupEntry(t->CriteriaID):nullptr;if(c&&c->Type==92&&(c->Asset==46288||c->Asset==46289||c->Asset==46358||c->Asset==46427))n+=row.second.Counter;}return n; }
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sWaypointMgr->Load();auto path=sWaypointMgr->GetPathScript(9859004);Require(path&&path->size()==7,"Native9859004 missing");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("creature_text","CreatureID=96468");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sCreatureTextMgr->LoadCreatureTexts();
 auto row=WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=361306 AND id=96468 AND map=1495 AND PhaseId='6999'");Require(bool(row),"Original spawn missing");auto f=row->Fetch();Position spawn{f[0].GetFloat(),f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat()};
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});Require(DisableMgr::IsPathfindingEnabled(1495),"Native pathfinding disabled");
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<8;++mode){
#ifdef HRUTHNIR_CURSOR_BASELINE
  if(mode)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9997100+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   map.LoadGrid(spawn.GetPositionX(),spawn.GetPositionY());for(auto const& n:*path)map.LoadGrid(n.x,n.y);for(auto const& p:{Position{4864.44f,-33.53f,48.68f,0},Position{4809.14f,164.68f,-11.20f,0},Position{4824.81f,246.52f,-24.29f,0},Position{4822.31f,328.56f,-30.14f,0},Position{4763.73f,340.87f,-37.19f,0}})map.LoadGrid(p.GetPositionX(),p.GetPositionY());
   NativePlayer player(session,map,901,spawn);CursorPlayerLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},false);
   HruthnirActor actor(&map,96468,14202,spawn);actor.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&actor)),"Companion admission failed");actor.SetPhaseId({6999},false);script->OnCreatureCreate(&actor);script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;actor.Brain().UpdateAI(0);
   auto tick=[&]{actor.Unit::Update(100);map.MoveAllCreaturesInMoveList();player.Relocate(&actor);actor.Brain().UpdateAI(100);};
   for(uint32 t=0;t<120000&&actor.Brain().magnarRoutePoint<4;t+=100)tick();
   Require(actor.Brain().magnarRoutePoint==4&&actor.GetExactDist(spawn)>5&&actor.GetExactDist(4763.73f,340.87f,-37.19f)>60,"Controlled middle corridor not reached");
   auto replace=[&]{auto pose=actor.GetPosition();auto guid=actor.GetGUID();Require(actor.AIM_Initialize(new HruthnirAI(&actor)),"Actual AIM_Initialize failed");Require(actor.GetGUID()==guid&&actor.GetExactDist(pose)<0.01f,"AI replacement moved/recreated actor");actor.Brain().UpdateAI(0);};
#ifdef HRUTHNIR_CURSOR_BASELINE
   replace();for(uint32 t=0;t<10000;t+=100)tick();Require(!actor.Brain().magnarRoutePoint&&actor.movespline->Finalized()&&!actor.Brain().magnarOfferToken&&!StartStories(map)&&!CursorCredit(scenario),"Pinned middle-corridor dead end did not reproduce");
   std::cout<<"COUNTERFACTUAL cursor race="<<race<<": same GUID actual AI replacement loses point4, remains idle after10s outside entrance/boss range; expected broken,not successful gameplay\n";
#else
   Require(script->GetData(MagnarStart::RouteCursor)==4,"Instance did not retain native route intent");
   if(mode==2){auto epoch=script->GetData(1060);script->Initialize();script->SetData(MagnarStart::RouteCursor,epoch);Require(!script->GetData(MagnarStart::RouteCursor),"Old epoch wrote cursor");replace();Require(!actor.Brain().magnarChamberStarted&&!actor.Brain().magnarRoutePoint,"Old epoch restored route");}
   else if(mode==4){
    actor.StopMoving();actor.GetMotionMaster()->MovementExpired(false);
    HruthnirActor recreated(&map,96468,14203,spawn);recreated.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&recreated)),"New fullGUID admission failed");recreated.SetPhaseId({6999},false);script->OnCreatureCreate(&recreated);Require(recreated.GetGUID()!=actor.GetGUID()&&!script->GetData(MagnarStart::RouteCursor),"New fullGUID inherited old cursor");recreated.Brain().UpdateAI(0);Require(recreated.Brain().magnarRoutePoint==1&&!recreated.movespline->Finalized()&&!actor.Brain().GetData(MagnarStart::RouteCursor),"New fullGUID failed entrance-only route or old actor retained authority");recreated.StopMoving();recreated.Brain().Reset();
   }
   else if(mode>=5){
    auto epoch=script->GetData(1060);actor.Brain().magnarRoutePoint=5;
    if(mode==5)script->magnarGuardsDelivered=false;if(mode==6)scenario->SetCurrentStep(4);if(mode==7)actor.IsAIEnabled=false;
    script->SetData(MagnarStart::RouteCursor,epoch);Require(script->magnarRouteCursor==4,"Ineligible actor/scenario/receipt overwrote cursor");
    script->magnarGuardsDelivered=true;scenario->SetCurrentStep(3);actor.IsAIEnabled=true;script->SetData(MagnarStart::RouteCursor,epoch+1);Require(script->magnarRouteCursor==4,"Caller invented epoch/cursor");
    actor.Brain().magnarRoutePoint=14;script->SetData(MagnarStart::RouteCursor,epoch);Require(script->magnarRouteCursor==4,"Out-of-range local cursor persisted");
   }
   else {
    replace();Require(actor.Brain().magnarRoutePoint==4,"Fresh AI lost native middle-corridor intent");
    if(mode==3){actor.StopMoving();actor.GetMotionMaster()->MovementExpired(false);actor.AddUnitState(UNIT_STATE_ROOT);replace();auto id=actor.movespline->GetId();for(uint32 t=0;t<3000;t+=100)tick();Require(actor.movespline->Finalized()&&actor.movespline->GetId()==id&&actor.Brain().magnarRoutePoint==4,"Replacement bypassed root");actor.ClearUnitState(UNIT_STATE_ROOT);}
    uint32 replaced=4;
    for(uint32 t=0;t<300000&&actor.Brain().magnarRoutePoint!=13;t+=100){tick();auto point=actor.Brain().magnarRoutePoint;Require(point>=4&&point<=13&&!CursorCredit(scenario)&&!StartStories(map)&&scenario->GetCurrentStep()==3,"Replacement replayed credits/story or skipped route");if(mode==1&&point>replaced){replaced=point;replace();Require(actor.Brain().magnarRoutePoint==point,"Repeated checkpoint replacement lost cursor");}}
    Require(actor.Brain().magnarRoutePoint==13&&actor.Brain().IsMagnarOfferPosition()&&!actor.Brain().magnarOfferToken,"Restored route did not reach physical chamber");
    MagnarActor boss(&map,96034,14201,{4714.82f,349.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(&boss)),"Boss admission failed");boss.SetPhaseId({6999},false);for(uint32 t=0;t<3000&&!actor.Brain().magnarOfferToken;t+=100)tick();Require(bool(actor.Brain().magnarOfferToken),"Real point6 offer missing");actor.Tick(12999);Require(!actor.Brain().GetData(1061),"Skipped13s");actor.Tick(1);Require(actor.Brain().GetData(1061),"Original offer not ready");auto& menu=player.PlayerTalkClass->GetGossipMenu();menu.SetMenuId(18743);menu.AddMenuItem(0,GossipOptionNpc::None,"",0,0,"",0);actor.Brain().sGossipSelect(&player,18743,0);for(uint32 t=0;t<30000&&!actor.Brain().magnarStartPresented;t+=100)tick();Require(actor.Brain().magnarStartPresented&&StartStories(map)==1&&!boss.Brain().encounterStarted,"Real point7/497 missing");actor.Tick(5999);Require(!boss.Brain().encounterStarted,"Skipped6s");actor.Tick(1);Require(boss.Brain().GetData(MagnarStart::Accepted)&&boss.Brain().reserveGUIDs[11]&&!CursorCredit(scenario),"Restored route failed real reserve/no-credit endpoint");boss.Brain().Reset();map.RemoveAllObjectsInRemoveList();
   }
   std::cout<<"PASS cursor race="<<race<<" mode="<<mode<<": actual AI replacement/native route/finalization; epoch/root/repeated checkpoint handling; real13s/menu/point7/497/6s/reserve for positive paths; no old credits; earlier receipt/player follow controlled,client traversal separate\n";
#endif
   actor.Brain().Reset();actor.StopMoving();CleanStories(map);
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native cursor cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
