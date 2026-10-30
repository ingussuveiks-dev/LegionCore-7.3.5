// Actual native condition target/negative/ElseGroup/scenario/nearest predicate.
// Grid enumeration, geometry and script-hook endpoints are controlled. No SQL
// mutation or scenario credit; actual repaired SQL1 is compared with original0.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;using int64=int64_t;
struct WorldObject;struct Creature;struct Condition;
struct Map{uint32 id=7;Map* ToInstanceMap(){return this;}uint32 GetInstanceId(){return id;}};
using InstanceMap=Map;
struct Scenario{uint32 id=940,step=3;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct ScenarioMgr{Scenario scenario;bool missing=false;Scenario* GetScenario(uint32 id){return missing||id!=7?nullptr:&scenario;}} manager;
auto sScenarioMgr=&manager;
std::vector<Creature*> grid;
struct WorldObject {
 float x=0,y=0,z=0;Map* map=nullptr;bool phase=true;
 bool InInstance(){return map!=nullptr;}Map* GetMap(){return map;}
 float GetDistance(Creature*)const;bool IsWithinDistInMap(Creature*,float)const;
 Creature* FindNearestCreature(uint32,float,bool=true);
};
struct Creature:WorldObject{uint32 entry=102244;bool alive=true;uint32 GetEntry(){return entry;}bool IsAlive(){return alive;}};
float WorldObject::GetDistance(Creature* u)const{return std::sqrt((x-u->x)*(x-u->x)+(y-u->y)*(y-u->y)+(z-u->z)*(z-u->z));}
bool WorldObject::IsWithinDistInMap(Creature* u,float range)const{return map==u->map&&phase==u->phase&&GetDistance(u)<=range;}
namespace Trinity {
class NearestCreatureEntryWithLiveStateInObjectRangeCheck {
 WorldObject const& i_obj;uint32 i_entry;bool i_alive;float i_range;
public:
 NearestCreatureEntryWithLiveStateInObjectRangeCheck(WorldObject const&,uint32,bool,float);
 bool operator()(Creature*);float GetLastRange()const{return i_range;}
};
#include "Nearest.inc"
}
Creature* WorldObject::FindNearestCreature(uint32 entry,float distance,bool alive){Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck check(*this,entry,alive,distance);Creature* found=nullptr;for(auto u:grid)if(check(u))found=u;return found;}
#include "Closest.inc"
constexpr unsigned MAX_CONDITION_TARGETS=3,CONDITION_NEAR_CREATURE=29,CONDITION_SCENARION_STEP=44;
struct ConditionSourceInfo{WorldObject* mConditionTargets[3];Condition* mLastFailedCondition=nullptr;ConditionSourceInfo(WorldObject*,WorldObject*,WorldObject* =nullptr);};
#include "SourceInfo.inc"
struct Scripts{bool allow=true;bool OnConditionCheck(Condition*,ConditionSourceInfo&){return allow;}} scripts;
auto sScriptMgr=&scripts;
#define TC_LOG_DEBUG(...) ((void)0)
struct Condition {
 uint32 ConditionTarget=0,ConditionType=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,SourceEntry=102244,SourceType=22,SourceGroup=4,ElseGroup=0,ReferenceId=0;
 bool NegativeCondition=false;
 bool isLoaded()const{return ConditionType!=0;}bool Meets(ConditionSourceInfo&);
};
#include "ConditionPrefix.inc"
 switch(ConditionType){
#include "NearCondition.inc"
#include "ScenarioCondition.inc"
 }
#include "ConditionTail.inc"
using ConditionList=std::vector<Condition*>;
using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr{ConditionReferenceContainer ConditionReferenceStore;bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;};
#include "ConditionList.inc"
ConditionSourceInfo EventInfo(WorldObject* unit,WorldObject* source){auto GetBaseObject=[&](){return source;};
#include "EventInfo.inc"
 return info;
}
int main(int argc,char** argv){
 assert(argc==3);std::ifstream input(argv[1]);assert(input);std::vector<Condition> values;Condition c;unsigned negative;
 while(input>>c.SourceGroup>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>negative){c.NegativeCondition=negative!=0;values.push_back(c);}
 assert(values.size()==2);Condition* near=nullptr;ConditionList list;for(auto& row:values){assert(row.SourceGroup==4&&row.ElseGroup==0);list.push_back(&row);if(row.ConditionType==29)near=&row;}
 assert(near&&near->ConditionTarget==0&&near->ConditionValue1==102244&&near->ConditionValue2==100&&near->NegativeCondition);
 std::ifstream applied(argv[2]);assert(applied);unsigned rowIndex=0,edits=0;
 while(applied>>c.SourceGroup>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>negative){
  assert(rowIndex<values.size());auto const& old=values[rowIndex++];
  assert(c.SourceGroup==old.SourceGroup&&c.ConditionType==old.ConditionType&&c.ConditionValue1==old.ConditionValue1&&c.ConditionValue2==old.ConditionValue2&&c.ConditionValue3==old.ConditionValue3&&bool(negative)==old.NegativeCondition);
  if(c.ConditionType==29){assert(c.ConditionTarget==1);++edits;}else assert(c.ConditionTarget==old.ConditionTarget);
 }
 assert(rowIndex==values.size()&&edits==1);
 ConditionMgr conditions;Map map,other;Creature source,remaining;WorldObject killer;source.map=remaining.map=killer.map=&map;source.alive=false;source.x=0;remaining.x=70;killer.x=-40;grid={&source,&remaining};
 auto admitted=[&](unsigned target,WorldObject* invoker){near->ConditionTarget=target;auto info=EventInfo(invoker,&source);return conditions.IsObjectMeetToConditionList(info,list);};
 // Same dead actor and living70yd neighbour; changing only killer position
 // changes current0 admission. Proposed1 consistently anchors the encounter.
 assert(admitted(0,&killer)&&!admitted(1,&killer));killer.x=0;assert(!admitted(0,&killer)&&!admitted(1,&killer));
 // A remote same-entry creature near the killer must not block a cleared
 // source encounter. Ordinary player/pet/guardian classification cannot fix
 // this spatial anchor: source0 uses the actual raw event object for all.
 remaining.x=200;killer.x=190;assert(!admitted(0,&killer)&&admitted(1,&killer));
 for(float x:{-1000.f,-40.f,0.f,190.f,1000.f}){killer.x=x;remaining.x=70;assert(!admitted(1,&killer));remaining.alive=false;assert(admitted(1,&killer));remaining.alive=true;}
 for(unsigned step:{0u,1u,2u,4u,5u,10u}){manager.scenario.step=step;remaining.alive=false;assert(!admitted(1,&killer));}manager.scenario.step=3;
 manager.scenario.id=941;assert(!admitted(1,&killer));manager.scenario.id=940;manager.missing=true;assert(!admitted(1,&killer));manager.missing=false;
 remaining.alive=true;remaining.x=100;assert(!admitted(1,&killer));remaining.x=100.01f;assert(admitted(1,&killer));
 remaining.x=70;remaining.entry=999;assert(admitted(1,&killer));remaining.entry=102244;remaining.map=&other;assert(admitted(1,&killer));remaining.map=&map;
 scripts.allow=false;remaining.alive=false;assert(!admitted(1,&killer));scripts.allow=true;
 assert(admitted(1,nullptr));source.map=nullptr;assert(!admitted(1,&killer));source.map=&map;
 near->ConditionTarget=3;assert(!admitted(3,&killer));
 std::cout<<"COUNTERFACTUAL original target0 can prematurely admit or falsely block guard-clear from killer position. PASS repaired actual SQL source1 native condition/negative/ElseGroup/940-step3 and actual nearest-alive predicate,source death excludes self,100 boundary,wrong/missing scenario,script veto/map and missing invoker. Geometry/grid enumeration controlled; full encounter/client traversal remains separate.\n";
}
