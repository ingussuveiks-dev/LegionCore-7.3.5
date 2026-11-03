// OPEN investigation counterfactual, not a passing repaired regression.
// Earlier quest/scenario and direct LOS dispatch are controlled. The complete
// current production Althea class and native Player/Scenario services execute.
#define ALTHEA_CREDIT_NATIVE_SUPPORT_ONLY
#include "../tests/AltheaCreditNative.cpp"
#include "AltheaIntro.inc"
static uint64 IntroCounter(Scenario* scenario){auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto found=rows->find(48577);return found==rows->end()?0:found->second.Counter;}
int main(int argc,char** argv){try{
 InitializeAltheaNative(argc,argv);
 for(bool gmFirst:{false,true}){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9996000+gmFirst,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(4);
  auto first=Session(),second=Session();
  {
   Position position{-8607.59f,394.141f,110.255f,0};Position visitor=position;visitor.m_positionX+=50;
   NativePlayer gm(first,map,101,visitor),eligible(second,map,102,visitor);
   for(auto p:{&gm,&eligible}){p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->SetQuestStatus(42504,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,42504);}
   gm.SetGameMaster(true);
   Actor source(&map,108218,17001,position),garona(&map,108214,17002,{-8643.02f,421.559f,102.051f,0});
   Require(map.AddToMap(static_cast<Creature*>(&source))&&map.AddToMap(static_cast<Creature*>(&garona)),"Native original actors unavailable");
   auto ai=new npc_sister_althea_ebonlocke::npc_sister_althea_ebonlockeAI(&source);Require(source.AIM_Initialize(ai),"Actual full-source Althea AI unavailable");
   if(gmFirst){ai->MoveInLineOfSight(&gm);Require(ai->introDone&&IntroCounter(scenario)==0&&scenario->GetCurrentStep()==4,"OPEN GM-first failure changed; replace counterfactual with repaired regression");}
   ai->MoveInLineOfSight(&eligible);
   Require(ai->introDone&&IntroCounter(scenario)==(gmFirst?0u:1u)&&scenario->GetCurrentStep()==(gmFirst?4u:5u),"Native eligible/control result changed");
   Require(!ai->introDone2,"50-yard first-intro probe consumed separate35-yard stage");
   std::cout<<(gmFirst?"COUNTERFACTUAL OPEN: GM consumes original one-shot introDone; later eligible Rogue cannot deliver51549; native stage4 remains blocked":"CONTROL: eligible Rogue first delivers native51549 once and advances4->5")<<std::endl;
   CleanStories(map);
  }
  first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 CloseAltheaNative();
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}}
