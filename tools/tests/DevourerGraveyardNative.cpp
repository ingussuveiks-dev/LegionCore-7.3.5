#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "MovementPackets.h"
#include "PlayerDefines.h"
#include <memory>
namespace DevourerGraveyardFixture {
#include "DevourerGraveyardInstance.inc"
}
static unsigned Checkpoint(unsigned step){return step>=5?5470:step>=3?5469:step>=1?5468:5467;}
static Position Pose(unsigned id){auto s=sWorldSafeLocsStore.LookupEntry(id);Require(s&&s->ID==id&&s->MapID==1607,"Native Devourer safe absent");return {s->Loc.X,s->Loc.Y,s->Loc.Z,s->Loc.O*float(M_PI)/180};}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings absent");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();
 auto steps=sScenarioMgr->GetScenarioSteps(1078);Require(steps&&steps->size()==7,"Native1078 seven steps absent");
 using Event=std::pair<unsigned,unsigned>;std::vector<std::vector<Event>> events{{{92,50261}},{{92,54824}},{{92,50908},{28,211687},{92,50536}},{{92,50298}},{{92,50299},{92,50404}},{{92,50300}},{{92,50301}}};
 for(unsigned step=0;step<7;++step)for(auto event:events[step]){bool found=false;for(auto t:sCriteriaTreeStore)if(t->Parent==(*steps)[step]->Criteriatreeid){auto c=sCriteriaStore.LookupEntry(t->CriteriaID);if(c&&unsigned(c->Type)==event.first&&unsigned(c->Asset)==event.second)found=true;}Require(found,"Native Devourer criterion boundary changed");}
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1607,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new DevourerGraveyardFixture::instance_devourer;sScriptMgr->SwapScriptContext(true);unsigned cases=0,releases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  auto parent=std::make_unique<MapInstanced>(1607,300);auto mapOwner=std::make_unique<InstanceMap>(1607,300,9999400+race,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;map.CreateInstanceData(nullptr);auto script=dynamic_cast<DevourerGraveyardFixture::instance_devourer::instance_devourer_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Registered whole1607 instance absent");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,1078);
  for(unsigned id:{5467u,5468u,5469u,5470u}){auto pos=Pose(id);auto p=std::make_unique<NativePlayer>(session,map,971,pos);p->SetPhaseId({7000},false);auto floor=map.GetHeight(p->GetPhases(),pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+5);std::cout<<"FLOOR "<<id<<" pose="<<pos.GetPositionZ()<<" native="<<floor<<std::endl;Require(floor<=pos.GetPositionZ()+.5f&&pos.GetPositionZ()-floor<=3.0f,"Native Devourer cemetery floor unsupported");}
  for(unsigned step=0;step<8;++step)for(unsigned context=0;context<4;++context){
   sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1078);Position death=context==0?Pose(5467):context==1?Pose(5470):context==2?Position{836.16f,1035.4f,48.61f,0}:Position{0,0,-1000,0};auto owner=std::make_unique<NativePlayer>(session,map,972,death);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p.setFactionForRace(race);p.SetPhaseId({7000},false);p.SetQuestStatus(41924,context%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,41924);scenario->SetCurrentStep(step==7?6:step);
   if(step==7){p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50301);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==7,"Native pickup did not complete1078");}
   // Earlier stage/cache and outro fields are controlled inputs; recovery
   // must not advance them or alter the instance's unrelated actor tracking.
   unsigned cached=step==7?6:step;Require(script->getScenarionStep()==cached,"Native cached-step notification changed");script->outrotimer=1375;script->trashGUIDconteiner={nullptr};auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto status=p.GetQuestStatus(41924);auto gy=map.GetClosestGraveYard(death.GetPositionX(),death.GetPositionY(),death.GetPositionZ());
#ifdef DEVOURER_GRAVEYARD_BASELINE
   unsigned expectedId=Checkpoint(cached);
#else
   unsigned expectedId=Checkpoint(step);
