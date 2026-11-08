#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "MovementPackets.h"
#include "PlayerDefines.h"
#include <array>
#include <memory>
static unsigned Checkpoint(unsigned step){return step>=7?5530:step>=5?5529:step>=2?5528:5518;}
static Position Pose(unsigned id){auto s=sWorldSafeLocsStore.LookupEntry(id);Require(s&&s->ID==id&&s->MapID==1620,"Native Kingslayers safe absent");return {s->Loc.X,s->Loc.Y,s->Loc.Z,s->Loc.O*float(M_PI)/180};}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 // Stormwind city/channel updates and the Horde honorless2479 ACK cast need
 // the ordinary startup string/proc services; do not disable either behavior.
 Require(sObjectMgr->LoadTrinityStrings(),"Native city strings absent");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();
 auto steps=sScenarioMgr->GetScenarioSteps(1123);Require(steps&&steps->size()==10,"Native1123 ten steps absent");
 std::vector<std::vector<unsigned>> assets{{51418,52672},{51435},{51437},{52832,52801},{51549},{51613,51654},{51614},{51615},{51616},{51617}};
 for(unsigned step=0;step<10;++step)for(unsigned asset:assets[step]){bool found=false;for(auto t:sCriteriaTreeStore)if(t->Parent==(*steps)[step]->Criteriatreeid){auto c=sCriteriaStore.LookupEntry(t->CriteriaID);if(c&&c->Type==92&&unsigned(c->Asset)==asset)found=true;}Require(found,"Native Kingslayers boundary changed");}
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}}});unsigned cases=0,releases=0,honorless=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)})for(unsigned quest:{42504u,42627u}){
  auto parent=std::make_unique<MapInstanced>(1620,300);auto mapOwner=std::make_unique<InstanceMap>(1620,300,9999100+race+quest,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;map.CreateInstanceData(nullptr);auto script=dynamic_cast<instance_kingslayers::instance_kingslayers_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Registered whole1620 instance absent");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,1123);
  for(unsigned id:{5518u,5528u,5529u,5530u}){auto pos=Pose(id);auto p=std::make_unique<NativePlayer>(session,map,961,pos);auto floor=map.GetHeight(p->GetPhases(),pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+5);std::cout<<"FLOOR "<<id<<" pose="<<pos.GetPositionZ()<<" native="<<floor<<std::endl;Require(floor<=pos.GetPositionZ()+.5f&&pos.GetPositionZ()-floor<=3.0f,"Native Kingslayers cemetery floor unsupported");}
  for(unsigned step=0;step<11;++step)for(unsigned context=0;context<4;++context){
   sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1123);Position death=context==0?Pose(5518):context==1?Pose(5530):context==2?Position{836.16f,1035.4f,48.61f,0}:Position{0,0,-1000,0};auto owner=std::make_unique<NativePlayer>(session,map,962,death);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p.setFactionForRace(race);p.SetQuestStatus(quest,context%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,quest);scenario->SetCurrentStep(step==10?9:step);
   if(step==10){p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,51617);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==10,"Native exit event did not complete1123");}
   // Native Scenario completion retains the last cached active step9.
   unsigned cached=step==10?9:step;Require(script->getScenarionStep()==cached,"Native cached-step notification changed");std::array<bool,3> guard{};std::array<ObjectGuid,3> guids{};
   for(unsigned i=0;i<3;++i){script->altheaGuards[i].guid=p.GetGUID();script->altheaGuards[i].defeated=(i+context)%2;guard[i]=script->altheaGuards[i].defeated;guids[i]=script->altheaGuards[i].guid;}
   auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto status=p.GetQuestStatus(quest);auto gy=map.GetClosestGraveYard(death.GetPositionX(),death.GetPositionY(),death.GetPositionZ());
#ifdef KINGSLAYERS_GRAVEYARD_BASELINE
   unsigned expectedId=Checkpoint(cached);
#else
   unsigned expectedId=Checkpoint(step);
#endif
   auto expected=Pose(expectedId);Require(gy&&gy->GetMapId()==1620&&gy->GetExactDist(expected)<.001f,"Native Kingslayers checkpoint failed");
#ifdef KINGSLAYERS_GRAVEYARD_BASELINE
   Require(gy->GetOrientation()==0,"Pinned facing changed");
#else
   Require(std::fabs(gy->GetOrientation()-expected.GetOrientation())<.001f,"Native facing lost");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost absent");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native release absent");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2&&p.GetExactDist(expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ()+2)<.001f,"Native release/ACK/health/ghost cleanup failed");if(p.pvpInfo.inHostileArea){Require(p.HasAura(2479),"Native hostile-area ACK lost honorless2479");++honorless;}session->HandleMoveTeleportAck(ack);Require(scenario->GetCurrentStep()==step&&script->getScenarionStep()==cached&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p.GetQuestStatus(quest)==status&&!p.HasItemCount(128870,1,true),"Recovery changed quest/item/criteria/cache");for(unsigned i=0;i<3;++i)Require(script->altheaGuards[i].guid==guids[i]&&script->altheaGuards[i].defeated==guard[i],"Recovery changed guard ledger");++cases;++releases;
  }
  for(unsigned order=0;order<2;++order){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1123);auto p=std::make_unique<NativePlayer>(session,map,963,Pose(5518));p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->SetQuestStatus(quest,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,quest);
   for(unsigned step=0;step<10;++step){auto events=assets[step];if(order)std::reverse(events.begin(),events.end());for(unsigned i=0;i<events.size();++i){p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,events[i]);if(i+1<events.size()){p->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,events[i]);Require(scenario->GetCurrentStep()==step,"Single/duplicate child bypassed native AND");}}Require(scenario->GetCurrentStep()==step+1,"Native ordered step failed");
#ifndef KINGSLAYERS_GRAVEYARD_BASELINE
    Require(script->GetClosestGraveYard(0,0,-1000)->GetExactDist(Pose(Checkpoint(step+1)))<.001f,"Native event did not select correct checkpoint");
#endif
   }Require(scenario->IsCompleted(false),"Native sequence failed completion");++cases;
  }
#ifndef KINGSLAYERS_GRAVEYARD_BASELINE
  for(unsigned id:{1012u,0u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=id?sScenarioMgr->AddScenario(&map,id):nullptr;Require(!script->GetClosestGraveYard(1,2,-1000),"Missing/foreign Scenario selected Rogue cemetery");++cases;}
#endif
  sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
 Require(honorless>0,"Horde hostile-area ACK not exercised");std::cout<<"HONORLESS_ACK "<<honorless<<std::endl;
#ifdef KINGSLAYERS_GRAVEYARD_BASELINE
 Require(cases==184&&releases==176,"Pinned count changed");std::cout<<"COUNTERFACTUAL pinned148c610e4 Kingslayers native184/release176:all10 old checkpoints preserved;natural completed10 retains cached9/5530/facing zero. Old whole instance/current support,not historical binary"<<std::endl;
#else
 Require(cases==192&&releases==176,"Native count changed");std::cout<<"PASS Kingslayers native192/release176:registered whole1620/1123,Human+Orc/both42504+42627/two active statuses/all10 stages+real complete10/four poses,four VMAP floors/facing,real ghost/repop/ACK/half-health/duplicate cleanup,guard ledger unchanged,all native ordered boundaries/AND both orders,foreign+missing null. Earlier stage/quest/cache/ledger/events controlled;full client/combat/pet/group separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
