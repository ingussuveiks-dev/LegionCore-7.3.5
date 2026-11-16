// Actual scout and boss classes with native Map/Player/criteria/AI dispatch.
// Earlier quest/stage/poses and the clock/LOS calls are controlled inputs.
#define ALTHEA_CREDIT_NATIVE_SUPPORT_ONLY
#include "AltheaCreditNative.cpp"
#include "CreatureTextMgr.h"
#include "../../src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/dreadblades_scout.h"
#include "Scout.inc"
struct ScoutDbTag{using Type=uint64 Creature::*;friend Type Access(ScoutDbTag);};
template struct FixtureAccess<ScoutDbTag,&Creature::m_DBTableGuid>;
struct ScoutActor:Actor{using Actor::Actor;void Bind(uint64 id){this->*Access(ScoutDbTag{})=id;}};
using ScoutAI=npc_small_events_dreadblades::npc_small_events_dreadbladesAI;
using BrinebeardAI=npc_lord_brinebeard::npc_lord_brinebeardAI;
static uint64 Arrival(Scenario* s){uint64 n=0;for(auto const&r:*s->GetAchievementMgr().GetCriteriaProgressMap()){auto t=sCriteriaTreeStore.LookupEntry(r.first);auto c=t?sCriteriaStore.LookupEntry(t->CriteriaID):nullptr;if(c&&c->Type==92&&c->Asset==48489)n+=r.second.Counter;}return n;}
int main(int argc,char**argv){try{
 InitializeAltheaNative(argc,argv);sCreatureTextMgr->LoadCreatureTexts();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1545,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1545,{}}});unsigned cases=0;
#ifdef SCOUT_BASELINE
 std::vector<unsigned> modes={1,3,4};
#elif defined(SCOUT_FAULTS)
 std::vector<unsigned> modes={1,17,18,19,20};
