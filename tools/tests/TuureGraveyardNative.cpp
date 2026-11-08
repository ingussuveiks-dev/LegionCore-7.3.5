// Whole actual instance class in a fixture namespace prevents baseline/current
// COMDAT aliasing. Native Player/release/ACK and support libraries stay current.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "MapManager.h"
#include "MovementPackets.h"
#include "PlayerDefines.h"
#include "DB2LoadInfo.h"
#include "WaypointMovementGenerator.h"
#include "CreatureTextMgr.h"
#include "MoveSpline.h"
#include "../../src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.h"
#include <memory>
namespace TuureGraveyardFixture {
#include "TuureGraveyardInstance.inc"
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();
 DB2Storage<CharBaseInfoEntry> pairs("CharBaseInfo.db2",CharBaseInfoLoadInfo::Instance());Require(pairs.Load(argv[1],0),"Actual Legion race/class data absent");
 auto safe=sWorldSafeLocsStore.LookupEntry(5480);auto d=sLFGMgr->GetLFGDungeon(1243);Require(safe&&safe->MapID==1489&&std::string(safe->Name->Str[0])=="7.0 Artifacts - Priest - Holy - Niskara Entrance Loc - LWB"&&d&&d->dbc&&d->dbc->ScenarioID==1085&&d->map==1489&&d->difficulty==DIFFICULTY_N_SCENARIO,"Native Holy source absent");
 Require(sScenarioMgr->GetScenarioSteps(1085)&&sScenarioMgr->GetScenarioSteps(1085)->size()==7,"Actual Holy seven-step sequence changed");
 Require(std::fabs(d->x-safe->Loc.X)<.01f&&std::fabs(d->y-safe->Loc.Y)<.01f&&std::fabs(d->z-safe->Loc.Z)<.01f,"Live1243 and5480 disagree");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new TuureGraveyardFixture::instance_tuure;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0,releases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_UNDEAD_PLAYER)}){
  bool valid=false;for(auto pair:pairs)if(pair->RaceID==race&&pair->ClassID==CLASS_PRIEST)valid=true;Require(valid,"Invalid native Priest race pair");
  MapInstanced parent(1489,300);InstanceMap map(1489,300,9998100+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto script=dynamic_cast<TuureGraveyardFixture::instance_tuure::instance_tuure_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual named1489 instance registration failed");auto session=Session();
  auto scenario=sScenarioMgr->AddScenario(&map,1085);
  {auto owner=std::make_unique<NativePlayer>(session,map,901,Position{safe->Loc.X,safe->Loc.Y,safe->Loc.Z,0});auto& p=*owner;Require(std::fabs(map.GetHeight(p.GetPhases(),safe->Loc.X,safe->Loc.Y,safe->Loc.Z+5)-safe->Loc.Z)<.1f,"Actual5480 floor unsupported");}
  auto exercise=[&](Position pos,unsigned step,bool completed){
   auto owner=std::make_unique<NativePlayer>(session,map,902,pos);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);p.setFactionForRace(race);p.SetQuestStatus(41993,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,41993);
   if(completed){p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50679);p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50702);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==7,"Actual completed ALL-root control failed");}
   auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto quest=p.GetQuestStatus(41993);auto fight=script->firstFight;auto route=script->firstRoute;
   auto gy=map.GetClosestGraveYard(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ());Require(gy&&gy->GetMapId()==1489,"Native instance dispatch failed");
#ifdef TUURE_GRAVEYARD_BASELINE
   Require(gy->GetExactDist(pos)<.001f,"Original corpse echo not reproduced");
#else
   Require(gy->GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z)<.001f&&std::fabs(gy->GetOrientation()-safe->Loc.O*float(M_PI)/180)<.001f,"Repaired instance lost native Holy entrance/facing");
   Require(map.GetClosestGraveYard(0,0,-1000)==gy&&gy->GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z)<.001f,"Repeated selection reused unsafe death pose");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost state absent");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native release did not start same-map transfer");
   WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);
   Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2,"Native ACK/delayed half-health revival/ghost cleanup failed");
#ifdef TUURE_GRAVEYARD_BASELINE
   Require(p.GetExactDist(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+2)<.001f,"Original unsafe release destination changed");
#else
   Require(p.GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z+2)<.001f,"Native ACK did not recover5480");
#endif
   session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&p.GetHealth()==p.GetMaxHealth()/2,"Duplicate ACK changed recovery");Require(scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&script->firstFight==fight&&script->firstRoute==route&&p.GetQuestStatus(41993)==quest&&!p.HasItemCount(128825,1,true),"Release changed scenario/actor/quest/item progression");++cases;++releases;
  };
  for(unsigned step=0;step<7;++step)for(unsigned place=0;place<4;++place){scenario->SetCurrentStep(step);Position pos=place==0?Position{-828.85f,4372,738.64f,0}:place==1?Position{71.9653f,1218.51f,-74.2702f,0}:place==2?Position{safe->Loc.X,safe->Loc.Y,-1000,0}:Position{236.392f,1952.189f,-55.7523f,0};exercise(pos,step,false);}
  scenario->SetCurrentStep(6);exercise({71.9653f,1218.51f,-74.2702f,0},7,true);
  for(unsigned id:{940u,972u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,id);auto gy=map.GetClosestGraveYard(12,34,-56);Require(gy&&gy->GetExactDist(12,34,-56)<.001f,"Shared non-Holy policy changed");++cases;}
  sScenarioMgr->RemoveScenario(map.GetInstanceId());auto gy=map.GetClosestGraveYard(12,34,-56);Require(gy&&gy->GetExactDist(12,34,-56)<.001f,"No-scenario policy changed");++cases;session->SetPlayer(nullptr);delete session;
 }
 Require(cases==64&&releases==58,"Native graveyard variant/count changed");
#ifdef TUURE_GRAVEYARD_BASELINE
 std::cout<<"COUNTERFACTUAL pinned562084277 Tuure native64/release58:registered whole old instance echoes corpse/foreign-world/below-map poses through real release+ACK;completed/shared/no-scenario controls. Current native support,not historical binary or successful safe recovery"<<std::endl;
#else
 std::cout<<"PASS Tuure native64/release58:registered whole instance/native5480+real floor,Human+Undead26972 pairs,all7 stages+real ALL-root completion,foreign-world/late/below-map/shared-wing death poses,actual ghost release+ACK/delayed half-health revival,repeated selection/ACK,shared940/972/no-scenario preserved;no scenario/actor/quest/item grants. Earlier quest/death/stage and final credit endpoints controlled;full client/corpse/pet/group traversal separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
