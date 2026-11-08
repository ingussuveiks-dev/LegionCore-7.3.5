// Whole actual1600 instance and native release/ACK; earlier stage inputs controlled.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "MapManager.h"
#include "MovementPackets.h"
#include "PlayerDefines.h"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "Conversation.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "WaypointManager.h"
#include "WaypointMovementGenerator.h"
#include "MoveSpline.h"
#include <array>
#include <memory>
namespace SharasdalGraveyardFixture {
#include "SharasdalGraveyardInstance.inc"
}
static_assert(sizeof(ObjectGuid)==16&&offsetof(CriteriaProgress,Counter)==16,"Native cemetery receipt ABI changed");
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 auto lower=sWorldSafeLocsStore.LookupEntry(5443),upper=sWorldSafeLocsStore.LookupEntry(5473);Require(lower&&upper&&lower->MapID==1600&&upper->MapID==1600&&std::string(lower->Name->Str[0])=="Artifact - Shaman Acquisition - Restoration - Enter"&&std::string(upper->Name->Str[0])=="Artifact - Shaman Acquisition - Restoration - Elevator Top","Native Sharasdal safe sources absent");
 auto steps=sScenarioMgr->GetScenarioSteps(1066);auto tree=sCriteriaTreeStore.LookupEntry(47131);auto criterion=tree?sCriteriaStore.LookupEntry(tree->CriteriaID):nullptr;Require(steps&&steps->size()==6&&(*steps)[3]->Criteriatreeid==47130&&tree&&tree->Parent==47130&&criterion&&criterion->ID==29795&&criterion->Type==92&&criterion->Asset==50336,"Native ascent criterion changed");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new SharasdalGraveyardFixture::instance_sharasdal;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0,releases=0;auto target=[&](bool ascended){auto s=ascended?upper:lower;return Position{s->Loc.X,s->Loc.Y,s->Loc.Z,s->Loc.O*float(M_PI)/180};};
 for(unsigned race:{unsigned(RACE_DRAENEI),unsigned(RACE_ORC)}){MapInstanced parent(1600,300);InstanceMap map(1600,300,9998900+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<SharasdalGraveyardFixture::instance_sharasdal::instance_sharasdal_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual registered1600 instance missing");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,1066);
  for(bool high:{false,true}){auto pos=target(high);auto p=std::make_unique<NativePlayer>(session,map,941,pos);p->SetPhaseId({6120,7001},false);float floor=map.GetHeight(p->GetPhases(),pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+5);std::cout<<"FLOOR "<<high<<" pose="<<pos.GetPositionZ()<<" native="<<floor<<std::endl;Require(floor<=pos.GetPositionZ()+.5f&&pos.GetPositionZ()-floor<=3.0f,"Native Sharasdal floor unsupported");}
  for(unsigned state=0;state<8;++state)for(unsigned context=0;context<4;++context){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1066);unsigned step=state<4?state:state-1;bool high=state>=4;Position death=context==0?target(false):context==1?target(true):context==2?Position{836.16f,1035.4f,48.61f,0}:Position{0,0,-1000,0};auto owner=std::make_unique<NativePlayer>(session,map,942,death);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);p.setFactionForRace(race);p.SetPhaseId({6120,7001,7002},false);p.SetQuestStatus(40341,context%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40341);scenario->SetCurrentStep(state==7?5:step);if(state==4)p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50336);if(state==7)p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,49820);Require(scenario->GetCurrentStep()==step,"Controlled native state changed");auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto quest=p.GetQuestStatus(40341);auto pending=script->ascentBound;auto recipient=script->ascentRecipient;auto gy=map.GetClosestGraveYard(death.GetPositionX(),death.GetPositionY(),death.GetPositionZ());Require(gy&&gy->GetMapId()==1600,"Native cemetery dispatch failed");
#ifdef SHARASDAL_GRAVEYARD_BASELINE
   Require(gy->GetExactDist(death)<.001f&&gy->GetOrientation()==0,"Pinned death-pose callback changed");
#else
   Require(gy->GetExactDist(target(high))<.001f&&std::fabs(gy->GetOrientation()-target(high).GetOrientation())<.001f,"Native checkpoint/facing failed");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost absent");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native release missing");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2,"Native revival/ACK/ghost cleanup failed");auto expected=death;
#ifndef SHARASDAL_GRAVEYARD_BASELINE
   expected=target(high);
#endif
   Require(p.GetExactDist(expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ()+2)<.001f,"Native release destination failed");session->HandleMoveTeleportAck(ack);Require(scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p.GetQuestStatus(40341)==quest&&!p.HasItemCount(128911,1,true)&&script->ascentBound==pending&&script->ascentRecipient==recipient,"Recovery granted progress/item or changed ascent owner");++cases;++releases;
  }
  for(unsigned order=0;order<2;++order){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1066);auto p=std::make_unique<NativePlayer>(session,map,943,target(true));p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);p->SetQuestStatus(40341,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,40341);scenario->SetCurrentStep(2);p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50336);Require(scenario->GetCurrentStep()==2&&!scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(47131),"Premature ascent accepted");scenario->SetCurrentStep(3);unsigned first=order?50314:50336,second=order?50336:50314;p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,first);Require(scenario->GetCurrentStep()==3,"Single child bypassed native AND tree");
#ifndef SHARASDAL_GRAVEYARD_BASELINE
   Require(script->GetClosestGraveYard(0,0,-1000)->GetExactDist(target(!order))<.001f,"Unrelated gauntlet root unlocked upper cemetery");
#endif
   p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,first);Require(scenario->GetCurrentStep()==3,"Duplicate single criterion advanced");p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,second);Require(scenario->GetCurrentStep()==4,"Both native children failed");++cases;
  }
  for(unsigned id:{1092u,0u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=id?sScenarioMgr->AddScenario(&map,id):nullptr;auto gy=script->GetClosestGraveYard(1,2,-1000);
#ifdef SHARASDAL_GRAVEYARD_BASELINE
   Require(gy&&gy->GetExactDist(1,2,-1000)<.001f,"Pinned foreign/no-scenario behavior changed");
#else
   Require(!gy,"Foreign/missing scenario selected class checkpoint");
#endif
   ++cases;
  }session->SetPlayer(nullptr);delete session;
 }
 Require(cases==72&&releases==64,"Native Sharasdal counts changed");
#ifdef SHARASDAL_GRAVEYARD_BASELINE
 std::cout<<"COUNTERFACTUAL pinnedd8cc8bb9e Sharasdal native72/release64:actual whole instance returns death pose even below-map/foreign-world;current native support,not historical binary"<<std::endl;
#else
 std::cout<<"PASS Sharasdal native72/release64:actual registered1600/native Player/Scenario,all6 stages+completed6 and earned/unearned3,Draenei+Orc/four death contexts/both active statuses,5443/5473 floors/facing,real ghost/repop/ACK/half-health/duplicate cleanup,real50336 and50314 AND in both orders/early+duplicate denial,foreign/no-scenario null,no progression/item/ascent-owner mutation. Earlier state/final criterion controlled;client/elevator/path/pet/group separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
