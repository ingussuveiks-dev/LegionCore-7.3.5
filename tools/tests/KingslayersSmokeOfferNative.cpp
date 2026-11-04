#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "SmallEvents.inc"
#include "AreaTriggerData.h"
#include "Smoke.inc"
struct SmokeOfferScenarioTag{using Type=uint32 Scenario::*;friend Type Access(SmokeOfferScenarioTag);};
template struct FixtureAccess<SmokeOfferScenarioTag,&Scenario::scenarioId>;
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);uint32 cases=0;
 sObjectAccessor->SetGuidSize(HighGuid::AreaTrigger,100000);sAreaTriggerDataStore->LoadAreaTriggerScripts();sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new spell_kingslayers_smoke_offer_probe;sScriptMgr->SwapScriptContext(true);
#ifdef KINGSLAYERS_SMOKE_OFFER_BASELINE
 uint32 modes=3;
#else
#ifdef KINGSLAYERS_SMOKE_OFFER_FAULTS

 uint32 modes=1;
#else
 uint32 modes=18;
#endif
#endif
 for(uint32 quest:{42504u,42627u})for(uint32 mode=0;mode<modes;++mode){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9945000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(mode==7?1:2);auto first=Session(),second=Session();
  {Position pose{-8822.22f,657.642f,98.1083f,.304745f};Position roguePose=pose;roguePose.m_positionX+=8;NativePlayer rogue(first,map,101,roguePose),visitor(second,map,102,pose);
   for(auto p:{&rogue,&visitor}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(p->getRace());p->SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,quest);}
   if(mode==0)visitor.SetGameMaster(true);if(mode==1)visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);if(mode==2){visitor.SetQuestStatus(quest,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);}
   if(mode==3)visitor.SetQuestStatus(quest,QUEST_STATUS_COMPLETE);if(mode==4)const_cast<RewardedQuestSet&>(visitor.getRewardedQuests()).insert(quest);if(mode==5)visitor.Unit::setDeathState(JUST_DIED);if(mode==6)visitor.SetPhaseMask(2,false);if(mode==8)visitor.Relocate(pose.GetPositionX()+30,pose.GetPositionY(),pose.GetPositionZ(),0);if(mode==9)rogue.SetQuestStatus(quest,QUEST_STATUS_NONE);if(mode==10)visitor.AddAura(227061,&visitor);if(mode==11)scenario->*Access(SmokeOfferScenarioTag{})=1;
   Actor craggle(&map,108177,29001,pose);craggle.*Access(AltheaGuardDbTag{})=mode==12?1:360801;Require(map.AddToMap(static_cast<Creature*>(&craggle)),"Native Craggle missing");auto ai=new npc_kingslayers_small_probe::npc_kingslayers_small_eventsAI(&craggle);Require(craggle.AIM_Initialize(ai),"Whole Craggle AI missing");
   auto counter=[&](){auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto at=rows->find(48533);return at==rows->end()?0:at->second.Counter;};Require(!counter(),"Fresh smoke event already present");
#ifdef KINGSLAYERS_SMOKE_OFFER_BASELINE
   rogue.Relocate(pose.GetPositionX()+200,pose.GetPositionY(),pose.GetPositionZ(),0);ai->UpdateAI(1000);Require(visitor.HasAura(227061)&&!rogue.HasAura(227061)&&ai->check,"Pinned nearest-visitor smoke offer differs");rogue.Relocate(roguePose);visitor.Relocate(pose.GetPositionX()+200,pose.GetPositionY(),pose.GetPositionZ(),0);ai->UpdateAI(5000);Require(!rogue.HasAura(227061),"Pinned smoke offer recovered after unrelated visitor");std::cout<<"COUNTERFACTUAL quest="<<quest<<" mode="<<mode<<" actual227061 goes to first GM/wrong-class/questless visitor and actor one-shot blocks real Rogue\n";
