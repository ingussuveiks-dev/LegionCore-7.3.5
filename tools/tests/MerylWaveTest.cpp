// Actual SetScript9 and position-summon branch; controlled DB/actor endpoints.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;
constexpr uint32 SMART_SCRIPT_TYPE_TIMED_ACTIONLIST=9,SMART_EVENT_UPDATE_OOC=1,SMART_EVENT_UPDATE_IC=0,SMART_EVENT_UPDATE=60,SMART_TARGET_POSITION=8,CONDITION_SCENARION_STEP=44;
using TempSummonType=uint32;
struct SmartScriptHolder {
 long entryOrGuid=0;uint32 source_type=0,event_id=0,actionType=0;bool enableTimed=false;
 struct {uint32 type=0;} event;
 union Action {struct {uint32 id,timerType;} timedActionList;struct {uint32 creature,type,duration,attackInvoker,phaseByTarget,pad;} summonCreature;uint32 raw[6];Action():raw{}{} } action;
 struct {uint32 type=0;float x=0,y=0,z=0,o=0;} target;
 uint32 GetEventType()const{return event.type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}uint32 GetTargetType()const{return target.type;}
};
using SmartAIEventList=std::vector<SmartScriptHolder>;
struct Manager {std::map<uint32,SmartAIEventList> lists;SmartAIEventList GetScript(uint32 id,uint32 type){assert(type==9);return lists[id];}} manager;auto sSmartScriptMgr=&manager;
struct Unit{};
struct Creature {
 struct AIType {unsigned attacks=0;void AttackStart(Unit*){++attacks;}} ai;
 AIType* AI(){return &ai;}
 void GetPosition(float& x,float& y){x=0;y=0;}float GetPositionZ(){return 0;}
};
struct SpawnEndpoint {
 struct Spawn {uint32 id,type,duration;float x,y,z,o;};std::vector<Spawn> requests;bool fail=false;Creature summon;
 Creature* SummonCreature(uint32 id,float x,float y,float z,float o,TempSummonType type,uint32 duration){requests.push_back({id,type,duration,x,y,z,o});return fail?nullptr:&summon;}
};
struct SmartScript {
 bool isProcessingTimedActionList=false;SmartAIEventList mTimedActionList;unsigned errors=0,initialized=0;
 SpawnEndpoint endpoint;Creature actor;Creature* me=&actor;
 void InitTimer(SmartScriptHolder&){++initialized;}
 SpawnEndpoint* GetBaseObject(){return &endpoint;}
 void SetScript9(SmartScriptHolder&,uint32);
 void PositionSummon(SmartScriptHolder const& e,Unit* unit){
#include "MerylPositionSummon.inc"
 }
};
#define SI64FMTD "%ld"
#define TC_LOG_ERROR(...) ++errors
#include "MerylSetScript9.inc"
struct Scenario {uint32 id=1122,step=3;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}} scenario;
struct Map {bool instance=true;uint32 id=1;Map* ToInstanceMap(){return instance?this:nullptr;}uint32 GetInstanceId()const{return id;}} map;
using InstanceMap=Map;
struct ScenarioManager {bool available=true;Scenario* GetScenario(uint32 id){return available&&id==1?&scenario:nullptr;}} scenarioManager;auto sScenarioMgr=&scenarioManager;
struct Actor {bool InInstance()const{return map.instance;}Map* GetMap(){return &map;}} actor;
bool Eligible(){auto object=&actor;uint32 ConditionValue1=1122,ConditionValue2=3;bool condMeets=false;switch(CONDITION_SCENARION_STEP){
#include "MerylScenarioCondition.inc"
 }return condMeets;}
