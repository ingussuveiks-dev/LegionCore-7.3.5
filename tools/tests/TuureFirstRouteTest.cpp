// Included after the first-fight native host. World/criteria/motion launch are
// adapters; native OnArrived/MovementInform and the full instance are extracted.
struct RouteAI:CreatureAI{uint32 actions=0;void DoAction(int32 action)override{assert(action==1);++actions;}};
struct OldRouteDeath:InstanceScript{
 using InstanceScript::InstanceScript;
#include "OldRouteDeath.inc"
};
void PrepareRoute(Run& r,bool nativeKill=true){
 scenario.step=2;scenario.achievement.progress[47519].Counter=1;
 if(nativeKill)scenario.achievement.progress[47883].Counter=1;
 r.jace.dbGuid=361432;r.script.OnCreatureCreate(&r.jace);
}
void Arrive(Creature& actor,uint32 index){actor.movement._currentNode=index;actor.movement.OnArrived(actor);}
void CheckRoute(){
#ifdef ROUTE_BASELINE
 {Run r;PrepareRoute(r,false);RouteAI ai;r.boros->primary=&ai;OldRouteDeath old(&r.map);
  old.OnUnitDeath(r.target.get());old.OnUnitDeath(r.target.get());
  assert(r.target->alive&&ai.actions==2&&!scenario.achievement.progress[47883].Counter);
  r.target->dbGuid=0;old.OnUnitDeath(r.target.get());assert(ai.actions==3);
 }
 std::cout<<"PASS labelled426827bf route counterfactual: state-only/live/wrong-spawn/repeated106221 death dispatches Boros DoAction1 without real kill or native47883. Null-AI dereference identified in pinned source, not executed as a passing runtime.\n";
#else
 uint32 variants=0,missingNative=0;
 for(uint32 status:{1u,3u})for(bool alive:{false,true})for(bool nativeKill:{false,true})for(int killer=0;killer<7;++killer){
  Run r;PrepareRoute(r,nativeKill);r.player.quest.Status=status;r.player.alive=alive;
  Unit* killers[]={&r.player,&r.pet,&r.guardian,&r.charmed,r.boros.get(),&r.jace,&r.mage};
  killers[killer]->Kill(r.target.get());r.script.CreatureDies(r.target.get(),killers[killer]);
  if(!nativeKill&&killer<4){r.script.Update(30000);assert(!r.boros->pathMoves&&!scenario.killDeliveries&&!scenario.routeDeliveries);++missingNative;continue;}
  r.script.Update(999);assert(!r.boros->pathMoves&&!scenario.routeDeliveries);
  r.script.Update(1);assert(r.boros->pathMoves==1&&r.boros->waypoint==uint32(-1));
  assert(scenario.achievement.progress[47883].Counter==1&&scenario.killDeliveries==uint32(!nativeKill));
  r.script.Update(20000);assert(!scenario.routeDeliveries); // time/start is never arrival
  Arrive(*r.boros,5);r.script.Update(0);assert(r.boros->conversations==1);
  Arrive(*r.boros,6);r.script.Update(0);assert(r.boros->conversations==2&&!scenario.routeDeliveries);
  Arrive(*r.boros,9);r.script.Update(0);assert(!scenario.routeDeliveries);
  Arrive(*r.boros,10);r.script.Update(0);assert(scenario.routeDeliveries==1&&scenario.step==3);
  r.script.Update(20000);assert(r.boros->pathMoves==1&&scenario.routeDeliveries==1);++variants;
 }
 for(int invalid=0;invalid<17;++invalid){Run r;PrepareRoute(r);switch(invalid){
  case 0:r.target->dbGuid=0;break;case 1:r.target->entry=106201;break;case 2:scenario.id=972;break;
  case 3:scenario.id=940;break;case 4:scenario.id=1145;break;case 5:scenario.id=1146;break;
  case 6:scenario.step=1;break;case 7:scenario.achievement.progress.clear();break;
  case 8:r.player.classId=4;break;case 9:r.player.quest.Status=0;break;case 10:r.player.rewarded=true;break;
  case 11:r.player.gm=true;break;case 12:r.player.phase=false;break;case 13:r.player.near=false;break;
  case 14:r.player.contact=false;break;case 15:r.player.pending=true;break;case 16:r.pet.owner={1,999};break;
 }r.pet.Kill(r.target.get());r.script.Update(30000);assert(!r.boros->pathMoves&&!scenario.killDeliveries&&!scenario.routeDeliveries);}
 {Run r;PrepareRoute(r);r.script.OnUnitDeath(r.target.get());r.script.Update(30000);assert(r.target->alive&&!r.boros->pathMoves);}
 {Run r;PrepareRoute(r);r.script.CreatureDies(r.target.get(),&r.pet);r.script.Update(30000);assert(!r.boros->pathMoves);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.boros->states=8;r.script.Update(1000);assert(!r.boros->pathMoves);r.boros->states=0;r.script.Update(1000);assert(r.boros->pathMoves==1);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.boros->charmer={1,99};r.script.Update(1000);assert(!r.boros->pathMoves);r.boros->charmer.Clear();r.script.Update(1000);assert(r.boros->pathMoves==1);}
 {Run r;PrepareRoute(r,false);r.jace.Kill(r.target.get());r.Remove(r.target);r.Remove(r.boros);r.script.Update(1000);r.Make(r.boros,106134,361419,11);r.script.Update(1000);assert(r.boros->pathMoves==1&&scenario.killDeliveries==1);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.script.Update(1000);Boros ai(r.boros.get());ai.Reset();r.boros->primary=nullptr;Arrive(*r.boros,10);r.script.Update(0);assert(scenario.routeDeliveries==1);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.script.Update(1000);r.Remove(r.boros);r.Make(r.boros,106134,361419,12);r.script.Update(1000);assert(!r.boros->pathMoves);r.script.Update(1000);assert(r.boros->pathMoves==1&&!scenario.routeDeliveries);Arrive(*r.boros,10);r.script.Update(0);assert(scenario.routeDeliveries==1);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.script.Update(1000);r.boros->movement._pathId=9859003;Arrive(*r.boros,10);r.script.Update(0);assert(!scenario.routeDeliveries);r.script.Update(1000);assert(r.boros->pathMoves==2&&r.boros->waypoint==uint32(-1));}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());waypointMgr.missing=true;r.script.Update(1000);assert(!r.boros->pathMoves&&warnings==1);waypointMgr.missing=false;r.script.Update(4999);assert(!r.boros->pathMoves);r.script.Update(1);assert(r.boros->pathMoves==1);}
 {Run r;PrepareRoute(r,false);r.jace.Kill(r.target.get());scenario.rejectKill=true;r.script.Update(1000);assert(!r.boros->pathMoves&&warnings==1);scenario.rejectKill=false;r.script.Update(4999);assert(scenario.killDeliveries==1);r.script.Update(1);assert(scenario.killDeliveries==2&&!r.boros->pathMoves);r.script.Update(1000);assert(r.boros->pathMoves==1);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());auto held=r.player.quest.ScriptLifetime;r.player.quest={};r.script.Update(1000);assert(!r.boros->pathMoves&&!scenario.killDeliveries);r.target->alive=true;r.pet.Kill(r.target.get());r.script.Update(1000);assert(r.boros->pathMoves==1&&r.player.quest.ScriptLifetime!=held);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.script.Update(1000);r.player.gm=true;Arrive(*r.boros,10);r.script.Update(0);assert(!scenario.routeDeliveries);r.player.gm=false;r.script.Update(0);assert(scenario.routeDeliveries==1);}
 {Run r;PrepareRoute(r,false);r.pet.Kill(r.target.get());r.script.Update(1000);assert(!r.boros->pathMoves&&!scenario.killDeliveries);scenario.achievement.progress[47883].Counter=1;r.script.Update(5000);assert(r.boros->pathMoves==1&&!scenario.killDeliveries);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.script.Update(1000);r.player.rejectCredit=true;Arrive(*r.boros,10);r.script.Update(0);assert(!scenario.routeDeliveries&&warnings==1);r.player.rejectCredit=false;r.script.Update(4999);assert(!scenario.routeDeliveries);r.script.Update(1);assert(scenario.routeDeliveries==1);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.script.Update(1000);r.player.near=false;Arrive(*r.boros,12);r.script.Update(0);assert(!scenario.routeDeliveries);r.player.near=true;r.script.Update(0);assert(scenario.routeDeliveries==1);}
 {Run r;PrepareRoute(r,false);r.jace.Kill(r.target.get());r.target->alive=true;r.script.OnCreatureCreate(r.target.get());r.script.Update(1000);assert(!r.boros->pathMoves&&!scenario.killDeliveries);}
 {Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.script.Update(1000);r.boros->failRoute=true;r.boros->motionPresent=false;r.script.Update(0);r.script.Update(1000);assert(r.boros->pathMoves==2&&!scenario.routeDeliveries);r.boros->failRoute=false;r.script.Update(0);r.script.Update(1000);Arrive(*r.boros,10);r.script.Update(0);assert(scenario.routeDeliveries==1);}
 {Run r;PrepareRoute(r,false);r.jace.Kill(r.target.get());r.player.onCredit=[&]{r.script.Update(1000);r.Remove(r.boros);};r.script.Update(1000);assert(scenario.killDeliveries==1&&!scenario.routeDeliveries);r.player.onCredit={};r.Make(r.boros,106134,361419,13);r.script.Update(1000);assert(r.boros->pathMoves==1);}
 {Run r;PrepareRoute(r,false);r.jace.Kill(r.target.get());r.player.onCredit=[&]{r.player.quest={};};r.script.Update(1000);assert(scenario.killDeliveries==1&&!r.boros->pathMoves);r.player.onCredit={};r.script.Update(1000);assert(!r.boros->pathMoves&&!scenario.routeDeliveries);}
 {Run r;PrepareRoute(r,false);r.jace.Kill(r.target.get());scenario.rejectKill=true;r.script.Update(1000);assert(warnings==1);scenario.rejectKill=false;waypointMgr.missing=true;r.script.Update(5000);assert(warnings==2);waypointMgr.missing=false;r.script.Update(5000);r.player.rejectCredit=true;Arrive(*r.boros,10);r.script.Update(0);assert(warnings==3&&!scenario.routeDeliveries);r.player.rejectCredit=false;r.script.Update(5000);assert(scenario.routeDeliveries==1);}
 for(uint32 node:{6u,10u}){Run r;PrepareRoute(r);r.pet.Kill(r.target.get());r.script.Update(1000);r.boros->onConversation=[&]{r.Remove(r.boros);};Arrive(*r.boros,node);r.script.Update(0);assert(!r.boros&&!scenario.routeDeliveries);r.Make(r.boros,106134,361419,14);r.script.Update(1000);if(node==10)assert(scenario.routeDeliveries==1);else assert(!scenario.routeDeliveries);}
 std::cout<<"PASS Tuure full actual second-death/route controller "<<variants<<" accepted and "<<missingNative<<" missing-native-owner-reward denials; native/forwarded kill x live/dead/status/player-pet-guardian-charmer-three-ally variants; no state/start/time credit, actual native OnArrived indexes6/10 without AI, accepted47883/47882, reset/replacement/interruption/missing path/corpse/quest retake/reentrant/GM and5s retry. Native motion launch, reward delivery and criteria application use documented adapters; actual route/client pending.\n";
#endif
}
