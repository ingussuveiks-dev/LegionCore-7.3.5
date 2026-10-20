#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
constexpr uint32 QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,SMART_EVENT_OOC_LOS=10,CONDITION_TYPE_MASK=32;
struct Creature;struct CreatureAI;
struct Unit {bool player=false;float distance=0;bool isType(uint32 mask){return (player?16u:8u)&mask;}};
struct Player:Unit {
 bool world=true,alive=true,teleport=false,phase=true,rewarded=false;uint32 map=1533,quest=QUEST_STATUS_INCOMPLETE;Creature* marker=nullptr;
 Player(){player=true;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}
 uint32 GetMapId(){return map;}uint32 GetQuestStatus(uint32 id){assert(id==40623);return quest;}
 bool GetQuestRewardStatus(uint32 id){assert(id==40623);return rewarded;}
 Creature* FindNearestCreature(uint32 id,float range){assert(id==100850&&range==48);return distance<=range?marker:nullptr;}
 bool InSamePhase(Creature*){return phase;}
};
struct Creature {
 uint32 db=369334;bool IsAIEnabled=true,combat=false,los=true;CreatureAI* ai=nullptr;
 uint32 GetDBTableGUIDLow(){return db;}CreatureAI* AI(){return ai;}
 bool isInCombat(){return combat;}bool IsWithinDistInMap(Unit* u,float range){return u&&u->distance<=range;}
 bool IsWithinLOSInMap(Unit*){return los;}bool IsHostileTo(Unit*){return true;}
};
bool PlayerCondition(Unit* object){bool condMeets=false;uint32 ConditionValue1=16;switch(CONDITION_TYPE_MASK){
#include "EntrancePlayerCondition.inc"
}return condMeets;}
struct CreatureAI {
 Creature* me;bool runOnce=false,m_MoveInLineOfSight_locked=false,optionalCastFails=false;uint32 step=0,credits=0,guides=0,cleanups=0;
 struct Event{struct{struct{uint32 maxDist=48,noHostile=0,cooldownMin=0,cooldownMax=0;}los;}event;}e;
 void RecalcTimer(Event&,uint32,uint32){}
 void ProcessAction(Event&,Unit* unit){
  assert(unit->player);runOnce=true;
  // Ordered SQL chain, asserted against the migration by the PowerShell test:
  // native criterion -> instance guide request at step1 -> optional199627.
  // Actual idempotence and mandatory retirement execute in the separate full
  // instance and native-controller tests; these are event action counters.
  ++credits;step=1;assert(step==1);++guides;if(!optionalCastFails)++cleanups;
 }
 void MoveInLineOfSight(Unit* unit){if(step||runOnce||!PlayerCondition(unit))return;switch(SMART_EVENT_OOC_LOS){
#include "EntranceLos.inc"
 }}
 void MoveInLineOfSight_Safe(Unit*);
};
#include "ArrivalSafeLos.inc"
#include "DarkRidersArrival.inc"
int main(){
 Player p;Creature marker;CreatureAI ai{&marker};marker.ai=&ai;p.marker=&marker;Unit pet,earlyFollower;
 ai.MoveInLineOfSight_Safe(&earlyFollower);ai.MoveInLineOfSight_Safe(&pet);assert(!ai.runOnce&&!ai.credits);
 p.distance=60;assert(!DarkRidersArrival::CheckArrival(&p,988,0));
 // Gateway places the player at the static Ariden without a fresh movement
 // callback. No follower is present in this fixture or needed by production.
 p.distance=0;assert(DarkRidersArrival::CheckArrival(&p,988,0));assert(ai.step==1&&ai.credits==1&&ai.guides==1&&ai.cleanups==1);
 for(int i=0;i<20;++i){DarkRidersArrival::CheckArrival(&p,988,ai.step);ai.MoveInLineOfSight_Safe(&p);}
 assert(ai.credits==1&&ai.guides==1&&ai.cleanups==1);
 auto reset=[&]{p=Player{};p.marker=&marker;marker=Creature{};marker.ai=&ai;ai=CreatureAI{&marker};};
 reset();p.distance=43.6f;ai.optionalCastFails=true;DarkRidersArrival::CheckArrival(&p,988,0);assert(ai.credits==1&&ai.guides==1&&!ai.cleanups);
 reset();marker.los=false;DarkRidersArrival::CheckArrival(&p,988,0);assert(!ai.credits);marker.los=true;DarkRidersArrival::CheckArrival(&p,988,0);assert(ai.credits==1);
 reset();marker.combat=true;DarkRidersArrival::CheckArrival(&p,988,0);assert(!ai.credits);marker.combat=false;DarkRidersArrival::CheckArrival(&p,988,0);assert(ai.credits==1);
 for(int branch=0;branch<10;++branch){reset();switch(branch){
  case 0:p.alive=false;break;case 1:p.world=false;break;case 2:p.teleport=true;break;case 3:p.map=1220;break;
  case 4:p.quest=0;break;case 5:p.phase=false;break;case 6:marker.db=371684;break;case 7:marker.IsAIEnabled=false;break;
  case 8:p.rewarded=true;break;case 9:p.marker=nullptr;break;
 }assert(!DarkRidersArrival::CheckArrival(&p,988,0)&&!ai.credits);}
 reset();assert(!DarkRidersArrival::CheckArrival(nullptr,988,0));
 for(uint32 scenario:{1026u,1014u,0u})assert(!DarkRidersArrival::CheckArrival(&p,scenario,0));
 for(uint32 step=1;step<7;++step)assert(!DarkRidersArrival::CheckArrival(&p,988,step));
 p.quest=QUEST_STATUS_COMPLETE;assert(DarkRidersArrival::CheckArrival(&p,988,0)&&ai.credits==1);
 std::cout<<"PASS: production stationary Gateway arrival, native safe LOS/Player admission, pet/early/missing follower, exact-once transition/guide, optional failure and retry/isolation boundaries\n";
}