#else
#ifdef KINGSLAYERS_SMOKE_OFFER_FAULTS
   visitor.SetGameMaster(false);
   for(auto p:{&rogue,&visitor})p->ApplySpellImmune(0,IMMUNITY_ID,227061,true);
   ai->UpdateAI(1000);ai->UpdateAI(1000);Require(!rogue.HasAura(227061)&&!visitor.HasAura(227061)&&ai->smokeOfferWarning,"Native forced aura refusal not retained");
   visitor.SetGameMaster(false);
   for(auto p:{&rogue,&visitor})p->ApplySpellImmune(0,IMMUNITY_ID,227061,false);
   ai->UpdateAI(999);Require(!rogue.HasAura(227061),"Refused grant retried before1s");ai->UpdateAI(1);Require(rogue.HasAura(227061)&&visitor.HasAura(227061),"Restored native aura could not recover");
#else
   if(mode==12)rogue.Relocate(pose.GetPositionX()+200,pose.GetPositionY(),pose.GetPositionZ(),0);
   if(mode==13)craggle.Unit::setDeathState(JUST_DIED);
   if(mode==14)rogue.SetPhaseMask(2,false);
   if(mode==15){rogue.Relocate(pose.GetPositionX()+30,pose.GetPositionY(),pose.GetPositionZ(),0);visitor.Relocate(pose.GetPositionX()+30,pose.GetPositionY(),pose.GetPositionZ(),0);}
   if(mode==16)rogue.SetQuestStatus(quest,QUEST_STATUS_COMPLETE);
   ai->UpdateAI(999);Require(!rogue.HasAura(227061),"Smoke grant before original1s poll");ai->UpdateAI(1);
   bool r=mode!=7&&mode!=9&&mode!=11&&mode!=13&&mode!=14&&mode!=15;
   bool v=mode==3||mode==9||mode==10||mode==14||mode==16||mode==17;
   if(mode==12)r=false; // Original noncanonical source retains its nearest-player one-shot.
   if(mode==12)v=true;
   Require(rogue.HasAura(227061)==r&&visitor.HasAura(227061)==v,"Native smoke offer recipient admission differs");
   if(mode==3){auto aura=rogue.GetAura(227061);Require(aura,"Actual aura missing");ai->UpdateAI(1000);Require(rogue.GetAura(227061)==aura,"Existing actual aura replaced/refreshed by polling");rogue.RemoveAurasDueToSpell(227061);visitor.SetQuestStatus(quest,QUEST_STATUS_NONE);ai->UpdateAI(1000);Require(rogue.HasAura(227061),"Eligible removed aura not recovered");}
   if(mode==9){rogue.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);visitor.SetQuestStatus(quest,QUEST_STATUS_NONE);ai->UpdateAI(1000);Require(rogue.HasAura(227061),"Later legitimate replacement blocked by actor one-shot");}
   if(mode==15){rogue.Relocate(roguePose);ai->UpdateAI(1000);Require(rogue.HasAura(227061),"Eligible reentry not supplied");}
#endif
   Require(!counter(),"Smoke offer awarded native51437 progression");
#ifndef KINGSLAYERS_SMOKE_OFFER_FAULTS
   if(mode==17){rogue.Relocate(-8822.94f,657.59f,96.5f,0);rogue.CastSpell(&rogue,214645,true);Require(counter()==1&&scenario->GetCurrentStep()==3,"Actual supplied Rogue could not dispatch original smoke event");rogue.RemoveAurasDueToSpell(227061);ai->UpdateAI(1000);Require(!rogue.HasAura(227061)&&counter()==1,"Completed native smoke leaf reopened offer/progression");}
#endif
#endif
   for(auto p:{&rogue,&visitor}){p->CastStop();p->RemoveAllAuras();p->m_Events.KillAllEvents(true);}CleanStories(map);
  }for(auto s:{first,second}){s->SetPlayer(nullptr);delete s;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
 std::cout<<"PASS "<<cases<<" whole-source Craggle native aura-offer contexts; seeded earlier quests/stages/poses, no client UI claim\n";CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
