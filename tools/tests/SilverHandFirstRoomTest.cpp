#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
using FunctionList=std::multimap<uint64,std::function<void()>>;
enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, CRITERIA_TYPE_SCRIPT_EVENT_2=92, UNIT_STATE_STUNNED=8, UNIT_STATE_ROAMING=512, UNIT_STATE_ROAMING_MOVE=1024, POINT_MOTION_TYPE=8 };
#include "FunctionClass.inc"
#include "FunctionMethods.inc"
static uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Creature;struct AI;struct Map;
struct Scenario {uint32 id=1092,step=3;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Manager {Scenario* scenario=nullptr;Scenario* GetScenario(uint32){return scenario;}} manager;
Manager* sScenarioMgr=&manager;
struct Player {
 Map* map=nullptr;uint32 instance=1,cls=CLASS_PALADIN,status=QUEST_STATUS_INCOMPLETE,credits=0;
 bool world=true,predelete=false,contact=true,gm=false,teleport=false,phase=true,rewarded=false,alive=true,accept=true;
 float distance=10;std::function<void()> onCredit;
 bool IsInWorld(){return world;}bool IsPreDelete(){return predelete;}bool CanContact(){return contact;}
 bool isGameMaster(){return gm;}bool IsBeingTeleported(){return teleport;}uint32 getClass(){return cls;}
 Map* GetMap(){return map;}uint32 GetInstanceId(){return instance;}bool InSamePhase(Creature*){return phase;}
 bool IsWithinDistInMap(Creature*,float r){assert(r==100);return distance<=r;}
 bool GetQuestRewardStatus(uint32 q){assert(q==42120);return rewarded;}uint32 GetQuestStatus(uint32 q){assert(q==42120);return status;}
 // Controlled native-event boundary: actual Scenario receipt is required by production.
 void UpdateAchievementCriteria(uint32 t,uint32 a){assert(t==92&&a==50872);++credits;if(onCredit)onCredit();if(accept&&manager.scenario&&manager.scenario->step==3)manager.scenario->step=4;}
};
struct Ref {Player* p;Player* getSource() const{return p;}};
struct Map {std::vector<Ref> players;auto const& GetPlayers(){return players;}};
struct GameObject {uint32 changes=0;void SetPhaseMask(uint32 mask,bool update){assert(mask==2&&update);++changes;}};
struct Motion {
 uint32 moves=0;std::function<void()> onMove;
 void MovePoint(uint32 id,float x,float y,float z){assert(id==3&&x==1919.26f&&y==2335.29f&&z==75.49f);++moves;if(onMove)onMove();}
};
struct Spline {bool finalized=false;bool Finalized(){return finalized;}};
struct Creature {
 Map* map=nullptr;uint32 entry=106429,mapid=1539,instance=1,states=0,despawns=0,visibilityChanges=0,removeAuraCalls=0;
 bool world=true,predelete=false,alive=true,IsAIEnabled=true,m_AI_locked=false,visible=false;
 ::AI* primary=nullptr;::AI* i_disabledAI=nullptr;Player* nearest=nullptr;Creature* guardian=nullptr;Motion motion;Spline spline;Spline* movespline=&spline;
 std::vector<Creature*> sparks;std::vector<GameObject*> gates;std::set<uint32> auras;
 bool IsInWorld(){return world;}bool IsPreDelete(){return predelete;}bool IsAlive(){return alive;}
 uint32 GetEntry(){return entry;}uint32 GetMapId(){return mapid;}Map* GetMap(){return map;}
 uint32 GetInstanceId(){return instance;}bool HasUnitState(uint32 s){return (states&s)!=0;}
 void ClearUnitState(uint32 s){states&=~s;}::AI* AI(){return primary;}
 Motion* GetMotionMaster(){return &motion;}Player* FindNearestPlayer(float r){assert(r==100);return nearest;}
 Creature* FindNearestCreature(uint32 id,float radius,bool living){assert(id==101461&&radius==20&&living);return guardian;}
 bool HasAura(uint32 id){return auras.count(id)!=0;}void RemoveAura(uint32 id){assert(id==185414);++removeAuraCalls;auras.erase(id);}
 void SetVisible(bool value){visible=value;++visibilityChanges;}void DespawnOrUnsummon(){++despawns;}
 void Tick(uint32);
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& list,Creature* actor,uint32 entry,float radius){assert(entry==106094&&radius==30);list.assign(actor->sparks.begin(),actor->sparks.end());}
void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& list,Creature* actor,uint32 entry,float radius){assert(entry==251349&&radius==12);list.assign(actor->gates.begin(),actor->gates.end());}
#ifndef BASELINE
#include "silver_hand_first_room.h"
#endif
struct AI {virtual ~AI()=default;virtual void UpdateLifecycle(uint32){}virtual void Reset(){}virtual void MovementInform(uint32,uint32){}};
using CreatureAI=AI;
void Creature::Tick(uint32 diff){::AI* i_AI=primary;
#include "Lifecycle.inc"
}
namespace SilverHandCompanion {void Recover(Creature*,bool&,bool&) {}}
struct Room : AI {
 explicit Room(Creature* actor):me(actor){}
 Creature* me;bool wait_heal=false,_pendingHealRecovery=false,_entryPending=false,_introDone=false;
 uint64 _entryGeneration=0;struct Events {void Reset(){}} events;
 FunctionProcessor _functions,_entryFunctions;std::vector<uint32> speech;uint32 casts=0;bool castSucceeds=true;
 std::function<void()> onTalk,onCast;
 void Talk(uint32 id){assert(id==19||id==20);speech.push_back(id);if(onTalk)onTalk();}
 void DoCast(uint32 spell){assert(spell==185414);++casts;if(castSucceeds)me->auras.insert(spell);if(onCast)onCast();}
 struct FightReceipt {bool completed=true;} _firstFight; // Owned-death source is separately tested in54 native cases.
 struct PathClock {void Update(uint32){}} _firstPath;void CompleteFirstPath(){} // Dedicated48-case first-path policy.
 void CompleteFirstFight(uint32){}
#ifndef BASELINE
 SilverHandFirstRoom::State _firstRoom;
#endif
#include "Methods.inc"
#include "Movement.inc"
#include "Ordinary.inc"
};
template<class T>struct PointMovementGenerator {uint32 _movementId;void DoFinalize(T&);void MovementInform(T&);};
#include "PointFinalize.inc"
struct Fixture {
 Scenario scenario;Map map,other;Player player;Creature actor,guardian;Room ai{&actor};Creature sparks[5];GameObject gates[2];
 Fixture(){manager.scenario=&scenario;warnings=0;player.map=&map;actor.map=&map;actor.nearest=&player;actor.primary=&ai;map.players={{&player}};guardian.entry=101461;actor.guardian=&guardian;
  for(auto& spark:sparks){spark.entry=106094;spark.auras.insert(211502);actor.sparks.push_back(&spark);}sparks[0].auras.clear();for(auto& gate:gates)actor.gates.push_back(&gate);
 }
 void Tick(uint32 diff){actor.Tick(diff);if(actor.primary==&ai&&actor.IsAIEnabled)ai.Ordinary(diff);}
 void Point(uint32 id,bool finished=true){actor.spline.finalized=finished;PointMovementGenerator<Creature> point{id};point.DoFinalize(actor);}
 void Start(){Point(2);}
 void Reach(){Point(3);}
 void Route(){Start();Tick(7000);assert(actor.motion.moves==1);}
 uint32 Cleanup(){return guardian.despawns+gates[0].changes+gates[1].changes;}
};
int main(){uint32 cases=0;
#ifdef BASELINE
 {Fixture f;f.actor.nearest=nullptr;f.Route();f.Reach();assert(f.scenario.step==3&&f.player.credits==0&&f.Cleanup()==3);++cases;}
 {Fixture f;f.player.gm=true;f.player.accept=false;f.Route();f.Reach();assert(f.scenario.step==3&&f.player.credits==1&&f.Cleanup()==3);f.Reach();assert(f.player.credits==2&&f.Cleanup()==6);++cases;}
 {Fixture f;f.Reach();assert(f.player.credits==1&&f.Cleanup()==3&&f.actor.motion.moves==0&&f.ai.casts==0);++cases;}
 std::cout<<"PASS "<<cases<<" pinned expected-broken first-room counterfactuals: absent/ineligible recipient still cleans room; duplicate/unissued point3 awards\n";
#else
 {Fixture f;f.Reach();f.Tick(10000);assert(f.player.credits==0&&f.Cleanup()==0);++cases;}
 {Fixture f;f.Point(2,false);f.ai.MovementInform(2,2);f.ai.MovementInform(2,3);assert(f.ai.casts==0&&f.Cleanup()==0);++cases;}
 {Fixture f;f.ai._firstFight.completed=false;f.Start();f.Tick(10000);f.Reach();assert(f.ai.casts==0&&f.player.credits==0&&f.Cleanup()==0);++cases;}
 {Fixture f;f.Start();f.Start();assert(f.ai.casts==1&&f.ai.speech.empty());assert(!f.sparks[0].visible&&f.sparks[1].visible&&f.sparks[2].visible&&f.sparks[3].visible&&!f.sparks[4].visible);f.Reach();assert(f.player.credits==0);f.Tick(3999);assert(f.ai.speech.empty());f.Tick(1);assert((f.ai.speech==std::vector<uint32>{19})&&!f.actor.HasAura(185414));for(auto& s:f.sparks)assert(!s.visible);f.Tick(2999);assert(f.actor.motion.moves==0);f.Tick(1);assert((f.ai.speech==std::vector<uint32>{19,20})&&f.actor.motion.moves==1&&f.player.credits==0);f.Reach();assert(f.player.credits==1&&f.scenario.step==4&&f.Cleanup()==3);f.Reach();f.Tick(10000);assert(f.player.credits==1&&f.Cleanup()==3&&f.ai.casts==1);++cases;}
 {Fixture f;f.map.players.clear();f.Route();f.Reach();f.Tick(10000);assert(f.player.credits==0&&f.Cleanup()==0&&warnings==1);f.map.players={{&f.player}};f.Tick(0);assert(f.player.credits==1&&f.Cleanup()==3);++cases;}
 for(uint32 i=0;i<14;++i){Fixture f;f.Route();Player invalid=f.player;
  switch(i){case 0:invalid.world=false;break;case 1:invalid.predelete=true;break;case 2:invalid.contact=false;break;case 3:invalid.gm=true;break;case 4:invalid.teleport=true;break;case 5:invalid.cls=1;break;case 6:invalid.map=&f.other;break;case 7:invalid.instance=2;break;case 8:invalid.phase=false;break;case 9:invalid.distance=100.01f;break;case 10:invalid.rewarded=true;break;case 11:invalid.status=0;break;case 12:invalid.status=1;break;case 13:invalid.status=6;break;}
  f.actor.nearest=&invalid;f.map.players={{nullptr},{&invalid}};f.Reach();f.Tick(10000);assert(invalid.credits==0&&f.Cleanup()==0);f.map.players.push_back({&f.player});f.Tick(0);assert(f.player.credits==1&&f.Cleanup()==3);++cases;
 }
 for(uint32 i=0;i<10;++i){Fixture f;
  switch(i){case 0:f.actor.world=false;break;case 1:f.actor.predelete=true;break;case 2:f.actor.alive=false;break;case 3:f.actor.entry=106639;break;case 4:f.actor.mapid=0;break;case 5:f.actor.map=nullptr;break;case 6:manager.scenario=nullptr;break;case 7:f.scenario.id=991;break;case 8:f.scenario.id=1037;break;case 9:f.scenario.step=2;break;}
  f.Start();f.Tick(10000);f.Reach();assert(f.ai.casts==0&&f.player.credits==0&&f.Cleanup()==0);++cases;
 }
 for(uint32 i=0;i<10;++i){Fixture f;f.Route();f.map.players.clear();f.Reach();
  switch(i){case 0:f.actor.world=false;break;case 1:f.actor.predelete=true;break;case 2:f.actor.alive=false;break;case 3:f.actor.entry=106639;break;case 4:f.actor.mapid=0;break;case 5:f.actor.map=nullptr;break;case 6:manager.scenario=nullptr;break;case 7:f.scenario.id=991;break;case 8:f.scenario.id=1037;break;case 9:f.scenario.step=2;break;}
  f.map.players={{&f.player}};f.Tick(10000);assert(f.player.credits==0&&f.Cleanup()==0);++cases;
 }
 {Fixture f;f.Route();f.player.alive=false;f.player.distance=100;f.Reach();assert(f.player.credits==1&&f.Cleanup()==3);++cases;}
 {Fixture f;f.Route();f.player.accept=false;f.Reach();f.Tick(999);assert(f.player.credits==1&&f.Cleanup()==0);f.player.accept=true;f.Tick(1);assert(f.player.credits==2&&f.Cleanup()==3);++cases;}
 {Fixture f;f.Route();f.player.accept=false;f.Reach();for(uint32 i=0;i<60;++i)f.Tick(1000);assert(f.player.credits==30&&f.Cleanup()==0);f.scenario.step=4;f.Tick(0);assert(f.Cleanup()==3&&f.player.credits==30);++cases;}
 {Fixture f;f.ai.onCast=[&]{f.Start();};f.ai.onTalk=[&]{f.Start();};f.actor.motion.onMove=[&]{f.Reach();};f.player.onCredit=[&]{f.Reach();f.actor.Tick(0);};f.Route();assert(f.player.credits==1&&f.Cleanup()==3&&f.ai.casts==1);++cases;}
 {Fixture f;f.Route();f.map.players.clear();f.Reach();f.ai.Reset();f.map.players={{&f.player}};f.actor.IsAIEnabled=false;f.actor.primary=nullptr;f.actor.i_disabledAI=&f.ai;f.Tick(10000);assert(f.Cleanup()==0);f.actor.primary=&f.ai;f.actor.i_disabledAI=nullptr;f.actor.IsAIEnabled=true;f.Tick(0);assert(f.Cleanup()==3&&f.player.credits==1);++cases;}
 {Fixture f;f.Start();f.actor.states=UNIT_STATE_STUNNED;f.Tick(20000);assert(f.actor.HasAura(185414)&&f.ai.speech.empty());f.actor.states=0;f.Tick(4000);assert(f.ai.speech.back()==19);f.Tick(3000);f.actor.states=UNIT_STATE_STUNNED;f.Reach();f.Tick(20000);assert(f.Cleanup()==0);f.actor.states=0;f.Tick(0);assert(f.Cleanup()==3);++cases;}
 {Fixture f;f.ai.castSucceeds=false;f.actor.sparks.clear();f.actor.guardian=nullptr;f.actor.gates.clear();f.Route();assert(f.player.credits==0);f.Reach();assert(f.player.credits==1&&f.ai._firstRoom.completed);++cases;}
 {Fixture f;f.Start();f.scenario.step=4;f.Tick(10000);assert(f.actor.motion.moves==0&&f.player.credits==0&&f.Cleanup()==0&&!f.actor.HasAura(185414));++cases;}
 {Fixture f;f.Route();f.scenario.step=4;f.map.players.clear();f.Reach();assert(f.player.credits==0&&f.Cleanup()==3);++cases;}
 {Fixture f;f.Route();f.player.onCredit=[&]{f.scenario.step=5;f.player.accept=false;};f.Reach();f.Tick(10000);assert(f.Cleanup()==0&&f.player.credits==1);++cases;}
 {Fixture f;auto doomed=std::make_unique<Room>(&f.actor);f.actor.primary=doomed.get();f.Point(2);assert(doomed->casts==1);doomed.reset();Room replacement(&f.actor);f.actor.primary=&replacement;f.Point(3);replacement.Ordinary(10000);assert(f.player.credits==0&&f.Cleanup()==0&&f.actor.motion.moves==0);f.actor.primary=&f.ai;++cases;}
 std::cout<<"PASS "<<cases<<" ASAN actual room policy/methods/native4s+7s queue/PointFinalize/lifecycle cases; controlled geometry, actor lists, casts, fight receipt and native-event result, no client traversal\n";
#endif
}
