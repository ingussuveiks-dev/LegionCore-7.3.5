#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "MapManager.h"
#include "MovementPackets.h"
#include "PlayerDefines.h"
#include "AreaTriggerData.h"
#include "LFGMgr.h"
#include "../../src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/silver_hand_first_fight.h"
#include "../../src/server/scripts/Scenario/Artifacts/Priest/slaghammer_handoff.h"
#include <memory>
namespace ArmsGraveyardFixture {
#include "ArmsGraveyardInstance.inc"
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();auto general=sWorldSafeLocsStore.LookupEntry(5281);auto early=sWorldSafeLocsStore.LookupEntry(5339);auto middle=sWorldSafeLocsStore.LookupEntry(5335);auto late=sWorldSafeLocsStore.LookupEntry(5705);Require(general&&early&&middle&&late&&early->MapID==1539&&middle->MapID==1539&&late->MapID==1539&&std::string(early->Name->Str[0])=="7.0 Artifacts - Warrior - Arms - Start - JMC"&&std::string(middle->Name->Str[0])=="7.0 Artifacts - Keeper's Rest - Entrance Graveyard - JMC"&&std::string(late->Name->Str[0])=="7.0 Artifacts - Warrior - Arms - Graveyard Inside 2 - JMC","Native Arms checkpoints absent");
 auto spell=sSpellMgr->GetSpellInfo(203920);auto entry=sSpellMgr->GetSpellTargetPosition(203920);Require(spell&&spell->GetEffect(EFFECT_1)->Effect==227&&entry&&entry->target_mapId==1539,"Actual Arms entry203920 absent");auto steps=sScenarioMgr->GetScenarioSteps(1037);Require(steps&&steps->size()==10,"Native Arms ten steps absent");for(auto pair:{std::make_pair(3u,29543u),std::make_pair(6u,29547u)}){auto c=sCriteriaStore.LookupEntry(pair.second);auto root=pair.first==3?46862u:46870u;Require(c&&c->Type==92&&c->Asset==(pair.first==3?49022:49058)&&(*steps)[pair.first]->Criteriatreeid==root,"Native Arms checkpoint boundary changed");bool contained=false;for(auto t:sCriteriaTreeStore)if(t->CriteriaID==pair.second){auto p=t;for(unsigned depth=0;p&&depth<64;++depth){if(p->ID==root)contained=true;p=sCriteriaTreeStore.LookupEntry(p->Parent);}}Require(contained,"Checkpoint criterion not in its actual root");}
 auto data=WorldDatabase.Query("SELECT entryorguid,id,action_param2,target_type FROM smart_scripts WHERE source_type=0 AND action_type=205 AND action_param1=92 AND action_param2 IN(49022,49058) ORDER BY action_param2");Require(data&&data->GetRowCount()==2,"Native entry/passage dispatch missing");for(unsigned i=0;i<2;++i){auto f=data->Fetch();Require(f[0].GetUInt32()==(i?103339:101114)&&f[1].GetUInt32()==(i?4:0)&&f[2].GetUInt32()==(i?49058:49022)&&f[3].GetUInt32()==7,"Native checkpoint invoker changed");if(!i)data->NextRow();}
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1539,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1539,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ArmsGraveyardFixture::instance_silver_hand;sScriptMgr->SwapScriptContext(true);unsigned cases=0,releases=0;
 auto target=[&](unsigned step){auto s=step<4?early:step<7?middle:late;return Position{s->Loc.X,s->Loc.Y,s->Loc.Z,s->Loc.O*float(M_PI)/180};};
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){MapInstanced parent(1539,300);InstanceMap map(1539,300,9998300+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<ArmsGraveyardFixture::instance_silver_hand::instance_silver_hand_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual registered whole1539 instance absent");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,1037);
  for(unsigned step:{0u,4u,7u}){auto pos=target(step);auto p=std::make_unique<NativePlayer>(session,map,943,pos);p->SetPhaseId({5739},false);auto floor=map.GetHeight(p->GetPhases(),pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+5);std::cout<<"FLOOR "<<step<<" pose="<<pos.GetPositionZ()<<" native="<<floor<<std::endl;Require(floor<=pos.GetPositionZ()+.5f&&pos.GetPositionZ()-floor<=3.0f,"Native Arms checkpoint floor unsupported");}
  auto exercise=[&](Position pos,unsigned step,bool complete,unsigned context){auto owner=std::make_unique<NativePlayer>(session,map,944,pos);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);p.setFactionForRace(race);p.SetPhaseId({5739},false);p.SetQuestStatus(41105,context%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,41105);if(complete){p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,49109);p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,49110);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==10,"Native Arms completed sentinel absent");}auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto quest=p.GetQuestStatus(41105);auto phase=script->slaghammer.phase;auto source=script->slaghammer.source;auto recipient=script->slaghammer.owner;auto finalActors=script->_finaleActorsStarted.size();auto expected=target(step);auto gy=map.GetClosestGraveYard(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ());Require(gy&&gy->GetMapId()==1539,"Actual Arms cemetery dispatch absent");