#endif
   auto expected=Pose(expectedId);Require(gy&&gy->GetMapId()==1607&&gy->GetExactDist(expected)<.001f,"Native Devourer checkpoint failed");
#ifdef DEVOURER_GRAVEYARD_BASELINE
   Require(gy->GetOrientation()==0,"Pinned facing changed");
#else
   Require(std::fabs(gy->GetOrientation()-expected.GetOrientation())<.001f,"Native facing lost");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost absent");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native release absent");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2&&p.GetExactDist(expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ()+2)<.001f,"Native release/ACK/health/ghost cleanup failed");session->HandleMoveTeleportAck(ack);Require(scenario->GetCurrentStep()==step&&script->getScenarionStep()==cached&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p.GetQuestStatus(41924)==status&&!p.HasItemCount(128476,1,true)&&script->outrotimer==1375&&script->trashGUIDconteiner.size()==1&&!script->trashGUIDconteiner.front(),"Recovery changed quest/item/progress/cache/outro tracking");++cases;++releases;
  }
  for(unsigned order=0;order<2;++order){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1078);auto p=std::make_unique<NativePlayer>(session,map,973,Pose(5467));p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,41924);
   auto emit=[&](Event event){if(event.first==28)scenario->UpdateAchievementCriteria(CriteriaTypes(event.first),event.second,0,0,p.get(),p.get());else p->UpdateAchievementCriteria(CriteriaTypes(event.first),event.second,1);};for(unsigned step=0;step<7;++step){auto chosen=events[step];if(order)std::reverse(chosen.begin(),chosen.end());if(step==1&&order)chosen={{42,136802}};for(unsigned i=0;i<chosen.size();++i){auto event=chosen[i];emit(event);if(i+1<chosen.size()){emit(event);Require(scenario->GetCurrentStep()==step,"Single/duplicate child bypassed native AND");}}if(scenario->GetCurrentStep()!=step+1){std::cerr<<"ORDER_FAILURE step="<<step<<" order="<<order<<" actual="<<scenario->GetCurrentStep()<<std::endl;for(auto const& row:*scenario->GetAchievementMgr().GetCriteriaProgressMap())std::cerr<<"RECEIPT "<<row.first<<" count="<<row.second.Counter<<std::endl;}Require(scenario->GetCurrentStep()==step+1,"Native ordered step failed");
#ifndef DEVOURER_GRAVEYARD_BASELINE
    Require(script->GetClosestGraveYard(0,0,-1000)->GetExactDist(Pose(Checkpoint(step+1)))<.001f,"Accepted event selected wrong cemetery");
#endif
   }Require(scenario->IsCompleted(false),"Native sequence incomplete");++cases;
  }
#ifndef DEVOURER_GRAVEYARD_BASELINE
  for(unsigned id:{1123u,0u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=id?sScenarioMgr->AddScenario(&map,id):nullptr;Require(!script->GetClosestGraveYard(1,2,-1000),"Absent/foreign scenario selected cemetery");++cases;}
#endif
  script->trashGUIDconteiner.clear();sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
#ifdef DEVOURER_GRAVEYARD_BASELINE
 Require(cases==68&&releases==64,"Pinned count changed");std::cout<<"COUNTERFACTUAL pinned3465a1d48 Devourer native68/release64:all0..6 original checkpoints retained,natural completed7 keeps cached6/5470,zero facing. Original whole instance/current support,not historical binary"<<std::endl;
#else
 Require(cases==72&&releases==64,"Native count changed");std::cout<<"PASS Devourer native72/release64:registered whole1607/1078,Human+Orc/two active41924 statuses/all7 stages+real complete7/four poses,four VMAP floors/facing,ghost/repop/ACK/half-health/duplicate cleanup,outro tracking unchanged,all native criteria boundaries/AND both orders and item/event OR variants,foreign+missing null. Earlier state/events controlled;full client/combat/pet/group separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