static SmartAIEventList Read(char const* path){std::ifstream f(path);std::string line;SmartAIEventList rows;while(std::getline(f,line)){
 std::stringstream ss(line);std::vector<std::string> fields;std::string cell;while(std::getline(ss,cell,'\t'))fields.push_back(cell);assert(fields.size()==31);
 auto n=[&](size_t i){return uint32(std::stoul(fields[i]));};SmartScriptHolder e;e.entryOrGuid=std::stol(fields[0]);e.source_type=n(1);e.event_id=n(2);e.event.type=n(5);e.actionType=n(14);for(size_t i=0;i<6;++i)e.action.raw[i]=n(15+i);
 e.target={n(21),std::stof(fields[26]),std::stof(fields[27]),std::stof(fields[28]),std::stof(fields[29])};rows.push_back(e);
 }return rows;}
int main(int argc,char** argv){
 if(argc!=3)return 1;auto original=Read(argv[1]),fixed=Read(argv[2]);SmartScriptHolder before,after;
 for(auto const& e:original)if(e.entryOrGuid==107691&&!e.source_type&&e.event_id==1)before=e;
 for(auto const& e:fixed){if(e.entryOrGuid==107691&&!e.source_type&&e.event_id==1)after=e;if(e.source_type==9)manager.lists[uint32(e.entryOrGuid)].push_back(e);}
 assert(before.action.raw[5]==10809706&&after.action.raw[5]==10809705&&Eligible());
 SmartScript old;old.SetScript9(before,before.action.raw[5]);assert(old.mTimedActionList.empty()); // Expected old sixth-branch no-wave counterfactual.
 uint32 expectedCounts[]={5,5,2,5,5,1};unsigned total=0;
 for(size_t choice=0;choice<6;++choice){
  SmartScript s;uint32 id=after.action.raw[choice];s.SetScript9(after,id);
  assert(!s.errors&&s.initialized==expectedCounts[choice]&&s.mTimedActionList.size()==expectedCounts[choice]);
  for(size_t i=0;i<s.mTimedActionList.size();++i){auto& e=s.mTimedActionList[i];assert(e.enableTimed==(i==0)&&e.event.type==SMART_EVENT_UPDATE&&e.GetActionType()==12&&e.GetTargetType()==8);s.PositionSummon(e,nullptr);}
  assert(s.endpoint.requests.size()==expectedCounts[choice]);total+=unsigned(s.endpoint.requests.size());
  if(choice==5){auto const& e=s.mTimedActionList.front();auto const& r=s.endpoint.requests.front();assert(e.event_id==4&&e.enableTimed&&r.id==107744&&r.type==8&&!r.duration&&r.x==e.target.x&&r.y==e.target.y&&r.z==e.target.z&&r.o==e.target.o);}
  // Repeated selections replace the list; no append or synthetic progress.
  s.SetScript9(after,id);assert(s.mTimedActionList.size()==expectedCounts[choice]);
  s.isProcessingTimedActionList=true;s.SetScript9(after,10809706);assert(s.errors==1&&s.mTimedActionList.size()==expectedCounts[choice]);
  s.isProcessingTimedActionList=false;s.SetScript9(after,0);assert(s.mTimedActionList.empty());
 }
 SmartScript failed;failed.endpoint.fail=true;failed.SetScript9(after,10809705);failed.PositionSummon(failed.mTimedActionList.front(),nullptr);assert(failed.endpoint.requests.size()==1&&!failed.endpoint.summon.ai.attacks);
 for(auto id:{1221u,1037u,0u}){scenario.id=id;assert(!Eligible());}scenario.id=1122;
 for(auto step:{0u,2u,4u,5u}){scenario.step=step;assert(!Eligible());}scenario.step=3;
 map.instance=false;assert(!Eligible());map.instance=true;map.id=0;assert(!Eligible());map.id=1;scenarioManager.available=false;assert(!Eligible());
 std::printf("PASS native SetScript9: old missing sixth list counterfactual; six repaired lists/%u original spawn requests, first id4 enabled/ALWAYS, native position107744/type8/zero-duration/null-invoker/failed-spawn branch, replacement/iteration refusal/clear; actual1122stage3 predicate excludes hub and other scenarios. Core192-choice ASan probe separately verifies RNG; timer tick/client/combat behavior not simulated here.\n",total);
}
