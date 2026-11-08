// Real native summon, vehicle teardown, passenger removal and delayed despawn.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "Vehicle.h"
#include <memory>
namespace SharasdalWaveFixture {
#include "SharasdalWave.inc"
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings absent");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();
 auto info=sObjectMgr->GetCreatureTemplate(105251);Require(info&&info->VehicleId==4717&&sObjectMgr->GetScriptName(info->ScriptID)=="npc_sharasdal_waterwall","Actual wave template changed");
 auto vehicle=sVehicleStore.LookupEntry(4717);Require(vehicle&&vehicle->SeatID[0]==16540,"Native wave seat absent");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new SharasdalWaveFixture::npc_sharasdal_waterwall;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0,missing=0;
 for(unsigned race:{unsigned(RACE_DRAENEI),unsigned(RACE_ORC)})for(unsigned mode=0;mode<6;++mode){
  auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,9999700+race*10+mode,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;auto scenario=sScenarioMgr->AddScenario(&map,1066);scenario->SetCurrentStep(3);auto session=Session();
  {
   Position pose{-148.25f,802.57f,796.49f,0};auto p=std::make_unique<NativePlayer>(session,map,990,pose);p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);p->setFactionForRace(race);p->SetPhaseId({6120},false);p->SetQuestStatus(40341,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,40341);
   auto raceInfo=sChrRacesStore.LookupEntry(race);Require(raceInfo&&sCreatureDisplayInfoStore.LookupEntry(raceInfo->MaleDisplayID),"Native male race display absent");p->SetNativeDisplayId(raceInfo->MaleDisplayID);p->SetDisplayId(raceInfo->MaleDisplayID);Require(p->GetCollisionHeight()>0,"Native passenger collision model absent");
   auto wave=map.SummonCreature(105251,pose,nullptr,0);Require(wave&&wave->IsInWorld()&&wave->GetVehicleKit(),"Native manual wave/vehicle missing");wave->SetPhaseId({6120},false);auto guid=wave->GetGUID();Require(dynamic_cast<SharasdalWaveFixture::npc_sharasdal_waterwall::npc_sharasdal_waterwallAI*>(wave->AI())&&wave->GetReactState()==REACT_PASSIVE,"Registered whole wave factory/passive state missing");
   if(mode==1||mode==3){p->EnterVehicle(wave,0);p->m_Events.Update(1);Require(p->GetVehicleBase()==wave&&wave->GetVehicleKit()->GetPassenger(0)==p.get(),"Actual native player boarding failed");}
   if(mode==2||mode==3){wave->RemoveVehicleKit();Require(!wave->GetVehicleKit()&&!p->GetVehicleBase(),"Native vehicle teardown incomplete");++missing;}
   auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto status=p->GetQuestStatus(40341);
   if(mode==4){wave->AI()->MovementInform(EFFECT_MOTION_TYPE,1);wave->m_Events.Update(200);map.RemoveAllObjectsInRemoveList();Require(map.GetCreature(guid)==wave&&wave->IsInWorld()&&!wave->IsDespawn(),"Foreign movement scheduled wave cleanup");}
   if(mode==5)wave->AI()->Reset();
   // Endpoint callback is controlled; native event queue/removal and vehicle
   // transitions are real. This does not certify physical wave travel/contact.
   wave->AI()->MovementInform(POINT_MOTION_TYPE,1);Require(!p->GetVehicleBase(),"Point cleanup retained passenger");
#ifdef SHARASDAL_WAVE_BASELINE
   bool retained=mode==2||mode==3;Require(wave->IsDespawn()!=retained,"Pinned kit-dependent scheduling changed");
#else
   Require(wave->IsDespawn(),"Completed wave was not scheduled for native cleanup");
#endif
   wave->m_Events.Update(99);map.RemoveAllObjectsInRemoveList();Require(map.GetCreature(guid)==wave&&wave->IsInWorld(),"Native100ms grace shortened");wave->m_Events.Update(1);map.RemoveAllObjectsInRemoveList();
#ifdef SHARASDAL_WAVE_BASELINE
   if(retained){Require(map.GetCreature(guid)==wave&&wave->IsInWorld(),"Pinned missing-kit retention not reproduced");wave->UnSummon();map.RemoveAllObjectsInRemoveList();}else Require(!map.GetCreature(guid),"Pinned ordinary cleanup failed");
#else
   Require(!map.GetCreature(guid),"Native delayed cleanup left finished wave");
#endif
   Require(!p->GetVehicleBase()&&scenario->GetCurrentStep()==3&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p->GetQuestStatus(40341)==status&&!p->HasItemCount(128911,1,true),"Cleanup changed player progress/item/vehicle");p->m_Events.Update(1);++cases;
  }
  sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
 Require(cases==12&&missing==4,"Native wave coverage changed");
#ifdef SHARASDAL_WAVE_BASELINE
 std::cout<<"COUNTERFACTUAL Sharasdal wave native12:original dbb95ce8d callback leaves4 actual vehicle-teardown waves alive;8 ordinary/reset/foreign-motion controls retain100ms cleanup. Current native support,not historical binary"<<std::endl;
#else
 std::cout<<"PASS Sharasdal wave native12:registered whole105251/4717/16540,Draenei+Orc,real manual summons/player boarding/native vehicle teardown,eject then99+1ms native removal,nonpoint refusal/reset controls,no quest/item/progression mutation. Endpoint/earlier quest-stage controlled;full movement/contact/camera/pet/client separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<std::endl;return 1;}}
