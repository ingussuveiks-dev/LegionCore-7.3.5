// Real CreatureAI/Threat/MotionMaster/waypoint generators with the full Erunak
// actor. Corpse creation and earlier scenario admission are controlled.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "ScriptedGossip.h"
#include "GossipData.h"
#include "FinalePolicy.inc"
#include "WaypointManager.h"
#include "WaypointMovementGenerator.h"
struct EvadeActor : Actor
{
 using Actor::Actor;
 void Bind(uint64 spawn){this->*Access(AltheaGuardDbTag{})=spawn;}
 void AddToWorld() override {Actor::AddToWorld();if(GetEntry()==102826)Require(AIM_Initialize(new npc_erunak_stonespeaker::npc_erunak_stonespeakerAI(this)),"Whole Erunak AI absent");}
};
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);sWaypointMgr->Load();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 auto rows=WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE map=1600 AND id=102826 AND guid IN(362063,362093) ORDER BY guid");Require(rows&&rows->GetRowCount()==2,"Both real Erunak spawns missing");
 uint32 cases=0;do{auto f=rows->Fetch();auto spawn=f[0].GetUInt64();Position pose{f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()};
 for(uint32 mode=0;mode<9;++mode){
#ifdef ERUNAK_EVADE_BASELINE
  if(mode)continue;
#endif
  MapInstanced parent(1600,300);InstanceMap map(1600,300,9987200+cases,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,1066);auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();
  {EvadeActor guide(&map,102826,8100,pose);guide.Bind(spawn);guide.SetPhaseId({6120},false);map.LoadGrid(pose.GetPositionX(),pose.GetPositionY());Require(map.AddToMap(static_cast<Creature*>(&guide)),"Guide admission failed");auto ai=static_cast<npc_erunak_stonespeaker::npc_erunak_stonespeakerAI*>(guide.AI());
   Position enemyPose=pose;enemyPose.m_positionX+=8;EvadeActor enemy(&map,102792,8101,enemyPose);enemy.SetPhaseId({6120},false);Require(map.AddToMap(static_cast<Creature*>(&enemy)),"Enemy admission failed");
   auto motion=guide.GetMotionMaster();uint32 path=spawn==362063?10993809:10993810;Require(sWaypointMgr->GetPathScript(path),"Authored route missing");
   if(mode==1)motion->MovePath(path,false); // already initialized before combat
   ai->AttackStart(&enemy);Require(motion->GetCurrentMovementGeneratorType()==CHASE_MOTION_TYPE&&guide.isInCombat(),"Actual combat/chase not installed");
   if(mode!=1)motion->MovePath(path,false);auto waypoint=motion->GetMotionSlot(MOTION_SLOT_IDLE);Require(waypoint&&waypoint->GetMovementGeneratorType()==WAYPOINT_MOTION_TYPE,"Underlying authored route missing");
   if(mode==2){motion->MovePoint(919,pose.GetPositionX()+2,pose.GetPositionY(),pose.GetPositionZ());}
   if(mode==3||mode==5)motion->MoveDistract(50);auto controlled=motion->GetMotionSlot(MOTION_SLOT_CONTROLLED);
   if(mode==4){ai->wait_heal=true;guide.AddAura(208093,&guide);guide.AddUnitState(UNIT_STATE_STUNNED);}
   ai->_functions.AddFunction([&](){++cases;},ai->_functions.CalculateTime(1));
   enemy.setDeathState(JUST_DIED); // Controlled corpse; no fabricated victory.
   if(mode==8)ai->UpdateAI(1);else ai->EnterEvadeMode();
#ifdef ERUNAK_EVADE_BASELINE
   motion->UpdateMotion(1);Require(motion->GetCurrentMovementGeneratorType()==CHASE_MOTION_TYPE&&motion->GetMotionSlot(MOTION_SLOT_IDLE)==waypoint,"Pinned empty evade no longer strands route");++cases;std::cout<<"COUNTERFACTUAL spawn="<<spawn<<" retained dead-target chase above waypoint; expected broken\n";
#else
   Require(!guide.isInCombat()&&!guide.getVictim()&&guide.getThreatManager().isThreatListEmpty(),"Evade retained actual combat/threat");
   if(mode==3||mode==5){Require(motion->GetMotionSlot(MOTION_SLOT_CONTROLLED)==controlled&&motion->GetCurrentMovementGeneratorType()==DISTRACT_MOTION_TYPE,"Evade erased controlled movement");if(mode==3)motion->UpdateMotion(51);}
   if(mode==5){ // a genuinely new combat must keep its own new chase
    enemy.setDeathState(ALIVE);enemy.SetHealth(enemy.GetMaxHealth());ai->AttackStart(&enemy);ai->UpdateLifecycle(1);motion->UpdateMotion(51);ai->UpdateLifecycle(0);Require(guide.isInCombat()&&motion->GetCurrentMovementGeneratorType()==CHASE_MOTION_TYPE,"Deferred cleanup removed new combat");ai->EnterEvadeMode();
   }
   ai->UpdateLifecycle(1);
   if(mode==2)Require(motion->GetCurrentMovementGeneratorType()==POINT_MOTION_TYPE,"Evade erased scripted point");
   else Require(motion->GetCurrentMovementGeneratorType()==WAYPOINT_MOTION_TYPE&&motion->GetMotionSlot(MOTION_SLOT_IDLE)==waypoint&&static_cast<WaypointMovementGenerator<Creature>*>(waypoint)->GetPathId()==path,"Evade failed to resume same authored route");
   if(mode!=2&&mode!=4){
    // A newly initialized native waypoint intentionally waits3000ms. Advance
    // that generator clock; do not mistake an old chase spline for resumption.
    motion->UpdateMotion(3001);Require(!guide.movespline->Finalized(),"Uncovered authored route did not launch actual movement");
    auto end=guide.movespline->FinalDestination();auto const& first=sWaypointMgr->GetPathScript(path)->front();
    auto dx=end.x-first.x,dy=end.y-first.y,dz=end.z-first.z;Require(dx*dx+dy*dy+dz*dz<=1.0f,"Resumed spline targets a stale chase instead of its actual waypoint");
   }
   Require(cases%2==1,"Evade erased narrative function clock");
   if(mode==4)Require(ai->wait_heal&&guide.HasAura(208093)&&guide.HasUnitState(UNIT_STATE_STUNNED),"Evade erased real wound/healing requirement");
   if(mode==6){auto top=motion->top();ai->EnterEvadeMode();ai->UpdateLifecycle(1);Require(top==motion->top(),"Duplicate evade replaced resumed route");}
   if(mode==7){guide.setDeathState(JUST_DIED);auto top=motion->top();ai->EnterEvadeMode();Require(top==motion->top(),"Dead guide changed movement");}
   Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&scenario->GetCurrentStep()==0,"Evade fabricated quest/scenario progress");++cases;std::cout<<"CASE spawn="<<spawn<<" mode="<<mode<<" native combat/route/control/wound/story PASS\n";
#endif
  }
  sScenarioMgr->RemoveScenario(map.GetInstanceId());map.UnloadAll();
 }
 }while(rows->NextRow());
#ifdef ERUNAK_EVADE_BASELINE
 std::cout<<"PASS expected-broken2 native corpse/chase controls;not repaired gameplay\n";
#else
 Require(cases==36,"Missing native contexts or narrative callbacks");std::cout<<"PASS18 native Erunak evade contexts plus18 preserved callbacks;no client/full-quest claim\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
