#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;
enum {SMART_EVENT_LINK=61,SMART_EVENT_UPDATE_IC=0,SMART_EVENT_UPDATE_OOC=1,SMART_EVENT_UPDATE=60,
    SMART_EVENT_HEALTH_PCT=2,SMART_EVENT_TARGET_HEALTH_PCT=3,SMART_EVENT_MANA_PCT=4,SMART_EVENT_TARGET_MANA_PCT=5,
    SMART_EVENT_RANGE=9,SMART_EVENT_VICTIM_CASTING=13,SMART_EVENT_FRIENDLY_HEALTH=14,SMART_EVENT_FRIENDLY_IS_CC=15,
    SMART_EVENT_FRIENDLY_MISSING_BUFF=16,SMART_EVENT_HAS_AURA=23,SMART_EVENT_TARGET_BUFFED=24,SMART_EVENT_IS_BEHIND_TARGET=67,
    SMART_EVENT_CHECK_DIST_TO_HOME=88,SMART_EVENT_DISTANCE_CREATURE=75,SMART_EVENT_FRIENDLY_HEALTH_PCT=74,
    SMART_ACTION_CAST=11,SMART_ACTION_CAST_CUSTOM=210,SMARTCAST_INTERRUPT_PREVIOUS=1,SMARTCAST_TRIGGERED=2,
    UNIT_STATE_CASTING=0x8000,SMART_SCRIPT_TYPE_TIMED_ACTIONLIST=9};
struct SmartScriptHolder {
    uint32 entryOrGuid=106730,event_id=0,timer=0,eventType=60,actionType=0,spell=0;
    bool enableTimed=false,active=true;
    struct {uint32 event_phase_mask=0;} event;
    struct {struct {uint32 flags=0;} cast;} action;
    uint32 GetEventType() const{return eventType;}
    uint32 GetActionType() const{return actionType;}
    uint32 GetScriptType() const{return 9;}
};
struct Creature {
    bool casting=false;
    bool isInCombat(){return false;}
    bool HasUnitState(uint32){return casting;}
};
struct Observation {uint32 spell,time;};
struct SmartScript {
    Creature actor;
    Creature* me=&actor;
    std::vector<SmartScriptHolder> mTimedActionList;
    std::vector<Observation> observed;
    uint32 time=0;
    bool IsInPhase(uint32){return true;}
    void ProcessEvent(SmartScriptHolder& event){observed.push_back({event.spell,time});}
    void UpdateTimer(SmartScriptHolder&,uint32 const);
    void Tick(uint32 diff){time+=diff;for(auto& event:mTimedActionList)if(event.enableTimed)UpdateTimer(event,diff);}
    uint32 When(uint32 spell){for(auto const& event:observed)if(event.spell==spell)return event.time;std::abort();}
};
#include "Timer.inc"
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    assert(argc==2);
    std::ifstream input(argv[1]);
    std::vector<SmartScriptHolder> definitions;
    std::string text;
    while(std::getline(input,text)){
        SmartScriptHolder event;
        uint32 maximum=0;
        std::istringstream row(text);
        assert(row>>event.event_id>>event.timer>>maximum>>event.actionType>>event.spell);
        assert(event.timer==maximum);
        event.enableTimed=definitions.empty();
        definitions.push_back(event);
    }
    assert(definitions.size()==7&&definitions[2].event_id==2);
#ifdef BASELINE
    definitions[2].timer=5000;
#endif
    for(uint32 tick:{1u,16u,50u,100u,200u,500u,1000u}){
        SmartScript script;
        script.mTimedActionList=definitions;
        while(script.observed.size()!=7&&script.time<60000)script.Tick(tick);
        assert(script.observed.size()==7);
        uint32 started=script.When(211753);
        uint32 camera=script.When(208447)-started;
        uint32 screen=script.When(211934)-started;
        assert(camera+tick>=15700);
        assert(screen>=19076);
        if(tick<=500)assert(screen>=20076);
        assert(screen<22000);
        assert(script.When(211848)==script.When(208447));
        assert(script.When(50921)==started);
        assert(script.When(1)==script.When(211934));
        assert(script.When(0)==script.When(211934));
        size_t count=script.observed.size();
        script.Tick(30000);
        assert(script.observed.size()==count);
    }
    SmartScript paused;
    paused.mTimedActionList=definitions;
    paused.actor.casting=true;
    paused.Tick(1000);
    assert(paused.observed.empty());
    paused.actor.casting=false;
    while(paused.observed.size()!=7&&paused.time<60000)paused.Tick(50);
    assert(paused.observed.size()==7&&paused.When(211934)-paused.When(211753)>=19076);
    std::cout<<"PASS: extracted native timed-list dispatcher preserves defeat/overload/screen/Data/death order and single execution;1..1000ms ticks place screen after actual19076ms warning voice end; controlled action sink, not client playback.\n";
}
