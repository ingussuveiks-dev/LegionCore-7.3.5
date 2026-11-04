// Complete selected reveal AI; actual native factories, conversations/map objects.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "GaronaReveal.inc"
void AddSC_Kingslayers();
static std::list<GameObject*> Weapons(Actor& actor){std::list<GameObject*> rows;actor.GetGameObjectListWithEntryInGrid(rows,251107,100.f);return rows;}
static std::list<Creature*> Visuals(Actor& actor){std::list<Creature*> rows;GetCreatureListWithEntryInGrid(rows,&actor,108325,100.f);return rows;}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();Require(sObjectMgr->LoadTrinityStrings(),"Native strings missing");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_Kingslayers();sScriptMgr->SwapScriptContext(true);
 unsigned cases=0;
#if defined(GARONA_REVEAL_BASELINE)
 std::vector<unsigned> modes{0,100};
#elif defined(GARONA_REVEAL_FAULTS)
 std::vector<unsigned> modes{100};
#else
 std::vector<unsigned> modes{0,1,2,3,4,5,6,7};
#endif
 for(uint32 quest:{42504u,42627u})for(unsigned mode:modes){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9998000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(8);auto session=Session();
  {NativePlayer p(session,map,101,{-8310.60f,284.40f,156.83f,0});p.SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p.setFactionForRace(p.getRace());p.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,quest);
   Actor actor(&map,108222,27001,{-8310.60f,284.40f,156.83f,0});Require(map.AddToMap(static_cast<Creature*>(&actor)),"Native original108222 admission failed");auto ai=new npc_garona_before_reveal::npc_garona_halforcen_outroAI(&actor);Require(actor.AIM_Initialize(ai),"Selected complete reveal AI failed");ai->DoAction(1);
   if(mode==1)ai->DoAction(1);if(mode==2)ai->Reset();
   Require(Stories(map,2449)==1&&Weapons(actor).empty(),"First voice/reveal delay differs");ai->UpdateAI(22999);Require(!Stories(map,2450)&&Weapons(actor).empty(),"23s shortened");ai->UpdateAI(1);Require(Stories(map,2450)==1,"Actual2450 not admitted");
   if(mode==3)ai->Reset();if(mode==4)ai->DoAction(1);ai->UpdateAI(10999);Require(Weapons(actor).empty(),"11s shortened");
   if(mode==5)actor.AddUnitState(UNIT_STATE_CASTING);if(mode==6)actor.AddUnitState(UNIT_STATE_STUNNED);
   auto templates=const_cast<GameObjectTemplateContainer*>(sObjectMgr->GetGameObjectTemplates());auto saved=templates->extract(251107);Require(!saved.empty(),"Actual artifact template missing before controlled failure");
   if(mode!=100)templates->insert(std::move(saved));ai->UpdateAI(1);if(mode==100)templates->insert(std::move(saved));
   if(mode==5||mode==6){Require(Weapons(actor).empty(),"Casting/stun deferral lost");actor.ClearUnitState(UNIT_STATE_CASTING|UNIT_STATE_STUNNED);ai->UpdateAI(1);}
#ifdef GARONA_REVEAL_BASELINE
   for(unsigned i=0;i<45;++i)ai->UpdateAI(1000);
   if(mode==100){Require(Weapons(actor).empty(),"Pinned reveal no longer loses refusal");std::cout<<"COUNTERFACTUAL quest="<<quest<<" actual251107 creation refused; restored template45s later still no weapon\n";}
   else{Require(Weapons(actor).size()==1,"Native baseline did not reveal");ai->DoAction(1);ai->UpdateAI(23000);ai->UpdateAI(11000);Require(Weapons(actor).size()==2,"Pinned duplicate no longer reproduced");std::cout<<"CONTROL quest="<<quest<<" real34s reveal and COUNTERFACTUAL second action creates second real object\n";}
#else
   if(mode==100){Require(Weapons(actor).empty()&&Visuals(actor).size()==1,"Refused artifact duplicated/lost independent visual");ai->Reset();ai->DoAction(1);ai->UpdateAI(1999);Require(Weapons(actor).empty(),"2s retry shortened");ai->UpdateAI(1);}
   Require(Weapons(actor).size()==1&&Visuals(actor).size()==1,"Accepted reveal not exactly once");
   if(mode==7){for(auto object:Weapons(actor))object->AddObjectToRemoveList();map.RemoveAllObjectsInRemoveList();}
   ai->DoAction(1);ai->Reset();for(unsigned i=0;i<60;++i)ai->UpdateAI(1000);
   Require(Weapons(actor).size()==(mode==7?0u:1u)&&Visuals(actor).size()==1&&Stories(map,2449)==1&&Stories(map,2450)==1,"Replay/reset duplicated accepted or consumed reveal");
   Require(p.GetQuestStatus(quest)==QUEST_STATUS_INCOMPLETE&&!p.HasItemCount(128870)&&scenario->GetCurrentStep()==8,"Reveal granted progress/item/quest completion");
   std::cout<<"CASE quest="<<quest<<" mode="<<mode<<" real factories/voices/delays/repeated-action/Reset/consumed-object verified\n";
#endif
   for(auto object:Weapons(actor))object->AddObjectToRemoveList();for(auto visual:Visuals(actor))visual->DespawnOrUnsummon();CleanStories(map);map.RemoveAllObjectsInRemoveList();
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef GARONA_REVEAL_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" expected-broken original reveal cases;2 intentional native missing-template errors; not a repaired/client/full encounter pass\n";
#else
 std::cout<<"PASS "<<cases<<" native complete-source reveal cases; actual34s factories/voices, exactly-once/retry/Reset/consumed object. Earlier victory/stage seeded; no client/full encounter claim\n";
#endif
 CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
