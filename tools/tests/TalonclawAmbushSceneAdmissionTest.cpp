#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
using int64=int64_t;
enum {CONDITION_SCENARION_STEP=44,SMART_EVENT_EVENTOBJECT_ONTRIGGER=89,SMART_EVENT_EVENTOBJECT_OFFTRIGGER=91};
#define TC_LOG_DEBUG(...) ((void)0)
struct Scenario {uint32 identity=973,step=9;uint32 GetScenarioId(){return identity;}uint32 GetCurrentStep(){return step;}};
struct ScenarioMgr {bool present=true;Scenario scenario;Scenario* GetScenario(uint32 instance){return present&&instance==42?&scenario:nullptr;}} scenarios;
auto sScenarioMgr=&scenarios;
struct InstanceMap {uint32 instance=42;uint32 GetInstanceId(){return instance;}};
struct Map {bool instanced=true;InstanceMap instance;InstanceMap* ToInstanceMap(){return instanced?&instance:nullptr;}};
struct WorldObject {Map map;bool InInstance(){return map.instanced;}Map* GetMap(){return &map;}};
struct Unit:WorldObject {};
struct Player:Unit {bool alive=true;bool IsAlive(){return alive;}};
struct EventObject:WorldObject {uint32 GetEntry(){return 108;}};
struct Condition {
    uint32 group,source,ConditionTarget,ConditionValue1,ConditionValue2;
    bool Meets(WorldObject* object){
        bool condMeets=false;
        switch(CONDITION_SCENARION_STEP){
#include "ScenarioCondition.inc"
        }
        return condMeets;
    }
};
struct ConditionSourceInfo {WorldObject* objects[2];ConditionSourceInfo(Unit* unit,WorldObject* base):objects{unit,base}{}};
using ConditionList=std::vector<Condition*>;
using ConditionTypeContainer=std::map<uint32,ConditionList>;
using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr {
    SmartEventConditionContainer SmartEventConditionStore;
    bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*) const;
    bool IsObjectMeetToConditions(ConditionSourceInfo& source,ConditionList const& conditions) const {
        for(auto condition:conditions)if(!source.objects[condition->ConditionTarget]||!condition->Meets(source.objects[condition->ConditionTarget]))return false;
        return true;
    }
} manager;
auto sConditionMgr=&manager;
#include "ConditionLookup.inc"
struct SmartScript {
    EventObject* actor=nullptr;
    static inline uint32 entries=0,first=0,scene=0,exits=0;
    void OnInitialize(EventObject* event,void*){actor=event;}
    void ProcessEventsFor(uint32 type,Player* player,uint32 entry){
        assert(actor&&entry==108);
        if(type==SMART_EVENT_EVENTOBJECT_OFFTRIGGER){++exits;return;}
        assert(type==SMART_EVENT_EVENTOBJECT_ONTRIGGER);++entries;
        if(sConditionMgr->IsObjectMeetingSmartEventConditions(entry,0,13,player,actor))++first;
        if(sConditionMgr->IsObjectMeetingSmartEventConditions(entry,2,13,player,actor))++scene;
    }
};
struct EventObjectScript {
    explicit EventObjectScript(char const*){}
    virtual bool OnTrigger(Player*,EventObject*,bool){return false;}
};
#include "Dispatcher.inc"
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    assert(argc==2);std::ifstream input(argv[1]);std::vector<Condition> rows;Condition row;
    while(input>>row.group>>row.source>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2)rows.push_back(row);
    assert(rows.size()==2);
    for(auto& condition:rows)manager.SmartEventConditionStore[{108,condition.source}][condition.group].push_back(&condition);
    Player player;EventObject actor;SmartEventObject dispatcher;
    assert(!manager.IsObjectMeetingSmartEventConditions(108,2,13,&player,&actor));
    for(uint32 stage=0;stage<13;++stage){
        scenarios.scenario.step=stage;
        uint32 beforeFirst=SmartScript::first,beforeScene=SmartScript::scene;
        assert(dispatcher.OnTrigger(&player,&actor,true));
        assert(SmartScript::first-beforeFirst==(stage==10?1u:0u));
        assert(SmartScript::scene-beforeScene==(stage==11?1u:0u));
    }
    for(uint32 failure=0;failure<4;++failure){
        scenarios={};scenarios.scenario.step=11;player.map={};
        if(failure==0)scenarios.present=false;
        if(failure==1)scenarios.scenario.identity=1099;
        if(failure==2)player.map.instanced=false;
        if(failure==3)player.map.instance.instance=43;
        assert(!manager.IsObjectMeetingSmartEventConditions(108,0,13,&player,&actor));
        assert(!manager.IsObjectMeetingSmartEventConditions(108,2,13,&player,&actor));
    }
    player.map={};scenarios={};scenarios.scenario.step=11;
    uint32 before=SmartScript::scene;player.alive=false;
    assert(!dispatcher.OnTrigger(&player,&actor,true)&&SmartScript::scene==before);
    player.alive=true;dispatcher.OnTrigger(&player,&actor,false);assert(SmartScript::exits==1&&SmartScript::scene==before);
    Player party;assert(dispatcher.OnTrigger(&party,&actor,true)&&SmartScript::scene==before+1);
    assert(manager.IsObjectMeetingSmartEventConditions(108,2,10,&player,&actor));
    std::cout<<"PASS: actual SmartEventObject dispatcher/ConditionMgr source-event lookup and native scenario predicate admit only973/10 versus973/11, reject missing/wrong context/dead player and preserve separate party/exit invocation. Action execution, geometry/cache and client playback are not simulated.\n";
}
