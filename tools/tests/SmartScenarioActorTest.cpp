#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
using uint32=uint32_t;
constexpr int CONDITION_SCENARION_STEP=44;
struct Scenario {uint32 id=0,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} scenario;
struct Map {uint32 id=1;uint32 GetInstanceId(){return id;}Map* ToInstanceMap(){return this;}} map;
using InstanceMap=Map;
struct Manager {Scenario* GetScenario(uint32 id){return id==1?&scenario:nullptr;}} manager;
auto sScenarioMgr=&manager;
struct WorldObject {bool instanced=true;bool InInstance(){return instanced;}Map* GetMap(){return &map;}};
using Unit=WorldObject;
struct ConditionSourceInfo {WorldObject* targets[2];ConditionSourceInfo(WorldObject* a,WorldObject* b):targets{a,b}{}};
bool evaluate(WorldObject* object,uint32 ConditionValue1,uint32 ConditionValue2)
{
 bool condMeets=false;if(!object)return false;
 switch(CONDITION_SCENARION_STEP){
#include "ScenarioCondition.inc"
 }
 return condMeets;
}
bool gate(uint32 target,Unit* unit,WorldObject* baseObject,uint32 id,uint32 step)
{
#include "SmartTargetBinding.inc"
 return evaluate(sourceInfo.targets[target],id,step);
}
int main(int argc,char** argv)
{
 assert(argc==2);std::ifstream input(argv[1]);assert(input);
 int64_t entry;uint32 source,group,id,step,negative,target,count=0;
 WorldObject base;
 while(input>>entry>>source>>group>>id>>step>>negative>>target){
  assert((source==0||source==1||source==9)&&target==1&&group);
  scenario.id=id;scenario.step=step;
  assert(!gate(0,nullptr,&base,id,step)); // actual initial native gate, before ProcessEvent fallback
  assert(gate(target,nullptr,&base,id,step));
  assert(!gate(target,nullptr,&base,id+1,step));
  assert(!gate(target,nullptr,&base,id,step+1));
  base.instanced=false;assert(!gate(target,nullptr,&base,id,step));base.instanced=true;
  assert(!gate(target,nullptr,nullptr,id,step));++count;
 }
 assert(count>300);
 std::cout<<"PASS: "<<count<<" migrated scenario actor conditions through native initial target binding and native scenario predicate; null callback, wrong stage/instance and missing actor boundaries\n";
}
