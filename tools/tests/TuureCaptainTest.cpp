#define main PreservationMain
#include "TuureFirstFightTest.cpp"
#undef main
struct CaptainBorosAI:CreatureAI{
 Creature* me;uint32 starts=0,legacy=0;
 explicit CaptainBorosAI(Creature* c):me(c){c->primary=this;c->IsAIEnabled=true;}
 void AttackStart(Unit* target)override{++starts;me->victim=target;}
 void DoAction(int32 action)override{assert(action==2);++legacy;}
};
struct OldCaptain:CreatureAI{
 Creature* me;explicit OldCaptain(Creature* c):me(c){c->primary=this;c->IsAIEnabled=true;}
#include "OldCaptainDeath.inc"
};
struct CaptainRun:Run{
 std::unique_ptr<Creature> captain,lady;
 CaptainRun(){
  scenario.step=4;scenario.achievement.progress[47561].Counter=1;
  Make(captain,106107,361436,200);captain->IsAIEnabled=false;captain->flags=0;captain->faction=2102;
  captain->x=12.9236f;captain->y=1192.45f;captain->z=-45.9861f;
  player.x=captain->x;player.y=captain->y;player.z=captain->z;
  boros->x=54.98f;boros->y=1217.8f;boros->z=-39.73f;
  jace.dbGuid=361432;jace.x=69.05f;jace.y=1220.01f;jace.z=-39.70f;script.OnCreatureCreate(&jace);
  mage.dbGuid=361433;mage.flags=0;mage.faction=1665;mage.x=58.6979f;mage.y=1212.91f;mage.z=-39.6491f;script.OnCreatureCreate(&mage);
  Make(lady,106318,361888,300);lady->x=183.875f;lady->y=1334.1f;lady->z=-58.6722f;
 }
 void FinishPoint(bool actual=true){
  boros->x=boros->targetX+(actual?0.f:1.f);boros->y=boros->targetY;boros->z=boros->targetZ;
  boros->movespline->done=true;boros->point.DoFinalize(*boros);boros->pointPresent=false;
 }
 void FinishPath(){
  boros->x=192.805f;boros->y=1277.29f;boros->z=-58.7333f;boros->movespline->done=true;
  auto& node=boros->movement.nodes.at(7);node.x=boros->x;node.y=boros->y;node.z=boros->z;
  Arrive(*boros,7);assert(!boros->movement.NativeLast(*boros));boros->motionPresent=false;
 }
 void ToPoint(){pet.Kill(captain.get());assert(player.captainCredits==1);script.Update(1000);assert(boros->pointMoves==1);}
 void ToPath(){ToPoint();FinishPoint();script.Update(0);script.Update(8000);assert(boros->pathMoves==1);}
};
int main(){
 assert(PreservationMain()==0);
#ifdef CAPTAIN_BASELINE
 {CaptainRun r;InstanceScript old(&r.map);r.map.script=&old;CaptainBorosAI b(r.boros.get());OldCaptain ai(r.captain.get());r.captain->nearest=&r.observer;r.pet.Kill(r.captain.get());assert(!r.player.captainCredits&&r.observer.captainCredits==1&&b.legacy==1);}
 {CaptainRun r;InstanceScript old(&r.map);r.map.script=&old;r.pet.Kill(r.captain.get());assert(!r.player.captainCredits&&scenario.step==4);}
 std::cout<<"PASS labelled23aacb79 real native AI/instance death dispatch counterfactual: nearest unrelated observer receives50634 while pet owner gets none; unavailable primary AI loses real death transition entirely. No deliberate null-AI crash claimed.\n";
#else
 uint32 variants=0;
 for(uint32 status:{1u,3u})for(bool alive:{false,true})for(bool aiAvailable:{false,true})for(int killer=0;killer<7;++killer){
  CaptainRun r;r.player.quest.Status=status;r.player.alive=alive;CreatureAI capAI;if(aiAvailable){r.captain->primary=&capAI;r.captain->IsAIEnabled=true;}
  Unit* attackers[]={&r.player,&r.pet,&r.guardian,&r.charmed,r.boros.get(),&r.jace,&r.mage};
  attackers[killer]->Kill(r.captain.get());r.script.CreatureDies(r.captain.get(),attackers[killer]);
  assert(r.player.captainCredits==1&&!r.observer.captainCredits&&scenario.step==5&&!r.boros->pointMoves);
  r.script.Update(999);assert(!r.boros->pointMoves);r.script.Update(1);assert(r.boros->pointMoves==1);
  r.script.Update(10000);assert(!r.boros->pathMoves);r.FinishPoint();r.script.Update(0);assert(r.boros->conversations==1);
  r.script.Update(7999);assert(!r.boros->pathMoves);r.script.Update(1);assert(r.boros->pathMoves==1&&r.boros->movement.GetPathId()==9859003);
  r.script.Update(20000);assert(r.script.captainState==decltype(r.script)::CaptainState::Path);
  r.FinishPath();r.script.Update(0);assert(r.script.captainState==decltype(r.script)::CaptainState::AttackWait&&!r.boros->motionPresent);
  CaptainBorosAI b(r.boros.get());r.script.Update(3999);assert(!b.starts);r.script.Update(1);assert(b.starts==1&&r.boros->victim==r.lady.get());
  r.script.Update(20000);assert(b.starts==1&&r.player.captainCredits==1&&textMgr.groups[1]==1&&textMgr.groups[2]==1);++variants;
 }
 for(int bad=0;bad<22;++bad){CaptainRun r;Unit* killer=&r.pet;switch(bad){
  case 0:r.captain->dbGuid=0;break;case 1:r.captain->entry=106108;break;case 2:r.captain->map=&r.other;break;
  case 3:scenario.id=940;break;case 4:scenario.id=972;break;case 5:scenario.id=1145;break;case 6:scenario.id=1146;break;
  case 7:scenario.step=3;break;case 8:scenario.step=5;break;case 9:scenario.achievement.progress.clear();break;
  case 10:r.player.classId=4;break;case 11:r.player.quest.Status=0;break;case 12:r.player.rewarded=true;break;
  case 13:r.player.gm=true;break;case 14:r.player.phase=false;break;case 15:r.player.near=false;break;
  case 16:r.player.contact=false;break;case 17:r.player.pending=true;break;case 18:r.pet.owner={1,999};break;
  case 19:killer=&r.unknown;break;case 20:r.player.world=false;break;case 21:r.player.x+=200;break;
 }killer->Kill(r.captain.get());r.script.Update(20000);assert(!r.player.captainCredits&&!r.boros->pointMoves);}
 {CaptainRun r;r.script.CreatureDies(r.captain.get(),&r.pet);r.script.OnUnitDeath(r.captain.get());r.script.Update(20000);assert(r.captain->alive&&!r.player.captainCredits);}
 {CaptainRun r;CaptainBorosAI b(r.boros.get());r.boros->primary=nullptr;r.boros->IsAIEnabled=false;r.captain->Engage(&r.pet);assert(r.script.engagements==1);r.script.Update(1000);assert(!b.starts&&r.script.captainAssist);r.boros->primary=&b;r.boros->IsAIEnabled=true;r.script.Update(0);r.script.Update(1000);assert(b.starts==1&&r.boros->victim==r.captain.get()&&!r.player.captainCredits);}
 {CaptainRun r;r.player.rejectCredit=true;r.pet.Kill(r.captain.get());assert(!r.player.captainCredits&&warnings==1);r.Remove(r.captain);r.player.rejectCredit=false;r.script.Update(4999);assert(!r.player.captainCredits);r.script.Update(1);assert(r.player.captainCredits==1&&!r.boros->pointMoves);r.script.Update(1000);assert(r.boros->pointMoves==1);}
 {CaptainRun r;r.Remove(r.boros);r.pet.Kill(r.captain.get());assert(r.player.captainCredits==1);r.script.Update(1000);r.Make(r.boros,106134,361419,11);r.boros->x=-23.2934f;r.boros->y=1170.98f;r.boros->z=-69.2009f;r.script.Update(1000);assert(r.boros->pointMoves==1);}
 {CaptainRun r;r.ToPoint();r.boros->objectSize=5;r.FinishPoint(false);r.script.Update(0);assert(!r.boros->conversations);r.script.Update(1000);assert(r.boros->pointMoves==2);r.FinishPoint();r.script.Update(0);assert(r.boros->conversations==1);}
 {CaptainRun r;r.ToPoint();Boros ai(r.boros.get());ai.Reset();r.boros->primary=nullptr;r.FinishPoint();r.script.Update(0);r.script.Update(8000);assert(r.boros->pathMoves==1);}
 {CaptainRun r;r.ToPath();r.boros->movement._pathId=9859002;r.FinishPath();r.boros->motionPresent=true;r.script.Update(0);assert(r.script.captainState==decltype(r.script)::CaptainState::PointWait);}
 {CaptainRun r;r.ToPath();Arrive(*r.boros,7);r.boros->motionPresent=false;r.boros->movespline->done=true;r.script.Update(0);assert(r.script.captainState!=decltype(r.script)::CaptainState::AttackWait);}
 {CaptainRun r;r.ToPoint();r.FinishPoint();r.script.Update(0);waypointMgr.missingSecond=true;r.script.Update(8000);assert(!r.boros->pathMoves&&warnings==1);waypointMgr.missingSecond=false;r.script.Update(5000);assert(r.boros->pathMoves==1);}
 {CaptainRun r;r.ToPath();r.FinishPath();r.script.Update(0);r.script.Update(4000);assert(r.script.captainState==decltype(r.script)::CaptainState::AttackWait);CaptainBorosAI b(r.boros.get());r.player.near=false;r.script.Update(0);assert(!b.starts);r.player.near=true;r.script.Update(0);assert(b.starts==1);}
 {CaptainRun r;r.player.rejectCredit=true;r.pet.Kill(r.captain.get());auto held=r.player.quest.ScriptLifetime;r.player.quest={};r.player.rejectCredit=false;r.script.Update(5000);assert(!r.player.captainCredits);r.captain->alive=true;r.script.OnCreatureCreate(r.captain.get());r.pet.Kill(r.captain.get());assert(r.player.captainCredits==1&&r.player.quest.ScriptLifetime!=held);}
 {CaptainRun r;r.ToPoint();r.player.quest={};r.FinishPoint();r.script.Update(8000);assert(!r.boros->pathMoves&&r.player.captainCredits==1);}
 {CaptainRun r;r.player.onCredit=[&]{r.script.Update(1000);r.Remove(r.captain);};r.pet.Kill(r.captain.get());assert(r.player.captainCredits==1);r.player.onCredit={};r.script.Update(1000);assert(r.boros->pointMoves==1);}
 {CaptainRun r;r.ToPoint();r.boros->onConversation=[&]{r.Remove(r.boros);};r.FinishPoint();r.script.Update(0);assert(!r.boros);r.Make(r.boros,106134,361419,12);r.boros->x=-23.2934f;r.boros->y=1170.98f;r.boros->z=-69.2009f;r.script.Update(8000);assert(!r.boros->pathMoves);r.script.Update(1000);r.FinishPoint();r.script.Update(0);r.script.Update(8000);assert(r.boros->pathMoves==1);}
 {CaptainRun r;r.captain->flags=15;r.captain->faction=35;r.mage.flags=4;r.script.Update(0);assert(!r.captain->flags&&r.captain->faction==2102&&!r.mage.flags&&r.mage.followMoves==1&&!r.player.captainCredits);r.script.Update(1000);assert(r.mage.followMoves==1);}
 {CaptainRun r;scenario.step=5;r.captain->flags=15;r.captain->faction=35;r.script.Update(0);assert(r.captain->flags==15&&r.captain->faction==35);}
 {CaptainRun r;r.ToPath();scenario.step=6;scenario.achievement.progress[47656].Counter=1;r.script.Update(1000);assert(r.script.captainState==decltype(r.script)::CaptainState::Done);}
 {CaptainRun r;r.observer.classId=5;r.jace.Kill(r.captain.get());assert(!r.player.captainCredits&&!r.observer.captainCredits);}
 {CaptainRun r;r.observer.classId=5;r.observer.gm=true;r.jace.Kill(r.captain.get());assert(r.player.captainCredits==1&&!r.observer.captainCredits);}
 {CaptainRun r;r.ToPath();r.FinishPath();r.script.Update(0);r.Remove(r.lady);CaptainBorosAI b(r.boros.get());r.script.Update(4000);assert(!b.starts&&warnings==1);r.Make(r.lady,106318,361888,301);r.script.Update(0);assert(b.starts==1);}
 {CaptainRun r;r.player.rejectCredit=true;r.pet.Kill(r.captain.get());r.captain->alive=true;r.script.OnCreatureCreate(r.captain.get());r.player.rejectCredit=false;r.script.Update(5000);assert(!r.player.captainCredits);r.pet.Kill(r.captain.get());assert(r.player.captainCredits==1);}
 for(float delta:{102.49f,102.51f}){CaptainRun r;r.captain->objectSize=2;r.player.objectSize=0.5f;r.player.x+=delta;r.pet.Kill(r.captain.get());assert(r.player.captainCredits==uint32(delta<102.5f));}
 std::cout<<"PASS Tuure actual captain controller "<<variants<<" primary/missing-AI, live/dead owner/status/player-pet-guardian-charmer-three-ally variants; exact real death/owner47909, native combat dispatch,1s point3/actual strict endpoint/8s path9859003/native last-node removal/4s genuine Lady handoff; denials, retries, quest lease, reset/replacement, missing actors/AI/path, earned cage respawn recovery and reentrant actor removal. Native motion/criteria/world endpoints remain adapters; full client/combat/geometry pending.\n";
#endif
}
