#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "SpellScript.h"
#include "AreaTriggerData.h"
#include "Smoke.inc"
struct SmokeScriptsTag{using Type=std::vector<SpellScript*> Spell::*;friend Type Access(SmokeScriptsTag);};
template struct FixtureAccess<SmokeScriptsTag,&Spell::m_loadedScripts>;
struct SmokeLoadTag{using Type=void(Spell::*)();friend Type Access(SmokeLoadTag);};
template struct FixtureAccess<SmokeLoadTag,&Spell::LoadScripts>;
struct SmokeCheckTag{using Type=SpellCastResult(Spell::*)();friend Type Access(SmokeCheckTag);};
template struct FixtureAccess<SmokeCheckTag,&Spell::CallScriptCheckCastHandlers>;
struct SmokeAfterTag{using Type=void(Spell::*)();friend Type Access(SmokeAfterTag);};
template struct FixtureAccess<SmokeAfterTag,&Spell::CallScriptAfterCastHandlers>;
static uint64 SmokeCount(Scenario* scenario){auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto at=rows->find(48533);return at==rows->end()?0:at->second.Counter;}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 sObjectAccessor->SetGuidSize(HighGuid::AreaTrigger,100000);sAreaTriggerDataStore->LoadAreaTriggerScripts();sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new spell_kingslayers_smoke_probe;sScriptMgr->SwapScriptContext(true);
 Require(sScriptMgr->GetSpellScriptLoader(sObjectMgr->GetScriptId("spell_kingslayers_smoke_bomb")),"Actual smoke hook not registered");
 auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id=30447");Require(rules&&!rules->Fetch()[0].GetUInt32(),"Smoke native criteria needs additional data loader");
 uint32 cases=0;

#ifdef KINGSLAYERS_SMOKE_BASELINE
 uint32 modes=3;
#else
 uint32 modes=17;
#endif
 for(uint32 quest:{42504u,42627u})for(uint32 mode=0;mode<modes;++mode){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9956000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(mode==8?1:2);auto session=Session();
  {Position pose=mode<3?Position{-9101.9f,406.198f,92.7232f,.643928f}:Position{-8822.94f,657.59f,96.5f,0};NativePlayer rogue(session,map,101,pose);rogue.SetByteValue(UNIT_FIELD_BYTES_0,0,quest==42504?RACE_HUMAN:RACE_ORC);rogue.SetByteValue(UNIT_FIELD_BYTES_0,1,(mode==1||mode==4)?CLASS_WARRIOR:CLASS_ROGUE);rogue.setFactionForRace(rogue.getRace());if(mode!=2&&mode!=5){rogue.SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);rogue.SetQuestSlot(0,quest);}
   if(mode==6)rogue.SetGameMaster(true);if(mode==7)rogue.SetQuestStatus(quest,QUEST_STATUS_COMPLETE);if(mode==9)rogue.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,51437);if(mode==10)rogue.Unit::setDeathState(JUST_DIED);
   Require((mode==9?SmokeCount(scenario)==1:!SmokeCount(scenario)),"Fresh native smoke leaf already complete");
   std::cout<<"CONTEXT quest="<<quest<<" mode="<<mode<<" area="<<rogue.GetAreaId()<<" distance="<<rogue.GetExactDist2d(-8822.94f,657.59f)<<'\n';
   std::vector<std::unique_ptr<Actor>> actors;
   if(mode==3||mode==16){uint32 id=25001;for(uint32 entry:{108057u,108159u,108155u,108178u,108158u,108177u,108161u,108179u,108157u,108160u,108107u}){auto actor=std::make_unique<Actor>(&map,entry,id++,Position{pose.GetPositionX()+10,pose.GetPositionY(),pose.GetPositionZ(),0});Require(map.AddToMap(static_cast<Creature*>(actor.get())),"Native panic actor missing");actors.push_back(std::move(actor));}}
#ifndef KINGSLAYERS_SMOKE_BASELINE
   if(mode>=13){
    TriggerCastData cast;cast.triggerFlags=TRIGGERED_FULL_MASK;Spell spell(&rogue,sSpellMgr->GetSpellInfo(214645),cast);(spell.*Access(SmokeLoadTag{}))();auto& scripts=spell.*Access(SmokeScriptsTag{});Require(scripts.size()==1,"Actual smoke binding is not unique");auto hook=dynamic_cast<spell_kingslayers_smoke_probe::spell_kingslayers_smoke_bomb_SpellScript*>(scripts[0]);Require(hook,"Actual production smoke script not selected");
    Require((spell.*Access(SmokeCheckTag{}))()==SPELL_CAST_OK,"Initial native effect endpoint not eligible");if(mode==13)rogue.Relocate(pose.GetPositionX()+20,pose.GetPositionY(),pose.GetPositionZ(),0);if(mode==14)rogue.SetQuestStatus(quest,QUEST_STATUS_NONE);if(mode==15)rogue.SetGameMaster(true);
    spell.HandleEffects(nullptr,nullptr,nullptr,EFFECT_0,SPELL_EFFECT_HANDLE_HIT);
    if(mode!=16)Require(hook->_IsDefaultEffectPrevented(EFFECT_0)&&!SmokeCount(scenario),"Late eligibility change bypassed native effect guard");
    else{Require(!hook->_IsDefaultEffectPrevented(EFFECT_0)&&SmokeCount(scenario)==1,"Eligible native effect incorrectly prevented");spell.HandleEffects(nullptr,nullptr,nullptr,EFFECT_0,SPELL_EFFECT_HANDLE_HIT);Require(hook->_IsDefaultEffectPrevented(EFFECT_0),"Repeated real native effect not prevented");}
    (spell.*Access(SmokeAfterTag{}))();(spell.*Access(SmokeAfterTag{}))();
   }else
#endif
   if(mode==11){CreditPet pet(rogue,map);pet.CastSpell(&pet,214645,true);}else if(mode==12){CreditGuardian guardian(rogue,map);guardian.CastSpell(&guardian,214645,true);}else rogue.CastSpell(&rogue,214645,true);
#ifdef KINGSLAYERS_SMOKE_BASELINE
   Require(SmokeCount(scenario)==1&&scenario->GetCurrentStep()==3,"Pinned native smoke no longer advances outside script admission");std::cout<<"COUNTERFACTUAL actual registered214645 native61/51437 advances2->3 before AfterCast refuses wrong area/location/class/quest\n";
#else
   if(mode==3||mode==7||mode==16)Require(SmokeCount(scenario)==1&&scenario->GetCurrentStep()==3,"Valid native smoke did not admit original51437 event");
   else if(mode==9)Require(SmokeCount(scenario)==1&&scenario->GetCurrentStep()==3,"Repeated native smoke replayed real event");
   else Require(!SmokeCount(scenario)&&scenario->GetCurrentStep()==(mode==8?1:2),"Invalid smoke cast advanced actual native scenario");
#endif
   if(mode==3||mode==16){Require(Stories(map,2409)==1&&Stories(map,2412)==1,"Original smoke conversations not admitted with real actors");for(size_t i=1;i<actors.size();++i)Require(actors[i]->HasAura(214304),"Original panic aura missing on native actor");}
   CleanStories(map);actors.clear();rogue.CastStop();rogue.m_Events.KillAllEvents(true);
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
 std::cout<<"PASS "<<cases<<" actual registered native smoke cast/effect endpoints; earlier quest/stage/pose seeded, no client traversal claim\n";CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
