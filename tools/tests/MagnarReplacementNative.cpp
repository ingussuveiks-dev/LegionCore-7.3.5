// Real native AIM_Destroy/InitializeAI after actual497/6s/acknowledgement.
// Earlier route/chamber/menu and the endpoint poses are controlled.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
struct ReplacementPlayerLink { Player& player; ReplacementPlayerLink(Player& p, Map& m):player(p){p.GetMapRef().link(&m,&p);} ~ReplacementPlayerLink(){player.GetMapRef().unlink();} };
struct ReplacementLockTag{using Type=bool Creature::*;friend Type Access(ReplacementLockTag);};
template struct FixtureAccess<ReplacementLockTag,&Creature::m_AI_locked>;
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("creature_text","CreatureID=96468");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sCreatureTextMgr->LoadCreatureTexts();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(auto status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE})for(uint32 mode=0;mode<10;++mode){
#ifdef MAGNAR_REPLACEMENT_BASELINE
  if(mode||status!=QUEST_STATUS_INCOMPLETE)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9997200+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   NativePlayer player(session,map,901,{4715,346,-37.7f,0});ReplacementPlayerLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,status);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},true);
   HruthnirActor actor(&map,96468,12502,{4720.80f,347.91f,-37.71f,0});actor.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&actor)),"Actual Hruthnir admission failed");actor.SetPhaseId({6999},true);script->OnCreatureCreate(&actor);
   MagnarActor boss(&map,96034,12501,{4714.82f,349.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(&boss)),"Actual Magnar admission failed");boss.SetPhaseId({6999},true);
   auto& brain=actor.Brain();script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;brain.magnarChamberStarted=true;brain.magnarIntroInstanceGeneration=script->GetData(1060);
   brain.magnarStartInstance=map.GetInstanceId();brain.magnarStartOwner=player.GetGUID();brain.magnarStartAttempt=player.GetQuestCancellationReceipt(39191);brain.magnarStartToken=std::make_shared<bool>(false);brain.magnarArrivalPending=true;auto consumed=brain.magnarStartToken;
   brain.MovementInform(POINT_MOTION_TYPE,7);Require(StartStories(map)==1&&!*consumed,"Actual497 missing");if(mode==5){auto story=brain.magnarConversation;Require(boss.AIM_Initialize(new MagnarAI(&boss)),"Queued native replacement failed");Require(brain.magnarStartToken==consumed&&!*consumed&&StartStories(map)==1&&brain.magnarConversation==story,"Initial Reset consumed queued request/story");}actor.Tick(5999);Require(!*consumed,"Start cue ran early");actor.Tick(1);Require(*consumed&&boss.Brain().GetData(MagnarStart::Accepted),"Actual boss did not acknowledge start");auto roster=boss.Brain().reserveGUIDs;Require(roster[11],"Actual reserve incomplete");
   if(mode==4){MagnarActor foreign(&map,96034,12503,{4714.82f,350.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(&foreign)),"Foreign native boss missing");foreign.SetPhaseId({6999},true);foreign.Brain().DoAction(1);Require(foreign.Brain().encounterStarted,"Foreign native start missing");Require(foreign.AIM_Initialize(new MagnarAI(&foreign)),"Foreign native replacement failed");map.RemoveAllObjectsInRemoveList();Require(brain.magnarStartToken==consumed&&*consumed&&StartStories(map)==1&&boss.Brain().reserveGUIDs==roster,"Foreign Reset revoked acknowledged boss");}
   if(mode==8){boss.*Access(ReplacementLockTag{})=true;auto rejected=new MagnarAI(&boss);Require(!boss.AIM_Initialize(rejected),"Locked native AI unexpectedly replaced");delete rejected;boss.*Access(ReplacementLockTag{})=false;Require(brain.magnarStartToken==consumed&&boss.Brain().GetData(MagnarStart::Accepted)&&boss.Brain().reserveGUIDs==roster,"Failed replacement revoked accepted request");}
   if(mode==9){PausedAI transient(boss,true);brain.UpdateAI(1000);Require(brain.magnarStartToken==consumed&&StartStories(map)==1,"Unavailable native boss alone revoked acknowledged request");}
   std::unique_ptr<PausedAI> pause;if(mode==1)pause=std::make_unique<PausedAI>(actor,false);
   if(mode==2)player.Unit::setDeathState(CORPSE);
   if(mode==3)player.SetQuestStatus(39191,QUEST_STATUS_NONE);
   if(mode==6)++brain.magnarIntroInstanceGeneration;
   if(mode==7)scenario->SetCurrentStep(4);
   Require(boss.AIM_Initialize(new MagnarAI(&boss)),"Actual acknowledged native replacement failed");map.RemoveAllObjectsInRemoveList();for(auto guid:roster)Require(!map.GetCreature(guid),"Actual Reset left old reserve");Require(!boss.Brain().GetData(MagnarStart::Accepted)&&!WaveCredit(scenario)&&scenario->GetCurrentStep()==(mode==7?4:3),"Reset granted victory");
