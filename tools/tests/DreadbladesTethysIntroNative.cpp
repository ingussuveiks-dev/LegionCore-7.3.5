// Native whole Tethys AI and real summon factory. Earlier quest/scenario,
// explicit LOS, Reset callback and clock are controlled; no client traversal.
#define ALTHEA_CREDIT_NATIVE_SUPPORT_ONLY
#include "AltheaCreditNative.cpp"
#include "MoveSpline.h"
#include "TethysIntro.inc"
static unsigned Stories(Map& map,unsigned entry){unsigned count=0;for(auto c:*Access(StoriesTag{}))if(c&&c->GetMap()==&map&&c->GetEntry()==entry)++count;return count;}
int main(int argc,char**argv){try{
 InitializeAltheaNative(argc,argv);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1545,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1545,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new npc_admiral_tethys_scenario;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)})for(unsigned entry:{102179u,102212u}){
#ifdef TETHYS_INTRO_BASELINE
  std::vector<unsigned> modes=entry==102179?std::vector<unsigned>{1,2}:std::vector<unsigned>{1};
#elif defined(TETHYS_INTRO_FAULTS)
  std::vector<unsigned> modes=entry==102179?std::vector<unsigned>{20,21,22}:std::vector<unsigned>{20};
#else
  std::vector<unsigned> modes=entry==102179?std::vector<unsigned>{0,1,2,3,4,5,6,7,8,9,10}:std::vector<unsigned>{0,1,9};
#endif
  for(unsigned mode:modes){
   MapInstanced parent(1545,300);InstanceMap map(1545,300,9938000+cases++,DIFFICULTY_N_SCENARIO,&parent);
   auto scenario=sScenarioMgr->AddScenario(&map,1012);scenario->SetCurrentStep(entry==102179?0:1);auto session=Session();{
    Position pos=entry==102179?Position{-1397.73f,5885.51f,.882879f,0}:Position{-1215.19f,5805.68f,19.41f,5.68f};
    NativePlayer player(session,map,101,pos);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(race);player.SetQuestStatus(40849,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40849);player.SetPhaseId({6199},false);
    Actor source(&map,102185,17001,pos),supporter(&map,102184,17002,pos);for(auto a:{&source,&supporter}){Require(map.AddToMap(static_cast<Creature*>(a)),"Native source/supporter admission failed");a->SetPhaseId({6199},false);}
    std::unique_ptr<Actor> initial;Creature* actor=nullptr;
    if(entry==102179){initial=std::make_unique<Actor>(&map,entry,17003,pos);actor=initial.get();Require(map.AddToMap(actor),"Initial Tethys admission failed");actor->SetPhaseId({6199},false);Require(actor->AIM_Initialize(new npc_admiral_tethys_scenario::npc_admiral_tethys_scenarioAI(actor)),"Initial Tethys AI absent");}
    else {actor=source.SummonCreature(entry,pos,TEMPSUMMON_MANUAL_DESPAWN);Require(actor&&actor->ToTempSummon()&&actor->ToTempSummon()->GetSummonerGUID()==source.GetGUID(),"Native original-source Tethys summon failed");}
    auto ai=dynamic_cast<npc_admiral_tethys_scenario::npc_admiral_tethys_scenarioAI*>(actor->AI());Require(ai,"Actual script-name summon factory did not install Tethys AI");
    unsigned delay=entry==102179?5000:3000,story=entry==102179?1114:1122;
    if(mode==2)player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
    if(mode==3)player.SetQuestStatus(40849,QUEST_STATUS_NONE);
    if(mode==4){player.SetHealth(0);player.setDeathState(CORPSE);}
    if(mode==5)player.Relocate(pos.GetPositionX()+100,pos.GetPositionY(),pos.GetPositionZ(),0);
    if(mode==6)player.SetPhaseId({999},false);
    if(mode==7)scenario->SetCurrentStep(1);
    if(mode==10){ai->MoveInLineOfSight(&source);Require(!ai->intro,"Nonplayer consumed initial Tethys");}
    if(entry==102179)ai->MoveInLineOfSight(&player);
    if(mode==1){ai->UpdateAI(1000);ai->Reset();delay-=1000;}
    if(mode==8){player.SetQuestStatus(40849,QUEST_STATUS_NONE);player.SetQuestStatus(40849,QUEST_STATUS_INCOMPLETE);}
#ifdef TETHYS_INTRO_FAULTS
    auto data=const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(uint32(story)));Require(data&&!data->empty(),"Native Tethys lines absent");auto saved=*data;data->clear();
#endif
    std::cout<<"CASE race="<<race<<" entry="<<entry<<" mode="<<mode<<std::endl;
    ai->UpdateAI(delay-1);Require(!Stories(map,story),"Native presentation started before authored delay");ai->UpdateAI(1);
#ifdef TETHYS_INTRO_BASELINE
    if(mode==1)Require(!Stories(map,story)&&actor->GetMotionMaster()->GetCurrentMovementGeneratorType()!=POINT_MOTION_TYPE,"Expected-broken Reset did not cancel Tethys story/movement");
    else Require(Stories(map,story)==1&&actor->GetMotionMaster()->GetCurrentMovementGeneratorType()==POINT_MOTION_TYPE,"Expected-broken wrong-class admission changed");
    std::cout<<"COUNTERFACTUAL Tethys Reset loses presentation or wrong-class starts it"<<std::endl;
#elif defined(TETHYS_INTRO_FAULTS)
    Require(!Stories(map,story)&&actor->GetMotionMaster()->GetCurrentMovementGeneratorType()!=POINT_MOTION_TYPE,"Failed native story departed");*data=saved;
    if(mode==21){player.SetQuestStatus(40849,QUEST_STATUS_NONE);player.SetQuestStatus(40849,QUEST_STATUS_INCOMPLETE);}
    if(mode==22)player.Relocate(pos.GetPositionX()+100,pos.GetPositionY(),pos.GetPositionZ(),0);
    ai->Reset();ai->UpdateAI(999);Require(!Stories(map,story),"Retry started early");ai->UpdateAI(1);
    if(mode!=20){Require(!Stories(map,story)&&!ai->intro,"Stale player attempt survived");player.Relocate(pos);ai->MoveInLineOfSight(&player);ai->UpdateAI(5000);}
    Require(Stories(map,story)==1&&actor->GetMotionMaster()->GetCurrentMovementGeneratorType()==POINT_MOTION_TYPE,"Same actor factory retry/fresh attempt failed");
#else
    bool accepted=entry==102212||mode==0||mode==1||mode==9||mode==10;
    Require(Stories(map,story)==unsigned(accepted),"Tethys native admission/publication differs");
    Require((actor->GetMotionMaster()->GetCurrentMovementGeneratorType()==POINT_MOTION_TYPE)==accepted,"Story and native departure disagree");
    if(accepted){ai->MoveInLineOfSight(&player);if(entry==102212)ai->IsSummonedBy(&source);ai->Reset();ai->UpdateAI(10000);Require(Stories(map,story)==1,"Repeated callback/reset replayed Tethys story");}
#endif
    Require(scenario->GetCurrentStep()==(entry==102179?(mode==7?1:0):1),"Tethys presentation granted scenario progression");
    CleanStories(map);if(entry==102212){actor->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();}
   }delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());map.RemoveAllObjectsInRemoveList();
  }
 }
 CloseAltheaNative();std::cout<<"PASS Tethys native "<<cases<<" contexts: original delays, real1114/1122, native summon/point request, Reset and eligibility. Actual route/client/combat separate."<<std::endl;return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
