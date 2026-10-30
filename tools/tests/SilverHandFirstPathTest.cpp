#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
using FunctionList=std::multimap<uint64,std::function<void()>>;
enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, CRITERIA_TYPE_SCRIPT_EVENT_2=92, UNIT_STATE_STUNNED=1 };
#include "FunctionClass.inc"
#include "FunctionMethods.inc"
static uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Creature;struct AI;
struct Scenario {uint32 id=1092,step=1;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Manager {Scenario* scenario=nullptr;Scenario* GetScenario(uint32){return scenario;}} manager;
Manager* sScenarioMgr=&manager;
struct Map;
struct Player {
 Map* map=nullptr;uint32 instance=1,cls=CLASS_PALADIN,status=QUEST_STATUS_INCOMPLETE,credits=0;
 bool world=true,predelete=false,contact=true,gm=false,teleport=false,phase=true,rewarded=false,alive=true,accept=true;
 float distance=10;std::function<void()> onCredit;
 bool IsInWorld(){return world;}bool IsPreDelete(){return predelete;}bool CanContact(){return contact;}
 bool isGameMaster(){return gm;}bool IsBeingTeleported(){return teleport;}uint32 getClass(){return cls;}
 Map* GetMap(){return map;}uint32 GetInstanceId(){return instance;}bool InSamePhase(Creature*){return phase;}
 bool IsWithinDistInMap(Creature*,float r){assert(r==100);return distance<=r;}
 bool GetQuestRewardStatus(uint32 q){assert(q==42120);return rewarded;}uint32 GetQuestStatus(uint32 q){assert(q==42120);return status;}
 // Controlled native-event boundary: policy must wait for the real Scenario receipt.
 void UpdateAchievementCriteria(uint32 t,uint32 a){assert(t==92&&a==50766);++credits;if(onCredit)onCredit();if(accept&&manager.scenario&&manager.scenario->step==1)manager.scenario->step=2;}
};
struct Ref {Player* p;Player* getSource() const{return p;}};
struct Map {std::vector<Ref> players;auto const& GetPlayers(){return players;}};
struct Motion {uint32 moves=0;void MovePath(uint32 path,bool repeat){assert(path==10993805&&!repeat);++moves;}};
struct Creature {
 Map* map=nullptr;uint32 entry=106429,mapid=1539,instance=1,states=0;
 bool world=true,predelete=false,alive=true,IsAIEnabled=true,m_AI_locked=false;
 ::AI* primary=nullptr;::AI* i_disabledAI=nullptr;Player* nearest=nullptr;Motion motion;
 ::AI* AI(){return primary;}
 bool IsInWorld(){return world;}bool IsPreDelete(){return predelete;}bool IsAlive(){return alive;}
 uint32 GetEntry(){return entry;}uint32 GetMapId(){return mapid;}Map* GetMap(){return map;}
 uint32 GetInstanceId(){return instance;}bool HasUnitState(uint32 s){return (states&s)!=0;}
 Motion* GetMotionMaster(){return &motion;}Player* FindNearestPlayer(float r){assert(r==100);return nearest;}
 void Tick(uint32);
};
using WaypointPath=std::vector<uint32>;
struct Waypoints {WaypointPath path{1};bool missing=false;WaypointPath const* GetPathScript(uint32 id){assert(id==10993805);return missing?nullptr:&path;}} waypoints;
Waypoints* sWaypointMgr=&waypoints;
#ifndef BASELINE
#include "silver_hand_first_path.h"
#endif
struct AI {virtual ~AI()=default;virtual void UpdateLifecycle(uint32){}virtual void Reset(){}};
using CreatureAI=AI;
void Creature::Tick(uint32 diff){::AI* i_AI=primary;
#include "Lifecycle.inc"
}
namespace SilverHandCompanion {void Recover(Creature*,bool&,bool&) {}}
struct Path : AI {
 explicit Path(Creature* actor):me(actor){}
 Creature* me;bool firstwp=true,secondwp=false,wait_heal=false,_pendingHealRecovery=false,_entryPending=false,_introDone=false;
 uint64 _entryGeneration=0;struct Events {void Reset(){}} events;
 FunctionProcessor _functions,_entryFunctions;std::vector<uint32> speech;uint32 doors=0;
 std::function<void()> onTalk,onDoor;
 void Talk(uint32 id){assert(id==6||id==7||id==8);speech.push_back(id);if(onTalk)onTalk();}
 void VisualOpenDoor(){++doors;if(onDoor)onDoor();}
 void CompleteFirstFight(uint32){} // Separately tested native fight policy.
 struct RoomClock {void Update(uint32){}} _firstRoom;void CompleteFirstRoom(){} // Dedicated first-room policy regression.
#ifndef BASELINE
 SilverHandFirstPath::State _firstPath;
#include "Methods.inc"
#else
 void UpdateLifecycle(uint32)override{}
#endif
#include "Arrival.inc"
#include "Ordinary.inc"
};
struct Fixture {
 Scenario scenario;Map map,other;Player player;Creature actor;Path ai{&actor};
 Fixture(){manager.scenario=&scenario;waypoints.missing=false;waypoints.path={1};warnings=0;player.map=&map;actor.map=&map;actor.nearest=&player;actor.primary=&ai;map.players={{&player}};}
 void Tick(uint32 diff){actor.Tick(diff);if(actor.primary==&ai&&actor.IsAIEnabled)ai.Ordinary(diff);}
 void Request(){
#ifndef BASELINE
 assert(ai._firstPath.Request(&actor));
#endif
 }
 void Arrive(){ai.Arrive();}
};
int main(){uint32 cases=0;
#ifdef BASELINE
 {Fixture f;f.actor.nearest=nullptr;f.Arrive();f.Tick(9001);assert(!f.ai.firstwp&&f.ai.secondwp&&f.actor.motion.moves==1&&f.player.credits==0);++cases;}
 {Fixture f;f.player.gm=true;f.player.accept=false;f.Arrive();f.Tick(9001);assert(f.player.credits==1&&f.scenario.step==1&&f.actor.motion.moves==1);++cases;}
 {Fixture f;waypoints.missing=true;f.Arrive();f.Tick(9001);assert(f.player.credits==1&&f.actor.motion.moves==1);++cases;}
 std::cout<<"PASS "<<cases<<" pinned counterfactuals: consumed absent/ineligible recipient and unchecked missing next path (expected broken)\n";
#else
 {Fixture f;f.Arrive();f.Tick(10000);assert(f.player.credits==0&&f.ai.doors==0&&f.actor.motion.moves==0);++cases;}
 {Fixture f;f.Request();assert(!f.ai._firstPath.Request(&f.actor));f.Arrive();assert(f.player.credits==1&&f.scenario.step==2&&!f.ai.firstwp&&f.ai.secondwp&&f.ai.doors==1);assert((f.ai.speech==std::vector<uint32>{6}));f.Tick(4999);assert(f.ai.speech.size()==1);f.Tick(1);assert((f.ai.speech==std::vector<uint32>{6,7}));f.Tick(3999);assert(f.actor.motion.moves==0);f.Tick(1);assert((f.ai.speech==std::vector<uint32>{6,7,8})&&f.actor.motion.moves==1);f.Tick(10000);f.Arrive();assert(f.player.credits==1&&f.ai.doors==1&&f.actor.motion.moves==1);++cases;}
 {Fixture f;f.Request();f.map.players={};f.Arrive();f.Tick(10000);assert(f.ai.firstwp&&!f.ai.secondwp&&f.ai.doors==0&&f.actor.motion.moves==0&&warnings==1);f.map.players={{&f.player}};f.Tick(0);assert(f.player.credits==1&&f.ai.doors==1);f.Tick(8999);assert(!f.actor.motion.moves);f.Tick(1);assert(f.actor.motion.moves==1);++cases;}
 {Fixture f;f.Request();f.map.players.clear();f.Arrive();f.Tick(20000);f.map.players={{&f.player}};f.Tick(20000);assert(f.ai.doors==1&&(f.ai.speech==std::vector<uint32>{6})&&f.actor.motion.moves==0);f.Tick(5000);assert(f.ai.speech.back()==7);f.Tick(4000);assert(f.actor.motion.moves==1);++cases;}
 for(uint32 i=0;i<14;++i){Fixture f;f.Request();Player invalid=f.player;
  switch(i){case 0:invalid.world=false;break;case 1:invalid.predelete=true;break;case 2:invalid.contact=false;break;case 3:invalid.gm=true;break;case 4:invalid.teleport=true;break;case 5:invalid.cls=1;break;case 6:invalid.map=&f.other;break;case 7:invalid.instance=2;break;case 8:invalid.phase=false;break;case 9:invalid.distance=100.01f;break;case 10:invalid.rewarded=true;break;case 11:invalid.status=0;break;case 12:invalid.status=1;break;case 13:invalid.status=6;break;}
  f.actor.nearest=&invalid;f.map.players={{nullptr},{&invalid}};f.Arrive();f.Tick(10000);assert(invalid.credits==0&&f.ai.doors==0&&f.actor.motion.moves==0);f.map.players.push_back({&f.player});f.Tick(0);assert(f.player.credits==1&&f.ai.doors==1);++cases;
 }
 for(uint32 i=0;i<9;++i){Fixture f;
  switch(i){case 0:f.actor.world=false;break;case 1:f.actor.predelete=true;break;case 2:f.actor.alive=false;break;case 3:f.actor.entry=106639;break;case 4:f.actor.mapid=0;break;case 5:f.actor.map=nullptr;break;case 6:manager.scenario=nullptr;break;case 7:f.scenario.id=991;break;case 8:f.scenario.step=0;break;}
  assert(!f.ai._firstPath.Request(&f.actor));f.Arrive();f.Tick(10000);assert(f.player.credits==0&&f.ai.doors==0&&f.actor.motion.moves==0);++cases;
 }
 for(uint32 i=0;i<9;++i){Fixture f;f.Request();f.map.players.clear();f.Arrive();
  switch(i){case 0:f.actor.world=false;break;case 1:f.actor.predelete=true;break;case 2:f.actor.alive=false;break;case 3:f.actor.entry=106639;break;case 4:f.actor.mapid=0;break;case 5:f.actor.map=nullptr;break;case 6:manager.scenario=nullptr;break;case 7:f.scenario.id=1037;break;case 8:f.scenario.step=0;break;}
  f.map.players={{&f.player}};f.Tick(10000);assert(f.player.credits==0&&f.ai.doors==0&&f.actor.motion.moves==0);++cases;
 }
 {Fixture f;f.Request();f.player.alive=false;f.player.distance=100;f.Arrive();assert(f.player.credits==1&&f.ai.doors==1);++cases;}
 {Fixture f;f.Request();f.player.accept=false;f.Arrive();assert(f.player.credits==1&&f.ai.doors==0);f.Tick(999);assert(f.player.credits==1);f.player.accept=true;f.Tick(1);assert(f.player.credits==2&&f.ai.doors==1&&f.scenario.step==2);f.Tick(9000);assert(f.actor.motion.moves==1);++cases;}
 {Fixture f;f.Request();f.player.accept=false;f.Arrive();for(uint32 i=0;i<60;++i)f.Tick(1000);assert(f.player.credits==30&&f.ai.doors==0&&f.actor.motion.moves==0);f.scenario.step=2;f.Tick(0);assert(f.ai.doors==1&&f.player.credits==30);++cases;}
 {Fixture f;f.Request();f.player.onCredit=[&]{f.Arrive();f.actor.Tick(0);};f.ai.onTalk=[&]{f.Arrive();f.actor.Tick(0);};f.ai.onDoor=[&]{f.Arrive();};f.Arrive();f.Tick(9000);assert(f.player.credits==1&&f.ai.doors==1&&f.actor.motion.moves==1);++cases;}
 {Fixture f;f.Request();waypoints.missing=true;f.Arrive();f.Tick(10000);assert(f.player.credits==0&&f.ai.doors==0&&f.ai.firstwp);waypoints.missing=false;waypoints.path.clear();f.Tick(0);assert(f.ai.doors==0);waypoints.path={1};f.Tick(0);assert(f.player.credits==1&&f.ai.doors==1);++cases;}
 {Fixture f;f.Request();f.Arrive();waypoints.missing=true;f.Tick(9000);assert(f.actor.motion.moves==0);waypoints.missing=false;f.Tick(0);assert(f.actor.motion.moves==1&&f.ai.speech.back()==8);f.Tick(10000);assert(f.actor.motion.moves==1);++cases;}
 {Fixture f;f.Request();f.actor.states=UNIT_STATE_STUNNED;f.Arrive();f.Tick(20000);assert(f.player.credits==0&&f.ai.doors==0);f.actor.states=0;f.Tick(0);assert(f.ai.doors==1);f.Tick(5000);f.actor.states=UNIT_STATE_STUNNED;f.Tick(20000);assert(f.actor.motion.moves==0);f.actor.states=0;f.Tick(4000);assert(f.actor.motion.moves==1);++cases;}
 {Fixture f;f.Request();f.map.players.clear();f.Arrive();f.ai.Reset();f.map.players={{&f.player}};f.actor.IsAIEnabled=false;f.actor.primary=nullptr;f.actor.i_disabledAI=&f.ai;f.Tick(10000);assert(f.ai.doors==0);f.actor.IsAIEnabled=true;f.actor.primary=&f.ai;f.actor.i_disabledAI=nullptr;f.Tick(10000);assert(f.ai.doors==1&&f.actor.motion.moves==0);f.Tick(9000);assert(f.actor.motion.moves==1);++cases;}
 {Fixture f;f.Request();f.Arrive();f.scenario.step=3;f.Tick(10000);assert(f.actor.motion.moves==0&&(f.ai.speech==std::vector<uint32>{6}));++cases;}
 {Fixture f;f.Request();f.scenario.step=2;f.map.players.clear();f.Arrive();f.Tick(9000);assert(f.player.credits==0&&f.ai.doors==1&&f.actor.motion.moves==1);++cases;}
 {Fixture f;f.Request();f.player.onCredit=[&]{f.scenario.step=3;f.player.accept=false;};f.Arrive();f.Tick(10000);assert(f.ai.doors==0&&f.actor.motion.moves==0);++cases;}
 {Fixture f;f.Request();f.Arrive();auto dead=std::make_unique<Path>(&f.actor);f.scenario.step=1;assert(dead->_firstPath.Request(&f.actor));dead->Arrive();dead.reset();f.scenario.step=2;f.Tick(10000);assert(f.actor.motion.moves==1);++cases;}
 std::cout<<"PASS "<<cases<<" ASAN actual first-path policy/methods/FunctionProcessor/lifecycle cases; controlled map, movement, visual and native-event receipt services, no client traversal\n";
#endif
}
