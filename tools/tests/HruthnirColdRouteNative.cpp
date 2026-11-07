// Real Unit spline/MotionMaster/Point finalization on matching1495 mmaps.
// Earlier scene+guard delivery and a following unsaved Player are controlled.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
#include "DisableMgr.h"
struct ColdRoutePlayerLink{Player& p;ColdRoutePlayerLink(Player&a,Map&m):p(a){p.GetMapRef().link(&m,&p);}~ColdRoutePlayerLink(){p.GetMapRef().unlink();}};
static uint64 ColdRouteCredit(Scenario* scenario){uint64 result=0;for(auto const& row:*scenario->GetAchievementMgr().GetCriteriaProgressMap()){auto tree=sCriteriaTreeStore.LookupEntry(row.first);auto c=tree?sCriteriaStore.LookupEntry(tree->CriteriaID):nullptr;if(c&&c->Type==92&&(c->Asset==46288||c->Asset==46289||c->Asset==46358||c->Asset==46427))result+=row.second.Counter;}return result;}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sWaypointMgr->Load();auto path=sWaypointMgr->GetPathScript(9859004);Require(path&&path->size()==7,"Actual7-node9859004 missing");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("creature_text","CreatureID=96468");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sCreatureTextMgr->LoadCreatureTexts();
 auto row=WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=361306 AND id=96468 AND map=1495 AND PhaseId='6999'");Require(bool(row),"Actual original spawn missing");auto f=row->Fetch();Position spawn{f[0].GetFloat(),f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat()};
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});Require(DisableMgr::IsPathfindingEnabled(1495),"Actual1495 pathfinding disabled");
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<7;++mode){
#ifdef HRUTHNIR_COLD_BASELINE
  if(mode)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9996100+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   map.LoadGrid(spawn.GetPositionX(),spawn.GetPositionY());for(auto const& n:*path)map.LoadGrid(n.x,n.y);for(auto const&p:{Position{4864.44f,-33.53f,48.68f,0},Position{4809.14f,164.68f,-11.20f,0},Position{4824.81f,246.52f,-24.29f,0},Position{4822.31f,328.56f,-30.14f,0},Position{4763.73f,340.87f,-37.19f,0}})map.LoadGrid(p.GetPositionX(),p.GetPositionY());
   NativePlayer player(session,map,901,spawn);ColdRoutePlayerLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},false);
   HruthnirActor actor(&map,96468,13202,spawn);actor.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&actor)),"Native companion admission failed");actor.SetPhaseId({6999},false);script->OnCreatureCreate(&actor);script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;auto& brain=actor.Brain();
   Require(actor.GetExactDist(4763.73f,340.87f,-37.19f)>400,"Cold actual spawn is unexpectedly near chamber");brain.UpdateAI(0);
#ifdef HRUTHNIR_COLD_BASELINE
   actor.Unit::Update(10000);map.MoveAllCreaturesInMoveList();brain.UpdateAI(10000);Require(brain.magnarChamberStarted&&brain._introDone&&actor.GetExactDist(spawn)<0.01f&&actor.movespline->Finalized()&&!brain.magnarOfferToken&&!StartStories(map)&&!ColdRouteCredit(scenario),"Pinned cold entrance dead end did not reproduce");
   std::cout<<"COUNTERFACTUAL cold route race="<<race<<": actual DB spawn remains>400yd from chamber,delivered receipt adopted but no nearby boss/reoffer motion after10s; old intro suppressed,not successful gameplay\n";
