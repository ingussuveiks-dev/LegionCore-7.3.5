#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
struct ReofferLink{Player& p;ReofferLink(Player& player,Map& map):p(player){p.GetMapRef().link(&map,&p);}~ReofferLink(){p.GetMapRef().unlink();}};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("creature_text","CreatureID=96468");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sCreatureTextMgr->LoadCreatureTexts();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<8;++mode){
#ifdef HRUTHNIR_REOFFER_BASELINE
  if(mode>=2)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9992200+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   NativePlayer player(session,map,901,{4730.8f,346,-37.71f,0});ReofferLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},true);
   HruthnirActor actor(&map,96468,12402,{4730.8f,347.91f,-37.71f,0});actor.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&actor)),"Native Hruthnir admission failed");actor.SetPhaseId({6999},true);script->OnCreatureCreate(&actor);
   MagnarActor boss(&map,96034,12401,{4714.82f,349.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native Magnar admission failed");boss.SetPhaseId({6999},true);auto& brain=actor.Brain();script->magnarGuardCreditSent=true;brain.magnarChamberStarted=true;brain.magnarIntroInstanceGeneration=script->GetData(1060);
   // Earlier guard victory/chamber/accepted menu endpoints controlled.
   brain.magnarStartInstance=map.GetInstanceId();brain.magnarStartOwner=player.GetGUID();brain.magnarStartAttempt=player.GetQuestCancellationReceipt(39191);brain.magnarStartToken=std::make_shared<bool>(false);brain.magnarArrivalPending=true;actor.RemoveFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP);
   if(mode==0){brain.ScheduleMagnarApproach();player.SetQuestStatus(39191,QUEST_STATUS_NONE);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);actor.Tick(1000);}else brain.Reset();
   Require(!brain.magnarStartToken&&!brain.magnarOfferToken&&!brain.GetData(1061),"Native cancellation did not revoke old offer");
#ifndef HRUTHNIR_REOFFER_BASELINE
   std::unique_ptr<PausedAI> pause;
   if(mode==2)player.Unit::setDeathState(CORPSE);
   if(mode==3)pause=std::make_unique<PausedAI>(boss,false);
   if(mode==4)script->magnarGuardCreditSent=false;
   if(mode==5)pause=std::make_unique<PausedAI>(actor,false);
   if(mode==6)brain.magnarStartToken=std::make_shared<bool>(false);
   if(mode==7){brain.magnarOfferToken=std::make_shared<bool>(true);brain.magnarOfferReady=true;actor.SetFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP);}
   if(mode>=2&&mode<=6){brain.UpdateAI(1000);Require(actor.movespline->Finalized()&&!brain.magnarOfferToken,"Unavailable native context/request was overridden");if(mode==2)player.Unit::setDeathState(ALIVE);if(pause)pause->Restore();if(mode==4)script->magnarGuardCreditSent=true;if(mode==6)brain.Reset();else{brain.UpdateAI(999);Require(actor.movespline->Finalized(),"Native reoffer1s retry ran early");}}
#endif
   brain.UpdateAI(mode>=2&&mode<=5?1:1000);
#ifdef HRUTHNIR_REOFFER_BASELINE
   brain.UpdateAI(60000);actor.Tick(60000);Require(!brain.GetData(1061)&&!brain.magnarOfferToken&&actor.movespline->Finalized()&&!actor.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP)&&!StartStories(map)&&!boss.Brain().encounterStarted,"Pinned missing reoffer no longer reproduces");
   std::cout<<"COUNTERFACTUAL native reoffer race="<<race<<" mode="<<mode<<": real cancellation/Reset clears selected request and gossip but never offers again; no client traversal claim\n";
#else
   float x,y,z;Require(actor.GetMotionMaster()->GetDestination(x,y,z)&&fabs(x-4763.73f)<0.01f&&fabs(y-340.87f)<0.01f&&fabs(z+37.19f)<0.01f&&!brain.magnarOfferToken,"Native recovery did not request actual original offer destination");
   actor.StopMoving();actor.Relocate(4763.73f,340.87f,-37.19f,0);actor.GetMotionMaster()->MovementExpired(false);Require(brain.magnarOfferToken&&!brain.GetData(1061),"Native real point6 did not arm original offer");
   actor.Tick(12999);Require(!brain.GetData(1061),"Reoffer skipped original13s");actor.Tick(1);Require(brain.GetData(1061)&&actor.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP)&&!StartStories(map)&&!boss.Brain().encounterStarted&&!WaveCredit(scenario)&&scenario->GetCurrentStep()==3,"Native full13s offer not usable without fabricated progress");
   std::cout<<"PASS native reoffer race="<<race<<" mode="<<mode<<": real canceled attempt/Reset,actual point6 request/native finalization and original12999+1ms offer,actual1061/gossip flag,no story/boss/credit grant; earlier route/menu and endpoint pose controlled\n";
   pause.reset();
#endif
   brain.Reset();CleanStories(map);
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native Hruthnir reoffer contexts="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
