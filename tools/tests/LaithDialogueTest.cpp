// Native SmartScript dispatch, TALK timer, TEXT_OVER case and LINK tail.
// Chat, target search, attack and cast services are controlled endpoints.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;
using uint8=uint8_t;
#include "EventConstants.inc"
struct ObjectGuid { uint32 value=0; static ObjectGuid const Empty; };
ObjectGuid const ObjectGuid::Empty{};
struct Creature;struct Player;
struct Unit {
    bool player=false,creature=false;
    Creature* ToCreature();Player* ToPlayer();
    ObjectGuid GetGUID() const { return {}; }
};
struct Player:Unit { Player(){player=true;} };
struct Creature:Unit {
    Creature(){creature=true;}
    uint32 GetEntry() const {return 102459;}
};
Creature* Unit::ToCreature(){return static_cast<Creature*>(this);}
Player* Unit::ToPlayer(){return static_cast<Player*>(this);}
bool IsCreature(Unit* u){return u&&u->creature;}
bool IsPlayer(Unit* u){return u&&u->player;}
using ObjectList=std::vector<Unit*>;
struct SpellInfo{};struct GameObject{};
struct SmartScriptHolder {
    uint32 entryOrGuid=102459,source_type=0,event_id=0,link=0,type=0,actionType=0,parameter=0,targetType=0;
    struct { struct {uint32 textGroupID=0,creatureEntry=0;} textOver; } event;
    struct { struct {uint32 textGroupID=0,duration=0;} talk; } action;
    uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}
};
struct Base {bool expired=false;bool IsQuestObjectExpired()const{return expired;}};
struct Conditions {bool allow=true;bool IsObjectMeetingSmartEventConditions(uint32,uint32,uint32,Unit*,Base*)const{return allow;}} conditions;
Conditions* sConditionMgr=&conditions;
struct Chat {void SendChat(Creature*,uint8_t,ObjectGuid){}} chat;
Chat* sCreatureTextMgr=&chat;
struct Call {uint32 time,action,parameter;bool operator==(Call const& b)const{return time==b.time&&action==b.action&&parameter==b.parameter;}};
struct SmartScript {
    std::vector<SmartScriptHolder> mEvents;std::vector<Call> calls;
    Base base;Creature actor;Creature* me=&actor;Unit* invoker=nullptr;
    unsigned errors=0;uint32 now=0,mTalkerEntry=0,mLastTextID=0,mTextTimer=0;ObjectGuid mTextGUID;bool mUseTextTimer=false;
    bool attackTarget=true,castAccepted=true;unsigned castAttempts=0;
    Base* GetBaseObject(){return &base;}Unit* GetLastInvoker(){return invoker;}
    ObjectList* GetTargets(SmartScriptHolder const&,Unit*){return new ObjectList{me};}
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessEventsFor(SMART_EVENT,Unit*,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessEvent(SmartScriptHolder&,Unit*,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessAction(SmartScriptHolder&,Unit*,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void Update(uint32 diff);
};
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_DEBUG(...) ((void)0)
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
void SmartScript::ProcessEvent(SmartScriptHolder& e,Unit* unit,uint32 var0,uint32 var1,bool bvar,SpellInfo const* spell,GameObject* gob)
{
    switch(e.GetEventType()) {
#include "LinkEvent.inc"
#include "NoParamEvents.inc"
#include "TextOverEvent.inc"
    }
}
void SmartScript::ProcessAction(SmartScriptHolder& e,Unit* unit,uint32 var0,uint32 var1,bool bvar,SpellInfo const* spell,GameObject* gob)
{
    if(unit)invoker=unit;
    if(e.actionType!=49||attackTarget)calls.push_back({now,e.actionType,e.parameter});
    switch(e.GetActionType()) {
#include "TalkAction.inc"
        case 11: ++castAttempts; if(!castAccepted)return; break;
    }
#include "LinkTail.inc"
void SmartScript::Update(uint32 diff)
{
    now+=diff;
#include "TextTimer.inc"
static std::vector<SmartScriptHolder> Read(char const* path)
{
    std::ifstream file(path);std::string line;std::vector<SmartScriptHolder> result;
    while(std::getline(file,line)) {
        std::stringstream s(line);std::string field;std::vector<std::string> f;
        while(std::getline(s,field,'\t'))f.push_back(field);
        if(f.size()!=31)return {};
        auto n=[&](size_t i){return uint32(std::stoul(f[i]));};
        SmartScriptHolder e;e.event_id=n(2);e.link=n(3);e.type=n(5);e.actionType=n(14);e.parameter=n(15);e.targetType=n(21);
        e.event.textOver={n(9),n(10)};e.action.talk={n(15),n(16)};result.push_back(e);
    }
    return result;
}
static bool Timeline(SmartScript& s,Unit* invoker)
{
    s.ProcessEventsFor(SMART_EVENT_JUST_SUMMONED,invoker);
    if(s.calls!=std::vector<Call>{{0,1,0}}||s.mTextTimer!=3000)return false;
    s.Update(2999);if(s.calls.size()!=1)return false;
    s.Update(1);if(s.calls.size()!=1)return false; // Native timer uses <, not <=.
    s.Update(1);
    if(s.calls!=std::vector<Call>{{0,1,0},{3001,1,1},{3001,49,0}}||s.mTextTimer!=3000)return false;
    s.Update(3001);s.Update(60000);
    return s.calls.size()==3&&!s.mUseTextTimer;
}
int main(int argc,char** argv)
{
    if(argc!=3)return 1;
    auto original=Read(argv[1]),repaired=Read(argv[2]);if(original.size()!=6||repaired.size()!=6)return 2;
    Player player;Unit pet,guardian;
    for(Unit* invoker:{static_cast<Unit*>(&player),&pet,&guardian,static_cast<Unit*>(nullptr)}) {
        SmartScript before,after;before.mEvents=original;after.mEvents=repaired;
        if(!Timeline(before,invoker)||!Timeline(after,invoker)||before.errors!=1||after.errors)return 3;
        if(before.calls!=after.calls)return 4;
        // The unchanged death event must use the creature's own native cast,
        // regardless of the killer. No modeled quest/scenario award is emitted.
        after.ProcessEventsFor(SMART_EVENT_DEATH,invoker);
        if(after.castAttempts!=1||after.calls.back().action!=11||after.calls.back().parameter!=202333||after.errors)return 5;
    }
    // Converting TEXT_OVER to LINK would erase the first-line delay.
    SmartScript naive;naive.mEvents=original;naive.mEvents[1].type=SMART_EVENT_LINK;
    naive.ProcessEventsFor(SMART_EVENT_JUST_SUMMONED,nullptr);
    if(naive.calls.size()!=3||naive.calls[1].time!=0||naive.calls[2].time!=0)return 6;
    for(uint32 group:{1u,99u}) {SmartScript s;s.mEvents=repaired;s.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,group,102459);if(!s.calls.empty()||s.errors)return 7;}
    SmartScript foreign;foreign.mEvents=repaired;foreign.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,0,999);if(!foreign.calls.empty())return 8;
    for(unsigned mode=0;mode<2;++mode) {
        SmartScript s;s.mEvents=repaired;conditions.allow=mode!=0;s.base.expired=mode==1;
        s.ProcessEventsFor(SMART_EVENT_JUST_SUMMONED,nullptr);if(!s.calls.empty()||s.errors)return 9;
    }
    conditions.allow=true;
    SmartScript absent;absent.mEvents=repaired;absent.attackTarget=false;
    absent.ProcessEventsFor(SMART_EVENT_JUST_SUMMONED,nullptr);absent.Update(3001);
    if(absent.calls!=std::vector<Call>{{0,1,0},{3001,1,1}}||absent.errors)return 10;
    SmartScript removed;removed.mEvents=repaired;removed.ProcessEventsFor(SMART_EVENT_JUST_SUMMONED,nullptr);removed.me=nullptr;removed.Update(10000);
    if(removed.calls.size()!=1)return 11;
    SmartScript retry;retry.mEvents=repaired;if(!Timeline(retry,nullptr)||retry.errors)return 12;
    SmartScript failed;failed.mEvents=repaired;failed.castAccepted=false;failed.ProcessEventsFor(SMART_EVENT_DEATH,&pet);
    if(failed.castAttempts!=1||failed.calls.size()!=1||failed.errors)return 13;
    std::puts("PASS native baseline invalid-LINK counterfactual; repaired 3000ms TEXT_OVER then second line/attack, player/pet/guardian/null, condition/expired/foreign/missing-target/removed-actor/new-summon/failing-death-cast isolation; naive LINK conversion breaks timing");
}
