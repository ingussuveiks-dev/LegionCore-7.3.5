#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>
using uint32=uint32_t;
enum {POINT_MOTION_TYPE=8,EFFECT_MOTION_TYPE=16,REACT_PASSIVE=0};
struct Creature;
struct CreatureAI {virtual ~CreatureAI()=default;virtual void Reset(){}virtual void MovementInform(uint32,uint32){}};
struct Vehicle {unsigned passengers=0,removals=0;std::function<void()> callback;void RemoveAllPassengers(){++removals;passengers=0;if(callback)callback();}};
struct Creature {Vehicle* kit=nullptr;unsigned state=1,despawns=0,delay=0;std::function<void()> onDespawn;
 Vehicle* GetVehicleKit(){return kit;}void SetReactState(unsigned value){state=value;}void DespawnOrUnsummon(unsigned value){++despawns;delay=value;if(onDespawn)onDespawn();}
};
struct ScriptedAI:CreatureAI {Creature* me;explicit ScriptedAI(Creature* c):me(c){}};
struct CreatureScript {explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual CreatureAI* GetAI(Creature*)const=0;};
#include "SharasdalWave.inc"
int main(){unsigned cases=0;
 for(unsigned kit=0;kit<2;++kit)for(unsigned mode=0;mode<4;++mode){
  Vehicle vehicle;vehicle.passengers=3;Creature wave;wave.kit=kit?&vehicle:nullptr;npc_sharasdal_waterwall script;auto ai=script.GetAI(&wave);assert(wave.state==REACT_PASSIVE);ai->Reset();
  wave.onDespawn=[&]{assert(!kit||vehicle.passengers==0);};
  if(mode==1)vehicle.callback=[&]{wave.kit=nullptr;};
  if(mode==2){ai->MovementInform(EFFECT_MOTION_TYPE,1);assert(!wave.despawns&&!vehicle.removals);}
  ai->MovementInform(POINT_MOTION_TYPE,mode==3?123:1);
#ifdef SHARASDAL_WAVE_BASELINE
  assert(wave.despawns==kit&&vehicle.removals==kit&&wave.delay==(kit?100u:0u));
#else
  assert(wave.despawns==1&&wave.delay==100&&vehicle.removals==kit);
#endif
  ai->Reset();delete ai;++cases;
 }
#ifdef SHARASDAL_WAVE_BASELINE
 std::cout<<"COUNTERFACTUAL Sharasdal wave source8 ASAN:missing kit suppresses authored cleanup;ordinary passenger order/100ms/nonpoint/reset retained\n";
#else
 std::cout<<"PASS Sharasdal wave source8 ASAN:whole factory/passive/reset,kit present/absent,passengers before despawn,kit removal during ejection,nonpoint refusal and original any-point/100ms cleanup\n";
#endif
 assert(cases==8);
}
