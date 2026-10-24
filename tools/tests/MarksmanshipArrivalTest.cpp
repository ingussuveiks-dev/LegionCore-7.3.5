#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using int64=int64_t;using uint8=uint8_t;
enum SMART_EVENT {SMART_EVENT_OOC_LOS=10,SMART_EVENT_TEXT_OVER=52,SMART_EVENT_LINK=61};
enum {SMART_ACTION_TALK=1,SMART_ACTION_CAST=11,SMART_ACTION_CALL_KILLEDMONSTER=33,SMART_ACTION_WP_START=53,
    SMART_TARGET_NONE=0,SMART_TARGET_ACTION_INVOKER=7,SMART_EVENT_FLAG_NOT_REPEATABLE=1,
    CONDITION_TYPE_MASK=32,CONDITION_SCENARION_STEP=44,SMART_ESCORT_TARGETS=0};
enum ReactStates {REACT_PASSIVE};
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
#define ASSERT(value) assert(value)
#define CAST_AI(type,value) static_cast<type*>(value)
struct Unit;struct Player;struct Creature;
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
struct InstanceMap {uint32 GetInstanceId(){return 42;}} instanceMap;
struct Map {bool instanced=true;InstanceMap* ToInstanceMap(){return instanced?&instanceMap:nullptr;}} worldMap;
struct WorldObject {
    bool expired=false,instanced=true;
    virtual ~WorldObject()=default;
    virtual Unit* ToUnit(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}
    bool IsQuestObjectExpired(){return expired;}
    bool InInstance(){return instanced;}Map* GetMap(){return &worldMap;}
    bool isType(uint32 mask){return ToPlayer()&&(mask&16);}
};
struct Unit:WorldObject {
    float distance=1;bool visible=true,hostile=false;
    Unit* ToUnit()override{return this;}ObjectGuid GetGUID(){return {this};}
};
struct Player:Unit {uint32 faction=0;Player* ToPlayer()override{return this;}};
struct SmartAI {
    uint32 paths=0,path=0,mEscortQuestID=0;
    bool StartPath(bool run,uint32 entry,bool repeat,Unit*){assert(run&&!repeat);++paths;path=entry;return true;}
    void SetDespawnTime(uint32 value){assert(value==0);}
};
struct Creature:Unit {
    uint32 entry=100397;bool combat=false;SmartAI ai;
    Creature* ToCreature()override{return this;}uint32 GetEntry(){return entry;}
    bool isInCombat(){return combat;}
    bool IsWithinDistInMap(Unit* unit,float range){return unit&&unit->distance<=range;}
    bool IsWithinLOSInMap(Unit* unit){return unit&&unit->visible;}
    bool IsHostileTo(Unit* unit){return unit&&unit->hostile;}
    void SetReactState(ReactStates){}SmartAI* AI(){return &ai;}
};
struct Scenario {uint32 id=972,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct ScenarioManager {bool present=true;Scenario scenario;Scenario* GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct ConditionSourceInfo {WorldObject* mConditionTargets[2];ConditionSourceInfo(WorldObject* first,WorldObject* second):mConditionTargets{first,second}{}};
struct Condition {
    uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;bool NegativeCondition=false;
    bool isLoaded(){return true;}
    bool Meets(ConditionSourceInfo& sourceInfo){
        WorldObject* object=sourceInfo.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;
        switch(ConditionType){
#include "Conditions.inc"
            default:assert(false);
        }return NegativeCondition?!condMeets:condMeets;
    }
};
using ConditionList=std::vector<Condition*>;
using ConditionReferenceContainer=std::map<uint32,ConditionList>;
using ConditionTypeContainer=std::map<uint32,ConditionList>;
using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr {
    ConditionReferenceContainer ConditionReferenceStore;SmartEventConditionContainer SmartEventConditionStore;
    bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
    bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& rows)const{return rows.empty()||IsObjectMeetToConditionList(info,rows);}
    bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*)const;
    ConditionList GetConditionsForSmartEvent(int64,uint32,uint32);
} conditionManager;
auto sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
#include "SmartEventConditions.inc"
#include "EventConditions.inc"
struct SmartScriptHolder {
    int64 entryOrGuid=100397;uint32 source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0;bool runOnce=false;
    struct {uint32 event_flags=1,event_phase_mask=0;struct{uint32 noHostile=1,maxDist=10,cooldownMin=0,cooldownMax=0;}los;struct{uint32 textGroupID=0,creatureEntry=100397;}textOver;}event;
    struct {struct{uint32 textGroupID=0,duration=0;}talk;struct{uint32 run=1,pathID=100397,repeat=0,quest=0,despawnTime=0,reactState=0;}wpStart;}action;
    uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}uint32 GetScriptType()const{return source_type;}
};
using SmartAIEventList=std::vector<SmartScriptHolder>;using ObjectList=std::list<WorldObject*>;
struct Chat{uint32 actor,group;Unit* owner;};
struct TextManager{std::vector<Chat> chats;void SendChat(Creature* actor,uint8 group,ObjectGuid owner){chats.push_back({actor->GetEntry(),group,owner.unit});}}textManager;
auto sCreatureTextMgr=&textManager;
struct SpellInfo{};struct GameObject{};
struct SmartScript {
    Creature* me=nullptr;SmartAIEventList mEvents;ObjectGuid mLastInvoker,mTextGUID;uint32 mTextTimer=0,mLastTextID=0,mTalkerEntry=0,casts=0,credits=0;bool mUseTextTimer=false;
    WorldObject* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}
    bool IsInPhase(uint32){return true;}bool IsSmart(){return true;}
    bool IsPlayer(WorldObject* object){return object&&object->ToPlayer();}bool IsCreature(WorldObject* object){return object&&object->ToCreature();}
    void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
    void StoreTargetList(ObjectList* targets,uint32){delete targets;}
    ObjectList* GetTargets(SmartScriptHolder const& row,Unit* unit){auto* list=new ObjectList;Unit* trigger=unit?unit:GetLastInvoker();switch(row.GetTargetType()){
        case SMART_TARGET_ACTION_INVOKER:if(trigger)list->push_back(trigger);break;
        case 1:list->push_back(me);break;
    }return list;}
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessEvent(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "RepeatGuard.inc"
#include "EventGate.inc"
        switch(e.GetEventType()){
#include "Los.inc"
#include "TextOver.inc"
            case SMART_EVENT_LINK:ProcessAction(e,unit,var0,var1,bvar,spell,gob);break;
        }
    }
    void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "ActionStart.inc"
        switch(e.GetActionType()){
#include "Talk.inc"
#include "Waypoint.inc"
            case SMART_ACTION_CAST:++casts;break;
            case SMART_ACTION_CALL_KILLEDMONSTER:++credits;break;
        }
