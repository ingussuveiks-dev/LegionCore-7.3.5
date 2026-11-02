// Actual production controller, full actor and instance, native OnArrived,
// PathMovementBase, Timer, scenario admission and Map-before-instance dispatch.
// Positions, motion installation/retirement, players and criteria writes are
// controlled endpoints. Separate native geometry probe tests the six mmap legs.
#define MONK_CHEN_ARRIVAL_TEST
#include "MonkChenRescueTest.cpp"
#include "ArrivalBaselineActor.inc"

struct Escort:Run
{
 void Begin(){Death(&guardian);Started();Tick(8000);assert(state().routeIssued);assert(chen.GetCurrentWaypointID()==uint32(-1));}
 void Endpoint(){auto const& p=waypointMgr.data.back();chen.x=p.x;chen.y=p.y;chen.z=p.z;chen.spline.done=true;chen.motion.waypoint.Node(5);}
 void Arrive(){Endpoint();chen.motion.waypoint.OnArrived(chen);}
 void Finished(){assert(state().arrivalDone&&state().arrivalClaim.IsEmpty()&&state().arrivalPlayer.IsEmpty());assert(scenario.step==5&&scenario.progress[29070].Counter==1);assert(actor()->_arrivalStarted);assert(player.credits==(std::vector<uint32>{46956,46947}));}
 void NoArrival(){assert(!state().arrivalDone&&!actor()->_arrivalStarted&&scenario.step==4&&!scenario.progress[29070].Counter);assert(player.credits==std::vector<uint32>{46956});}
};

