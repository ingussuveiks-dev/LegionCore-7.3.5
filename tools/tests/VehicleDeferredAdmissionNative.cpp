#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "Vehicle.h"
#include <memory>
#ifdef VEHICLE_DEFERRED_BASELINE
#include "VehicleDeferredPinned.inc"
#endif
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings missing");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 unsigned cases=0;
 for(unsigned mode=0;mode<6;++mode){
 auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,9999851+mode,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;
 auto session=Session();{
 NativePlayer carrier(session,map,995,Position{50,803,805,0});carrier.SetByteValue(UNIT_FIELD_BYTES_0,0,RACE_ORC);carrier.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);carrier.setFactionForRace(RACE_ORC);
 auto display=sChrRacesStore.LookupEntry(RACE_ORC);carrier.SetNativeDisplayId(display->MaleDisplayID);carrier.SetDisplayId(display->MaleDisplayID);
 Require(carrier.CreateVehicleKit(4050,0,179244),"Real chauffeur vehicle missing");auto vehicle=carrier.GetVehicleKit();auto before=vehicle->GetAvailableSeatCount();
 auto passenger=map.SummonCreature(89713,Position{51,803,805,0},nullptr,0);Require(passenger,"Native chauffeur missing");
 if(mode==2)passenger->setDeathState(JUST_DIED);
 passenger->EnterVehicle(&carrier);Require(!passenger->GetVehicleBase(),"Join unexpectedly synchronous");
 if(mode==0||mode==3)passenger->setDeathState(JUST_DIED);
 if(mode==4)carrier.RemoveVehicleKit();
 if(mode==5)passenger->RemoveFromWorld();
 passenger->m_Events.Update(1);bool boarded=passenger->GetVehicleBase()!=nullptr;
#ifdef VEHICLE_DEFERRED_BASELINE
 if(mode==0||mode==3){Require(boarded,"Pinned dead-driver admission not reproduced");std::cout<<"COUNTERFACTUAL queued native chauffeur mode="<<mode<<" boards after actual setDeathState(JUST_DIED)\n";}
#else
 Require(boarded==(mode==1),"Dead/not-world/missing-kit join was accepted or living join rejected");
 if(mode!=4)Require(vehicle->GetAvailableSeatCount()==before-(boarded?1:0),"Failed join consumed real seat capacity");
 if(mode==3){passenger->setDeathState(ALIVE);passenger->SetHealth(passenger->GetMaxHealth());passenger->EnterVehicle(&carrier);passenger->m_Events.Update(1);Require(passenger->GetVehicleBase()==&carrier,"Revived passenger cannot retry native join");}
 std::cout<<"CASE native deferred admission mode="<<mode<<" pass\n";
#endif
 if(passenger->GetVehicleBase())passenger->ExitVehicle();passenger->m_Events.Update(1);if(mode==5)passenger->AddToWorld();passenger->UnSummon();if(carrier.GetVehicleKit())carrier.RemoveVehicleKit();map.RemoveAllObjectsInRemoveList();++cases;
 }session->SetPlayer(nullptr);delete session;
 }
 Require(cases==6,"Expected six native admission contexts");CloseAltheaNative();
#ifdef VEHICLE_DEFERRED_BASELINE
 std::cout<<"COUNTERFACTUAL original native dead chauffeur boarding reproduced; not passing repaired behavior\n";
#else
 std::cout<<"PASS deferred native admission6:dead during delay/dead before request/living/revive-retry/missing kit/removed-world;seat capacity preserved\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