#include "LinkTail.inc"
    void Tick(uint32 diff){
#include "TextTimer.inc"
    }
};
#include "Dispatch.inc"
#include "FindLink.inc"
SmartAIEventList definitions;
struct Fixture {
    Creature actor,pet;Player owner;SmartScript script;
    Fixture(){scenarioManager={};textManager.chats.clear();pet.entry=416;script.me=&actor;script.mEvents=definitions;}
    void See(Unit* unit){script.ProcessEventsFor(SMART_EVENT_OOC_LOS,unit);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
    std::vector<Condition> rows(2);std::ifstream conditions(argv[2]);
    for(auto& row:rows)assert(conditions>>row.ElseGroup>>row.ConditionType>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2>>row.ConditionValue3>>row.NegativeCondition);
    for(auto& row:rows)conditionManager.SmartEventConditionStore[{100397,0}][1].push_back(&row);
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<5;++index){SmartScriptHolder row;uint32 first,second,third,fourth;
        assert(events>>row.event_id>>row.link>>row.type>>row.event.event_flags>>row.event.textOver.textGroupID>>row.event.textOver.creatureEntry>>row.actionType>>first>>second>>third>>fourth>>row.targetType);
        if(row.type==10){row.event.los.noHostile=row.event.textOver.textGroupID;row.event.los.maxDist=row.event.textOver.creatureEntry;}
        row.action.talk={first,second};row.action.wpStart.run=first;row.action.wpStart.pathID=second;row.action.wpStart.repeat=third;row.action.wpStart.quest=fourth;definitions.push_back(row);
    }
    assert(definitions.front().event_id==0&&definitions.front().targetType==1);
    {
        auto saved=conditionManager.SmartEventConditionStore;conditionManager.SmartEventConditionStore[{100397,0}][1]={&rows[1]};
        assert(rows[1].ConditionType==44);Fixture baseline;baseline.script.mEvents[0].targetType=7;baseline.See(&baseline.pet);
        assert(textManager.chats.size()==1&&textManager.chats[0].actor==416);baseline.script.Tick(6001);baseline.See(&baseline.owner);
        assert(textManager.chats.size()==1&&baseline.script.mEvents[0].runOnce&&!baseline.actor.ai.paths);conditionManager.SmartEventConditionStore=saved;
    }
    for(uint32 faction=0;faction<2;++faction){
        Fixture fixed;fixed.owner.faction=faction;Creature guardian,npc;guardian.entry=1863;npc.entry=100836;
        for(Unit* nonplayer:{static_cast<Unit*>(&fixed.pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)})fixed.See(nonplayer);
        assert(textManager.chats.empty()&&!fixed.script.mEvents[0].runOnce&&!fixed.script.casts&&!fixed.script.credits);
        fixed.See(&fixed.owner);assert(textManager.chats.size()==1&&textManager.chats[0].actor==100397&&textManager.chats[0].owner==&fixed.owner);
        assert(fixed.script.casts==1&&fixed.script.credits==1);fixed.script.Tick(6001);assert(textManager.chats.size()==2&&textManager.chats[1].group==1);
        fixed.script.Tick(4001);assert(fixed.actor.ai.paths==1&&fixed.actor.ai.path==100397);
        for(uint32 repeat=0;repeat<20;++repeat)fixed.See(&fixed.owner);assert(fixed.script.casts==1&&fixed.script.credits==1&&fixed.actor.ai.paths==1);
    }
    for(uint32 stage=1;stage<15;++stage){Fixture gated;scenarioManager.scenario.step=stage;gated.See(&gated.owner);assert(textManager.chats.empty()&&!gated.script.mEvents[0].runOnce);}
    for(uint32 blocked=0;blocked<8;++blocked){Fixture retry;
        switch(blocked){case 0:retry.owner.distance=11;break;case 1:retry.owner.visible=false;break;case 2:retry.owner.hostile=true;break;case 3:retry.actor.combat=true;break;case 4:scenarioManager.present=false;break;case 5:scenarioManager.scenario.id=973;break;case 6:retry.actor.instanced=false;break;case 7:retry.actor.expired=true;break;}
        retry.See(&retry.owner);assert(textManager.chats.empty()&&!retry.script.mEvents[0].runOnce);
        retry.owner.distance=1;retry.owner.visible=true;retry.owner.hostile=false;retry.actor.combat=false;retry.actor.instanced=true;retry.actor.expired=false;scenarioManager={};retry.See(&retry.owner);assert(textManager.chats.size()==1);
    }
    {Fixture missing;missing.script.me=nullptr;missing.See(&missing.owner);assert(textManager.chats.empty()&&!missing.script.mEvents[0].runOnce);missing.script.me=&missing.actor;missing.See(&missing.owner);assert(textManager.chats.size()==1);}
    std::cout<<"PASS: actual LOS/condition lookup/TALK/timer/TEXT_OVER/LINK/WP request bodies reproduce pet-owned missing speaker and stalled intro; scoped player admission/self speaker preserve0 ->1 ->path100397 once, both faction hosts, nonplayers, stages and retry. Cast, credit and path execution are controlled boundaries, not client traversal.\n";
}