#ifdef ARMS_GRAVEYARD_BASELINE
   Require(gy->GetExactDist(general->Loc.X,general->Loc.Y,general->Loc.Z)<.001f&&gy->GetOrientation()==0,"Pinned generic5281 baseline changed");
#else
   Require(gy->GetExactDist(expected)<.001f&&std::fabs(gy->GetOrientation()-expected.GetOrientation())<.001f,"Arms native checkpoint/facing changed");Require(map.GetClosestGraveYard(0,0,-1000)==gy&&gy->GetExactDist(expected)<.001f,"Repeated recovery changed checkpoint");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost absent");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native same-map release absent");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2,"Native ACK/revival/ghost cleanup failed");
#ifdef ARMS_GRAVEYARD_BASELINE
   Require(p.GetExactDist(general->Loc.X,general->Loc.Y,general->Loc.Z+2)<.001f,"Pinned release destination changed");
#else
   Require(p.GetExactDist(expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ()+2)<.001f,"Native release lost class checkpoint");
#endif
   session->HandleMoveTeleportAck(ack);Require(scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p.GetQuestStatus(41105)==quest&&!p.HasItemCount(128910,1,true)&&script->slaghammer.phase==phase&&script->slaghammer.source==source&&script->slaghammer.owner==recipient&&script->_finaleActorsStarted.size()==finalActors,"Recovery changed shared actors/quest/item/progression");++cases;++releases;};
  for(unsigned step=0;step<10;++step)for(unsigned context=0;context<4;++context){scenario->SetCurrentStep(step);auto pos=context==0?target(0):context==1?target(7):context==2?Position{-828.85f,4372,738.64f,0}:Position{late->Loc.X,late->Loc.Y,-1000,0};exercise(pos,step,false,context);}scenario->SetCurrentStep(9);exercise(target(7),10,true,0);
  for(unsigned step:{3u,6u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1037);scenario->SetCurrentStep(step-1);auto owner=std::make_unique<NativePlayer>(session,map,945,target(7));auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);p.SetQuestStatus(41105,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,41105);unsigned event=step==3?49022:49058;p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,event);Require(scenario->GetCurrentStep()==step-1,"Premature entry/passage event advanced");scenario->SetCurrentStep(step);auto gy=map.GetClosestGraveYard(late->Loc.X,late->Loc.Y,late->Loc.Z);
#ifndef ARMS_GRAVEYARD_BASELINE
   Require(gy&&gy->GetExactDist(target(step))<.001f,"Unaccepted event unlocked later graveyard");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,event);Require(scenario->GetCurrentStep()==step+1,"Real native entry/passage criterion failed");gy=map.GetClosestGraveYard(early->Loc.X,early->Loc.Y,early->Loc.Z);
#ifndef ARMS_GRAVEYARD_BASELINE
   Require(gy&&gy->GetExactDist(target(step+1))<.001f,"Acknowledged passage did not select its checkpoint");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,event);Require(scenario->GetCurrentStep()==step+1&&!p.HasItemCount(128910,1,true),"Duplicate event skipped stage or granted item");++cases;}
  for(unsigned id:{991u,1092u,1146u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,id);auto gy=map.GetClosestGraveYard(12,34,-56);
#ifndef ARMS_GRAVEYARD_BASELINE
 if(id==1092){auto holy=sWorldSafeLocsStore.LookupEntry(5488);Require(holy&&gy&&gy->GetExactDist(holy->Loc.X,holy->Loc.Y,holy->Loc.Z)<.001f&&std::fabs(gy->GetOrientation()-holy->Loc.O*float(M_PI)/180)<.001f,"Holy outside recovery changed");}else
#endif
 Require(gy&&gy->GetExactDist(general->Loc.X,general->Loc.Y,general->Loc.Z)<.001f&&gy->GetOrientation()==0,"Retained shared generic recovery changed");++cases;}sScenarioMgr->RemoveScenario(map.GetInstanceId());auto gy=map.GetClosestGraveYard(12,34,-56);Require(gy&&gy->GetExactDist(general->Loc.X,general->Loc.Y,general->Loc.Z)<.001f,"No-scenario policy changed");++cases;session->SetPlayer(nullptr);delete session;
 }
 Require(cases==94&&releases==82,"Native Arms test count changed");
#ifdef ARMS_GRAVEYARD_BASELINE
 std::cout<<"COUNTERFACTUAL pinned96ca7646c Arms native94/release82:all class stages revive at generic5281/zero-facing;current native support,not historical binary or class-checkpoint recovery"<<std::endl;
#else
 std::cout<<"PASS Arms native94/release82:actual registered whole1539,all10 stages+real completed10,Human+Orc/active statuses/four death contexts,5339/5335/5705 phase5739 floors/facing,real ghost/release/ACK/half-health/repeated cleanup,both real49022 at3->4 and49058 at6->7/early+duplicate denials,shared991/1092/other/no-scenario retained,no progression/item/actor grants. Earlier states/final criteria controlled;client/corpse/path/pet/group separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
