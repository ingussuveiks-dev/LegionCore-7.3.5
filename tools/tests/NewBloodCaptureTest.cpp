#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <iostream>
using uint32=uint32_t; using uint64=uint64_t; using int64=int64_t;
#define TC_LOG_DEBUG(...) ((void)0)
struct WorldObject {uint32 map=1502;uint32 GetMapId() const{return map;}};
struct Unit:WorldObject {};
struct ConditionSourceInfo {WorldObject* mConditionTargets[3];ConditionSourceInfo(WorldObject* a,WorldObject* b):mConditionTargets{a,b,nullptr}{}};
struct Condition {uint32 target,map;};
using ConditionTypeContainer=std::map<uint32,std::vector<Condition>>;
using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr {
 SmartEventConditionContainer SmartEventConditionStore;
 bool IsObjectMeetToConditions(ConditionSourceInfo& info,std::vector<Condition> const& conditions) const {
  for(auto c:conditions){auto object=info.mConditionTargets[c.target];if(object && object->GetMapId()==c.map)return true;}return false;
 }
 bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*) const;
};
#include "ConditionsCall.inc"
int main() {
 ConditionMgr mgr; WorldObject boss; Unit wrongInvoker;wrongInvoker.map=1522;
 mgr.SmartEventConditionStore[{98831,0}][3]={{0,1502},{0,1220}};
 // Reproduce the old data against the REAL dispatcher: SetData passes null.
 assert(!mgr.IsObjectMeetingSmartEventConditions(98831,2,0,nullptr,&boss));
 mgr.SmartEventConditionStore[{98831,0}][3]={
#include "CaptureConditions.inc"
 };
 for(auto map:{1502u,1220u,1522u}) {
  boss.map=map;
  assert(mgr.IsObjectMeetingSmartEventConditions(98831,2,0,nullptr,&boss)==(map!=1522));
  assert(mgr.IsObjectMeetingSmartEventConditions(98831,2,0,&wrongInvoker,&boss)==(map!=1522));
 }
 std::cout<<"PASS: production SmartEvent condition dispatcher rejects old null-invoker guard; migrated base-object guards allow both Dalaran maps and exclude later map 1522.\n";
}
