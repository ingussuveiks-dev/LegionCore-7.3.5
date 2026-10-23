#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
using uint32=uint32_t;
enum { SMART_EVENT_LINK=61, SMART_EVENT_FLAG_NOT_REPEATABLE=1 };
uint32 errors=0;
#define TC_LOG_ERROR(...) (++errors)
struct SmartScriptHolder {
    uint32 event_id=0,link=0,eventType=0,actionType=0,first=0,second=0,target=0,entryOrGuid=100397;
    struct {uint32 event_flags=0,event_phase_mask=0;} event;
    bool runOnce=false;
    uint32 GetActionType() const{return actionType;}
    uint32 GetEventType() const{return eventType;}
    uint32 GetScriptType() const{return 0;}
};
struct SmartScript {
    std::vector<SmartScriptHolder> mEvents,executed;
    SmartScriptHolder FindLinkedEvent(uint32);
    bool IsInPhase(uint32) const{return true;}
    void ProcessEvent(SmartScriptHolder& event,void*,uint32,uint32,bool,void const*,void*){
        auto& e=event;
#include "RepeatGuard.inc"
        e.runOnce=true;executed.push_back(e);
    }
    void Follow(SmartScriptHolder& event){
        auto& e=event;void* unit=nullptr;uint32 var0=0,var1=0;bool bvar=false;void const* spell=nullptr;void* gob=nullptr;
#include "FollowLink.inc"
    }
    void Attempt(SmartScriptHolder& e){
#include "RepeatGuard.inc"
        e.runOnce=true;Follow(e);
    }
};
#include "FindLink.inc"
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);
    std::ifstream input(argv[1]);SmartScript script;
    for(uint32 count=0;count<4;++count){SmartScriptHolder row;assert(input>>row.event_id>>row.link>>row.eventType>>row.actionType>>row.first>>row.second>>row.target>>row.event.event_flags>>row.event.event_phase_mask);script.mEvents.push_back(row);}
    auto flag=script.FindLinkedEvent(55);assert(flag.eventType==61&&flag.actionType==81&&flag.first==1&&flag.target==1);
    auto speech=script.FindLinkedEvent(54);script.Follow(speech);
    assert(script.executed.size()==1&&script.executed.back().event_id==55);
    auto choice=script.FindLinkedEvent(59);assert(choice.eventType==62&&choice.actionType==33&&choice.first==103481&&choice.target==7);
    script.Follow(choice);assert(script.executed.size()==2);
    auto travel=script.executed.back();assert(travel.event_id==56&&travel.eventType==61&&travel.actionType==85&&travel.first==205040&&travel.target==7);
    assert(flag.event.event_flags==1&&travel.event.event_flags==1&&choice.event.event_flags==0);
    script.executed.clear();auto original=choice;original.event.event_flags|=1;
    script.Attempt(original);script.Attempt(original);assert(script.executed.size()==1);
    script.executed.clear();script.Attempt(choice);script.Attempt(choice);assert(script.executed.size()==2);
    assert(errors==0);
    std::cout<<"PASS: actual SmartAI link lookup/follow preserves text54 -> LINK55 gossip flag and isolated gossip59 -> LINK56 native205040 without a shared condition-slot collision\n";
    std::cout<<"PASS: actual native runOnce guard reproduces the old one-shot retry loss; only the return choice retries native travel, leaving flag/travel LINK event flags unchanged\n";
}
