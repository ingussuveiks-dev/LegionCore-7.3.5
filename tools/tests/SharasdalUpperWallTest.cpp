#include "NativeHost.inc"
template<class T> struct PointMovementGenerator {
 uint32 _movementId; explicit PointMovementGenerator(uint32 id):_movementId(id){}
 void DoFinalize(T&);void MovementInform(T&);
};
#include "PointFinalize.inc"
#include "Policy.inc"
#include "Actor.inc"
using Grash=npc_grash::npc_grashAI;
struct EntryBridge {
 Map* instance=&liveMap;ObjectGuid lowerGrash,upperGrash{2,362092};
#include "Reentry.inc"
};
struct Run {
 Player owner,neighbor;Creature actor,erunak,adel,lady;GameObject wall;
 CreatureAI erunakAI,adelAI;std::unique_ptr<Grash> ai;npc_grash script;
 Run(){scenario=Scenario{};scenario.step=4;manager.active=&scenario;warnings=0;InstanceScript::opens=0;InstanceScript::opensSucceed=true;
  owner.guid={1,1};neighbor.guid={1,2};ObjectAccessor::players={&owner,&neighbor};
  actor.dbGuid=362092;actor.guid={2,362092};actor.z=796.974f;actor.health=actor.maximum=1000;actor.wall=&wall;actor.nearest=&neighbor;
  wall.dbGuid=134298;wall.guid={4,134298};erunak.dbGuid=362093;erunak.entry=102826;erunak.primary=&erunakAI;
  adel.dbGuid=362094;adel.entry=104853;adel.primary=&adelAI;lady.dbGuid=362095;lady.entry=104856;
  actor.erunak=&erunak;actor.adel=&adel;actor.lady=&lady;ai=std::make_unique<Grash>(&actor);liveMap.upper=&actor;
 }
 ~Run(){ObjectAccessor::players.clear();liveMap.upper=nullptr;}
 void Select(){script.OnGossipHello(&owner,&actor);ai->sGossipSelect(&owner,19467,0);}
 uint32 Point(){assert(!actor.motion.points.empty());return actor.motion.points.back();}
 void Finalize(uint32 id){PointMovementGenerator<Creature>(id).DoFinalize(actor);}
 void Pose(){actor.x=-17.96f;actor.y=802.05f;actor.z=807.668579f;}
 void Arrive(){Pose();Finalize(Point());}
 uint64 Count(){return scenario.achievements.progress[47133].Counter;}
};
int main(){
#ifdef BASELINE
 {Run r;r.Select();assert(r.owner.credits[49819]==1&&r.Count()==1&&!InstanceScript::opens&&r.actor.x==-181.168f);}
 {Run r;r.Select();r.wall.dbGuid=134294;r.Finalize(1);assert(r.Count()==1&&InstanceScript::opens==1);}
 {Run r;r.ai->DoAction(3);r.Select();r.ai->UpdateAI(1000);assert(r.actor.motion.points==std::vector<uint32>({1,0}));}
 std::cout<<"PASS COUNTERFACTUAL da3f4b8bb: final49819 before movement/opening; actual native Point::DoFinalize also dispatches after StopMoving at wrong pose and opens an unrelated door; queued approach overrides final route. Expected broken baseline.\n";
#else
 unsigned cases=0;
 for(uint32 status:{1u,3u})for(int mode=0;mode<4;++mode){Run r;r.owner.quest.Status=status;r.Select();assert(!r.Count()&&!r.owner.credits[49819]&&!InstanceScript::opens&&r.Point()==1);
  r.ai->sGossipSelect(&r.neighbor,19467,0);assert(r.actor.motion.points.size()==1);
  r.Arrive();assert(r.Count()==1&&r.owner.credits[49819]==1&&!r.neighbor.credits[49819]&&r.wall.state==0&&r.actor.casts==1&&InstanceScript::opens==1);
  assert(r.ai->attacks==std::vector<Unit*>{&r.lady}&&r.erunakAI.attacks.size()==1&&r.adelAI.attacks.size()==1);
  CreatureAI temporary;if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}if(mode==2)r.actor.i_disabledAI=r.ai.get();if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();r.actor.IsAIEnabled=false;}
  r.actor.Tick(5000);r.actor.primary=r.ai.get();r.actor.i_disabledAI=nullptr;r.Finalize(1);
  assert(r.owner.credits[49819]==1&&InstanceScript::opens==1&&r.actor.casts==1);++cases;
 }
 // Native finalize alone proves no arrival. Wrong/obsolete IDs cannot consume
 // an issued request; a replaced generator may finalize synchronously.
 {Run r;r.Select();r.Finalize(0);r.Finalize(42);assert(r.ai->_upperMoving&&!r.Count());r.Finalize(1);assert(!r.Count()&&!InstanceScript::opens&&warnings==1&&r.actor.flags);r.Select();assert(r.Point()==2);r.Pose();r.Finalize(1);assert(!r.Count());r.Finalize(2);assert(r.Count()==1);++cases;}
 {Run r;r.Select();r.Pose();r.actor.finalized=false;r.Finalize(1);assert(!r.Count()&&r.ai->_upperMoving);r.actor.finalized=true;r.Finalize(1);assert(r.Count()==1);++cases;}
 {Run r;r.ai->DoAction(3);r.Select();r.ai->UpdateAI(1000);assert(r.actor.motion.points==std::vector<uint32>{1});r.Arrive();assert(r.Count()==1);++cases;}
 {Run r;scenario.step=3;r.ai->DoAction(3);r.ai->UpdateAI(999);assert(r.actor.motion.points.empty());r.ai->UpdateAI(1);assert(r.actor.motion.points==std::vector<uint32>{0});++cases;}
 for(int deny=0;deny<29;++deny){Run r;r.Select();r.Pose();auto held=r.owner.quest.ScriptLifetime;
  switch(deny){
   case 0:r.actor.alive=false;break;case 1:r.actor.world=false;break;case 2:r.actor.preDelete=true;break;case 3:r.actor.mapId=1;break;
   case 4:scenario.id=7;break;case 5:scenario.step=3;break;case 6:r.actor.x+=1.01f;break;case 7:r.actor.z+=1.01f;break;
   case 8:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 9:r.owner.record=false;break;case 10:r.owner.rewarded=true;break;
   case 11:r.owner.quest.Status=0;break;case 12:r.actor.castResult=1;break;case 13:r.actor.wall=nullptr;break;case 14:r.wall.dbGuid=134294;break;
   case 15:r.wall.world=false;break;case 16:r.wall.phase=false;break;case 17:r.actor.wallNear=false;break;case 18:InstanceScript::opensSucceed=false;break;
   case 19:r.owner.world=false;break;case 20:r.owner.alive=false;break;case 21:r.owner.classId=5;break;case 22:r.owner.gm=true;break;
   case 23:r.owner.phase=false;break;case 24:r.owner.near=false;break;case 25:r.owner.los=false;break;case 26:r.actor.states=UNIT_STATE_STUNNED;break;
   case 27:r.wall.preDelete=true;break;case 28:r.wall.map=&otherMap;break;
  }
  r.Finalize(1);r.actor.Tick(1000);assert(!r.Count()&&!r.owner.credits[49819]&&!r.neighbor.credits[49819]);++cases;
 }
 for(int deny=0;deny<4;++deny){Run r;switch(deny){case 0:r.actor.dbGuid=362066;break;case 1:r.owner.interact=false;break;case 2:r.actor.flags=0;break;case 3:r.ai->_upperSerial=uint32(-1);break;}r.Select();assert(r.actor.motion.points.empty()&&!r.Count());++cases;}
 // Cast and physical wall failures retry finitely, without a second movement
 // or a second successful Thud. Optional combat actors never manufacture50397.
 for(int failure=0;failure<4;++failure){Run r;r.Select();switch(failure){case 0:r.actor.castResult=1;break;case 1:r.actor.wall=nullptr;break;case 2:InstanceScript::opensSucceed=false;break;case 3:scenario.rejected.insert(49819);break;}
  r.Arrive();assert(!r.Count());auto casts=r.actor.casts;r.actor.Tick(999);assert(r.actor.casts==casts);
  r.actor.castResult=0;r.actor.wall=&r.wall;InstanceScript::opensSucceed=true;scenario.rejected.clear();r.actor.Tick(1);
  assert(r.Count()==1&&r.actor.motion.points.size()==1&&r.actor.casts==(failure==0?2u:1u));++cases;
 }
 for(int missing=0;missing<6;++missing){Run r;switch(missing){case 0:r.actor.lady=nullptr;break;case 1:r.lady.dbGuid=362091;break;case 2:r.lady.alive=false;break;case 3:r.erunak.primary=nullptr;break;case 4:r.erunak.dbGuid=362063;break;case 5:r.actor.adel=nullptr;break;}r.Select();r.Arrive();assert(r.Count()==1&&!r.owner.credits[50397]);++cases;}
 {Run r;r.Select();r.actor.onCast=[&]{r.Finalize(1);r.actor.Tick(0);};r.owner.onCredit=[&](uint32 asset){assert(asset==49819);r.Finalize(1);r.actor.Tick(0);scenario.step=5;};r.Arrive();assert(r.Count()==1&&r.owner.credits[49819]==1&&r.actor.casts==1&&InstanceScript::opens==1);++cases;}
 // The real opened wall survives ordinary AI resets/owner absence; receipt
 // retries never replay movement, spell, combat initiation or object opening.
 for(int mode=0;mode<4;++mode){Run r;r.Select();scenario.rejected.insert(49819);r.Arrive();r.ai->Reset();assert(r.ai->_upperOpened&&r.ai->_upperArrived);
  for(int i=0;i<40;++i)r.actor.Tick(1000);assert(warnings==1&&r.ai->_upperExhausted&&r.owner.credits[49819]==30);
  CreatureAI temporary;if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}if(mode==2)r.actor.i_disabledAI=r.ai.get();if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();r.actor.IsAIEnabled=false;}
  EntryBridge entry;entry.RearmGrashWall(&r.neighbor);assert(r.ai->_upperExhausted);r.ai->SetGUID(r.owner.guid,0);assert(r.ai->_upperExhausted);
  scenario.rejected.clear();entry.RearmGrashWall(&r.owner);r.actor.Tick(0);assert(r.Count()==1&&r.owner.credits[49819]==31&&r.actor.motion.points.size()==1&&r.actor.casts==1&&InstanceScript::opens==1);++cases;
 }
 {Run r;r.Select();r.owner.world=false;r.Arrive();for(int i=0;i<40;++i)r.actor.Tick(1000);assert(r.ai->_upperExhausted&&!r.Count());r.owner.world=true;r.actor.Tick(0);assert(r.Count()==1&&!r.neighbor.credits[49819]&&InstanceScript::opens==1);++cases;}
 {Run r;r.Select();r.ai->Reset();assert(r.actor.flags&&!r.ai->_upperMoving);r.Pose();r.Finalize(1);assert(!r.Count());r.Select();assert(r.Point()==2);r.Finalize(1);assert(!r.Count());r.Finalize(2);assert(r.Count()==1);++cases;}
 {Run r;r.Select();r.actor.states=UNIT_STATE_STUNNED;r.actor.Tick(300000);assert(r.ai->_upperMoving&&!warnings);r.actor.states=0;r.actor.Tick(299999);assert(r.ai->_upperMoving);r.actor.Tick(1);assert(!r.ai->_upperMoving&&r.actor.flags&&warnings==1&&!r.Count());++cases;}
 {Run r;r.Select();r.actor.states=UNIT_STATE_STUNNED;r.Arrive();r.actor.Tick(300000);assert(r.ai->_upperArrived&&!r.ai->_upperExhausted&&!warnings&&!r.Count());r.actor.states=0;r.actor.Tick(0);assert(r.Count()==1);++cases;}
 for(int failed=0;failed<2;++failed){Run r;r.Select();if(failed)r.actor.castResult=1;else r.actor.wall=nullptr;r.Arrive();for(int i=0;i<40;++i)r.actor.Tick(1000);assert(warnings==1&&!r.ai->_upperArrived&&r.actor.flags&&!r.Count());r.actor.wall=&r.wall;r.actor.castResult=0;r.Select();assert(r.Point()==2);r.Finalize(1);assert(!r.Count());r.Arrive();assert(r.Count()==1);++cases;}
 {Run r;r.Select();r.owner.quest.ScriptLifetime.reset();r.actor.Tick(0);assert(!r.ai->_upperMoving&&!r.Count()&&r.actor.flags);++cases;}
 {Run r;r.Select();r.ai.reset();r.actor.primary=nullptr;r.actor.IsAIEnabled=false;r.actor.Tick(1000);assert(!r.Count());++cases;}
 std::cout<<"PASS "<<cases<<" actual upper Grash actor/quest lease/native Point finalize/lifecycle/re-entry cases under ASAN; native cast, geometry, object and scenario services controlled. No client traversal claim.\n";
#endif
}
