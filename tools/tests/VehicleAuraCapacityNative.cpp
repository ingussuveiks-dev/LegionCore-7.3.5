// Real ride auras/queued native seats;no persistent character writes.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "Vehicle.h"
#include <memory>
#ifdef VEHICLE_AURA_CAPACITY_BASELINE
#include "VehicleAuraCapacityPinned.inc"
#endif
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings missing");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});unsigned cases=0;
 for(unsigned entry:{105251u,105298u})for(unsigned mode=0;mode<8;++mode){
#ifdef VEHICLE_AURA_CAPACITY_BASELINE
  if(mode!=1)continue;
#endif
  auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,9998800+cases,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;auto scenario=sScenarioMgr->AddScenario(&map,1066);scenario->SetCurrentStep(4);
  auto session=Session();{
  auto carrier=map.SummonCreature(entry,Position{50,803,805,0},nullptr,0);auto first=map.SummonCreature(104856,Position{51,803,805,0},nullptr,0);auto second=map.SummonCreature(102826,Position{51,804,805,0},nullptr,0);TempSummon* npcIncoming=mode==7?nullptr:map.SummonCreature(104853,Position{51,802,805,0},nullptr,0);std::unique_ptr<NativePlayer> player;Unit* incoming=npcIncoming;
  if(mode==7){auto race=entry==105251?RACE_DRAENEI:RACE_ORC;player=std::make_unique<NativePlayer>(session,map,995,Position{51,802,805,0});player->SetByteValue(UNIT_FIELD_BYTES_0,0,race);player->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);player->setFactionForRace(race);auto display=sChrRacesStore.LookupEntry(race);Require(display,"Actual player race missing");player->SetNativeDisplayId(display->MaleDisplayID);player->SetDisplayId(display->MaleDisplayID);incoming=player.get();}
  Require(carrier&&first&&second&&incoming&&carrier->GetVehicleKit(),"Native capacity actors missing");for(Unit* actor:std::array<Unit*,4>{carrier,first,second,incoming})actor->SetPhaseId({6120},false);auto vehicle=carrier->GetVehicleKit();unsigned seats=entry==105251?1:2;
  if(mode){first->EnterVehicle(carrier,0);first->m_Events.Update(1);Require(vehicle->GetPassenger(0)==first,"Initial native seat0 missing");}
  if(seats==2&&mode!=0&&mode!=3){second->EnterVehicle(carrier,1);second->m_Events.Update(1);Require(vehicle->GetPassenger(0)==first&&vehicle->GetPassenger(1)==second,"Native full-seat setup differs");}
  bool full=mode&&!(seats==2&&mode==3);Require(vehicle->GetAvailableSeatCount()==(mode==0?seats:(full?0:1)),"Actual seat capacity differs");auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();
  if(mode==4)first->EnterVehicle(carrier,0);else incoming->EnterVehicle(carrier,mode==2?0:(mode==5?int(seats-1):-1));
  first->m_Events.Update(1);incoming->m_Events.Update(1);
#ifdef VEHICLE_AURA_CAPACITY_BASELINE
  {bool evicted0=!first->GetVehicleBase(),evicted1=seats==2&&!second->GetVehicleBase();Require(incoming->GetVehicleBase()==carrier&&(evicted0!=evicted1)&&vehicle->GetPassenger(evicted0?0:1)==incoming,"Pinned full-vehicle eviction not reproduced");std::cout<<"COUNTERFACTUAL capacity entry="<<entry<<" full"<<seats<<": incoming ordinary ride removes an existing native rider aura and takes that seat instead of refusing\n";}
#else
  if(mode==0)Require(incoming->GetVehicleBase()==carrier&&vehicle->GetPassenger(0)==incoming,"Empty native ride rejected");
  if(mode==1||mode==7||(mode==3&&seats==1))Require(!incoming->GetVehicleBase()&&vehicle->GetPassenger(0)==first&&first->GetVehicleBase()==carrier&&(seats==1||vehicle->GetPassenger(1)==second)&&!carrier->HasAuraTypeWithCaster(SPELL_AURA_CONTROL_VEHICLE,incoming->GetGUID()),"Full automatic ride displaced existing passengers or leaked its failed aura");
  if(mode==2)Require(incoming->GetVehicleBase()==carrier&&vehicle->GetPassenger(0)==incoming&&!first->GetVehicleBase()&&(seats==1||vehicle->GetPassenger(1)==second),"Explicit native seat replacement affected unrelated passenger");
  if(mode==3&&seats==2)Require(incoming->GetVehicleBase()==carrier&&vehicle->GetPassenger(0)==first&&vehicle->GetPassenger(1)==incoming,"Native last free seat selection changed");
  if(mode==4)Require(!incoming->GetVehicleBase()&&vehicle->GetPassenger(0)==first&&first->GetVehicleBase()==carrier&&(seats==1||vehicle->GetPassenger(1)==second),"Existing rider refresh displaced a passenger");
  if(mode==5)Require(incoming->GetVehicleBase()==carrier&&vehicle->GetPassenger(int(seats-1))==incoming&&(seats==1?!first->GetVehicleBase():(vehicle->GetPassenger(0)==first&&!second->GetVehicleBase())),"Explicit last-seat replacement displaced unrelated first passenger");
  if(mode==6){Require(!incoming->GetVehicleBase()&&vehicle->GetPassenger(0)==first&&(seats==1||vehicle->GetPassenger(1)==second),"Full retry precondition displaced riders");first->ExitVehicle();incoming->EnterVehicle(carrier);incoming->m_Events.Update(1);Require(incoming->GetVehicleBase()==carrier&&vehicle->GetPassenger(0)==incoming&&(seats==1||vehicle->GetPassenger(1)==second),"Native retry after genuine seat release failed");}
  std::cout<<"CAPACITY entry="<<entry<<" seats="<<seats<<" mode="<<mode<<" pass\n";
#endif
  Require(scenario->GetCurrentStep()==4&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count,"Capacity changed progression");for(Unit* actor:std::array<Unit*,3>{incoming,second,first}){if(actor->GetVehicleBase())actor->ExitVehicle();actor->m_Events.Update(1);}if(npcIncoming)npcIncoming->UnSummon();for(auto actor:{second,first,carrier})actor->UnSummon();map.RemoveAllObjectsInRemoveList();}session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef VEHICLE_AURA_CAPACITY_BASELINE
 Require(cases==2,"Expected two baseline full vehicles");std::cout<<"COUNTERFACTUAL native full capacity2 reproduced;not a passing repaired run"<<std::endl;
#else
 Require(cases==16,"Expected sixteen capacity contexts");std::cout<<"PASS native vehicle aura capacity16:real1/2-seat kits,NPC/player full refusal retains riders and removes only failed aura,empty/last-seat/both explicit replacements/own refresh/retry after seat release native behavior retained;no progress change"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
