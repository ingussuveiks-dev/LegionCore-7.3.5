#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
using int64=int64_t;
#define ASSERT(x) assert(x)
#define TC_LOG_DEBUG(...) ((void)0)
enum{CONDITION_SCENARION_STEP=44};
struct InstanceMap{uint32 id=42;uint32 GetInstanceId(){return id;}}instance;
struct Map{bool instanced=true;InstanceMap*ToInstanceMap(){return instanced?&instance:nullptr;}}world;
struct Scenario{uint32 id=1037,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}}scenario;
struct ScenarioManager{bool present=true;Scenario*GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}}manager;
auto*sScenarioMgr=&manager;
struct WorldObject{bool InInstance(){return world.instanced;}Map*GetMap(){return &world;}};
using Unit=WorldObject;
struct Condition;
struct ConditionSourceInfo{WorldObject*mConditionTargets[2];ConditionSourceInfo(WorldObject*a,WorldObject*b):mConditionTargets{a,b}{}};
struct Condition{
 uint32 ElseGroup=0,ConditionType=44,ConditionTarget=1,ConditionValue1=1037,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;bool NegativeCondition=false;
 bool isLoaded()const{return true;}
 bool Meets(ConditionSourceInfo&source){auto*object=source.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;switch(ConditionType){
#include "ScenarioCondition.inc"
 default:assert(false);
 }return NegativeCondition?!condMeets:condMeets;}
};
using ConditionList=std::vector<Condition*>;
using ConditionReferenceContainer=std::map<uint32,ConditionList>;
using ConditionTypeContainer=std::map<uint32,ConditionList>;
using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr{
 ConditionReferenceContainer ConditionReferenceStore;SmartEventConditionContainer SmartEventConditionStore;
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(ConditionSourceInfo&source,ConditionList const&rows)const{return rows.empty()||IsObjectMeetToConditionList(source,rows);}
 bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*)const;
}conditions;
#include "ElseGroups.inc"
#include "SmartConditions.inc"
int main(int argc,char**argv){
 assert(argc==2);std::ifstream input(argv[1]);std::vector<Condition>rows(2);ConditionList list;
 for(auto&c:rows){assert(input>>c.ElseGroup>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition);list.push_back(&c);}
 WorldObject ritualist,pet;conditions.SmartEventConditionStore[{101875,0}][7]=list;
 for(uint32 id:{1037u,991u,1092u})for(uint32 step=0;step<12;++step){scenario.id=id;scenario.step=step;bool allowed=id==1037&&step<=1;
  assert(conditions.IsObjectMeetingSmartEventConditions(101875,6,0,nullptr,&ritualist)==allowed);
  assert(conditions.IsObjectMeetingSmartEventConditions(101875,6,0,&pet,&ritualist)==allowed);
 }
 // Counterfactual original AND group refuses every stage, even for the real actor.
 rows[0].ElseGroup=rows[1].ElseGroup=0;scenario.id=1037;
 for(uint32 step=0;step<12;++step){scenario.step=step;assert(!conditions.IsObjectMeetingSmartEventConditions(101875,6,0,nullptr,&ritualist));}
 rows[0].ElseGroup=rows[0].ConditionValue2==0?1:0;rows[1].ElseGroup=rows[1].ConditionValue2==0?1:0;scenario.step=1;
 manager.present=false;assert(!conditions.IsObjectMeetingSmartEventConditions(101875,6,0,nullptr,&ritualist));manager.present=true;
 instance.id=43;assert(!conditions.IsObjectMeetingSmartEventConditions(101875,6,0,nullptr,&ritualist));instance.id=42;
 world.instanced=false;assert(!conditions.IsObjectMeetingSmartEventConditions(101875,6,0,nullptr,&ritualist));world.instanced=true;
 assert(!conditions.IsObjectMeetingSmartEventConditions(101875,6,0,nullptr,nullptr));
 assert(conditions.IsObjectMeetingSmartEventConditions(101875,0,0,nullptr,&ritualist)); // untouched Priest event remains native default admission
 std::cout<<"PASS native source22 event+1/base-target dispatch and native scenario/ElseGroups: original0AND1 never admits; repaired0OR1 accepts only real1037 early stages, null/pet invoker, rejects wrong/missing instance/scenario/base. Does not simulate cast/channel/rendering.\n";
}