#else
   Require(brain.magnarRoutePoint==1&&!actor.movespline->Finalized(),"Cold route did not start without a nearby boss");
   if(mode==1||mode==2||mode==4||mode==5){actor.StopMoving();actor.GetMotionMaster()->MovementExpired(false);Require(brain.magnarRoutePoint==1,"False stopped arrival advanced route");
    if(mode==1)actor.AddUnitState(UNIT_STATE_ROOT);if(mode==2)player.Unit::setDeathState(JUST_DIED);if(mode==4)actor.SetPhaseId({6985},false);if(mode==5)player.Relocate(5000,500,0,0);
    auto spline=actor.movespline->GetId();brain.UpdateAI(1000);Require(actor.movespline->Finalized()&&actor.movespline->GetId()==spline&&!brain.magnarOfferToken,"Native deferral restarted a route");actor.ClearUnitState(UNIT_STATE_ROOT);player.Unit::setDeathState(ALIVE);actor.SetPhaseId({6999},false);player.Relocate(&actor);
   }
   if(mode==6){actor.StopMoving();actor.GetMotionMaster()->MovementExpired(false);script->Initialize();brain.UpdateAI(1000);Require(brain.magnarRoutePoint==1&&!brain.HasMagnarChamberRequest()&&actor.movespline->Finalized()&&!brain.magnarOfferToken&&!ColdRouteCredit(scenario),"Reset epoch retained old route progress");actor.Relocate(4763.73f,340.87f,-37.19f,0);player.Relocate(&actor);script->OnCreatureCreate(&actor);script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;brain.magnarRouteTimer=500;brain.magnarRouteFailureReported=true;brain.DoAction(2);Require(brain.magnarRoutePoint==12&&!brain.magnarRouteTimer&&!brain.magnarRouteFailureReported&&brain.HasMagnarChamberRequest(),"New native epoch retained old cold route state");actor.GetMotionMaster()->UpdateMotion(1);Require(bool(brain.magnarOfferToken)&&!brain.GetData(1061),"New epoch original point6 did not arm13s offer");actor.Tick(12999);Require(!brain.GetData(1061),"New epoch skipped13s");actor.Tick(1);Require(brain.GetData(1061)&&!ColdRouteCredit(scenario),"New epoch offer did not recover without old route");}
   else{
    bool interrupted=false;uint32 last=1,arrivals=0;
    for(uint32 elapsed=0;elapsed<300000&&brain.magnarRoutePoint!=13;elapsed+=100){
     actor.Unit::Update(100);map.MoveAllCreaturesInMoveList();player.Relocate(&actor);brain.UpdateAI(100);
     if(mode==3&&!interrupted&&brain.magnarRoutePoint==4&&!actor.movespline->Finalized()){actor.StopMoving();actor.GetMotionMaster()->MovementExpired(false);Require(brain.magnarRoutePoint==4,"Stopped spline fabricated physical arrival");interrupted=true;}
     if(brain.magnarRoutePoint!=last){Require(brain.magnarRoutePoint==last+1,"Native route skipped a checkpoint");last=brain.magnarRoutePoint;++arrivals;}
     Require(!ColdRouteCredit(scenario)&&!StartStories(map)&&scenario->GetCurrentStep()==3,"Cold corridor replayed combat/entry/guard/victory progress or497");
    }
    Require(arrivals==12&&brain.magnarRoutePoint==13&&brain.IsMagnarOfferPosition()&&!brain.magnarOfferToken&&!brain.GetData(1061),"Actual native corridor did not reach final physical point6");
    MagnarActor boss(&map,96034,13201,{4714.82f,349.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native boss admission failed");boss.SetPhaseId({6999},false);if(mode==1){boss.Brain().DoAction(1);Require(boss.Brain().GetData(MagnarStart::Accepted),"Native accepted-boss control missing");brain.UpdateAI(1000);Require(!brain.magnarOfferToken,"Cold endpoint bypassed accepted-boss admission");boss.Brain().Reset();map.RemoveAllObjectsInRemoveList();}for(uint32 elapsed=0;elapsed<3000&&!brain.magnarOfferToken;elapsed+=100){actor.Unit::Update(100);map.MoveAllCreaturesInMoveList();brain.UpdateAI(100);}Require(bool(brain.magnarOfferToken),"Original nearby point6 did not arm offer");actor.Tick(12999);Require(!brain.GetData(1061),"Cold corridor skipped13s offer");actor.Tick(1);Require(brain.GetData(1061)&&!ColdRouteCredit(scenario),"Cold route did not publish original offer");auto& menu=player.PlayerTalkClass->GetGossipMenu();menu.SetMenuId(18743);menu.AddMenuItem(0,GossipOptionNpc::None,"",0,0,"",0);brain.sGossipSelect(&player,18743,0);Require(bool(brain.magnarStartToken),"Real cold restored cached option refused");
    for(uint32 elapsed=0;elapsed<30000&&!brain.magnarStartPresented;elapsed+=100){actor.Unit::Update(100);map.MoveAllCreaturesInMoveList();player.Relocate(&actor);brain.UpdateAI(100);}
    Require(brain.magnarStartPresented&&StartStories(map)==1&&!boss.Brain().encounterStarted,"Actual cold point7/497 missing or cue skipped");actor.Tick(5999);Require(!boss.Brain().encounterStarted,"Cold cached start skipped6s");actor.Tick(1);Require(boss.Brain().GetData(MagnarStart::Accepted)&&boss.Brain().reserveGUIDs[11]&&!ColdRouteCredit(scenario),"Cold route/native cached start did not reach real reserve");boss.Brain().Reset();map.RemoveAllObjectsInRemoveList();
   }
   std::cout<<"PASS cold route race="<<race<<" mode="<<mode<<": native Unit spline/Map relocation/Point finalization/12 ordinary mmap legs or epoch refusal,root/dead participant/phase/range/Stop retry,original13s cached offer/real point7/497/6s/acknowledged reserve; no old credits; earlier delivered scene and following Player controlled,static geometry/client traversal separate\n";
#endif
   brain.Reset();actor.StopMoving();CleanStories(map);
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS cold native cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
