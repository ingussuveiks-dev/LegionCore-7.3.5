// Native DATA_SET predicate, action bodies, dispatcher and LINK validation.
// Producer timing is read from its unchanged SQL rows, not client playback.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
using uint32=uint32_t;using SMART_EVENT=uint32;
#include "Constants.inc"
struct Unit {
    bool expired=false;float angle=4.7516f;unsigned turns=0,emotes=0;
    Unit* ToUnit(){return this;}bool IsQuestObjectExpired()const{return expired;}
    struct Home {float GetOrientation()const{return 4.7516f;}} home;
    Home const& GetHomePosition()const{return home;}
    void SetFacingTo(float value){angle=value;++turns;}void SetFacingToObject(Unit*){++turns;}
    void HandleEmoteCommand(uint32 id){if(id==17)++emotes;}
};
bool IsUnit(Unit* unit){return unit!=nullptr;}
using ObjectList=std::vector<Unit*>;struct SpellInfo{};struct GameObject{};
struct SmartScriptHolder {
    uint32 entryOrGuid=0,source_type=0,event_id=0,link=0,type=0,actionType=0,parameter=0,second=0,targetType=0,delay=0;
    struct {struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;} dataSet;} event;
    struct {struct {uint32 emote=0;} emote;} action;
    struct {float o=0;} target;
    uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}
};
struct Conditions {bool allow=true;bool IsObjectMeetingSmartEventConditions(uint32,uint32,uint32,Unit*,Unit*)const{return allow;}} conditions;
auto sConditionMgr=&conditions;
struct SmartScript {
    std::vector<SmartScriptHolder> mEvents;Unit actor;Unit* me=&actor;unsigned errors=0;
    Unit* GetBaseObject(){return me;}
    ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){return e.targetType==SMART_TARGET_SELF?new ObjectList{me}:nullptr;}
    void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessEvent(SmartScriptHolder&,Unit*,uint32,uint32,bool,SpellInfo const*,GameObject*);
    void ProcessAction(SmartScriptHolder&,Unit*,uint32,uint32,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_DEBUG(...) ((void)0)
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
void SmartScript::ProcessEvent(SmartScriptHolder& e,Unit* unit,uint32 var0,uint32 var1,bool bvar,SpellInfo const* spell,GameObject* gob)
{
    switch(e.GetEventType()) {
#include "LinkEvent.inc"
#include "DataSetEvent.inc"
    }
}
void SmartScript::ProcessAction(SmartScriptHolder& e,Unit* unit,uint32 var0,uint32 var1,bool bvar,SpellInfo const* spell,GameObject* gob)
{
    switch(e.GetActionType()) {
#include "FacingAction.inc"
#include "EmoteAction.inc"
    }
#include "LinkTail.inc"
static std::vector<SmartScriptHolder> Read(char const* path,uint32 entry)
{
    std::ifstream file(path);std::string line;std::vector<SmartScriptHolder> rows;
    while(std::getline(file,line)) {
        std::stringstream stream(line);std::string field;std::vector<std::string> f;
        while(std::getline(stream,field,'\t'))f.push_back(field);
        if(!line.empty()&&line.back()=='\t')f.push_back("");
        if(f.size()!=31)return {};
        auto n=[&](size_t i){return uint32(std::stoul(f[i]));};if(n(0)!=entry)continue;
        SmartScriptHolder e;e.entryOrGuid=n(0);e.source_type=n(1);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.dataSet={n(9),n(10),n(11),n(12)};
        e.delay=n(9);e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.emote={n(15)};e.targetType=n(21);e.target.o=std::stof(f[29]);rows.push_back(e);
    }
    return rows;
}
bool Near(float a,float b){return std::abs(a-b)<0.00001f;}
int main(int argc,char** argv)
{
    if(argc!=3)return 1;
    auto old=Read(argv[1],90418),fixed=Read(argv[2],90418),producer=Read(argv[1],9041700);
    if(old.size()!=15||fixed.size()!=15||producer.size()!=8)return 2;
    if(producer[5].delay!=4000||producer[5].parameter!=1||producer[5].second!=1||producer[6].delay||producer[6].parameter!=2||producer[6].second!=2||producer[7].delay!=2000||producer[7].parameter!=3||producer[7].second!=3)return 3;
    Unit player,pet;
    for(Unit* invoker:{&player,&pet,static_cast<Unit*>(nullptr)})for(bool repaired:{false,true}) {
        SmartScript s;s.mEvents=repaired?fixed:old;
        s.ProcessEventsFor(SMART_EVENT_DATA_SET,invoker,1,1);
        if(!Near(s.actor.angle,6.09605f)||s.actor.turns!=1||s.errors!=unsigned(!repaired))return 4;
        s.ProcessEventsFor(SMART_EVENT_DATA_SET,invoker,2,2);
        if(s.actor.emotes!=1||s.actor.turns!=1||!Near(s.actor.angle,6.09605f))return 5;
        // There is no consumer-owned timer: no3/3 callback, no second turn.
        s.ProcessEventsFor(0,invoker);if(s.actor.turns!=1)return 6;
        s.ProcessEventsFor(SMART_EVENT_DATA_SET,invoker,3,3);
        if(!Near(s.actor.angle,4.61951f)||s.actor.turns!=2||s.errors!=unsigned(!repaired))return 7;
        s.ProcessEventsFor(SMART_EVENT_DATA_SET,invoker,1,1);
        if(!Near(s.actor.angle,6.09605f)||s.actor.turns!=3||s.errors!=2u*unsigned(!repaired))return 8;
    }
    SmartScript naive;naive.mEvents=old;naive.mEvents[2].type=SMART_EVENT_LINK;
    naive.ProcessEventsFor(SMART_EVENT_DATA_SET,nullptr,1,1);
    if(!Near(naive.actor.angle,4.61951f)||naive.actor.turns!=2)return 9;
    for(auto pair:{std::pair<uint32,uint32>{99,99},{1,2},{3,1}}){SmartScript s;s.mEvents=fixed;s.ProcessEventsFor(SMART_EVENT_DATA_SET,nullptr,pair.first,pair.second);if(s.actor.turns||s.actor.emotes||s.errors)return 10;}
    for(unsigned mode=0;mode<3;++mode){SmartScript s;s.mEvents=fixed;conditions.allow=mode!=0;s.actor.expired=mode==1;if(mode==2)s.me=nullptr;s.ProcessEventsFor(SMART_EVENT_DATA_SET,nullptr,1,1);if(s.actor.turns||s.errors)return 11;}
    std::puts("PASS extracted-native first facing/emote/independent final facing; original invalid-LINK counterfactual, repaired repeats/null-player-pet/refusals/missing actor, unchanged SQL2000ms producer gap; naive LINK conversion turns back immediately. Native source9/client playback not certified.");
}
