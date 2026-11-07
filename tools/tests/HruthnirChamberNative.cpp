// Fresh native Hruthnir AI/actor in a legitimately delivered live chamber.
// Earlier escort/guard scene and physical endpoint poses are controlled.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
struct ChamberPlayerLink {Player& p;ChamberPlayerLink(Player& a,Map& m):p(a){p.GetMapRef().link(&m,&p);}~ChamberPlayerLink(){p.GetMapRef().unlink();}};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("creature_text","CreatureID=96468");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sCreatureTextMgr->LoadCreatureTexts();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<10;++mode){
#ifdef HRUTHNIR_CHAMBER_BASELINE
  if(mode>=2)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9994200+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   NativePlayer player(session,map,901,{4760,340,-37.19f,0});ChamberPlayerLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},true);
   MagnarActor boss(&map,96034,12601,{4714.82f,349.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native Magnar missing");boss.SetPhaseId({6999},true);
   std::unique_ptr<HruthnirActor> actor;
   auto add=[&](uint32 guid){actor=std::make_unique<HruthnirActor>(&map,96468,guid,mode==1?Position{4760,342.5f,-37.19f,0}:Position{4763.73f,340.87f,-37.19f,0});actor.get()->*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(actor.get())),"Native Hruthnir missing");actor->SetPhaseId({6999},true);script->OnCreatureCreate(actor.get());};add(12602);
   // Retain the actual instance epoch/receipt across a real AIM replacement
   // or a real remove/add with a different full creature GUID.
   script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;actor->Brain().magnarChamberStarted=true;actor->Brain().magnarIntroInstanceGeneration=script->GetData(1060);actor->Brain()._introDone=true;
   if(mode==6){boss.Brain().DoAction(1);Require(boss.Brain().GetData(MagnarStart::Accepted),"Native active encounter witness missing");}
   if(mode!=1)Require(actor->AIM_Initialize(new HruthnirAI(actor.get())),"Native AI replacement failed");else{actor.reset();add(12603);}
   auto& brain=actor->Brain();Require(!brain.magnarChamberStarted&&!brain._introDone&&script->magnarGuardsDelivered&&script->GetData(1059),"Native replacement did not retain instance-only receipt");
   std::unique_ptr<PausedAI> pause;
#ifndef HRUTHNIR_CHAMBER_BASELINE
   if(mode==2)script->magnarGuardCreditSent=false;
   if(mode==3)script->magnarGuardsDelivered=false;
   if(mode==4)script->magnarCompanionGUID=boss.GetGUID();
   if(mode==5)pause=std::make_unique<PausedAI>(*actor,false);
   if(mode==7)player.Unit::setDeathState(CORPSE);
   if(mode==8)scenario->SetCurrentStep(4);
   if(mode==9)actor->AddUnitState(UNIT_STATE_ROOT);
   if(mode>=2){brain.UpdateAI(1000);Require(actor->movespline->Finalized()&&!brain.magnarOfferToken,"Unavailable native context moved/published offer");
    if(mode==2||mode==3||mode==4||mode==5||mode==8)Require(!brain.magnarChamberStarted&&!brain._introDone,"Invalid native handoff context adopted chamber");
    else Require(brain.magnarChamberStarted&&brain._introDone,"Metadata-only adoption was lost during legitimate wait");
    if(mode==2)Require(!script->GetData(MagnarStart::ChamberDelivered),"Receipt ignored real guard credit");
    if(mode==3)Require(!script->GetData(MagnarStart::ChamberDelivered),"Receipt ignored real chamber delivery");
    script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;script->OnCreatureCreate(actor.get());if(pause)pause->Restore();if(mode==6){boss.Brain().Reset();map.RemoveAllObjectsInRemoveList();}if(mode==7)player.Unit::setDeathState(ALIVE);if(mode==8)scenario->SetCurrentStep(3);if(mode==9)actor->ClearUnitState(UNIT_STATE_ROOT);
   }
#endif
   brain.MoveInLineOfSight(&player);brain.UpdateAI(1000);
