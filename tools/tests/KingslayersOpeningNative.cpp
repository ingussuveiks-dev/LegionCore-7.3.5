// Actual full opening actor, native Players/Scenario/Conversation. Earlier
// quest state, LOS delivery and clock endpoints are controlled.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "GaronaOpening.inc"
static uint32 OpeningStoryDuration(Map& map,uint32 entry){for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==entry)return story->GetDuration()>15000?uint32(story->GetDuration()):15000;return 15000;}
static uint64 OpeningCount(Scenario* scenario,uint32 leaf){auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto at=rows->find(leaf);return at==rows->end()?0:at->second.Counter;}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);uint32 cases=0;
#ifdef KINGSLAYERS_OPENING_BASELINE
 std::vector<uint32> modes{0,1,2};
#elif defined(KINGSLAYERS_OPENING_FAULTS)
 std::vector<uint32> modes{16};
#else
 std::vector<uint32> modes{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
#endif
 for(uint32 quest:{42504u,42627u})for(uint32 mode:modes){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9996000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(0);auto first=Session(),second=Session();
  {Position pose{-9101.9f,406.198f,92.7232f,.643928f};NativePlayer rogue(first,map,101,pose),visitor(second,map,102,pose);
   for(auto p:{&rogue,&visitor}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(p->getRace());p->SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,quest);}
   if(mode==0||mode>=3)visitor.SetGameMaster(true);if(mode==1)visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);if(mode==2){visitor.SetQuestStatus(quest,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);}
   Actor garona(&map,108015,29001,pose),auditor(&map,98094,29002,{-9078.58f,423.865f,92.6397f,3.83066f});Require(map.AddToMap(static_cast<Creature*>(&garona))&&map.AddToMap(static_cast<Creature*>(&auditor)),"Native original opening actors missing");auto ai=new npc_garona_opening_probe::npc_garona_halforcenAI(&garona);Require(garona.AIM_Initialize(ai),"Whole opening AI missing");
   ai->MoveInLineOfSight(&visitor);
#ifdef KINGSLAYERS_OPENING_BASELINE
   Require(ai->introDone&&ai->recipientGuid==visitor.GetGUID(),"Pinned first-visitor consumption changed");ai->MoveInLineOfSight(&rogue);Require(ai->recipientGuid==visitor.GetGUID(),"Pinned source rebound eligible player");ai->UpdateAI(30000);if(mode==0)Require(!OpeningCount(scenario,48526)&&!OpeningCount(scenario,49374)&&scenario->GetCurrentStep()==0,"Actual GM counterfactual no longer blocks native opening");
   std::cout<<"COUNTERFACTUAL quest="<<quest<<" mode="<<mode<<" unqualified visitor owns one-shot; eligible Rogue ignored, native leaves="<<OpeningCount(scenario,48526)<<','<<OpeningCount(scenario,49374)<<" stage="<<scenario->GetCurrentStep()<<'\n';
#else
   Require(!ai->introDone,"Ineligible visitor consumed opening");
   std::shared_ptr<QuestObjectScope> scope;std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditParty> party;
   if(mode==3)rogue.SetQuestStatus(quest,QUEST_STATUS_COMPLETE);
   if(mode==4){pet=std::make_unique<CreditPet>(rogue,map);ai->MoveInLineOfSight(pet.get());Require(!ai->introDone,"Pet-first LOS consumed opening");}
   if(mode==5){scenario->SetCurrentStep(1);ai->MoveInLineOfSight(&rogue);Require(!ai->introDone,"Later stage consumed new intro");scenario->SetCurrentStep(0);}
   if(mode==6){garona.SetPhaseMask(2,false);ai->MoveInLineOfSight(&rogue);Require(!ai->introDone,"Other phase consumed intro");garona.SetPhaseMask(1,false);}
   if(mode==7)map.RemoveFromMap(static_cast<Creature*>(&auditor),false);
   if(mode==10)rogue.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,51418);
   if(mode==13){rogue.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);ai->MoveInLineOfSight(&rogue);Require(!ai->introDone,"Wrong class consumed intro");rogue.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);}
   if(mode==14||mode==15){scope=std::make_shared<QuestObjectScope>();scope->Owner=visitor.GetGUID();garona.BindQuestObjectScope(scope);ai->MoveInLineOfSight(&rogue);Require(!ai->introDone,"Foreign scope consumed intro");scope->Owner=rogue.GetGUID();}
   ai->MoveInLineOfSight(&rogue);Require(ai->introDone&&ai->recipientGuid==rogue.GetGUID(),"Eligible Rogue not admitted");
   if(mode==8)rogue.Unit::RemoveFromWorld();
   if(mode==9){rogue.SetQuestStatus(quest,QUEST_STATUS_NONE);visitor.SetGameMaster(false);party=std::make_unique<CreditParty>(&rogue,&visitor);}
   if(mode==12)ai->Reset();
   if(mode==15)scope->Cancel();
   uint32 firstDelay=OpeningStoryDuration(map,2361);
   ai->UpdateAI(firstDelay-1);Require(!ai->openingMiddle&&!ai->openingComplete&&OpeningCount(scenario,48526)==(mode==10?1u:0u),"Opening crossed first accepted payload early");ai->UpdateAI(1);
   uint32 secondDelay=OpeningStoryDuration(map,2377);
   if(mode==11)rogue.SetQuestStatus(quest,QUEST_STATUS_NONE);
   if(mode==16)scenario->SetCurrentStep(1);
   ai->UpdateAI(secondDelay-1);Require(OpeningCount(scenario,48526)==(mode==10?1u:0u),"Opening shortened second accepted payload");ai->UpdateAI(1);
   if(mode==8||mode==9||mode==11||mode==15||mode==16){Require(!ai->openingComplete&&!OpeningCount(scenario,49374),"Unqualified/unacknowledged opening departed or progressed");
    if(mode==8)rogue.Unit::AddToWorld();if(mode==9)ai->MoveInLineOfSight(&visitor);if(mode==11)rogue.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);if(mode==16)scenario->SetCurrentStep(0);
    if(mode!=15){bool middle=ai->openingMiddle;ai->UpdateAI(1000);if(!middle){Require(ai->openingMiddle&&!ai->openingComplete,"Recovered middle phase did not retain its own speech window");ai->UpdateAI(OpeningStoryDuration(map,2377));}}
   }
   if(mode!=15){Require(ai->openingComplete&&OpeningCount(scenario,48526)==1&&OpeningCount(scenario,49374)==1&&scenario->GetCurrentStep()==1,"Actual51418/52672 did not advance opening");ai->MoveInLineOfSight(&rogue);ai->UpdateAI(60000);Require(OpeningCount(scenario,48526)==1&&OpeningCount(scenario,49374)==1,"Opening repeated native leaves");}
   Require(!rogue.HasItemCount(128870,1)&&!visitor.HasItemCount(128870,1),"Opening granted artifact");
#endif
   CleanStories(map);
  }
  for(auto session:{first,second}){session->SetPlayer(nullptr);delete session;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
 std::cout<<"PASS "<<cases<<" selected full-source opening native admission contexts; direct LOS/earlier state/clocks seeded, no client traversal claim\n";CloseAltheaNative();return 0;
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