#else
 std::vector<unsigned> modes={0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
#endif
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)})for(unsigned mode:modes){
  MapInstanced parent(1545,300);InstanceMap map(1545,300,9939000+cases++,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,1012);scenario->SetCurrentStep(1);auto session=Session();{
   Position pos{-1137.19f,6100.02f,43.9221f,0};NativePlayer player(session,map,101,pos);
   player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(race);player.SetQuestStatus(40849,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40849);player.SetPhaseId({6199},false);
   ScoutActor scout(&map,102249,17001,pos);Require(map.AddToMap(static_cast<Creature*>(&scout)),"Native scout admission failed");scout.Bind(360404);scout.SetPhaseId({6199},false);Require(scout.AIM_Initialize(new ScoutAI(&scout)),"Actual scout AI absent");auto ai=static_cast<ScoutAI*>(scout.AI());
   std::unique_ptr<ScoutActor> boss,clone;
   auto makeBoss=[&](bool original){auto value=std::make_unique<ScoutActor>(&map,102239,original?17002:17003,original?Position{-1150.77f,6114.25f,43.9221f,0}:pos);Require(map.AddToMap(static_cast<Creature*>(value.get())),"Native Brinebeard admission failed");value->Bind(original?360405:999);value->SetPhaseId({6199},false);Require(value->AIM_Initialize(new BrinebeardAI(value.get())),"Whole Brinebeard AI absent");return value;};
   if(mode!=1&&mode!=20)boss=makeBoss(true);
   if(mode==4||mode==20)clone=makeBoss(false);
   if(mode==17)Require(boss->AIM_Initialize(new NullCreatureAI(boss.get())),"Null AI control absent");
   if(mode==18)boss->IsAIEnabled=false;
   if(mode==19)boss->SetPhaseId({999},false);
   if(mode==2)scenario->SetCurrentStep(0);
   if(mode==3)player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
   if(mode==5)player.SetQuestStatus(40849,QUEST_STATUS_NONE);
   if(mode==6){player.SetHealth(0);player.setDeathState(CORPSE);}
   if(mode==7)player.Relocate(pos.GetPositionX()+100,pos.GetPositionY(),pos.GetPositionZ(),0);
   if(mode==8)player.SetPhaseId({999},false);
   if(mode==9)player.SetGameMaster(true);
   if(mode==10)scenario->SetCurrentStep(2);
   if(mode==12)scout.Bind(999);
   if(mode==11){ai->MoveInLineOfSight(boss.get());Require(!ai->intro&&scenario->GetCurrentStep()==1,"NPC consumed scout arrival");}
   std::cout<<"CASE race="<<race<<" mode="<<mode<<std::endl;ai->MoveInLineOfSight(&player);
#ifdef SCOUT_BASELINE
   Require(ai->intro&&scenario->GetCurrentStep()==2&&Arrival(scenario)==1,"Expected-broken scout control changed");ai->UpdateAI(6000);
   if(mode==1){Require(!ai->eventcheck,"Baseline retained missing-boss handoff unexpectedly");boss=makeBoss(true);ai->UpdateAI(1000);Require(!static_cast<BrinebeardAI*>(boss->AI())->intro,"Baseline recovered missing boss unexpectedly");}
   if(mode==4)Require(static_cast<BrinebeardAI*>(clone->AI())->intro&&!static_cast<BrinebeardAI*>(boss->AI())->intro,"Baseline nearest foreign boss control changed");
   std::cout<<"COUNTERFACTUAL scout loses missing boss, admits wrong class or starts nearer clone"<<std::endl;
#elif defined(SCOUT_FAULTS)
   Require(ai->eventcheck&&scenario->GetCurrentStep()==2&&Arrival(scenario)==1,"Original native scout arrival failed");ai->UpdateAI(6000);Require(ai->eventcheck&&ai->handoffLine,"Unavailable target consumed handoff");
   if(mode==1||mode==20)boss=makeBoss(true);
   if(mode==17)Require(boss->AIM_Initialize(new BrinebeardAI(boss.get())),"Original boss AI restoration failed");
   if(mode==18)boss->IsAIEnabled=true;
   if(mode==19)boss->SetPhaseId({6199},false);
   // Actual arrival is already earned in this shared instance. The remaining
   // NPC handoff does not give the departing/abandoning player new credit.
   player.SetQuestStatus(40849,QUEST_STATUS_NONE);
   ai->UpdateAI(999);Require(!boss->AI()->GetData(DreadbladesScout::BrinebeardIntroRequested),"Handoff retry ran early");ai->UpdateAI(1);
   Require(!ai->eventcheck&&boss->AI()->GetData(DreadbladesScout::BrinebeardIntroRequested)&&Arrival(scenario)==1,"Same-source retry did not acknowledge actual original boss");
   if(clone)Require(!clone->AI()->GetData(DreadbladesScout::BrinebeardIntroRequested),"Recovery activated foreign clone");
#else
   bool accepted=mode==0||mode==4||mode==11||mode==13||mode==14||mode==15||mode==16;
   Require(ai->intro==accepted&&ai->eventcheck==accepted,"Scout admission differs");
   Require(scenario->GetCurrentStep()==(accepted?2:mode==2?0:mode==10?2:1)&&Arrival(scenario)==unsigned(accepted),"Native arrival refused/duplicated incorrectly");
   if(accepted){
    ai->MoveInLineOfSight(&player);ai->Reset();ai->UpdateAI(5999);Require(!boss->AI()->GetData(DreadbladesScout::BrinebeardIntroRequested),"Six-second scout cue ran early");
    if(mode==15)scenario->SetCurrentStep(3);
    ai->UpdateAI(1);Require(!ai->eventcheck,"Accepted/obsolete scout request remained pending");
    Require(bool(boss->AI()->GetData(DreadbladesScout::BrinebeardIntroRequested))==(mode!=15),"Actual Brinebeard receipt differs");
    if(clone)Require(!clone->AI()->GetData(DreadbladesScout::BrinebeardIntroRequested),"Nearer foreign clone was activated");
    if(mode==14){auto original=static_cast<BrinebeardAI*>(boss->AI());original->UpdateAI(5000);Require(original->eventphase==2,"Native boss first phase missing");original->DoAction(1);Require(original->eventphase==2&&original->eventtimer==5000,"Duplicate request restarted accepted boss intro");original->Reset();Require(original->GetData(DreadbladesScout::BrinebeardIntroRequested),"Combat reset lost accepted receipt");}
    ai->UpdateAI(10000);Require(Arrival(scenario)==1,"Repeated scout update granted extra credit");
   }
   if(mode==16){for(unsigned entry:{102192u,102194u}){ScoutActor ambient(&map,entry,17010+entry-102192,pos);Require(map.AddToMap(static_cast<Creature*>(&ambient)),"Ambient shared actor missing");Require(ambient.AIM_Initialize(new ScoutAI(&ambient)),"Ambient shared AI absent");auto other=static_cast<ScoutAI*>(ambient.AI());other->MoveInLineOfSight(&player);Require(other->intro&&!other->eventcheck&&Arrival(scenario)==1,"Shared ambient branch changed progression");}}
#endif
   CleanStories(map);
  }delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());map.RemoveAllObjectsInRemoveList();
 }
 CloseAltheaNative();std::cout<<"PASS scout native "<<cases<<" contexts: actual48489, six-second cue, original-boss acknowledgement and retry. Full combat/route/client separate."<<std::endl;return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