#ifdef MAGNAR_REPLACEMENT_BASELINE
   brain.UpdateAI(1000);brain.UpdateAI(60000);actor.Tick(60000);Require(brain.magnarStartToken==consumed&&*consumed&&!brain.GetData(1061)&&actor.movespline->Finalized(),"Pinned consumed-start replacement dead end did not reproduce");
   std::cout<<"COUNTERFACTUAL native Magnar replacement race="<<race<<": actual accepted encounter/native AIM_Destroy+InitializeAI despawns old reserve but fresh Reset misses old encounterStarted and Hruthnir retains consumed token and cannot reoffer\n";
#else
   if(mode==6||mode==7){Require(brain.magnarStartToken==consumed&&StartStories(map)==1,"Wrong guard epoch/later stage revoked claim");brain.Reset();CleanStories(map);std::cout<<"PASS native replacement rejection race="<<race<<" status="<<uint32(status)<<" mode="<<mode<<": actual reserve cleanup with revoked epoch/later scenario; no ready offer or credit\n";}
   else{
   Require(!brain.magnarStartToken&&!StartStories(map),"Actual boss Reset did not release its acknowledged start/story");boss.Brain().Reset();Require(!brain.magnarStartToken&&!StartStories(map),"Repeated actual Reset re-created state");
   if(mode==1){brain.UpdateAI(1000);Require(actor.movespline->Finalized(),"Disabled companion moved");pause->Restore();}
   if(mode==2||mode==3){brain.UpdateAI(1000);Require(actor.movespline->Finalized(),"Absent/dead quest participant moved companion");if(mode==2)player.Unit::setDeathState(ALIVE);else{player.SetQuestStatus(39191,status);player.SetQuestSlot(0,39191);}}
   brain.UpdateAI(1000);float x,y,z;Require(actor.GetMotionMaster()->GetDestination(x,y,z)&&fabs(x-4763.73f)<0.01f&&fabs(y-340.87f)<0.01f&&fabs(z+37.19f)<0.01f,"Reset did not request original point6");
   actor.StopMoving();actor.Relocate(4763.73f,340.87f,-37.19f,0);actor.GetMotionMaster()->MovementExpired(false);actor.Tick(12999);Require(!brain.GetData(1061),"Reset skipped13s offer");actor.Tick(1);Require(brain.GetData(1061)&&!StartStories(map)&&!boss.Brain().GetData(MagnarStart::Accepted)&&!WaveCredit(scenario),"Reset reoffer fabricated progress or start");
   // The endpoint pose/menu cache is controlled; admission and selection run
   // their actual native predicates, then real point7 finalization/497/6s.
   player.Relocate(actor);auto& menu=player.PlayerTalkClass->GetGossipMenu();menu.SetMenuId(18743);menu.AddMenuItem(0,GossipOptionNpc::None,"",0,0,"",0);
   Require(HruthnirGossip::CanStart(&player,&actor),"Native ready actor cannot admit real selection");brain.sGossipSelect(&player,18743,0);Require(brain.magnarArrivalPending&&brain.magnarStartToken&&!*brain.magnarStartToken,"Native retry selection failed");
   actor.StopMoving();actor.Relocate(4720.80f,347.91f,-37.71f,0);player.Relocate(4715,346,-37.7f,0);actor.GetMotionMaster()->MovementExpired(false);Require(StartStories(map)==1&&brain.magnarStartPresented,"Native retry point7/497 missing");actor.Tick(5999);Require(!boss.Brain().encounterStarted,"Retry6s cue ran early");actor.Tick(1);Require(*brain.magnarStartToken&&boss.Brain().GetData(MagnarStart::Accepted)&&!WaveCredit(scenario),"Native retry did not reach real acknowledgement");auto replacement=boss.Brain().reserveGUIDs;Require(replacement[11],"Retry reserve missing");for(auto guid:replacement)for(auto old:roster)Require(guid!=old,"Retry reused old roster");brain.TryMagnarStart();Require(boss.Brain().reserveGUIDs==replacement,"Repeated retry duplicated roster");boss.Brain().Reset();map.RemoveAllObjectsInRemoveList();for(auto guid:replacement)Require(!map.GetCreature(guid),"Second Reset retained roster");
   std::cout<<"PASS native Magnar replacement race="<<race<<" status="<<uint32(status)<<" mode="<<mode<<": actual acknowledged encounter/native AI replacement/old roster cleanup,point6/12999+1ms offer,real admission/cached option/point7/497/5999+1ms/new roster,repeat reset/no victory; earlier route/menu and endpoint poses controlled\n";
   }
#endif
   brain.Reset();CleanStories(map);
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native Magnar replacement cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
