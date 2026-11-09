// Registered whole instance, native Player quest cancellation and Unit queue.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "WarswordsStageFourInstance.inc"
struct StageQueueTag{using Type=FunctionProcessor Unit::*;friend Type Access(StageQueueTag);};
template struct FixtureAccess<StageQueueTag,&Unit::m_Functions>;
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings missing");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellScene();
 auto spell=sSpellMgr->GetSpellInfo(189740);Require(spell,"Matching26972 scene spell missing");
 for(unsigned i=0;i<3;++i)std::cout<<"CATALOG189740 effect"<<i<<"="<<spell->Effects[i]->Effect<<" aura="<<spell->Effects[i]->ApplyAuraName<<" misc="<<spell->Effects[i]->MiscValue<<'\n';
 Require(spell->HasAura(SPELL_AURA_ACTIVATE_SCENE),"189740 is no longer a native scene aura");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_warswords;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)})for(unsigned mode=0;mode<6;++mode){
  auto parent=std::make_unique<MapInstanced>(1511,300);auto owner=std::make_unique<InstanceMap>(1511,300,9999700+race*10+mode,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*owner;map.CreateInstanceData(nullptr);
  auto script=dynamic_cast<instance_warswords::instance_warswords_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Registered original instance missing");auto scenario=sScenarioMgr->AddScenario(&map,944);scenario->SetCurrentStep(4);script->myscenario=true;auto session=Session();{
   NativePlayer p(session,map,995,{3380.29f,1401.9f,69.5355f,0});p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);p.SetQuestStatus(40043,mode==5?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40043);p.GetMapRef().link(&map,&p);
   auto queued=(p.*Access(StageQueueTag{})).SizeQueue();script->onScenarionNextStep(4);Require((p.*Access(StageQueueTag{})).SizeQueue()==queued+1,"Real Player-owned callback missing");
   if(mode==1)p.SetQuestStatus(40043,QUEST_STATUS_NONE);
   if(mode==2){p.SetQuestStatus(40043,QUEST_STATUS_NONE);p.SetQuestStatus(40043,QUEST_STATUS_INCOMPLETE);}
   if(mode==3)scenario->SetCurrentStep(5);
   if(mode==4){script->Initialize();script->myscenario=true;}
   (p.*Access(StageQueueTag{})).Update(2999);Require(!p.HasAura(189740),"Scene ran before original3s deadline");(p.*Access(StageQueueTag{})).Update(1);
   bool ordinary=mode==0||mode==5;
#ifdef WARSWORDS_STAGE_FOUR_BASELINE
   Require(p.HasAura(189740),"Pinned stale native scene did not reproduce");
#else
   Require(p.HasAura(189740)==ordinary,"Actual native scene escaped quest/stage/instance lifetime or ordinary scene failed");
#endif
   auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();(p.*Access(StageQueueTag{})).Update(3000);Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&!p.HasItemCount(128908,1,true),"Scene guard granted progression/item");
   std::cout<<"CASE native stage4 race="<<race<<" mode="<<mode<<" aura="<<p.HasAura(189740)<<" ordinary="<<ordinary<<'\n';p.GetMapRef().unlink();++cases;
  }sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
 CloseAltheaNative();
#ifdef WARSWORDS_STAGE_FOUR_BASELINE
 std::cout<<"COUNTERFACTUAL native stage4 twelve real registered instance/Player queue/scene aura cases: cancelled/retaken quest,advanced stage and reset still play189740;ordinary controls retained. Not successful gameplay\n";
#else
 std::cout<<"PASS native stage4 twelve Human/Orc registered whole instance/Player queue/real26972 scene aura cases: incomplete/complete ordinary3s scene,actual CancelQuestObjects/retake,stage advance/reset refuse stale work;no item/credit mutation. Earlier stage and poses controlled;client separate\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
