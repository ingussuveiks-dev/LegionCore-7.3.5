// Actual native caller cadence and moving-wave cleanup after a rejected FX cast.
// Earlier gauntlet admission is prepared in memory; no character save or grants.
#define main RetainedWaveMotionMain
#include "SharasdalWaveMotionNative.cpp"
#undef main
namespace SharasdalCastFixture {
using SharasdalMotionFixture::sumpos;
bool refuse=false;
uint32 attempts=0,rejections=0;
class zz_probe_sharasdal_wave_cast_refusal : public SpellScript {
    PrepareSpellScript(zz_probe_sharasdal_wave_cast_refusal);
    SpellCastResult Check(){if(GetCaster()->GetEntry()!=105251)return SPELL_CAST_OK;++attempts;if(refuse){++rejections;return SPELL_FAILED_NOT_READY;}return SPELL_CAST_OK;}
    void Register() override {OnCheckCast+=SpellCheckCastFn(zz_probe_sharasdal_wave_cast_refusal::Check);}
};
void RegisterFault(){RegisterSpellScript(zz_probe_sharasdal_wave_cast_refusal);}
struct Emitter : ScriptedAI {
    explicit Emitter(Creature* actor):ScriptedAI(actor){}
    bool intro=true;uint32 introtimer=0;
#include "WaveCallerContext.inc"
    void Emit(uint32 diff){Scenario* gauntlet=GauntletWorld();
#include "WaveCallerSpawn.inc"
    }
};
}
struct WaveCastBindingsTag{using Type=SpellScriptsContainer ScriptDataStoreMgr::*;friend Type Access(WaveCastBindingsTag);};
template struct FixtureAccess<WaveCastBindingsTag,&ScriptDataStoreMgr::_spellScriptsStore>;
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings absent");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");
 for(auto name:{"areatrigger_template","areatrigger_data","areatrigger_actions"})Shadow(name,"entry=6547 OR customEntry=11160");
 for(auto name:{"areatrigger_template_circle","areatrigger_template_sequence","areatrigger_polygon","areatrigger_scripts"})Shadow(name,"entry IN(6547,11160)");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();sObjectAccessor->SetGuidSize(HighGuid::AreaTrigger,100000);sAreaTriggerDataStore->LoadAreaTriggerScripts();sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
 auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());std::string probe="zz_probe_sharasdal_wave_cast_refusal";
 Require(names.back()<probe,"Probe name must append without shifting existing native script IDs");names.push_back(probe);
 auto id=sObjectMgr->GetScriptId(probe.c_str());Require(id,"Native fault script ID absent");
 (sScriptDataStore->*Access(WaveCastBindingsTag{})).emplace(209132,std::make_pair(id,true));
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new SharasdalMotionFixture::npc_sharasdal_waterwall;SharasdalCastFixture::RegisterFault();sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_DRAENEI),uint32(RACE_ORC)})for(uint32 mode=0;mode<3;++mode){
  MapInstanced parent(1600,300);InstanceMap map(1600,300,9999100+cases,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,1066);scenario->SetCurrentStep(3);auto session=Session();
  {
   Position pose{-97,802.57f,796.8f,0};NativePlayer player(session,map,997,pose);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);player.setFactionForRace(race);player.SetPhaseId({6120},false);player.SetQuestStatus(40341,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40341);
   auto model=sChrRacesStore.LookupEntry(race);Require(model,"Native race missing");player.SetNativeDisplayId(model->MaleDisplayID);player.SetDisplayId(model->MaleDisplayID);
   Actor lady(&map,104856,7991,{-44.86f,802.57f,797.15f,0});lady.*Access(AltheaGuardDbTag{})=362091;lady.SetPhaseId({6120},false);Require(map.AddToMap(static_cast<Creature*>(&lady)),"Native illusion fixture absent");SharasdalCastFixture::Emitter emitter(&lady);
   auto waves=[&]{std::list<Creature*> actors;lady.GetCreatureListWithEntryInGrid(actors,105251,200);return actors;};
   auto criteria=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();uint32 rejectedBefore=SharasdalCastFixture::rejections;
   for(uint32 number=0;number<3;++number){
    SharasdalCastFixture::refuse=mode==2||(mode==1&&number==0);
    auto before=SharasdalCastFixture::attempts;emitter.Emit(number?1:1000);auto actors=waves();Require(actors.size()==1&&SharasdalCastFixture::attempts==before+1,"Actual native caller did not attempt exactly one FX cast");auto wave=actors.front();auto guid=wave->GetGUID();
    map.PlayerRelocation(&player,(wave->GetPositionX()+SharasdalMotionFixture::waveEnd.GetPositionX())/2,(wave->GetPositionY()+SharasdalMotionFixture::waveEnd.GetPositionY())/2,(wave->GetPositionZ()+SharasdalMotionFixture::waveEnd.GetPositionZ())/2,0);
    bool refused=SharasdalCastFixture::refuse;auto area=wave->GetAreaObject(209132);Require(refused?!area:bool(area),"Rejected native CheckCast created FX or successful cast omitted FX");Require(!wave->movespline->Finalized()&&emitter.introtimer==6000,"Cast rejection changed original route or six-second cadence");
    bool boarded=false;uint32 ticks=0;
    while(!wave->IsDespawn()&&ticks++<2400){TickMotion(wave,50);map.MoveAllCreaturesInMoveList();wave->GetMotionMaster()->UpdateMotion(50);map.MoveAllCreaturesInMoveList();if(area){area->Update(50);map.MoveAllAreaTriggersInMoveList();}player.m_Events.Update(50);TickMotion(&player,50);if(player.GetVehicleBase()==wave)boarded=true;}
    Require(wave->IsDespawn()&&wave->GetExactDist(&SharasdalMotionFixture::waveEnd)<.1f,"Rejected FX stranded actual moving wave");Require(refused?!boarded:boarded,"Rejected FX boarded player or recovered next wave missed native contact");Require(!player.GetVehicleBase(),"Endpoint retained passenger");
    wave->m_Events.Update(99);map.RemoveAllObjectsInRemoveList();Require(map.GetCreature(guid)==wave,"Native grace shortened");wave->m_Events.Update(1);map.RemoveAllObjectsInRemoveList();Require(!map.GetCreature(guid)&&waves().empty(),"Completed wave leaked after failed FX");
    // Explicit caller-clock boundary: the route clock above does not replace
    // the original gauntlet AI clock. A subsequent ordinary six-second emission
    // is the existing recovery, not a fabricated retry cast on the old wave.
    emitter.Emit(5999);Require(waves().empty()&&emitter.introtimer==1,"Native caller retried before authored cadence");
    if(player.GetVehicleBase()||scenario->GetCurrentStep()!=3||scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()!=criteria||player.GetQuestStatus(40341)!=QUEST_STATUS_INCOMPLETE||player.HasItemCount(128911,1,true))std::cerr<<"CONTEXT race="<<race<<" mode="<<mode<<" emission="<<number<<" vehicle="<<bool(player.GetVehicleBase())<<" step="<<scenario->GetCurrentStep()<<" criteria="<<scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()<<" original="<<criteria<<" quest="<<player.GetQuestStatus(40341)<<" item="<<player.HasItemCount(128911,1,true)<<std::endl;
    Require(!player.GetVehicleBase()&&scenario->GetCurrentStep()==3&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==criteria&&player.GetQuestStatus(40341)==QUEST_STATUS_INCOMPLETE&&!player.HasItemCount(128911,1,true),"Fault path changed player progress/item/vehicle");
    // Restore the stationary witness for the next ordinary native wave.
    map.PlayerRelocation(&player,pose.GetPositionX(),pose.GetPositionY(),pose.GetPositionZ(),0);
   }
   Require(SharasdalCastFixture::rejections-rejectedBefore==(mode==0?0u:mode==1?1u:3u),"Fault dispatch count changed");
   scenario->SetCurrentStep(4);emitter.Emit(1000);Require(waves().empty(),"Gauntlet stage exit emitted another wave");
   std::cout<<"CASE race="<<race<<" mode="<<mode<<" original caller3 emissions/native CheckCast refusal,route cleanup,next6000ms emission/recovery,stage exit,no progression mutation"<<std::endl;++cases;
  }sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
 std::cout<<"PASS6 native gauntlet caller sequences18 wave lifecycles:ordinary,first FX refusal then next-wave recovery,repeated FX refusal;actual MovePoint/spatial contact/VehicleJoinEvent/endpoint ejection100ms removal/native6000ms cadence/no leaks or progress grants. Fault CheckCast and earlier admission/clocks controlled;client/full1066/living Lady separate"<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
