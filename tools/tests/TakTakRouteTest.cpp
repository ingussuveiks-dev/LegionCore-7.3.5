// Actual native gossip dispatch/LINK lookup/tail and selected event cases.
// Credit/cast/teleport endpoints record dispatch, not client or quest awards.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using uint32 = uint32_t;
enum SMART_EVENT { SMART_EVENT_LINK=61, SMART_EVENT_GOSSIP_SELECT=62 };
struct Unit {};
struct SpellInfo {};
struct GameObject {};
struct SmartScriptHolder
{
    uint32 entryOrGuid=0, source_type=0, event_id=0, link=0, type=0, actionType=0, parameter=0, targetType=0;
    struct { struct {uint32 sender=0,action=0,cooldownMin=0,cooldownMax=0;} gossip; } event;
    struct {float x=0,y=0,z=0,o=0;} target;
    uint32 GetActionType() const {return actionType;}
    uint32 GetEventType() const {return type;}
};
struct Base { bool expired=false; bool IsQuestObjectExpired() const{return expired;} };
struct Conditions {bool allow=true; bool IsObjectMeetingSmartEventConditions(uint32,uint32,uint32,Unit*,Base*) const{return allow;}} conditions;
Conditions* sConditionMgr=&conditions;
struct SmartScript
{
    std::vector<SmartScriptHolder> mEvents;
    Base base;
    unsigned errors=0;
    std::vector<std::pair<uint32,uint32>> calls;
    Base* GetBaseObject(){return &base;}
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessEventsFor(SMART_EVENT,Unit*,uint32,uint32,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessEvent(SmartScriptHolder&,Unit*,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessAction(SmartScriptHolder&,Unit*,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void RecalcTimer(SmartScriptHolder&,uint32,uint32) {}
};
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_DEBUG(...) ((void)0)
#include "FindLinkedEvent.inc"
#include "GossipDispatch.inc"
void SmartScript::ProcessEvent(SmartScriptHolder& e,Unit* unit,uint32 var0,uint32 var1,bool bvar,SpellInfo const* spell,GameObject* gob)
{
    switch(e.GetEventType()) {
#include "LinkEvent.inc"
#include "GossipEvent.inc"
    }
}
void SmartScript::ProcessAction(SmartScriptHolder& e,Unit* unit,uint32 var0,uint32 var1,bool bvar,SpellInfo const* spell,GameObject* gob)
{
    if(unit) calls.emplace_back(e.actionType,e.parameter);
#include "LinkTail.inc"

static std::vector<SmartScriptHolder> Read(char const* path)
{
    std::ifstream file(path);std::string line;std::vector<SmartScriptHolder> result;
    while(std::getline(file,line)) {
        std::stringstream stream(line);std::string field;std::vector<std::string> f;
        while(std::getline(stream,field,'\t')) f.push_back(field);
        if(f.size()!=31) return {};
        auto number=[&](size_t i){return uint32(std::stoul(f[i]));};
        SmartScriptHolder e;e.entryOrGuid=number(0);e.source_type=number(1);e.event_id=number(2);e.link=number(3);e.type=number(5);
        e.event.gossip={number(9),number(10),number(11),number(12)};e.actionType=number(14);e.parameter=number(15);e.targetType=number(21);
        e.target={std::stof(f[26]),std::stof(f[27]),std::stof(f[28]),std::stof(f[29])};
        result.push_back(e);
    }
    return result;
}
int main(int argc,char** argv)
{
    if(argc!=3)return 1;
    auto original=Read(argv[1]), repaired=Read(argv[2]);
    if(original.size()!=12||repaired.size()!=12)return 2;
    Unit player;
    for(auto const* rows:{&original,&repaired}) {
        SmartScript s;s.mEvents=*rows;s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&player,19254,3);
        std::vector<std::pair<uint32,uint32>> expected{{33,104976},{33,105485},{62,1514}};
        if(s.calls!=expected||s.errors!=unsigned(rows==&original))return 3;
        s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&player,19254,3);
        expected.insert(expected.end(),{{33,104976},{33,105485},{62,1514}});
        if(s.calls!=expected)return 4; // Per-selection native dispatch, not duplicate quest credit.
    }
    for(uint32 option:{0,1,2,4,10,11,5,99}) {
        SmartScript before,after;before.mEvents=original;after.mEvents=repaired;
        before.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&player,19254,option);
        after.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&player,19254,option);
        if(before.calls!=after.calls||before.errors||after.errors)return 5;
    }
    for(unsigned mode=0;mode<4;++mode){
        SmartScript s;s.mEvents=repaired;conditions.allow=mode!=0;s.base.expired=mode==1;
        s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,mode==2?nullptr:&player,mode==3?999:19254,3);
        if(!s.calls.empty()||s.errors)return 6;
    }
    conditions.allow=true;
    std::puts("PASS native original counterfactual logs invalid LINK; repaired gossip runs two distinct credits then transfer once, skips LINK as independent gossip, preserves eight other options/repeat/refusal dispatches");
    return 0;
}
