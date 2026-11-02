// Production death receipt and native Smart205/owner/waypoint dispatch. World,
// packet/criteria writes and motion installation are explicit controlled services.
#define MONK_JORVINAX_TEST
#include "MonkChenRoutesTest.cpp"
#include <set>
using WorldObject=Unit;
using CriteriaTypes=uint32;
#include "SmartOwner.inc"
enum { SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205 };
struct Native205
{
 using ObjectList=std::vector<WorldObject*>;
 struct Event { struct Action { struct Criteria { uint32 type=92,misc1=46951,misc2=0,misc3=0; } achievementCriteria; } action; } e;
 ObjectList invokers;
 WorldObject* GetBaseObject(){return invokers.empty()?nullptr:invokers.front();}
 ObjectList* GetTargets(Event const&,Unit*){return new ObjectList(invokers);}
 void Run(Unit* unit){switch(205){
 #include "Smart205.inc"
 }}
};
struct HaitMovement:CreatureAI
{
 using CreatureAI::CreatureAI;
 FunctionProcessor _presentationFunctions;
 #include "HaitMovement.inc"
};
struct HaitBefore:CreatureAI
{
 using CreatureAI::CreatureAI;
 FunctionProcessor _presentationFunctions;
 #include "HaitMovementBaseline.inc"
};
struct BossSmart:CreatureAI
{
 using CreatureAI::CreatureAI;
 void JustDied(Unit* killer) override {Native205 script;script.invokers={killer,killer};script.Run(killer);}
};
struct Battle:Escort
{
 Creature boss;BossSmart smart{&boss};HaitMovement movement{&hait};
 Battle(){boss.map=&map;boss.guid={91,2};boss.entry=98217;boss.spawn=362203;boss.i_AI=&smart;map.creatures[boss.guid]=&boss;instance.OnCreatureCreate(&boss);hait.i_AI=&movement;hait.nearest=&player;pet.guid={92,2};guardian.guid={93,2};ObjectAccessor::units={{pet.guid,&pet},{guardian.guid,&guardian},{boss.guid,&boss}};}
 ~Battle(){ObjectAccessor::units.clear();}
 auto& receipt(){return instance.jorvinax;}
 void Kill(Unit* killer,bool native=true){boss.deathState=CORPSE;boss.IsAIEnabled=native;if(killer)killer->DispatchDeath(&boss);else instance.CreatureDies(&boss,nullptr);}
 void Handover(){Begin();Arrive();Tick(15000);assert(hait.motion.paths==std::vector<uint32>{9859006});}
 void HaitPose(){auto const& end=waypointMgr.haitData.back();hait.x=end.x;hait.y=end.y;hait.z=end.z;hait.spline.done=true;hait.motion.waypoint.Node(8);}
 void HaitArrive(){HaitPose();hait.motion.waypoint.OnArrived(hait);}
 uint32 Attempts(){uint32 n=0;for(auto id:player.credits)if(id==46951)++n;return n;}
 void Won(){assert(scenario.step==6&&scenario.progress[29071].Counter==1&&receipt().earned&&!receipt().pending&&state().genuineBossDefeat);}
};
int main()
{
 uint32 retained=RunRoutesCases(),cases=0;
 // Full pinned native movement body demonstrates the real premature event,
 // without inventing a broken pet-owner implementation.
 {Battle r;r.Handover();HaitBefore old(&r.hait);old.MovementInform(WAYPOINT_MOTION_TYPE,8);assert(r.boss.IsAlive()&&r.scenario.step==6&&r.Attempts()==1&&Count(r.hait,15)==1);++cases;}
 std::cout<<"COUNTERFACTUAL pinned7bf4d7a50 native Hait final-node handler advances46951 while actual boss remains alive. Controlled native current-tree endpoint; no client traversal claimed.\n";
 {Battle r;r.Handover();r.HaitArrive();for(uint32 i=0;i<100;++i)r.movement.MovementInform(WAYPOINT_MOTION_TYPE,8);r.Tick(60000);assert(r.receipt().arrivalSeen&&r.scenario.step==5&&r.Attempts()==0&&Count(r.hait,15)==0);++cases;}
 // Keep native SmartAI death205. Deduplication and owner routing are the
 // actual production handlers, and instance fallback must never replay them.
 for(uint32 owner=0;owner<3;++owner){Battle r;r.Handover();Unit* killer=owner==0?static_cast<Unit*>(&r.player):owner==1?&r.pet:&r.guardian;r.Kill(killer);r.Won();assert(r.Attempts()==1);r.HaitArrive();assert(Count(r.hait,15)==1&&r.hait.talks.back().second==r.player.guid);for(uint32 i=0;i<20;++i){r.instance.CreatureDies(&r.boss,killer);r.Tick(2000);}assert(r.Attempts()==1&&Count(r.hait,15)==1);++cases;}
 for(uint32 step=0;step<5;++step)for(uint32 kind=0;kind<5;++kind){Battle r;r.scenario.step=step;Unit* killer=kind==0?static_cast<Unit*>(&r.player):kind==1?&r.pet:kind==2?&r.guardian:kind==3?static_cast<Unit*>(&r.hait):nullptr;r.Kill(killer,kind<3);assert(r.receipt().pending&&!r.receipt().earned);uint32 rejected=r.Attempts();r.Tick(100000);assert(r.Attempts()==rejected&&r.scenario.step==step);r.map.creatures.erase(r.boss.guid);r.scenario.step=5;r.Tick(0);r.Won();assert(r.Attempts()==rejected+1);r.Tick(50000);assert(r.Attempts()==rejected+1);++cases;}
 // Genuine early victory must not erase the original fifteen-second route.
 {Battle r;r.Begin();r.Arrive();r.Kill(&r.guardian);r.Won();r.Tick(14999);assert(r.hait.motion.paths.empty());r.Tick(1);assert(r.hait.motion.paths==std::vector<uint32>{9859006});r.HaitArrive();assert(Count(r.hait,15)==1&&r.Attempts()==1);++cases;}
 {Battle r;r.Begin();r.Arrive();r.scenario.step=6;r.scenario.progress[29071].Counter=1;r.Tick(15000);assert(r.hait.motion.paths.empty()&&!r.state().genuineBossDefeat);++cases;}
 // Progress is a true death receipt, independent of guide presentation/AI.
 for(uint32 mode=0;mode<4;++mode){Battle r;r.Handover();if(mode==0)r.hait.i_AI=nullptr;if(mode==1)r.hait.world=false;if(mode==2)r.map.creatures.erase(r.hait.guid);if(mode==3)r.player.distance=1000;r.Kill(&r.guardian,false);r.Won();assert(r.Attempts()==1&&Count(r.hait,15)==0);++cases;}
 for(uint32 invalid=0;invalid<18;++invalid){Battle r;r.scenario.step=5;switch(invalid){case 0:r.boss.spawn=1;break;case 1:r.boss.entry=98353;break;case 2:r.boss.world=false;break;case 3:r.boss.predelete=true;break;case 4:r.map.creatures.erase(r.boss.guid);break;case 5:r.player.type=1;break;case 6:r.player.status=0;break;case 7:r.player.reward=true;break;case 8:r.player.slot=25;break;case 9:r.player.gm=true;break;case 10:r.player.world=false;break;case 11:r.player.contact=false;break;case 12:r.player.teleport=true;break;case 13:r.player.phase=false;break;case 14:r.boss.hidden=true;break;case 15:r.scenario.id=999;break;case 16:r.scenario.step=7;break;case 17:r.map.id=2;break;}r.Kill(&r.pet,false);r.Tick(50000);assert(r.Attempts()==0&&!r.receipt().pending&&!r.receipt().earned);++cases;}
 {Battle r;r.scenario.step=5;r.instance.CreatureDies(&r.boss,&r.player);assert(!r.receipt().pending&&r.Attempts()==0);++cases;}
 for(uint32 change=0;change<7;++change){Battle r;r.scenario.step=2;r.Kill(&r.pet);switch(change){case 0:r.player.quest.ScriptLifetime.reset();break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.player.map=nullptr;break;case 3:r.player.status=0;break;case 4:r.map.unloading=true;break;case 5:r.scenario.id=999;break;case 6:r.scenario.step=1;break;}r.Tick(2000);assert(!r.receipt().pending&&!r.receipt().earned&&r.Attempts()==1);++cases;}
 {Battle r;r.scenario.step=5;r.player.accept=false;r.Kill(&r.pet,false);assert(r.receipt().pending&&r.Attempts()==1);r.Tick(1999);assert(r.Attempts()==1);r.player.accept=true;r.Tick(1);r.Won();assert(r.Attempts()==2);++cases;}
 for(uint32 gate=0;gate<5;++gate){Battle r;r.scenario.step=2;r.Kill(&r.guardian,false);r.scenario.step=5;switch(gate){case 0:r.player.world=false;break;case 1:r.player.contact=false;break;case 2:r.player.gm=true;break;case 3:r.player.teleport=true;break;case 4:r.player.phase=false;break;}r.Tick(0);assert(r.receipt().pending&&r.Attempts()==0);r.player.world=r.player.contact=r.player.phase=true;r.player.gm=r.player.teleport=false;r.Tick(1999);assert(r.Attempts()==0);r.Tick(1);r.Won();assert(r.Attempts()==1);++cases;}
 {Battle r;r.scenario.step=5;r.other.quest.ScriptLifetime=std::make_shared<uint8>();r.map.players={{&r.other},{&r.player}};r.Kill(&r.guardian,false);r.Won();assert(r.Attempts()==1&&r.other.credits.empty());++cases;}
 {Battle r;r.scenario.step=5;r.player.status=0;r.Kill(&r.player,false);assert(!r.receipt().pending&&r.Attempts()==0);r.player.status=3;r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.instance.CreatureDies(&r.boss,&r.player);assert(r.Attempts()==0);r.boss.deathState=ALIVE;r.Tick(0);r.Kill(&r.player,false);r.Won();assert(r.Attempts()==1);++cases;}
 // Reentrant native delivery, cancellation, membership and scenario changes.
 for(uint32 change=0;change<5;++change){Battle r;r.scenario.step=5;r.player.onCredit=[&]{switch(change){case 0:r.receipt().Update(0);break;case 1:r.receipt().Cancel();break;case 2:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 3:manager.current=nullptr;break;case 4:r.scenario.step=4;break;}};r.Kill(&r.guardian,false);r.player.onCredit={};assert(r.Attempts()==1);if(change==0)r.Won();else assert(!r.receipt().earned);++cases;}
 // Persistent actual endpoint can survive missing AI, generator retirement,
 // and be cached before victory and before a later ordinary combat move.
 for(uint32 mode=0;mode<3;++mode){Battle r;r.Handover();r.hait.i_AI=nullptr;r.HaitPose();r.hait.UpdateWaypointID(8);if(mode==1)r.hait.motion.Retire();r.Tick(0);assert(r.receipt().arrivalSeen);if(mode==2){r.hait.x+=20;r.hait.UpdateWaypointID(0);}r.Kill(&r.pet,false);r.Won();assert(Count(r.hait,15)==1);++cases;}
 for(uint32 bad=0;bad<7;++bad){Battle r;r.Handover();r.HaitPose();r.Kill(&r.player);switch(bad){case 0:r.hait.motion.waypoint._pathId=7;break;case 1:r.hait.motion.waypoint.Node(7);break;case 2:r.hait.spline.done=false;break;case 3:r.hait.x+=1;break;case 4:r.state().handoffPending=true;break;case 5:r.state().handoffPlayer.Clear();break;case 6:r.hait.spawn=1;break;}if(bad==6)r.instance.SetGuidData(MonkJorvinax::ArrivalData,r.hait.guid);else r.movement.MovementInform(WAYPOINT_MOTION_TYPE,8);assert(!r.receipt().arrivalSeen&&Count(r.hait,15)==0&&r.Attempts()==1);++cases;}
 {Battle r;r.Handover();r.HaitArrive();r.player.distance=101;r.Kill(&r.pet,false);r.Won();assert(Count(r.hait,15)==0);r.player.distance=100;r.Tick(1999);assert(Count(r.hait,15)==0);r.Tick(1);assert(Count(r.hait,15)==1);++cases;}
 for(uint32 gate=0;gate<5;++gate){Battle r;r.Handover();r.Kill(&r.pet,false);switch(gate){case 0:r.hait.world=false;break;case 1:r.hait.predelete=true;break;case 2:r.hait.deathState=CORPSE;break;case 3:r.hait.hidden=true;break;case 4:r.player.phase=false;break;}r.HaitArrive();assert(Count(r.hait,15)==0&&r.Attempts()==1);r.hait.world=true;r.hait.predelete=r.hait.hidden=false;r.hait.deathState=ALIVE;r.player.phase=true;r.Tick(2000);assert(Count(r.hait,15)==1&&r.Attempts()==1);++cases;}
 {Battle r;r.Handover();r.HaitArrive();r.hait.onTalk=[&]{r.receipt().Update(0);};r.Kill(&r.pet,false);assert(Count(r.hait,15)==1);++cases;}
 {Battle r;r.Handover();r.HaitArrive();r.player.distance=101;r.Kill(&r.pet,false);r.scenario.step=7;r.player.distance=10;r.Tick(2000);assert(Count(r.hait,15)==0&&r.receipt().voiceDone);++cases;}
 // A genuine native death can arrive reentrantly inside the preceding
 // accepted native46947 callback. Both real leaves remain required.
 {Battle r;r.Begin();r.player.onCredit=[&]{if(r.scenario.step==5)r.Kill(&r.guardian);};r.Arrive();r.player.onCredit={};r.Won();assert(r.state().arrivalDone);r.Tick(15000);assert(r.hait.motion.paths==std::vector<uint32>{9859006});++cases;}
 std::cout<<"PASS "<<cases<<" Jorvinax ASAN contexts plus "<<retained<<" retained route contexts/50 arrival/70 rescue: full pinned premature-arrival counterfactual, actual native Smart205 player/pet/guardian dedupe, real death/early prerequisite receipt, native deadline continuation, no-AI/retired endpoint, original weak attempts, reentrant cancellation, exactly-once personal optional victory text. Criteria writes/motion/world services controlled; client combat/visuals/full943/all36 remain OPEN.\n";
}