#ifdef HRUTHNIR_CHAMBER_BASELINE
   brain.UpdateLifecycle(4999);brain.UpdateLifecycle(1);float x=0,y=0,z=0;bool destination=actor->GetMotionMaster()->GetDestination(x,y,z);std::cout<<"CONTROL baseline intro="<<brain._introDone<<" chamber="<<brain.magnarChamberStarted<<" ready="<<brain.GetData(1061)<<" destination="<<destination<<" x="<<x<<" y="<<y<<" z="<<z<<std::endl;Require(destination&&fabs(x-4864.44f)<0.01f&&!brain.magnarChamberStarted&&!brain.GetData(1061)&&!WaveCredit(scenario),"Pinned fresh-AI chamber dead end/initial-route replay did not reproduce");
   std::cout<<"COUNTERFACTUAL fresh Hruthnir race="<<race<<" mode="<<mode<<": native instance retains delivered guard receipt,real new AI/actor loses chamber and LOS replays old point1; no ready offer\n";
#else
   Require(brain.magnarChamberStarted&&brain._introDone&&brain.HasMagnarChamberRequest(),"Actual delivered chamber receipt not adopted");float x=0,y=0,z=0;bool destination=actor->GetMotionMaster()->GetDestination(x,y,z);
   if(mode!=1){Require(!destination&&actor->movespline->Finalized()&&actor->GetMotionMaster()->GetCurrentMovementGeneratorType()==POINT_MOTION_TYPE&&brain.IsMagnarOfferPosition(),"Native coincident point6 not installed");actor->GetMotionMaster()->UpdateMotion(1);}
   else{Require(destination&&fabs(x-4763.73f)<0.01f&&fabs(y-340.87f)<0.01f&&fabs(z+37.19f)<0.01f,"Fresh native AI did not request original point6");actor->StopMoving();actor->Relocate(4763.73f,340.87f,-37.19f,0);actor->GetMotionMaster()->MovementExpired(false);}
   Require(bool(brain.magnarOfferToken),"Actual point6 finalization did not arm offer");brain.UpdateLifecycle(4999);brain.UpdateLifecycle(1);Require(actor->GetMotionMaster()->GetCurrentMovementGeneratorType()==IDLE_MOTION_TYPE,"Fresh chamber replayed old5s point1");actor->Tick(12999);Require(!brain.GetData(1061),"Fresh native AI skipped13s");actor->Tick(1);Require(brain.GetData(1061)&&!StartStories(map)&&!boss.Brain().encounterStarted&&!WaveCredit(scenario),"Fresh ready offer fabricated start/progress");
   player.Relocate(actor.get());auto& menu=player.PlayerTalkClass->GetGossipMenu();menu.SetMenuId(18743);menu.AddMenuItem(0,GossipOptionNpc::None,"",0,0,"",0);Require(HruthnirGossip::CanStart(&player,actor.get()),"Native restored offer interaction denied");brain.sGossipSelect(&player,18743,0);Require(brain.magnarStartToken&&brain.magnarArrivalPending,"Native restored cached option failed");actor->StopMoving();actor->Relocate(4720.80f,347.91f,-37.71f,0);player.Relocate(4715,346,-37.7f,0);actor->GetMotionMaster()->MovementExpired(false);Require(StartStories(map)==1&&brain.magnarStartPresented,"Native restored point7/497 failed");actor->Tick(5999);Require(!boss.Brain().encounterStarted,"Fresh start skipped6s");actor->Tick(1);Require(*brain.magnarStartToken&&boss.Brain().GetData(MagnarStart::Accepted)&&boss.Brain().reserveGUIDs[11]&&!WaveCredit(scenario),"Fresh cached start did not reach native acknowledged reserve");auto roster=boss.Brain().reserveGUIDs;brain.TryMagnarStart();Require(boss.Brain().reserveGUIDs==roster,"Restored request duplicated roster");boss.Brain().Reset();map.RemoveAllObjectsInRemoveList();for(auto guid:roster)Require(!map.GetCreature(guid),"Restored encounter Reset left reserve");
   std::cout<<"PASS fresh Hruthnir race="<<race<<" mode="<<mode<<": actual delivered instance/new AI or full-GUID actor/admission deferral/LOS suppression/real point6 finalization/12999+1ms/native cached option/point7/497/5999+1ms/acknowledged roster/Reset cleanup; no victory grant; earlier route/guard and endpoint poses controlled\n";
#endif
   brain.Reset();actor->StopMoving();CleanStories(map);actor.reset();
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native fresh chamber cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