int main()
{
 uint32 retained=RunRescueCases(),cases=0;
 // Complete predecessor loses the native receipt when its nearest player is
 // absent. No second arrival is invented when that original player returns.
 {Escort r;delete r.chen.i_AI;r.chen.i_AI=new npc_monk_chen_porter_arrival_baseline::npc_monk_chen_porter_arrival_baselineAI(&r.chen);r.Death(&r.pet);r.Tick(8000);r.chen.nearest=nullptr;r.Endpoint();r.chen.motion.waypoint.OnArrived(r.chen);r.chen.motion.Retire();r.chen.nearest=&r.player;r.Tick(30000);assert(r.scenario.step==4&&!r.scenario.progress[29070].Counter&&r.player.credits==std::vector<uint32>{46956});std::cout<<"COUNTERFACTUAL pinned b015d4e28 full actor: actual last-node dispatch with absent nearest player loses46947; controlled endpoints, not a client reproduction.\n";}
 {Escort r;r.Begin();r.other.status=0;r.chen.nearest=&r.other;r.Arrive();r.Finished();assert(r.chen.GetCurrentWaypointID()==5&&r.other.credits.empty());
  r.Tick(2999);assert(r.hait.talks.empty());r.Tick(1);assert(r.hait.talks.size()==1&&r.hait.talks[0].first==10);
  r.Tick(3999);assert(r.chen.talks.back().first==7);r.Tick(1);assert(r.chen.talks.back()==(std::pair<uint32,ObjectGuid>{8,r.player.guid}));
  r.Tick(3999);assert(r.hait.talks.size()==1);r.Tick(1);assert(r.hait.talks.size()==2);
  r.Tick(3999);assert(r.hait.motion.paths.empty());r.Tick(1);assert(r.hait.talks.size()==4&&r.hait.motion.paths==std::vector<uint32>{9859006});
  r.Tick(999);assert(!r.chen.motion.points);r.Tick(1);assert(r.chen.motion.points==1);for(uint32 i=1;i<4;++i)assert(r.hait.talks[i].second==r.player.guid);
  for(uint32 i=0;i<100;++i){r.actor()->MovementInform(2,5);r.state().Arrived(r.chen.guid);r.state().Update(100000);}r.Finished();assert(r.hait.talks.size()==4&&r.chen.motion.points==1);++cases;}
 // Timer expiry, raw callbacks, stale IDs and endpoint proximity cannot replace
 // the receipt of an actually issued native script route.
 for(uint32 bad=0;bad<13;++bad){Escort r;r.Begin();r.Endpoint();switch(bad){
  case 0:r.state().routeIssued=false;break;case 1:r.chen.motion.waypoint._pathId=9859006;break;
  case 2:r.chen.motion.waypoint.Node(4);break;case 3:r.chen.spline.done=false;break;
  case 4:r.chen.x+=0.51f;break;case 5:r.chen.motion.current=nullptr;break;
  case 6:waypointMgr.available=false;break;case 7:waypointMgr.data.resize(5);break;
  case 8:r.chen.motion.Retire();break;case 9:r.chen.UpdateWaypointID(5);r.chen.spline.done=false;break;
  case 10:r.chen.motion.waypoint._pathId=0;r.chen.UpdateWaypointID(5);break;
  case 11:r.chen.world=false;break;case 12:r.chen.spawn=362134;break;}
  // Wrong-copy actors are tested through the controller, since their native
  // unscoped behavior deliberately remains outside this private-instance fix.
  if(bad!=12)r.actor()->MovementInform(2,5);else r.state().Arrived(r.chen.guid);
  r.Tick(50000);r.NoArrival();++cases;}
 {Escort r;r.Death();r.Endpoint();r.actor()->MovementInform(2,5);r.Tick(7999);r.NoArrival();assert(!r.state().routeIssued);r.Tick(1);assert(r.state().routeIssued&&r.chen.GetCurrentWaypointID()==uint32(-1)&&!r.chen.spline.done);r.NoArrival();++cases;}
 // OnArrived persists the true zero-based final node even if AI dispatch is
 // unavailable. The original generator may retire before the instance update.
 for(uint32 mode=0;mode<4;++mode){Escort r;r.Begin();auto saved=r.chen.i_AI;r.chen.i_AI=nullptr;if(mode==1||mode==2)r.chen.i_disabledAI=saved;r.Endpoint();r.chen.motion.waypoint.OnArrived(r.chen);assert(r.chen.GetCurrentWaypointID()==5);r.chen.motion.Retire();if(mode==3){delete saved;saved=new Chen(&r.chen);}if(mode==0||mode==3)r.chen.i_AI=saved;
  r.Tick(0);r.Finished();if(mode==2){r.chen.i_AI=saved;r.chen.i_disabledAI=nullptr;}++cases;}
 {Escort r;r.Begin();auto saved=r.chen.i_AI;r.chen.i_AI=nullptr;r.Endpoint();r.chen.motion.waypoint.OnArrived(r.chen);r.chen.motion.Retire();r.Tick(0);assert(r.state().arrivalSeen&&!r.state().arrivalDone);r.chen.i_AI=saved;r.Tick(1999);r.NoArrival();r.Tick(1);r.Finished();++cases;}
 // Original weak quest ownership survives a temporary visibility/range gate;
 // unrelated nearest players, including a GM, never inherit the receipt.
 for(uint32 gate=0;gate<8;++gate){Escort r;r.Begin();r.other.gm=true;r.chen.nearest=&r.other;
  switch(gate){case 0:r.player.distance=101;break;case 1:r.player.phase=false;break;case 2:r.chen.hidden=true;break;case 3:r.player.world=false;break;case 4:r.player.contact=false;break;case 5:r.player.teleport=true;break;case 6:r.player.gm=true;break;case 7:r.chen.near=false;break;}
  r.Arrive();r.NoArrival();assert(r.state().arrivalSeen);r.player.distance=100;r.player.phase=true;r.chen.hidden=false;r.player.world=true;r.player.contact=true;r.player.teleport=false;r.player.gm=false;r.chen.near=true;r.Tick(1999);r.NoArrival();r.Tick(1);r.Finished();assert(r.other.credits.empty());++cases;}
 {Escort r;r.Begin();r.player.accept=false;r.Arrive();assert(r.player.credits==(std::vector<uint32>{46956,46947})&&!r.state().arrivalDone);for(uint32 i=0;i<100;++i){r.actor()->MovementInform(2,5);r.state().Update(999999);}assert(r.player.credits.size()==2);r.player.accept=true;r.Tick(1999);assert(r.player.credits.size()==2);r.Tick(1);assert(r.player.credits==(std::vector<uint32>{46956,46947,46947})&&r.state().arrivalDone);++cases;}
 {Escort r;r.Begin();r.player.onCredit=[&]{r.player.distance=101;};r.Arrive();assert(r.scenario.step==5&&r.scenario.progress[29070].Counter&&!r.state().arrivalDone);r.player.onCredit={};r.player.distance=10;r.Tick(2000);r.Finished();++cases;}
 for(uint32 cancel=0;cancel<8;++cancel){Escort r;r.Begin();r.player.distance=101;r.Arrive();switch(cancel){
  case 0:r.player.quest.ScriptLifetime.reset();break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;
  case 2:r.player.map=nullptr;break;case 3:r.map.unloading=true;break;case 4:r.scenario.id=999;break;
  case 5:r.scenario.step=3;break;case 6:r.player.status=0;break;case 7:r.scenario.step=6;break;}
  r.Tick(2000);assert(r.state().arrivalDone&&!r.actor()->_arrivalStarted&&r.player.credits==std::vector<uint32>{46956});r.player.map=&r.map;r.map.unloading=false;r.scenario.id=943;r.scenario.step=4;r.player.status=3;r.player.distance=10;r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.Tick(30000);assert(!r.actor()->_arrivalStarted&&r.player.credits==std::vector<uint32>{46956});++cases;}
 for(uint32 reentry=0;reentry<6;++reentry){Escort r;r.Begin();r.player.onCredit=[&]{switch(reentry){case 0:r.state().Update(0);break;case 1:r.state().Cancel();break;case 2:delete r.chen.i_AI;r.chen.i_AI=new Chen(&r.chen);break;case 3:manager.current=nullptr;break;case 4:r.scenario.step=3;break;case 5:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;}};
  r.Arrive();r.player.onCredit={};if(reentry==0||reentry==2)r.Finished();else assert(!r.actor()->_arrivalStarted&&!r.state().arrivalClaim.low);++cases;}
 {Escort r;r.Begin();r.player.onCredit=[&]{r.player.distance=101;};r.Arrive();r.player.onCredit={};assert(r.scenario.step==5);r.scenario.step=4;r.player.distance=10;r.Tick(2000);assert(r.state().arrivalDone&&!r.actor()->_arrivalStarted&&r.player.credits.size()==2);++cases;}
 {Escort r;r.Begin();r.player.distance=101;r.Arrive();r.map.creatures.erase(r.chen.guid);r.player.distance=10;r.Tick(2000);assert(!r.state().arrivalDone);r.map.creatures[r.chen.guid]=&r.chen;r.Tick(2000);r.Finished();++cases;}
 // Large Map frames must not spend pre-arrival time a second time. The native
 // Timer helper is also exercised across its uint32 rollover convention.
 for(bool wrap:{false,true}){Escort r;r.Begin();r.player.distance=101;if(wrap)clockNow=0xFFFFFE00;r.map.onUpdate=[&](uint32){r.Arrive();};r.map.i_data=&r.instance;clockNow+=100000;r.map.Update(100000);assert(r.state().arrivalWaiting);r.player.distance=10;for(uint32 i=0;i<1999;++i){++clockNow;r.state().Update(100000);}r.NoArrival();++clockNow;r.state().Update(0);r.Finished();++cases;}
 {Escort r;r.Begin();clockNow=0xFFFFFE00;r.player.distance=101;r.Arrive();r.player.distance=10;clockNow+=2000;r.state().Update(0);r.NoArrival();++clockNow;r.state().Update(0);r.Finished();++cases;}
 {Escort r;r.Begin();r.map.players={{&r.other},{&r.player}};r.other.quest.ScriptLifetime=std::make_shared<uint8>();r.chen.nearest=&r.other;r.Arrive();r.Finished();r.player.world=false;r.Tick(20000);for(uint32 i=1;i<4;++i)assert(r.hait.talks[i].second==r.player.guid);assert(r.other.credits.empty());++cases;}
 std::cout<<"PASS "<<cases<<" Chen arrival ASAN contexts plus "<<retained<<" retained rescue contexts; actual native waypoint dispatch/index/persistent receipt, original weak owner, exactly-once native46947/29070 and six authored outro deadlines. Endpoint installation/geometry/criteria/network controlled; native mmap probe separate; later Hait/boss/full943 and client traversal remain OPEN.\n";
}
