#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using uint32=uint32_t;
enum {SMART_EVENT_GO_STATE_CHANGED=70,SMART_EVENT_GOSSIP_HELLO=64,SMART_EVENT_LINK=61,SMART_EVENT_FLAG_NOT_REPEATABLE=1};
uint32 errors=0;
#define TC_LOG_ERROR(...) (++errors)
struct Unit {bool owned=true;};
struct SmartScriptHolder {
    uint32 entryOrGuid=247690,event_id=0,link=0,eventType=0,actionType=0,first=0,second=0;
    bool runOnce=false;
    struct {uint32 event_flags=0,event_phase_mask=0;struct {uint32 state=0;}goStateChanged;}event;
    uint32 GetEventType()const{return eventType;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return 1;}
};
struct SmartScript {
    std::vector<SmartScriptHolder> mEvents;
    std::vector<uint32> actions;
    std::vector<Unit*> invokers;
    bool IsInPhase(uint32){return true;}
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessAction(SmartScriptHolder& nativeEvent,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,void const* spell=nullptr,void* gob=nullptr){
        nativeEvent.runOnce=true;
        if(unit){actions.push_back(nativeEvent.actionType==205?nativeEvent.second:nativeEvent.first);invokers.push_back(unit);}
#include "Link.inc"
    }
    void ProcessEvent(SmartScriptHolder& nativeEvent,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,void const* spell=nullptr,void* gob=nullptr){
#include "Repeat.inc"
        switch(nativeEvent.GetEventType()){
#include "State.inc"
        case SMART_EVENT_LINK:case SMART_EVENT_GOSSIP_HELLO:ProcessAction(nativeEvent,unit,var0,var1,bvar,spell,gob);break;
        }
    }
    void Dispatch(uint32 type,Unit* unit,uint32 state=0){for(auto& event:mEvents)if(event.GetEventType()==type){if(type==64&&(!unit||!unit->owned))continue;ProcessEvent(event,unit,state);}}
};
#include "Find.inc"
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);
    std::ifstream input(argv[1]);std::vector<SmartScriptHolder> definitions;
    for(uint32 count=0;count<5;++count){SmartScriptHolder row;assert(input>>row.event_id>>row.link>>row.eventType>>row.event.event_flags>>row.event.goStateChanged.state>>row.actionType>>row.first>>row.second);definitions.push_back(row);}
    {
        Unit invoker;SmartScript script;script.mEvents=definitions;errors=0;
        script.Dispatch(64,&invoker);
        assert(errors==0&&script.actions==std::vector<uint32>({201780,48493,204919,201917}));
        for(auto context:script.invokers)assert(context==&invoker);
        script.Dispatch(64,&invoker);assert(script.actions.size()==4);
        SmartScript state;state.mEvents=definitions;state.Dispatch(70,&invoker,2);assert(state.actions.empty());
        state.Dispatch(70,&invoker,3);assert(errors==0&&state.actions==script.actions);
        SmartScript denied;denied.mEvents=definitions;invoker.owned=false;denied.Dispatch(64,&invoker);assert(denied.actions.empty());
        SmartScript missing;missing.mEvents=definitions;missing.Dispatch(70,nullptr,3);assert(missing.actions.empty()&&!errors);
    }
    std::cout<<"PASS: actual LINK resolver/tail and GO state/repeat guards preserve scene ->48493 ->204919 ->201917 in one chain; repeat gossip, wrong state and missing context retained. Action/ownership sinks controlled, not loot/equip/scene success or client traversal.\n";
}
