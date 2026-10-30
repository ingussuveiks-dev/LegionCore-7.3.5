#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;using int64=int64_t;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
#include "Constants.inc"
enum {CONDITION_MAPID=22,CONDITION_SCENARION_STEP=44};
struct InstanceMap{uint32 id=42;uint32 GetInstanceId()const{return id;}} instanceMap;
struct Map{bool instanced=true;InstanceMap* ToInstanceMap(){return instanced?&instanceMap:nullptr;}} worldMap;
struct Unit;
struct ObjectGuid{Unit* unit=nullptr;};
struct Unit{
 uint32 map=1480;bool expired=false;ObjectGuid GetGUID(){return {this};}bool isInCombat()const{return false;}
 uint32 GetMapId()const{return map;}Map* GetMap(){return &worldMap;}bool InInstance()const{return worldMap.instanced;}bool IsQuestObjectExpired()const{return expired;}
};
using WorldObject=Unit;using Creature=Unit;struct SpellInfo{};struct GameObject{};
struct Scenario{uint32 id=957,step=3;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}};
struct ScenarioMgr{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct ConditionSourceInfo{WorldObject* mConditionTargets[2];ConditionSourceInfo(WorldObject* first,WorldObject* second):mConditionTargets{first,second}{}};
struct Condition{
 uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;bool NegativeCondition=false;
 bool isLoaded()const{return true;}bool Meets(ConditionSourceInfo& sourceInfo){auto object=sourceInfo.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;switch(ConditionType){
#include "ActorConditions.inc"
 default:assert(false);}
 return NegativeCondition?!condMeets:condMeets;}
};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;using ConditionTypeContainer=std::map<uint32,ConditionList>;using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr{
 SmartEventConditionContainer SmartEventConditionStore;ConditionReferenceContainer ConditionReferenceStore;
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& rows)const{return rows.empty()||IsObjectMeetToConditionList(info,rows);}
 bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*)const;
 ConditionList GetConditionsForSmartEvent(int64,uint32,uint32);
} conditionManager;
auto sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
#include "SmartEventConditions.inc"
#include "SmartEventLookup.inc"
struct SmartScriptHolder{
 uint32 entryOrGuid=99763,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,parameter=0;bool active=true,runOnce=false;
 struct{uint32 event_flags=0,event_phase_mask=0,event_chance=100;struct{uint32 min=0,max=0,repeatMin=0,repeatMax=0;}minMaxRepeat;struct{uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;}dataSet;}event;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}
};
struct Call{uint32 action,parameter;};
bool roll_chance_i(uint32){return true;}
struct SmartScript{
 explicit SmartScript(std::vector<SmartScriptHolder> rows):mEvents(std::move(rows)){}
 std::vector<SmartScriptHolder> mEvents;std::vector<Call> calls;Creature actor;Creature* me=&actor;ObjectGuid mLastInvoker;
 Unit* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}bool IsInPhase(uint32)const{return true;}void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LinkEvent.inc"
#include "UpdateEvent.inc"
#include "DataSetEvent.inc"
 }
}
#include "ActionPrefix.inc"
 // Native cast/clear/motion/TALK endpoints are observed as requests only.
 calls.push_back({e.actionType,e.parameter});
#include "LinkTail.inc"
static std::vector<SmartScriptHolder> Read(char const* path){std::ifstream input(path);assert(input);std::vector<SmartScriptHolder> rows;std::string line;while(std::getline(input,line)){
 std::stringstream ss(line);std::vector<std::string> f;std::string cell;while(std::getline(ss,cell,'\t'))f.push_back(cell);assert(f.size()==31);auto n=[&](unsigned i){return uint32(std::stoul(f[i]));};assert(n(0)==99763&&n(1)==0);SmartScriptHolder e;e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.actionType=n(14);e.parameter=n(15);e.targetType=n(21);rows.push_back(e);
 }return rows;}
static void Intro(SmartScript& s,bool expected){s.ProcessEventsFor(SMART_EVENT_UPDATE);assert(s.calls.size()==unsigned(expected?3:0));if(expected)assert(s.calls[0].parameter==12980&&s.calls[1].parameter==196561&&s.calls[2].parameter==196347);s.ProcessEventsFor(SMART_EVENT_UPDATE);assert(s.calls.size()==unsigned(expected?3:0));}
static void Outro(SmartScript& s){auto size=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DATA_SET,nullptr,3,3);assert(s.calls.size()==size+3&&s.calls[size].action==28&&s.calls[size+1].action==53&&s.calls[size+2].action==1);s.ProcessEventsFor(SMART_EVENT_DATA_SET,nullptr,3,3);assert(s.calls.size()==size+3);}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);auto actors=Read(argv[1]);assert(actors.size()==11);std::ifstream input(argv[2]);assert(input);std::vector<Condition> rules(2);for(auto& c:rules){assert(input>>c.ElseGroup>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition);assert(c.ConditionTarget==1);}
 auto make=[&](){return SmartScript(actors);};
 // Original native data3 before first UPDATE starts the outro then battle intro.
 scenarioManager.scenario.step=4;{auto s=make();Outro(s);s.ProcessEventsFor(SMART_EVENT_UPDATE);assert(s.calls.size()==6&&s.calls.back().parameter==196347);}
 for(auto& c:rules)conditionManager.SmartEventConditionStore[{99763,0}][1].push_back(&c);
 for(uint32 step=0;step<=6;++step){scenarioManager.scenario.step=step;auto s=make();Intro(s,step==3);if(step>=4){Outro(s);auto size=s.calls.size();s.ProcessEventsFor(SMART_EVENT_UPDATE);assert(s.calls.size()==size);}}
 scenarioManager.scenario.step=3;{auto s=make();Intro(s,true);scenarioManager.scenario.step=4;Outro(s);s.ProcessEventsFor(SMART_EVENT_UPDATE);assert(s.calls.size()==6);}
 for(unsigned mode=0;mode<5;++mode){scenarioManager={{957,3},true};worldMap.instanced=true;instanceMap.id=42;auto s=make();if(mode==0)s.actor.map=1489;if(mode==1)scenarioManager.scenario.id=901;if(mode==2)scenarioManager.present=false;if(mode==3)worldMap.instanced=false;if(mode==4)instanceMap.id=43;Intro(s,false);}
 scenarioManager={{957,3},true};worldMap.instanced=true;instanceMap.id=42;
 // A nullable UPDATE invoker cannot be used as target0 for actor admission.
 for(auto& c:rules)c.ConditionTarget=0;{auto s=make();Intro(s,false);}for(auto& c:rules)c.ConditionTarget=1;{auto s=make();Intro(s,true);}
 {auto s=make();s.me=nullptr;Intro(s,false);}
 std::cout<<"PASS actual native SmartEvent outer+inner condition dispatch, actor-target1 map/scenario cases and ElseGroups: original late UPDATE battle-intro after data3 baseline vs retained957/3 only; null target0 counterfactual, stage0..6/shared901/wrongmap/instance/missing actor rejection; real data3 cleanup/waypoint/TALK requests remain once at4/5, with repeated first updates and ordinary intro->outro retained. World/cast/movement endpoints controlled.\n";
}
