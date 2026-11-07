// Actual registered instance, native graveyard dispatch and Player release/ACK.
// Prior quest/stage and dead character state are controlled in memory.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "MapManager.h"
#include "MovementPackets.h"
#include "WarswordsGraveyardInstance.inc"
#include "../../src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp"

int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}},{1511,{}},{1539,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}},{1511,{}},{1539,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_warswords;new instance_silver_hand;sScriptMgr->SwapScriptContext(true);
 auto safe=sWorldSafeLocsStore.LookupEntry(5228);Require(safe&&safe->MapID==1511&&std::string(safe->Name->Str[0])=="Fury Warrior Acquisition GY 1 - CRT","Matching native grounded Fury cemetery changed");
 unsigned cases=0,releases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  MapInstanced parent(1511,300);InstanceMap map(1511,300,9999300+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto script=dynamic_cast<instance_warswords::instance_warswords_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual Fury instance registration missing");
  auto scenario=sScenarioMgr->AddScenario(&map,944);auto session=Session();
  map.LoadGrid(safe->Loc.X,safe->Loc.Y);
  {NativePlayer floorPlayer(session,map,501,{safe->Loc.X,safe->Loc.Y,safe->Loc.Z,0});floorPlayer.SetByteValue(UNIT_FIELD_BYTES_0,0,race);floorPlayer.setFactionForRace(race);
  Require(std::fabs(map.GetHeight(floorPlayer.GetPhases(),safe->Loc.X,safe->Loc.Y,safe->Loc.Z+5)-safe->Loc.Z)<0.1f,"Native cemetery not supported by real map floor");}
  for(unsigned step=0;step<7;++step)for(unsigned location:{5228u,5229u,5273u,5131u}){
   auto pos=sWorldSafeLocsStore.LookupEntry(location);Require(pos&&pos->MapID==1511,"Native Fury death-site control missing");scenario->SetCurrentStep(step);
   auto before=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto waves=script->waves.wave;auto active=script->waves.active;
   for(unsigned repeat=0;repeat<2;++repeat){
    auto grave=map.GetClosestGraveYard(pos->Loc.X,pos->Loc.Y,pos->Loc.Z);
#ifdef FURY_GRAVEYARD_BASELINE
    Require(!grave,"Pinned missing Fury graveyard no longer reproduced");
#else
    Require(grave&&grave->GetMapId()==1511&&grave->GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z)<0.001f&&std::fabs(grave->GetOrientation()-safe->Loc.O*float(M_PI)/180.0f)<0.001f,"Actual instance failed grounded native cemetery or orientation");
#endif
   }
#ifndef FURY_GRAVEYARD_BASELINE
   {
    NativePlayer player(session,map,502,{pos->Loc.X,pos->Loc.Y,pos->Loc.Z,0});player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(40043,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40043);
    player.SetHealth(0);player.setDeathState(CORPSE);
    player.RepopAtGraveyard();auto const& dest=player.GetTeleportDest();
    Require(player.IsBeingTeleportedNear()&&!player.IsAlive()&&dest.GetMapId()==1511&&dest.GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z+2)<0.001f,"Native release failed to schedule grounded near transfer before resurrection");
    WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=player.GetGUID();session->HandleMoveTeleportAck(ack);
    Require(!player.IsBeingTeleported()&&player.IsAlive()&&player.GetHealth()==player.GetMaxHealth()/2&&player.GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z+2)<0.001f,"Native teleport ACK/delayed resurrection failed");
    session->HandleMoveTeleportAck(ack);Require(player.IsAlive()&&player.GetHealth()==player.GetMaxHealth()/2,"Duplicate ACK changed native revival");
    Require(player.GetQuestStatus(40043)==QUEST_STATUS_INCOMPLETE&&!player.HasItemCount(128908,1,true),"Release granted quest or artifact");++releases;
   }
#endif
   Require(scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==before&&script->waves.wave==waves&&script->waves.active==active,"Graveyard request/release altered scenario or wave progress");++cases;
  }
  // The same registered map script serves Hunter973; its new distinct native
  // cemetery must never borrow the Warrior position or a prior result.
  sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,973);
#ifdef FURY_GRAVEYARD_BASELINE
  Require(!map.GetClosestGraveYard(safe->Loc.X,safe->Loc.Y,safe->Loc.Z),"Fury cemetery leaked into shared Hunter scenario");++cases;
#else
  auto hunter=sWorldSafeLocsStore.LookupEntry(5295);auto hunterGy=map.GetClosestGraveYard(safe->Loc.X,safe->Loc.Y,safe->Loc.Z);
  Require(hunter&&hunterGy&&hunterGy->GetExactDist(hunter->Loc.X,hunter->Loc.Y,hunter->Loc.Z)<0.001f,"Shared Hunter cemetery did not retain its distinct native source");++cases;
#endif
  sScenarioMgr->RemoveScenario(map.GetInstanceId());Require(!map.GetClosestGraveYard(safe->Loc.X,safe->Loc.Y,safe->Loc.Z),"Fury cemetery used without an actual scenario");++cases;
 }
 // Protection's LFG1028 and the actual shared Arms/Priest/Paladin callback are
 // controls, not replaced by the raw generic-map fallback from early research.
 for(unsigned id:{1495u,1539u}){
  MapInstanced parent(id,300);InstanceMap map(id,300,9999200+id,DIFFICULTY_N_SCENARIO,&parent);if(id==1539)map.CreateInstanceData(nullptr);
  auto expected=id==1495?sSpellMgr->GetSpellTargetPosition(187803):nullptr;auto shared=sWorldSafeLocsStore.LookupEntry(5281);
  for(unsigned scenarioId:id==1495?std::initializer_list<unsigned>{909u}:std::initializer_list<unsigned>{1037u,991u,1092u}){
   sScenarioMgr->AddScenario(&map,scenarioId);auto gy=map.GetClosestGraveYard(2000,2400,130);Require(gy&&gy->GetMapId()==id,"Retained non-Fury cemetery missing");
   Require(expected?gy->GetExactDist(expected->target_X,expected->target_Y,expected->target_Z)<0.001f:gy->GetExactDist(shared->Loc.X,shared->Loc.Y,shared->Loc.Z)<0.001f,"Retained Protection/shared crypt cemetery changed");sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
  }
 }
#ifdef FURY_GRAVEYARD_BASELINE
 Require(cases==64&&!releases,"Pinned graveyard counterfactual variant/count changed");std::cout<<"COUNTERFACTUAL pinned c252ae5ee actual instance:56 Fury contexts lack graveyard;8 retained shared/scenario controls;no successful release claimed"<<std::endl;
#else
 Require(cases==64&&releases==56,"Native graveyard regression variant/count changed");std::cout<<"PASS actual Fury graveyard64/native release56:Human+Orc,steps0-6,four native death locations,repeated lookup/ACK,grounded5228/floor/orientation,real near teleport ACK and delayed half-health revival;shared Hunter uses distinct5295,Arms/Priest/Paladin/Protection unchanged;no wave/quest/item credit. Dead/quest/stage state controlled;full client/corpse/reconnect separate"<<std::endl;
#endif
 return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<std::endl;return 1;}}
