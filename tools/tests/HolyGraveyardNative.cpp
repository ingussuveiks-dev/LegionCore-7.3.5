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
namespace HolyGraveyardFixture {
#include "HolyGraveyardInstance.inc"
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sCreatureTextMgr->LoadCreatureTexts();sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();auto general=sWorldSafeLocsStore.LookupEntry(5281);auto early=sWorldSafeLocsStore.LookupEntry(5488);auto middle=sWorldSafeLocsStore.LookupEntry(5693);auto late=sWorldSafeLocsStore.LookupEntry(5694);Require(general&&early&&middle&&late&&early->MapID==1539&&middle->MapID==1539&&late->MapID==1539&&std::string(early->Name->Str[0])=="7.0 Artifacts - Paladin - Holy - Graveyard 1 (outside)"&&std::string(middle->Name->Str[0])=="7.0 Artifacts - Paladin - Holy - Graveyard 2 (inside)"&&std::string(late->Name->Str[0])=="7.0 Artifacts - Paladin - Holy - Graveyard 3 (inside)","Native Holy checkpoints absent");
 auto steps=sScenarioMgr->GetScenarioSteps(1092);Require(steps&&steps->size()==11,"Native Holy eleven steps absent");for(auto pair:{std::make_pair(1u,30017u),std::make_pair(4u,30079u),std::make_pair(6u,30086u)}){auto c=sCriteriaStore.LookupEntry(pair.second);auto root=pair.first==1?47896u:pair.first==4?47965u:48010u;Require(c&&c->Type==92&&c->Asset==(pair.first==1?50766:pair.first==4?50897:50912)&&(*steps)[pair.first]->Criteriatreeid==root,"Native Holy boundary changed");bool contained=false;for(auto t:sCriteriaTreeStore)if(t->CriteriaID==pair.second){auto p=t;for(unsigned depth=0;p&&depth<64;++depth){if(p->ID==root)contained=true;p=sCriteriaTreeStore.LookupEntry(p->Parent);}}Require(contained,"Checkpoint criterion absent from actual root");}
 auto barriers=WorldDatabase.Query("SELECT PhaseId,position_x,position_y,position_z FROM gameobject WHERE guid=134276 AND id=251349 AND map=1539");Require(barriers&&barriers->Fetch()[0].GetString()=="7001"&&std::fabs(barriers->Fetch()[1].GetFloat()-1874.82f)<.01f&&std::fabs(barriers->Fetch()[2].GetFloat()-2269.67f)<.01f,"Native escape barrier changed");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1539,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1539,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new HolyGraveyardFixture::instance_silver_hand;sScriptMgr->SwapScriptContext(true);unsigned cases=0,releases=0;
 auto target=[&](unsigned step){auto s=step<2?early:step>=5&&step<7?late:middle;return Position{s->Loc.X,s->Loc.Y,s->Loc.Z,s->Loc.O*float(M_PI)/180};};
 auto phases=[](unsigned step){auto result=std::set<uint32>{5738,step<4?7005u:step==4?7000u:step<9?7002u:7003u};if(step==7)result.insert(7001);if(step==10)result.insert(7004);return result;};
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_BLOODELF)}){MapInstanced parent(1539,300);InstanceMap map(1539,300,9998700+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<HolyGraveyardFixture::instance_silver_hand::instance_silver_hand_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual registered whole1539 instance absent");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,1092);
  for(unsigned step:{0u,2u,5u,7u}){auto pos=target(step);auto p=std::make_unique<NativePlayer>(session,map,943,pos);p->SetPhaseId(phases(step),false);auto floor=map.GetHeight(p->GetPhases(),pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+5);std::cout<<"FLOOR "<<step<<" pose="<<pos.GetPositionZ()<<" native="<<floor<<std::endl;Require(floor<=pos.GetPositionZ()+.5f&&pos.GetPositionZ()-floor<=3.0f,"Native Holy checkpoint floor unsupported");}
  auto exercise=[&](Position pos,unsigned step,bool complete,unsigned context){auto owner=std::make_unique<NativePlayer>(session,map,944,pos);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PALADIN);p.setFactionForRace(race);p.SetPhaseId(phases(step),false);p.SetQuestStatus(42120,context%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,42120);std::unique_ptr<Actor> finalTravard,finalBoss;if(step>=10){finalTravard=std::make_unique<Actor>(&map,106777,98000+cases*2,pos);finalBoss=std::make_unique<Actor>(&map,106719,98001+cases*2,pos);for(auto actor:{finalTravard.get(),finalBoss.get()}){actor->SetPhaseId(p.GetPhases(),false);Require(map.AddToMap(static_cast<Creature*>(actor)),"Native finale control actor admission failed");}}scenario->SetCurrentStep(complete?10:step);if(complete){p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50966);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==11,"Native Holy completed sentinel absent");}auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto quest=p.GetQuestStatus(42120);auto phase=script->slaghammer.phase;auto source=script->slaghammer.source;auto recipient=script->slaghammer.owner;auto finalActors=script->_finaleActorsStarted.size();auto expected=target(step);auto gy=map.GetClosestGraveYard(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ());Require(gy&&gy->GetMapId()==1539,"Actual Holy cemetery dispatch absent");
