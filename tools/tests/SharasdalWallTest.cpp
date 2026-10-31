#include "NativeHost.inc"
#include "Policy.inc"
#include "Actor.inc"
using Grash=npc_grash::npc_grashAI;
struct EntryBridge{
 Map* instance=&liveMap;ObjectGuid lowerGrash{2,362066};
#include "Reentry.inc"
};
struct Run{
 Player owner,neighbor;Creature actor;GameObject wall;std::unique_ptr<Grash> ai;npc_grash script;
 Run(){scenario=Scenario{};scenario.step=2;warnings=0;InstanceScript::opens=0;InstanceScript::opensSucceed=true;
  owner.guid={1,1};neighbor.guid={1,2};ObjectAccessor::players={&owner,&neighbor};
  actor.health=actor.maximum=1000;actor.guid={2,362066};actor.nearest=&neighbor;actor.wall=&wall;
  wall.guid={4,134287};ai=std::make_unique<Grash>(&actor);
  liveMap.lower=&actor;
 }
 ~Run(){ObjectAccessor::players.clear();liveMap.lower=nullptr;}
 void Select(){script.OnGossipHello(&owner,&actor);ai->sGossipSelect(&owner,19483,0);}
 void Start(){Select();ai->UpdateAI(1000);assert(actor.motion.paths==std::vector<uint32>{10993810});}
 void Arrive(uint32 id){auto path=static_cast<WaypointMovementGenerator<Creature>*>(actor.motion.idle.get());assert(path);path->_currentNode=id;path->MovementInform(actor);}
 uint64 Count(){return scenario.achievements.progress[47127].Counter;}
};
int main(){
#ifdef BASELINE
 {Run r;r.actor.motion.MovePath(10993809,false);r.Arrive(0);
  assert(r.neighbor.credits[49817]==1&&!r.owner.credits[49817]&&r.Count()==1&&InstanceScript::opens==1);
  r.Arrive(3);assert(r.neighbor.credits[49817]==2&&r.Count()==1);
 }
 {Run r;r.actor.wall=nullptr;r.actor.motion.MovePath(10993810,false);r.Arrive(0);
  assert(!InstanceScript::opens&&r.neighbor.credits[49817]==1&&r.Count()==1);
 }
 std::cout<<"PASS COUNTERFACTUAL007b8732a: actual native waypoint dispatcher on unissued/wrong10993809 delivers49817 to nearest unrelated player; repeated nodes resend; missing wall still grants criterion. Native amount1 caps modeled separately. Expected broken, not successful ordinary run.\n";
#else
 unsigned cases=0;
 for(uint32 status:{1u,3u})for(int mode=0;mode<4;++mode){
  Run r;r.owner.quest.Status=status;r.Start();r.Arrive(0);
  assert(r.owner.credits[49817]==1&&!r.neighbor.credits[49817]&&r.Count()==1&&r.wall.state==0&&InstanceScript::opens==1);
  CreatureAI temporary;
  if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}
  if(mode==2)r.actor.i_disabledAI=r.ai.get();
  if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();r.actor.IsAIEnabled=false;}
  r.actor.Tick(10000);assert(r.owner.credits[49817]==1&&r.Count()==1);
  r.actor.primary=r.ai.get();r.actor.i_disabledAI=nullptr;
  r.Arrive(0);assert(r.owner.credits[49817]==1);
  r.wall.dbGuid=134290;r.Arrive(3);assert(InstanceScript::opens==3&&r.owner.credits[49817]==1);
  ++cases;
 }
 for(int denied=0;denied<17;++denied){
  Run r;r.Start();
  switch(denied){
   case 0:r.actor.dbGuid=362092;break;case 1:r.actor.entry=999;break;
   case 2:r.actor.mapId=1;break;case 3:r.actor.alive=false;break;
   case 4:r.actor.world=false;break;case 5:r.actor.preDelete=true;break;
   case 6:scenario.id=7;break;case 7:scenario.step=1;break;
   case 8:r.ai->_gossipActions=0;break;case 9:r.actor.motion.MovePath(10993809,false);break;
   case 10:r.actor.wall=nullptr;break;case 11:r.wall.dbGuid=134290;break;
   case 12:r.wall.world=false;break;case 13:r.wall.phase=false;break;
   case 14:r.actor.wallNear=false;break;case 15:InstanceScript::opensSucceed=false;break;
   case 16:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;
  }
  r.Arrive(0);r.actor.Tick(10000);
  assert(!r.Count()&&!r.owner.credits[49817]&&!r.neighbor.credits[49817]);++cases;
 }
 {Run r;r.Start();r.actor.motion.idle=std::make_unique<MovementGenerator>();r.ai->MovementInform(WAYPOINT_MOTION_TYPE,0);assert(!r.Count()&&!InstanceScript::opens);}
 {Run r;r.Start();for(uint32 node:{1u,2u,4u,5u})r.Arrive(node);assert(!r.Count()&&!InstanceScript::opens);}
 // The first recruitment route only opens its own approach wall at node3.
 {Run r;scenario.step=1;r.ai->_gossipActions=1;r.actor.motion.MovePath(10993808,false);r.wall.dbGuid=134291;
  r.Arrive(0);assert(!InstanceScript::opens);r.Arrive(3);assert(InstanceScript::opens==1&&r.wall.state==0&&!r.owner.credits[49817]&&!r.neighbor.credits[49817]);}
 // A rejected delivery waits without replaying the route/open/cast; original
 // caller, native receipt and busy guard own exactly-once progression.
 for(int mode=0;mode<4;++mode){Run r;r.Start();scenario.rejected.insert(49817);r.Arrive(0);
  assert(!r.Count()&&r.owner.credits[49817]==1&&r.ai->_wallPending);
  CreatureAI temporary;
  if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}
  if(mode==2)r.actor.i_disabledAI=r.ai.get();
  if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();r.actor.IsAIEnabled=false;}
  r.actor.Tick(999);assert(r.owner.credits[49817]==1);scenario.rejected.clear();r.actor.Tick(1);
  assert(r.Count()==1&&r.owner.credits[49817]==2&&!r.ai->_wallPending&&InstanceScript::opens==1&&r.actor.motion.paths.size()==1);++cases;
 }
 {Run r;r.Start();r.owner.onCredit=[&](uint32 asset){assert(asset==49817);r.Arrive(0);r.actor.Tick(0);};
  r.Arrive(0);assert(r.Count()==1&&r.owner.credits[49817]==1&&!r.ai->_wallPending);}
 {Run r;r.Start();scenario.achievements.progress[47330].Counter=1;
  r.owner.onCredit=[&](uint32 asset){assert(asset==49817);scenario.step=3;};
  r.Arrive(0);assert(r.Count()==1&&!r.ai->_wallPending&&scenario.step==3);
  r.wall.dbGuid=134290;r.Arrive(3);assert(InstanceScript::opens==2&&r.owner.credits[49817]==1);}
 {Run r;r.Start();r.wall.state=0;r.Arrive(0);assert(r.Count()==1&&r.owner.credits[49817]==1);}
 {Run r;r.Start();r.owner.world=false;r.Arrive(0);assert(r.ai->_wallPending&&!r.Count());
  for(int i=0;i<40;++i)r.actor.Tick(1000);assert(warnings==1&&r.ai->_wallExhausted&&!r.neighbor.credits[49817]);
  r.owner.world=true;r.actor.Tick(0);assert(r.Count()==1&&!r.ai->_wallPending&&r.owner.credits[49817]==1&&r.actor.motion.paths.size()==1);}
 {Run r;r.Start();scenario.rejected.insert(49817);r.Arrive(0);
  for(int i=0;i<40;++i)r.actor.Tick(1000);assert(warnings==1&&r.ai->_wallExhausted&&r.owner.credits[49817]==30);
  scenario.rejected.clear();r.actor.Tick(10000);assert(!r.Count());
  r.owner.near=false;r.actor.Tick(0);r.owner.near=true;r.actor.Tick(0);assert(r.Count()==1&&r.owner.credits[49817]==31);}
 for(int mode=0;mode<4;++mode){Run r;r.Start();scenario.rejected.insert(49817);r.Arrive(0);
  for(int i=0;i<40;++i)r.actor.Tick(1000);assert(r.ai->_wallExhausted&&r.ai->_wallPresent&&warnings==1);
  CreatureAI temporary;
  if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}
  if(mode==2)r.actor.i_disabledAI=r.ai.get();
  if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();r.actor.IsAIEnabled=false;}
  // The actor receives no updates during the absence. Native instance entry
  // must rearm the pending original recipient even without a presence edge.
  EntryBridge entry;entry.RearmGrashWall(&r.neighbor);assert(r.ai->_wallExhausted);
  r.ai->SetGUID(r.owner.GetGUID(),0);assert(r.ai->_wallExhausted);
  scenario.rejected.clear();entry.RearmGrashWall(&r.owner);assert(!r.ai->_wallExhausted&&!r.Count());
  r.actor.Tick(0);assert(r.Count()==1&&r.owner.credits[49817]==31&&r.actor.motion.paths.size()==1);++cases;
 }
 for(int change=0;change<4;++change){Run r;r.Start();scenario.rejected.insert(49817);r.Arrive(0);auto held=r.owner.quest.ScriptLifetime;
  switch(change){case 0:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:r.owner.record=false;break;case 2:r.owner.quest.Status=0;break;case 3:r.owner.rewarded=true;break;}
  scenario.rejected.clear();r.actor.Tick(1000);assert(!r.Count()&&!r.ai->_wallPending&&warnings==1&&r.owner.credits[49817]==1);++cases;
 }
 {Run r;r.Start();scenario.rejected.insert(49817);r.Arrive(0);r.ai->Reset();scenario.rejected.clear();r.actor.Tick(1000);assert(r.Count()==1&&r.owner.credits[49817]==2);}
 {Run r;r.Start();scenario.rejected.insert(49817);r.Arrive(0);scenario.step=3;r.actor.Tick(1000);assert(!r.Count()&&!r.ai->_wallPending&&warnings==1);}
 {Run r;r.Start();r.owner.alive=false;r.Arrive(0);assert(!r.Count()&&r.ai->_wallPending);r.owner.alive=true;r.actor.Tick(1000);assert(r.Count()==1);}
 {Run r;r.Start();scenario.rejected.insert(49817);r.Arrive(0);r.ai.reset();r.actor.primary=nullptr;r.actor.Tick(10000);assert(!r.Count());}
 {Run r;r.actor.dbGuid=362092;r.Select();assert(r.actor.motion.paths.empty()&&!r.ai->_gossipActions&&(r.actor.flags&1));}
 std::cout<<"PASS "<<cases<<" wall contexts plus actual native waypoint dispatch, raw accepted gossip/original40341 lease, wrong/unissued path/node/lower actor/wall/phase/open rejection, first approach visual-only, duplicate/reentrant/native ack,1s thirty-attempt refusal/absent original/return rearm, ended/replaced quest/reset/dead owner/AI lifetime. Full Grash handlers and native owner/quest/DealHeal/Notify/lifecycle/FunctionProcessor compiled; path installation, geometry/GO state and scenario amount1 gate controlled. No route interruption replay or client/full-chain claim.\n";
#endif
}
