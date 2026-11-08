// Actual EventObject range/phase discovery -> registered native Smart callback
// -> source13 conditions -> native Scenario205 -> LINK61 aura28 cleanup.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "EventObject.h"
#include "EventObjectData.h"
#include "PlayerDefines.h"
#include <memory>
namespace HodirLandingFixture {
#include "HodirSmartEventNative.inc"
}
struct LandingEvent:EventObject{
 LandingEvent(InstanceMap& map,uint32 guid){Require(Create(guid,&map,1,157,220.3f,1529.03f,-30.4891f,2.80915f,40,0,0),"Native EO157 create failed");SetPhaseId({5896},false);map.LoadGrid(GetPositionX(),GetPositionY());map.AddToGrid(static_cast<EventObject*>(this),Cell(Trinity::ComputeCellCoord(GetPositionX(),GetPositionY())));AddToWorld();}
 ~LandingEvent(){RemoveFromWorld();RemoveFromGrid();ResetMap();}
 void Pulse(){m_isUpdate=false;Update(1);}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("smart_scripts","entryorguid=157 AND source_type IN(10,13)");Shadow("conditions","SourceTypeOrReferenceId=22 AND SourceEntry=157");Shadow("eventobject_template","entry=157");Shadow("eventobject","id=157");Shadow("scenario_data","ScenarioID=1146");
#ifndef HODIR_LANDING_LIVE
 WorldDatabase.DirectExecute("UPDATE conditions SET SourceId=10 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=157");WorldDatabase.DirectExecute("UPDATE smart_scripts SET event_type=89 WHERE entryorguid=157 AND source_type=13 AND id=1");
#ifdef HODIR_LANDING_BASELINE
 // Isolate wrong-source admission with a valid LINK. The full original
 //10/89 invalid-LINK graph is reproduced by extracted ASAN instead of being
 // misreported as an ordinary native run with unexplained DBErrors.
 WorldDatabase.DirectExecute("UPDATE smart_scripts SET event_type=61 WHERE entryorguid=157 AND source_type=13 AND id=1");
#else
 std::string sql=
#include "HodirLandingMigration.inc"
 ;std::string statement;bool quote=false;for(char c:sql){if(c=='\'')quote=!quote;if(c==';'&&!quote){WorldDatabase.DirectExecute(statement.c_str());statement.clear();}else statement+=c;}
 auto applied=WorldDatabase.Query("SELECT COALESCE(@hodir_landing_safe,0)");Require(applied&&applied->Fetch()[0].GetBool(),"Native migration refused matching original graph");
#endif
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sEventObjectDataStore->LoadEventObjectTemplates();sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();
 auto root=sCriteriaStore.LookupEntry(30856);auto steps=sScenarioMgr->GetScenarioSteps(1146);Require(root&&root->Type==92&&root->Asset==52537&&steps&&steps->size()==6&&(*steps)[2]->Criteriatreeid==49314,"Native Hodir landing graph changed");
 auto aura=sSpellMgr->GetSpellInfo(217815);Require(aura,"Native Empowered Leap missing");std::cout<<"META native217815 death-persistent="<<aura->HasAttribute(SPELL_ATTR3_DEATH_PERSISTENT)<<";producer110122 and flight/actual-death recovery remain separate"<<std::endl;
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});HashMapHolder<EventObject>::SetSize(1000);
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new HodirLandingFixture::SmartEventObject;sScriptMgr->SwapScriptContext(true);unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)})for(auto status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){
  MapInstanced parent(1489,300);InstanceMap map(1489,300,9997800+cases,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,1146);
  auto player=[&](uint32 id){auto p=std::make_unique<NativePlayer>(session,map,id,Position{220.3f,1529.03f,-30.4891f,0});p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);p->setFactionForRace(race);p->SetQuestStatus(42974,status);p->SetQuestSlot(0,42974);p->SetPhaseId({5896},false);p->CastSpell(p.get(),217815,true);Require(p->HasAura(217815),"Actual217815 override could not be established");return p;};
  for(unsigned stage=0;stage<6;++stage){scenario->SetCurrentStep(stage);auto p=player(931+cases);LandingEvent event(map,991000+cases);auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();event.Pulse();Require(event.ActivatedForPlayer(p.get()),"Actual event range/phase discovery did not find native Player");
#ifdef HODIR_LANDING_BASELINE
   Require(!p->HasAura(217815),"Wrong-source component did not remove aura prematurely");
#else
   Require(p->HasAura(217815)==(stage!=2),"Native source13 root did not protect stage admission/cleanup");
#endif
   Require(scenario->GetCurrentStep()==(stage==2?3:stage)&&p->GetQuestStatus(42974)==status&&!p->HasItemCount(128910,1,true),"Trigger changed unrelated stage/quest/item state");if(stage!=2)Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count,"Rejected trigger wrote native criteria");
   auto accepted=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();event.Pulse();Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==accepted,"Inside-zone repeat changed native criteria");++cases;
  }