#ifdef HOLY_GRAVEYARD_BASELINE
   Require(gy->GetExactDist(general->Loc.X,general->Loc.Y,general->Loc.Z)<.001f&&gy->GetOrientation()==0,"Pinned generic5281 baseline changed");
#else
   Require(gy->GetExactDist(expected)<.001f&&std::fabs(gy->GetOrientation()-expected.GetOrientation())<.001f,"Holy native checkpoint/facing changed");Require(map.GetClosestGraveYard(0,0,-1000)==gy&&gy->GetExactDist(expected)<.001f,"Repeated recovery changed checkpoint");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost absent");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native same-map release absent");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2,"Native ACK/revival/ghost cleanup failed");
#ifdef HOLY_GRAVEYARD_BASELINE
   Require(p.GetExactDist(general->Loc.X,general->Loc.Y,general->Loc.Z+2)<.001f,"Pinned release destination changed");
#else
   Require(p.GetExactDist(expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ()+2)<.001f,"Native release lost class checkpoint");
#endif
   session->HandleMoveTeleportAck(ack);Require(scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p.GetQuestStatus(42120)==quest&&!p.HasItemCount(128823,1,true)&&script->slaghammer.phase==phase&&script->slaghammer.source==source&&script->slaghammer.owner==recipient&&script->_finaleActorsStarted.size()==finalActors,"Recovery changed shared actors/quest/item/progression");++cases;++releases;};
  for(unsigned step=0;step<11;++step)for(unsigned context=0;context<4;++context){auto pos=context==0?target(0):context==1?target(7):context==2?Position{-828.85f,4372,738.64f,0}:Position{late->Loc.X,late->Loc.Y,-1000,0};exercise(pos,step,false,context);}exercise(target(7),11,true,0);
  for(unsigned step:{1u,4u,6u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1092);scenario->SetCurrentStep(step-1);auto owner=std::make_unique<NativePlayer>(session,map,945,target(7));auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PALADIN);p.SetQuestStatus(42120,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,42120);unsigned event=step==1?50766:step==4?50897:50912;p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,event);Require(scenario->GetCurrentStep()==step-1,"Premature entry/passage event advanced");scenario->SetCurrentStep(step);auto gy=map.GetClosestGraveYard(late->Loc.X,late->Loc.Y,late->Loc.Z);
#ifndef HOLY_GRAVEYARD_BASELINE
   Require(gy&&gy->GetExactDist(target(step))<.001f,"Unaccepted event unlocked later graveyard");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,event);Require(scenario->GetCurrentStep()==step+1,"Real native entry/passage criterion failed");gy=map.GetClosestGraveYard(early->Loc.X,early->Loc.Y,early->Loc.Z);
#ifndef HOLY_GRAVEYARD_BASELINE
   Require(gy&&gy->GetExactDist(target(step+1))<.001f,"Acknowledged passage did not select its checkpoint");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,event);Require(scenario->GetCurrentStep()==step+1&&!p.HasItemCount(128823,1,true),"Duplicate event skipped stage or granted item");++cases;}
  for(unsigned id:{1037u,991u,1146u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,id);auto gy=map.GetClosestGraveYard(12,34,-56);if(id==1037){auto arms=sWorldSafeLocsStore.LookupEntry(5339);Require(gy&&gy->GetExactDist(arms->Loc.X,arms->Loc.Y,arms->Loc.Z)<.001f&&std::fabs(gy->GetOrientation()-arms->Loc.O*float(M_PI)/180)<.001f,"Retained Arms recovery changed");}else Require(gy&&gy->GetExactDist(general->Loc.X,general->Loc.Y,general->Loc.Z)<.001f&&gy->GetOrientation()==0,"Retained shared generic recovery changed");++cases;}sScenarioMgr->RemoveScenario(map.GetInstanceId());auto gy=map.GetClosestGraveYard(12,34,-56);Require(gy&&gy->GetExactDist(general->Loc.X,general->Loc.Y,general->Loc.Z)<.001f,"No-scenario policy changed");++cases;session->SetPlayer(nullptr);delete session;
 }
 Require(cases==104&&releases==90,"Native Holy test count changed");
#ifdef HOLY_GRAVEYARD_BASELINE
 std::cout<<"COUNTERFACTUAL pinned2bd533cfd Holy native104/release90:all class stages revive at generic5281/zero-facing;current native support,not historical binary or class-checkpoint recovery"<<std::endl;
#else
 std::cout<<"PASS Holy native104/release90:actual registered whole1539,all11 stages+real completed11,Human+BloodElf/active statuses/four death contexts,native5488/5693/5694 phase floors/facing,real ghost/release/ACK/half-health/repeated cleanup,real50766 at1->2/50897 at4->5/50912 at6->7/early+duplicate denials,shared1037/991/other/no-scenario retained,no progression/item/actor grants. Earlier states/final criteria controlled;client/corpse/path/pet/group separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
