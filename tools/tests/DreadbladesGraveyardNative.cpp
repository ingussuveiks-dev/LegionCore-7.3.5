#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "MapManager.h"
#include "MovementPackets.h"
#include "PlayerDefines.h"
#include <memory>
namespace DreadbladesGraveyardFixture {
#include "DreadbladesGraveyardInstance.inc"
static void Register(){RegisterInstanceScript(instance_dreadblades,1545);}
}
static unsigned Checkpoint(unsigned step){return step>=4?5361:step>=2?5360:step==1?5359:5299;}
static Position Pose(unsigned id){auto s=sWorldSafeLocsStore.LookupEntry(id);Require(s&&s->ID==id&&s->MapID==1545,"Native Dreadblades safe source absent");return {s->Loc.X,s->Loc.Y,s->Loc.Z,s->Loc.O*float(M_PI)/180};}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);auto steps=sScenarioMgr->GetScenarioSteps(1012);Require(steps&&steps->size()==5,"Native1012 five steps absent");unsigned assets[]={48449,48489,48531,48538};
 for(unsigned step=0;step<4;++step){auto root=(*steps)[step]->Criteriatreeid;bool found=false;for(auto t:sCriteriaTreeStore)if(t->Parent==root){auto c=sCriteriaStore.LookupEntry(t->CriteriaID);if(c&&c->Type==92&&unsigned(c->Asset)==assets[step])found=true;}Require(found,"Native Dreadblades checkpoint boundary changed");}
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1545,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1545,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());DreadbladesGraveyardFixture::Register();sScriptMgr->SwapScriptContext(true);unsigned cases=0,releases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  auto parent=std::make_unique<MapInstanced>(1545,300);auto mapOwner=std::make_unique<InstanceMap>(1545,300,9999000+race,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;map.CreateInstanceData(nullptr);auto script=dynamic_cast<DreadbladesGraveyardFixture::instance_dreadblades*>(map.GetInstanceScript());Require(script,"Actual registered whole1545 instance absent");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,1012);
  for(unsigned id:{5299u,5359u,5360u,5361u}){auto pos=Pose(id);auto p=std::make_unique<NativePlayer>(session,map,951,pos);p->SetPhaseId({6199},false);auto floor=map.GetHeight(p->GetPhases(),pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+5);std::cout<<"FLOOR "<<id<<" pose="<<pos.GetPositionZ()<<" native="<<floor<<std::endl;Require(floor<=pos.GetPositionZ()+.5f&&pos.GetPositionZ()-floor<=3.0f,"Native Dreadblades checkpoint floor unsupported");}
  for(unsigned step=0;step<6;++step)for(unsigned context=0;context<4;++context){
   sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1012);Position death=context==0?Pose(5299):context==1?Pose(5361):context==2?Position{836.16f,1035.4f,48.61f,0}:Position{0,0,-1000,0};auto owner=std::make_unique<NativePlayer>(session,map,952,death);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p.setFactionForRace(race);p.SetPhaseId({6199},false);p.SetQuestStatus(40849,context%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40849);scenario->SetCurrentStep(step==5?4:step);
   if(step==5){p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,48582);p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,48553);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==5,"Actual native final AND tree did not complete");}
   // Earlier stage is controlled. This local map is not in MapManager's global
   // registry, so mirror the native cached-step notification for the old host.
   script->setScenarioStep(step);
   auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto status=p.GetQuestStatus(40849);auto gy=map.GetClosestGraveYard(death.GetPositionX(),death.GetPositionY(),death.GetPositionZ());
#ifdef DREADBLADES_GRAVEYARD_BASELINE
   unsigned expectedId=step==3||step==4?5360:5299;
#else
   unsigned expectedId=Checkpoint(step);
#endif
   auto expected=Pose(expectedId);Require(gy&&gy->GetMapId()==1545&&gy->GetExactDist(expected)<.001f,"Native selected Dreadblades checkpoint failed");
#ifdef DREADBLADES_GRAVEYARD_BASELINE
   Require(gy->GetOrientation()==0,"Pinned zero facing changed");
#else
   Require(std::fabs(gy->GetOrientation()-expected.GetOrientation())<.001f,"Authored cemetery facing lost");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost absent");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native release absent");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2&&p.GetExactDist(expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ()+2)<.001f,"Native release/ACK/half-health/ghost cleanup failed");session->HandleMoveTeleportAck(ack);Require(scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p.GetQuestStatus(40849)==status&&!p.HasItemCount(128872,1,true),"Recovery changed progress/quest/item");++cases;++releases;
  }
  for(unsigned order=0;order<2;++order){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1012);auto p=std::make_unique<NativePlayer>(session,map,953,Pose(5299));p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->SetQuestStatus(40849,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,40849);
   for(unsigned step=0;step<4;++step){p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,assets[step]);Require(scenario->GetCurrentStep()==step+1,"Native ordered boundary failed");
#ifndef DREADBLADES_GRAVEYARD_BASELINE
    Require(script->GetClosestGraveYard(0,0,-1000)->GetExactDist(Pose(Checkpoint(step+1)))<.001f,"Native accepted event did not select earned checkpoint");
#endif
   }
   unsigned first=order?48553:48582,second=order?48582:48553;p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,first);p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,first);Require(scenario->GetCurrentStep()==4,"Single/duplicate final child bypassed AND tree");p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,second);Require(scenario->GetCurrentStep()==5&&scenario->IsCompleted(false),"Native final second child failed");++cases;
  }
#ifndef DREADBLADES_GRAVEYARD_BASELINE
  for(unsigned id:{1066u,0u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=id?sScenarioMgr->AddScenario(&map,id):nullptr;Require(!script->GetClosestGraveYard(1,2,-1000),"Foreign/missing Scenario selected Rogue checkpoint");++cases;}
#endif
  sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
#ifdef DREADBLADES_GRAVEYARD_BASELINE
 Require(cases==52&&releases==48,"Pinned native count changed");std::cout<<"COUNTERFACTUAL pinned9a8f9340d Dreadblades native52/release48:original stage1/2=start,stage4=hill,completed5=start,zero facing. Original whole instance/current native support,not historical binary"<<std::endl;
#else
 Require(cases==56&&releases==48,"Native Dreadblades count changed");std::cout<<"PASS Dreadblades native56/release48:registered whole1545/native Player/Scenario,all5 stages+real completed5,Human+Orc/four death contexts/two active statuses,four VMAP floors/facing,ghost/repop/ACK/half-health/duplicate cleanup,actual ordered4 boundaries and final death/pickup AND both orders,foreign+missing null,no quest/item/progress grant. Earlier pose/stage/final event endpoints controlled;client/combat/pet/group separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