#ifndef HODIR_LANDING_BASELINE
  scenario->SetCurrentStep(1);{auto p=player(931+cases);LandingEvent event(map,991000+cases);event.Pulse();Require(p->HasAura(217815),"Early encounter stripped leap");scenario->SetCurrentStep(2);event.Pulse();Require(p->HasAura(217815)&&scenario->GetCurrentStep()==2,"Same-entry suppression unexpectedly reran trigger");p->Relocate(220.3f,1479.03f,-30.4891f,0);event.Pulse();Require(!event.ActivatedForPlayer(p.get()),"Actual exit did not clear trigger admission");p->Relocate(220.3f,1529.03f,-30.4891f,0);event.Pulse();Require(!p->HasAura(217815)&&scenario->GetCurrentStep()==3,"Reentry at actual2 did not accept landing and clear aura");p->CastSpell(p.get(),217815,true);p->Relocate(220.3f,1479.03f,-30.4891f,0);event.Pulse();p->Relocate(220.3f,1529.03f,-30.4891f,0);event.Pulse();Require(p->HasAura(217815)&&scenario->GetCurrentStep()==3,"Repeated accepted landing stripped a new aura or skipped stage");++cases;}
#endif
  sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,972);scenario->SetCurrentStep(2);{auto p=player(931+cases);LandingEvent event(map,991000+cases);event.Pulse();
#ifdef HODIR_LANDING_BASELINE
   Require(!p->HasAura(217815),"Wrong-source shared-map strip not reproduced");
#else
   Require(p->HasAura(217815),"Shared972 trigger consumed Warrior leap");
#endif
   Require(scenario->GetCurrentStep()==2,"Shared scenario advanced");++cases;}
  sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1146);scenario->SetCurrentStep(2);{auto p=player(931+cases);LandingEvent event(map,991000+cases);p->SetHealth(0);p->setDeathState(CORPSE);auto before=p->HasAura(217815);Require(!sScriptMgr->OnEventObject(p.get(),&event,true)&&scenario->GetCurrentStep()==2&&p->HasAura(217815)==before,"Native dead-player callback gate failed");++cases;}
#ifndef HODIR_LANDING_BASELINE
  scenario->SetCurrentStep(2);{auto p=player(931+cases);p->RemoveAurasDueToSpell(217815);LandingEvent event(map,991000+cases);event.Pulse();Require(!p->HasAura(217815)&&scenario->GetCurrentStep()==3&&p->GetQuestStatus(42974)==status,"Native landing without leap aura failed or granted aura/quest");++cases;}
  scenario->SetCurrentStep(2);{auto p=player(931+cases);p->SetPhaseId({5955},false);LandingEvent event(map,991000+cases);event.Pulse();Require(!event.ActivatedForPlayer(p.get())&&p->HasAura(217815)&&scenario->GetCurrentStep()==2,"Foreign phase consumed landing");p->SetPhaseId({5896},false);event.Pulse();Require(!p->HasAura(217815)&&scenario->GetCurrentStep()==3,"Restored real phase did not accept landing");++cases;}
  sScenarioMgr->RemoveScenario(map.GetInstanceId());{auto p=player(931+cases);LandingEvent event(map,991000+cases);event.Pulse();Require(p->HasAura(217815)&&!sScenarioMgr->GetScenario(map.GetInstanceId()),"Missing scenario consumed aura or created progression");++cases;}
#endif
  sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
#ifdef HODIR_LANDING_BASELINE
 Require(cases==32,"Native component counterfactual count changed");std::cout<<"COUNTERFACTUAL Hodir native32:wrong SourceId10 with explicitly controlled valid LINK61 strips217815 at wrong stages/shared map;actual six-stage graph/current support,not the full historical10/89 graph. ASAN separately captures original invalid LINK diagnostics"<<std::endl;
#else
 Require(cases==48,"Native landing count changed");std::cout<<"PASS Hodir native48:actual registered SmartEventObject/range/phase discovery/source13 condition44/native52537 transition2->3/LINK61 aura217815 removal,all6 stages/both statuses/Human+Orc,actual exit/reentry+new-aura repeat protection,shared972/missing scenario/wrong phase/no-aura landing and native dead-player callback denial. Earlier accepted quest/aura/pose/stage controlled;actual felbat/producer/death-retry/client separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
