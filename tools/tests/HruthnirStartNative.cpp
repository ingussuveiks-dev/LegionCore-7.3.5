// Actual pending-start methods, Unit queue, Conversation497, Player receipt,
// shared instance re-entry and Magnar controller. Earlier escort is controlled.
#define MAGNAR_READINESS_SUPPORT_ONLY
#include "MagnarReadinessNative.cpp"
#include "QuestObjectScope.h"
#include "../../src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/hruthnir_pillik.h"
#include "FunctionProcessor.h"
#include "ScriptedGossip.h"
#include "CreatureTextMgr.h"
#include "WaypointMovementGenerator.h"
#include "PathGenerator.h"
#include "HruthnirNative.inc"
namespace ShieldsFixture {
#include "../../src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.h"
#include "ShieldsNative.inc"
}
using HruthnirAI=npc_hrutnir::npc_hrutnirAI;
struct StartFunctionsTag { using Type=FunctionProcessor Unit::*; friend Type Access(StartFunctionsTag); };
template struct FixtureAccess<StartFunctionsTag,&Unit::m_Functions>;
struct HruthnirActor:Actor {
 using Actor::Actor;
 void AddToWorld()override{Actor::AddToWorld();Require(AIM_Initialize(new HruthnirAI(this)),"Native Hruthnir AI missing");}
 HruthnirAI& Brain(){return *static_cast<HruthnirAI*>(AI());}
 void Tick(uint32 diff){(this->*Access(StartFunctionsTag{})).Update(diff);}
};
static uint32 StartStories(Map& map){uint32 n=0;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==497)++n;return n;}
#ifndef HRUTHNIR_START_SUPPORT_ONLY
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<7;++mode){
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9988200+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual exported shared instance missing");
  auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   auto owner=std::make_unique<NativePlayer>(session,map,901,Position{4715,346,-37.7f,0});
   auto prepare=[&]{owner->SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner->setFactionForRace(race);owner->SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);owner->SetQuestSlot(0,39191);};prepare();
   HruthnirActor companion(&map,96468,12102,{4720.80f,347.91f,-37.71f,0});Require(map.AddToMap(static_cast<Creature*>(&companion)),"Native Hruthnir admission failed");
   std::unique_ptr<MagnarActor> boss;
   auto addBoss=[&]{boss=std::make_unique<MagnarActor>(&map,96034,12101,Position{4714.82f,349.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(boss.get())),"Native boss admission failed");};
   if(mode!=0)addBoss();
   auto& brain=companion.Brain();
   // Controlled earlier chamber/accepted menu endpoints. Point7 notification,
   // actual497 factory, six-second queue and native DoAction1 remain real.
   script->magnarGuardCreditSent=true;script->magnarCompanionGUID=companion.GetGUID();
   brain.magnarChamberStarted=true;brain.magnarIntroInstanceGeneration=script->GetData(1060);
   brain.magnarStartInstance=map.GetInstanceId();brain.magnarStartOwner=owner->GetGUID();brain.magnarStartAttempt=owner->GetQuestCancellationReceipt(39191);
   brain.magnarStartToken=std::make_shared<bool>(false);brain.magnarArrivalPending=true;auto token=brain.magnarStartToken;
   Require(brain.magnarStartAttempt&&!brain.magnarStartAttempt->load(),"Native original quest receipt missing");
   brain.MovementInform(POINT_MOTION_TYPE,7);
   if(mode==0){Require(!brain.magnarStartPresented&&!*token&&!StartStories(map),"Missing actual497 actor consumed arrival");addBoss();companion.Tick(999);Require(!brain.magnarStartPresented,"Native factory retry ran early");companion.Tick(1);}
   Require(brain.magnarStartPresented&&!brain.magnarArrivalPending&&StartStories(map)==1&&!*token,"Actual497 not published exactly once");auto conversation=brain.magnarConversation;auto* originalBossAI=&boss->Brain();
   std::unique_ptr<PausedAI> pause;if(mode==1||mode==2)pause=std::make_unique<PausedAI>(*boss,mode==2);
   if(mode==3)owner->Relocate(5000,346,-37.7f,0);
   if(mode==4||mode==5){
    auto receipt=brain.magnarStartAttempt;owner.reset();session->SetPlayer(nullptr);
    owner=std::make_unique<NativePlayer>(session,map,901,Position{4715,346,-37.7f,0});prepare();
    Require(owner->GetQuestCancellationReceipt(39191)!=receipt,"Fresh native Player reused old object receipt");
    script->OnPlayerEnter(owner.get());Require(owner->GetQuestCancellationReceipt(39191)==receipt,"Real shared OnPlayerEnter failed original receipt adoption");
    if(mode==5){owner->SetQuestStatus(39191,QUEST_STATUS_NONE);owner->SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);Require(receipt->load(),"Native abandon/reaccept did not cancel original receipt");}
   }
   companion.Tick(5999);Require(!originalBossAI->encounterStarted&&!*token,"Six-second native cue ran early");companion.Tick(1);
   if(mode==5){Require(!brain.magnarStartToken&&!originalBossAI->encounterStarted&&!StartStories(map),"Canceled native warm-rejoin attempt started combat");}
   else {
    if(mode>=1&&mode<=3){Require(!*token&&!originalBossAI->encounterStarted,"Native transient fault consumed cue");if(pause)pause->Restore();owner->Relocate(4715,346,-37.7f,0);companion.Tick(999);Require(!*token,"Native retry ran before1s");companion.Tick(1);}
    Require(*token&&boss->Brain().GetData(MagnarStart::Accepted)&&boss->Brain().reserveGUIDs[11],"Actual Magnar acknowledgement not retained");
    auto roster=boss->Brain().reserveGUIDs;Require(StartStories(map)==1&&brain.magnarConversation==conversation&&!WaveCredit(scenario),"Start retry replayed497 or fabricated victory");
    if(mode==6){
     Require(boss->AIM_Initialize(new MagnarAI(boss.get())),"Native first-phase AI replacement failed");map.RemoveAllObjectsInRemoveList();
     for(auto guid:roster)Require(!map.GetCreature(guid),"Replaced native AI orphaned its reserve");
     Require(!boss->Brain().GetData(MagnarStart::Accepted)&&!WaveCredit(scenario),"AI cleanup gave progress/retained old acknowledgement");
     boss->Brain().DoAction(1);auto replacement=boss->Brain().reserveGUIDs;Require(replacement[11],"Native replacement did not restart its owned reserve");
     for(auto guid:replacement)for(auto old:roster)Require(guid!=old,"Native replacement reused stale reserve identity");
     roster=replacement;
    }
    brain.MovementInform(POINT_MOTION_TYPE,7);brain.TryMagnarStart();companion.Tick(10000);
    Require(boss->Brain().reserveGUIDs==roster&&StartStories(map)==1&&!WaveCredit(scenario),"Repeated native start duplicated reserve/story/credit");
    boss->Brain().Reset();map.RemoveAllObjectsInRemoveList();for(auto guid:roster)Require(!map.GetCreature(guid),"Native reset retained a reserve actor");
   }
   pause.reset();brain.Reset();CleanStories(map);boss.reset();owner.reset();session->SetPlayer(nullptr);
   std::cout<<"PASS native Hruthnir start race="<<race<<" mode="<<mode<<": actual497/Unit queue/quest receipt/shared entry/Magnar acknowledgement; no player progress or character save; earlier escort/menu endpoint controlled\n";
  }
  delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native Hruthnir start completed cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
