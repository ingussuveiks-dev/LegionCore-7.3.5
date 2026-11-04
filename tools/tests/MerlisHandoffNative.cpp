// Whole selected Merlis AI, real Unit::Kill/owner credit and native summon path.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "MapManager.h"
#include "MerlisHandoff.inc"
void AddSC_Kingslayers();
struct HandoffTemplatesTag{using Type=CreatureTemplateContainer ObjectMgr::*;friend Type Access(HandoffTemplatesTag);};
template struct FixtureAccess<HandoffTemplatesTag,&ObjectMgr::_creatureTemplateStore>;
// Controlled native factory observer: preserve the complete production
// JustSummoned policy, then decline only this summon AI's enabled state.
struct HandoffObservedAI:npc_merlis_handoff_probe::npc_merlis_malaganAI{
 unsigned mode;
 HandoffObservedAI(Creature* actor,unsigned value):npc_merlis_malaganAI(actor),mode(value){}
 void JustSummoned(Creature* actor)override{
  npc_merlis_malaganAI::JustSummoned(actor);
  if((mode==101||mode==102)&&actor->GetEntry()==108222)actor->IsAIEnabled=false;
 }
};
static std::list<Creature*> OutParents(Actor& boss){std::list<Creature*> values;GetCreatureListWithEntryInGrid(values,&boss,108222,100.f);values.remove_if([&](Creature* c){return !c->IsAlive()||!c->ToTempSummon()||c->ToTempSummon()->GetSummonerGUID()!=boss.GetGUID();});return values;}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings missing");sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellProcs();sSpellMgr->LoadSpellScene();sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_Kingslayers();sScriptMgr->SwapScriptContext(true);
 unsigned cases=0;
#ifdef MERLIS_HANDOFF_BASELINE
 std::vector<unsigned> modes{0,100};
#elif defined(MERLIS_HANDOFF_FAULTS)
 std::vector<unsigned> modes{100,101,102};
#else
 std::vector<unsigned> modes{0};
#endif
 for(uint32 quest:{42504u,42627u})for(unsigned killerKind=0;killerKind<3;++killerKind)for(unsigned mode:modes){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9999000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(7);auto session=Session();
  {NativePlayer p(session,map,101,{-8315.93f,287.852f,156.916f,0});p.SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p.setFactionForRace(p.getRace());p.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,quest);
   Actor boss(&map,107831,28001,{-8321.93f,287.852f,156.916f,.7101f}),old(&map,108222,28002,{-8312.8f,284.535f,160.054f,3.24861f});Require(map.AddToMap(static_cast<Creature*>(&boss))&&map.AddToMap(static_cast<Creature*>(&old)),"Native original actors admission failed");
   auto ai=new HandoffObservedAI(&boss,mode);Require(boss.AIM_Initialize(ai),"Whole Merlis AI missing");
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;if(killerKind==1)pet=std::make_unique<CreditPet>(p,map);if(killerKind==2)guardian=std::make_unique<CreditGuardian>(p,map);Unit* killer=pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&p);
   auto& templates=sObjectMgr->*Access(HandoffTemplatesTag{});auto original=templates.at(108222);if(mode==100)templates[108222]=nullptr;
   boss.SetLootRecipient(&p);boss.LowerPlayerDamageReq(uint32(boss.GetMaxHealth()));if(!killerKind)killer->DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);else killer->Kill(&boss,false);templates[108222]=original;
   Require(!boss.IsAlive()&&scenario->GetCurrentStep()==8&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(48589).Counter==1,"Real owner killing blow lost native51615");
#ifdef MERLIS_HANDOFF_BASELINE
   for(unsigned i=0;i<45;++i)ai->UpdateLifecycle(1000);
   Require(OutParents(boss).size()==(mode==100?0u:1u),"Pinned source handoff changed");Require(!old.IsAlive(),"Original source no longer consumes old actor before accepted replacement");
   std::cout<<(mode==100?"COUNTERFACTUAL ":"CONTROL ")<<"quest="<<quest<<" killer="<<killerKind<<" original real death/51615, native108222="<<OutParents(boss).size()<<" after restored template45s; no repaired/client claim\n";
#else
   if(mode==102){
    Require(OutParents(boss).size()==1&&old.IsAlive(),"Pending disabled-AI actor missing");auto guid=OutParents(boss).front()->GetGUID();
    // Seed a source revival after the completed real Unit::Kill. Drive the
    // actual lifecycle policy; map object removal remains entirely native.
    boss.Unit::setDeathState(ALIVE);ai->UpdateLifecycle(1);map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(guid)&&old.IsAlive()&&!ai->outroPending&&!ai->outroComplete,"Canceled unaccepted native summon stranded/removed original actor");boss.Unit::setDeathState(CORPSE);
   }else{
   if(mode==100){Require(OutParents(boss).empty()&&old.IsAlive(),"Refusal consumed old actor or accepted missing replacement");boss.Update(1999);Require(boss.IsInWorld()&&OutParents(boss).empty(),"Native corpse tick shortened2s or removed source");boss.Update(1);}
   if(mode==101){Require(OutParents(boss).size()==1&&old.IsAlive(),"Declined AI consumed original actor or did not retain same summon");auto candidate=OutParents(boss).front();auto guid=candidate->GetGUID();candidate->IsAIEnabled=true;boss.Update(1999);Require(old.IsAlive()&&OutParents(boss).size()==1,"Same actor retry shortened");boss.Update(1);Require(OutParents(boss).size()==1&&OutParents(boss).front()->GetGUID()==guid,"AI recovery cloned its accepted-world actor");}
   Require(OutParents(boss).size()==1&&!old.IsAlive(),"Real source retry/old actor handoff did not commit");
   ai->Reset();killer->Kill(&boss,false);for(unsigned i=0;i<45;++i)boss.Update(1000);Require(OutParents(boss).size()==1,"Reset/repeated kill lost or duplicated accepted Garona");
   auto garona=OutParents(boss).front();garona->AI()->UpdateAI(23000);garona->AI()->UpdateAI(11000);std::list<GameObject*> weapons;garona->GetGameObjectListWithEntryInGrid(weapons,251107,100.f);Require(weapons.size()==1,"Actual accepted Garona did not reach native weapon reveal");for(auto object:weapons)object->AddObjectToRemoveList();std::list<Creature*> visuals;GetCreatureListWithEntryInGrid(visuals,garona,108325,100.f);for(auto visual:visuals)visual->DespawnOrUnsummon();
   Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(48589).Counter==1&&!scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(48591)&&!p.HasItemCount(128870),"Source retry fabricated/repeated progress/pickup/item");
   }
   Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(48589).Counter==1&&scenario->GetCurrentStep()==8&&!scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(48591)&&!p.HasItemCount(128870)&&p.GetQuestStatus(quest)==QUEST_STATUS_INCOMPLETE,"Handoff/cancellation changed earned victory or manufactured pickup/item/completion");
   std::cout<<"CASE quest="<<quest<<" killer="<<killerKind<<" mode="<<mode<<" real Unit.Kill/51615/handoff or explicit revived-source cancellation; no direct grants\n";
#endif
   for(auto actor:OutParents(boss))actor->DespawnOrUnsummon();CleanStories(map);map.RemoveAllObjectsInRemoveList();
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef MERLIS_HANDOFF_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" whole-source native controls/failures; intentional6 missing108222 template errors; not repaired ordinary success\n";
#else
 std::cout<<"PASS "<<cases<<" native complete-source death/handoff cases; source actors/earlier stage seeded, no full client/combat claim\n";
#endif
 CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
