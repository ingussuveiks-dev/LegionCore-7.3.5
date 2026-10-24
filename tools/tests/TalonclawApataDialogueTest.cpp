#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>
using uint32=uint32_t;
using uint8=uint8_t;
using ReactStates=uint32;
enum {SMART_ACTION_TALK=1,SMART_ACTION_CAST=11,SMART_ACTION_DIE=37,SMART_ACTION_WP_START=53,SMART_EVENT_TEXT_OVER=52,SMART_EVENT_DATA_SET=38,SMART_EVENT_LINK=61,SMART_EVENT_FLAG_NOT_REPEATABLE=1,SMART_ESCORT_TARGETS=1,SMART_TARGET_POSITION=8,SMARTCAST_TRIGGERED=2,SMARTCAST_INTERRUPT_PREVIOUS=1,SMARTCAST_AURA_NOT_PRESENT=32,CAST_FORCE_TARGET_SELF=16,SMARTCAST_COMBAT_MOVE=64};
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS};
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) (++errors)
#define CAST_AI(type,actor) (actor)
#define ENSURE_AI(type,actor) (actor)
uint32 errors=0;
struct ObjectGuid {inline static constexpr uint32 Empty=0;uint32 identity=0;ObjectGuid(uint32 value=0):identity(value){} uint32 GetCounter() const{return identity;}};
struct Player;
struct Creature;
struct Unit {
    virtual ~Unit()=default;
    virtual Creature* ToCreature(){return nullptr;}
    virtual Player* ToPlayer(){return nullptr;}
    Unit* ToUnit(){return this;}
    bool IsUnit(){return true;}
    bool HasAura(uint32){return false;}
    ObjectGuid GetGUID(){return 7;}
    virtual SpellCastResult CastSpell(Unit*,uint32,bool){return SPELL_CAST_OK;}
};
struct Player:Unit {Player* ToPlayer() override{return this;}};
struct AI {
    bool pathAvailable=true,running=false;
    uint32 mEscortQuestID=0,pathRequests=0;
    bool StartPath(bool run,uint32 path,bool repeat,Unit*){assert(run&&path==10023501&&!repeat);++pathRequests;running=pathAvailable;return running;}
    void SetDespawnTime(uint32 time){assert(!time);}
    void SetCombatMove(bool){}
};
struct Creature:Unit {
    struct AI controller;
    std::vector<uint32> casts;
    std::vector<uint32> dialogue;
    uint32 deaths=0;
    bool isDead(){return deaths!=0;}
    void Kill(Creature* target){assert(target==this&&!dialogue.empty());++deaths;}
    Creature* ToCreature() override{return this;}
    struct AI* AI(){return &controller;}
    uint32 GetEntry(){return 100235;}
    void SetReactState(ReactStates){}
    void InterruptNonMeleeSpells(bool){}
    void GetPosition(float& positionX,float& positionY,float& positionZ){positionX=positionY=positionZ=0;}
    SpellCastResult CastSpell(Unit*,uint32 spell,bool) override{casts.push_back(spell);return SPELL_CAST_OK;}
    SpellCastResult CastSpell(float,float,float,uint32 spell,bool){casts.push_back(spell);return SPELL_CAST_OK;}
};
struct GameObject {SpellCastResult CastSpell(Unit*,uint32){return SPELL_CAST_OK;}};
struct Chat {uint32 group,time;bool running;};
struct TextManager {
    uint32 time=0;
    std::vector<Chat> chats;
    void SendChat(Creature* actor,uint8 group,ObjectGuid){actor->dialogue.push_back(group);chats.push_back({group,time,actor->controller.running});}
} textManager;
auto sCreatureTextMgr=&textManager;
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder {
    uint32 event_id=0,link=0,eventType=0,actionType=0,entryOrGuid=100235;
    bool runOnce=false;
    struct {uint32 event_flags=1,event_phase_mask=0;struct{uint32 textGroupID=0,creatureEntry=0;}textOver;struct{uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;}dataSet;} event;
    struct {
        struct {uint32 textGroupID=0,duration=0;}talk;
        struct {uint32 spell=0,flags=0;}cast;
        struct {uint32 run=0,pathID=0,repeat=0,quest=0,despawnTime=0,reactState=0;}wpStart;
    } action;
    struct {float x=0,y=0,z=0;} target;
    uint32 GetEventType() const{return eventType;}
    uint32 GetActionType() const{return actionType;}
    uint32 GetScriptType() const{return 0;}
    uint32 GetTargetType() const{return 1;}
};
struct SmartScript {
    Creature* me=nullptr;
    GameObject* go=nullptr;
    Unit* invoker=nullptr;
    std::vector<SmartScriptHolder> mEvents;
    uint32 mTalkerEntry=0,mLastTextID=0,mTextTimer=0;
    ObjectGuid mTextGUID;
    bool mUseTextTimer=false;
    bool IsSmart(){return me!=nullptr;}
    bool IsInPhase(uint32){return true;}
    bool IsCreature(Unit* object){return object&&object->ToCreature();}
    bool IsPlayer(Unit* object){return object&&object->ToPlayer();}
    bool IsUnit(Unit* object){return object!=nullptr;}
    Unit* GetLastInvoker(){return invoker;}
    ObjectList* GetTargets(SmartScriptHolder const&,Unit*){return me?new ObjectList{me}:nullptr;}
    void StoreTargetList(ObjectList* targets,uint32){delete targets;}
    void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessEvent(SmartScriptHolder& nativeEvent,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,void const* spell=nullptr,GameObject* gob=nullptr){
#include "RepeatGuard.inc"
        switch(nativeEvent.GetEventType()){
#include "TextOver.inc"
#include "DataSet.inc"
        case SMART_EVENT_LINK:ProcessAction(nativeEvent,unit,var0,var1,bvar,spell,gob);break;
        }
    }
    void ProcessEventsFor(uint32 event,Unit* unit=nullptr,uint32 group=0,uint32 entry=0){for(auto& nativeEvent:mEvents)if(nativeEvent.GetEventType()==event)ProcessEvent(nativeEvent,unit,group,entry);}
    void ProcessAction(SmartScriptHolder& nativeEvent,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,void const* spell=nullptr,GameObject* gob=nullptr){
        nativeEvent.runOnce=true;
        switch(nativeEvent.GetActionType()){
#include "Talk.inc"
#include "Die.inc"
#include "Cast.inc"
#include "Waypoint.inc"
        }
#include "FollowLink.inc"
    }
    void Tick(uint32 diff){textManager.time+=diff;
#include "TextTimer.inc"
    }
    void Start(){auto& first=mEvents.front();ProcessAction(first,invoker);}
};
#include "FindLink.inc"
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2||argc==3);
    bool deathWarning=argc==3&&std::string(argv[2])=="death";
    std::ifstream input(argv[1]);std::vector<SmartScriptHolder> definitions;
    for(uint32 count=0;count<(deathWarning?2u:5u);++count){SmartScriptHolder row;uint32 first=0,second=0;assert(input>>row.event_id>>row.link>>row.eventType>>row.event.event_flags>>row.event.textOver.textGroupID>>row.event.textOver.creatureEntry>>row.actionType>>first>>second);row.event.dataSet={row.event.textOver.textGroupID,row.event.textOver.creatureEntry,0,0};row.action.talk={first,second};row.action.cast={first,second};row.action.wpStart={first,second,0,0,0,0};definitions.push_back(row);}
    if(deathWarning){
        for(uint32 invokerType:{0u,1u,2u,3u}){
            Creature actor;Player player;Unit pet,guardian;SmartScript script;script.me=&actor;script.invoker=invokerType==0?&player:invokerType==1?&pet:invokerType==2?&guardian:nullptr;script.mEvents=definitions;textManager={};
            script.ProcessEventsFor(38,script.invoker,5,6);assert(textManager.chats.empty()&&!actor.deaths);
            script.ProcessEventsFor(38,script.invoker,6,6);assert(textManager.chats.size()==1&&textManager.chats[0].group==20&&actor.deaths==1);
            script.ProcessEventsFor(38,script.invoker,6,6);assert(textManager.chats.size()==1&&actor.deaths==1);
            script.Tick(16);assert(textManager.chats.size()==1&&actor.casts.empty());
        }
        SmartScript missing;missing.mEvents=definitions;textManager={};missing.ProcessEventsFor(38,nullptr,6,6);assert(textManager.chats.empty());
        assert(errors==0);std::cout<<"PASS: actual DATA_SET/repeat/TALK/DIE/LINK bodies emit world warning20 once before self death for player/pet/guardian/null invokers, never scene-owned21; wrong signal/missing actor safe. Controlled chat/death sinks, not scene rendering.\n";return 0;
    }
    assert(definitions.size()==5&&definitions.front().event_id==15);
    for(uint32 tick:{1u,16u,100u,500u,1000u})for(uint32 invokerType:{0u,1u,2u}){
        Creature actor;Player player;Unit pet;
        SmartScript script;script.me=&actor;script.invoker=invokerType==0?&player:invokerType==1?&pet:nullptr;script.mEvents=definitions;textManager={};
        script.Start();assert(textManager.chats.size()==1&&textManager.chats.front().group==11);
        while(textManager.time<30000)script.Tick(tick);
        assert(actor.casts.empty()&&!script.mUseTextTimer);
        assert(textManager.chats.size()==4);
        assert(textManager.chats[1].group==12&&textManager.chats[2].group==13&&textManager.chats[3].group==14);
        assert(textManager.chats[1].time>3000&&textManager.chats[2].time-textManager.chats[1].time>6000);
        assert(textManager.chats[3].time-textManager.chats[2].time>8000&&textManager.chats[2].running);
        size_t chats=textManager.chats.size();script.ProcessEventsFor(52,nullptr,12,100235);script.ProcessEventsFor(52,nullptr,13,100235);assert(textManager.chats.size()==chats&&actor.controller.pathRequests==1);
    }
    Creature actor;SmartScript script;script.me=&actor;script.mEvents=definitions;textManager={};
    actor.controller.pathAvailable=false;script.ProcessEventsFor(52,nullptr,12,100235);assert(textManager.chats.empty()&&!script.mEvents[2].runOnce);
    actor.controller.pathAvailable=true;script.ProcessEventsFor(52,nullptr,12,100235);assert(textManager.chats.size()==1&&textManager.chats[0].group==13);
    script.Tick(8000);assert(textManager.chats.size()==1);script.Tick(1);assert(textManager.chats.size()==2&&textManager.chats[1].group==14);
    script.ProcessEventsFor(52,nullptr,13,100236);assert(textManager.chats.size()==2);
    SmartScript missing;missing.mEvents=definitions;textManager={};missing.ProcessEventsFor(52,nullptr,12,100235);assert(textManager.chats.empty());
    assert(errors==0);
    std::cout<<"PASS: production TALK/CAST/WP/LINK/text timer bodies restore11 ->12 ->walking13 ->14, retain3/6/8/7s delays, null/pet/Player invokers, one-shot and explicit redispatch after failed path; no Swim Speed13 cast. Controlled chat/path/cast sinks, not client traversal or voice verification.\n";
}
