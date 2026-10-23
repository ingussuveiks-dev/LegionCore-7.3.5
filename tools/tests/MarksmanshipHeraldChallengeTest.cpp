#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;using int64=int64_t;
enum SMART_EVENT {SMART_EVENT_UPDATE=60,SMART_EVENT_LINK=61,SMART_EVENT_DATA_SET=38,SMART_EVENT_TEXT_OVER=52};
enum {SMART_EVENT_UPDATE_IC=0,SMART_EVENT_UPDATE_OOC=1,SMART_EVENT_HEALTH_PCT=2,SMART_EVENT_TARGET_HEALTH_PCT=3,
    SMART_EVENT_MANA_PCT=4,SMART_EVENT_TARGET_MANA_PCT=5,SMART_EVENT_RANGE=9,SMART_EVENT_VICTIM_CASTING=13,SMART_EVENT_FRIENDLY_HEALTH=14,
    SMART_EVENT_FRIENDLY_IS_CC=15,SMART_EVENT_FRIENDLY_MISSING_BUFF=16,SMART_EVENT_HAS_AURA=23,SMART_EVENT_TARGET_BUFFED=24,
    SMART_EVENT_IS_BEHIND_TARGET=67,SMART_EVENT_CHECK_DIST_TO_HOME=73,SMART_EVENT_DISTANCE_CREATURE=75,SMART_EVENT_FRIENDLY_HEALTH_PCT=74,
    SMART_ACTION_CAST=11,SMART_ACTION_CAST_CUSTOM=75,UNIT_STATE_CASTING=1,SMART_SCRIPT_TYPE_TIMED_ACTIONLIST=9,
    SMARTCAST_INTERRUPT_PREVIOUS=1,SMARTCAST_TRIGGERED=2,SMART_EVENT_FLAG_NOT_REPEATABLE=1,CONDITION_NEAR_CREATURE=29,
    CONDITION_SCENARION_STEP=44,SMART_ACTION_SET_DATA=45,SMART_ACTION_TALK=1,SMART_TARGET_CLOSEST_CREATURE=19};
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
struct Unit;struct Creature;struct Player;struct GameObject;struct SpellInfo;struct SmartScript;
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
struct InstanceMap {uint32 GetInstanceId(){return 42;}} instanceMap;
struct Map {InstanceMap* ToInstanceMap(){return &instanceMap;}} worldMap;
struct WorldObject {
    bool instanced=true,expired=false;std::vector<Creature*> nearby;
    virtual ~WorldObject()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}virtual GameObject* ToGameObject(){return nullptr;}
    bool InInstance(){return instanced;}Map* GetMap(){return &worldMap;}bool IsQuestObjectExpired(){return expired;}
    bool HasQuestObjectScope(){return false;}bool CanTargetQuestObject(WorldObject*){return true;}
    Creature* FindNearestCreature(uint32,float,bool=true);bool IsWithinDistInMap(Creature*,float);float GetDistance(Creature*);
};
struct Unit:WorldObject {ObjectGuid GetGUID(){return {this};}};
struct Player:Unit {Player* ToPlayer()override{return this;}};
struct SmartAI {SmartScript* script=nullptr;SmartScript* GetScript(){return script;}void SetData(uint32,uint32);};
struct GameObject:WorldObject {SmartAI ai;SmartAI* AI(){return &ai;}};
struct Creature:Unit {
    uint32 identity=0,speeches=0,lastSpeech=99;float distance=1;bool alive=true,sameMap=true,samePhase=true,available=true,combat=false;
    SmartAI ai;Creature* ToCreature()override{return this;}SmartAI* AI(){return &ai;}
    bool IsAlive(){return alive;}uint32 GetEntry(){return identity;}bool isInCombat(){return combat;}bool HasUnitState(uint32){return false;}
};
bool WorldObject::IsWithinDistInMap(Creature* actor,float range){return actor&&actor->available&&actor->sameMap&&actor->samePhase&&actor->distance<=range;}
float WorldObject::GetDistance(Creature* actor){return actor->distance;}
Creature* WorldObject::FindNearestCreature(uint32 identity,float range,bool alive){
    Creature* nearest=nullptr;for(Creature* actor:nearby)if(actor->identity==identity&&actor->alive==alive&&IsWithinDistInMap(actor,range)){nearest=actor;range=actor->distance;}return nearest;
}
#include "NearestHelper.inc"
struct Scenario {uint32 id=972,step=4;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct ScenarioManager {bool present=true;Scenario scenario;Scenario* GetScenario(uint32 identity){return present&&identity==42?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct ConditionSourceInfo {WorldObject* mConditionTargets[2];ConditionSourceInfo(WorldObject* first,WorldObject* second):mConditionTargets{first,second}{}};
struct Condition {
    uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;bool NegativeCondition=false;
    bool isLoaded(){return true;}
    bool Meets(ConditionSourceInfo& sourceInfo){WorldObject* object=sourceInfo.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;
        switch(ConditionType){
#include "HeraldConditions.inc"
#include "HeraldScenario.inc"
            default:assert(false);
        }return NegativeCondition?!condMeets:condMeets;
    }
};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
using ConditionTypeContainer=std::map<uint32,ConditionList>;using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
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
struct TextManager {void SendChat(Creature* actor,uint8 group,ObjectGuid){++actor->speeches;actor->lastSpeech=group;}} textManager;
auto sCreatureTextMgr=&textManager;
struct SmartScriptHolder {
    int64 entryOrGuid=0;uint32 event_id=0,source_type=0,link=0,type=0,actionType=0,targetType=0,timer=0;bool runOnce=false,active=true,enableTimed=false;
    struct {uint32 event_flags=0,event_phase_mask=0;
        struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;}minMaxRepeat;
        struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;}dataSet;
        struct {uint32 textGroupID=0,creatureEntry=0;}textOver;
    }event;
    struct {struct {uint32 field=0,data=0;}setData;struct {uint32 textGroupID=0,duration=0;}talk;struct {uint32 flags=0;}cast;}action;
    struct {struct {uint32 entry=0,dist=0,dead=0;}closest;}target;
    uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}uint32 GetScriptType()const{return source_type;}
};
using SmartAIEventList=std::vector<SmartScriptHolder>;using ObjectList=std::list<WorldObject*>;
uint32 urand(uint32 minimum,uint32 maximum){assert(minimum==maximum);return minimum;}
struct SmartScript {
    Creature* me=nullptr;SmartAIEventList mEvents,mTimedActionList;ObjectGuid mLastInvoker,mTextGUID;
    uint32 mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false;
    WorldObject* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}bool IsInPhase(uint32){return false;}
    bool IsCreature(WorldObject* object){return object&&object->ToCreature();}bool IsPlayer(WorldObject* object){return object&&object->ToPlayer();}bool IsGameObject(WorldObject*){return false;}
    ObjectList* GetWorldObjectsInDist(float){auto* result=new ObjectList;for(auto* actor:me->nearby)result->push_back(actor);return result;}
    ObjectList* GetTargets(SmartScriptHolder const& row,Unit*){auto* targets=new ObjectList;
        switch(row.GetTargetType()){
#define e row
#define l targets
#include "ClosestCreature.inc"
#undef l
#undef e
            case 1:if(me)targets->push_back(me);break;
            default:assert(false);
        }if(targets->empty()){delete targets;return nullptr;}return targets;
    }
    void RecalcTimer(SmartScriptHolder&,uint32,uint32);void UpdateTimer(SmartScriptHolder&,uint32);
    void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
#define e nativeEvent
    void ProcessEvent(SmartScriptHolder& nativeEvent,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "RepeatGuard.inc"
#include "EventGate.inc"
        switch(nativeEvent.GetEventType()){
#include "TimedUpdate.inc"
#include "DataEvent.inc"
#include "TextOver.inc"
        }
    }
    void ProcessAction(SmartScriptHolder& nativeEvent,Unit* unit=nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr){
        if(GetBaseObject()&&GetBaseObject()->IsQuestObjectExpired())return;
#include "ActionStart.inc"
        switch(nativeEvent.GetActionType()){
#include "SetData.inc"
#include "Talk.inc"
            default:assert(false);
        }
    }
#undef e
    void TextTick(uint32 diff){
#include "TextTimer.inc"
    }
};
#include "Dispatch.inc"
#include "RecalcTimer.inc"
#include "UpdateTimer.inc"
#include "AiSetData.inc"
SmartAIEventList definitions;
struct Fixture {
    Creature vereesa,herald;SmartScript sender,receiver;
    Fixture(){scenarioManager={};vereesa.identity=100397;herald.identity=100836;vereesa.nearby={&herald};herald.nearby={&vereesa};
        sender.me=&vereesa;receiver.me=&herald;vereesa.ai.script=&sender;herald.ai.script=&receiver;
        for(auto const& row:definitions)(row.entryOrGuid==100397?sender.mEvents:receiver.mEvents).push_back(row);
    }
    void Tick(){sender.UpdateTimer(sender.mEvents[0],1001);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
    std::vector<Condition> gates(3);std::ifstream conditions(argv[2]);
    for(auto& gate:gates){int64 actor;uint32 group;assert(conditions>>actor>>group>>gate.ElseGroup>>gate.ConditionType>>gate.ConditionTarget>>gate.ConditionValue1>>gate.ConditionValue2>>gate.ConditionValue3>>gate.NegativeCondition);assert(gate.ConditionTarget==1);conditionManager.SmartEventConditionStore[{actor,0}][group].push_back(&gate);}
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<4;++index){SmartScriptHolder row;uint32 first,second,third,fourth,actionFirst,actionSecond;
        assert(events>>row.entryOrGuid>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags>>first>>second>>third>>fourth>>row.actionType>>actionFirst>>actionSecond>>row.targetType>>row.target.closest.entry>>row.target.closest.dist>>row.target.closest.dead);
        row.event.minMaxRepeat={first,second,third,fourth};row.event.dataSet={first,second,third,fourth};row.event.textOver={first,second};row.action.setData={actionFirst,actionSecond};row.action.talk={actionFirst,actionSecond};assert(!row.link);definitions.push_back(row);
    }
    assert(definitions[0].entryOrGuid==100397&&definitions[0].event_id==25&&definitions[0].type==60&&definitions[0].actionType==45);
    assert(definitions[2].entryOrGuid==100836&&definitions[2].event_id==0&&definitions[2].actionType==1&&definitions[2].action.talk.duration==8000);
    auto saved=conditionManager.SmartEventConditionStore;
    conditionManager.SmartEventConditionStore.clear();
    {Fixture baseline;scenarioManager.scenario.step=0;baseline.vereesa.nearby.clear();baseline.Tick();assert(baseline.sender.mEvents[0].runOnce&&!baseline.herald.speeches);
        scenarioManager.scenario.step=4;baseline.vereesa.nearby={&baseline.herald};baseline.Tick();assert(!baseline.herald.speeches);}
    {Fixture premature;scenarioManager.scenario.step=3;premature.Tick();assert(premature.herald.speeches==1&&premature.receiver.mEvents[0].runOnce);}
    conditionManager.SmartEventConditionStore=saved;
    for(uint32 stage=0;stage<12;++stage){if(stage==4)continue;Fixture retry;scenarioManager.scenario.step=stage;
        for(uint32 tick=0;tick<10;++tick)retry.Tick();assert(!retry.sender.mEvents[0].runOnce&&!retry.receiver.mEvents[0].runOnce&&!retry.herald.speeches);
        retry.herald.AI()->SetData(1,1);assert(!retry.receiver.mEvents[0].runOnce);
        scenarioManager.scenario.step=4;retry.Tick();assert(retry.herald.speeches==1&&retry.sender.mEvents[0].runOnce&&retry.receiver.mEvents[0].runOnce);
        assert(retry.receiver.mUseTextTimer&&retry.receiver.mTextTimer==8000&&retry.receiver.mTalkerEntry==100836&&!retry.receiver.mLastTextID);
        for(uint32 tick=0;tick<20;++tick){retry.Tick();retry.herald.AI()->SetData(1,1);}assert(retry.herald.speeches==1);
    }
    for(uint32 blocked=0;blocked<9;++blocked){Fixture retry;
        switch(blocked){case 0:retry.vereesa.nearby.clear();break;case 1:retry.herald.distance=50.01f;break;case 2:retry.herald.sameMap=false;break;case 3:retry.herald.samePhase=false;break;case 4:retry.herald.alive=false;break;case 5:scenarioManager.present=false;break;case 6:scenarioManager.scenario.id=1010;break;case 7:retry.vereesa.instanced=false;break;case 8:retry.vereesa.expired=true;break;}
        retry.Tick();assert(!retry.sender.mEvents[0].runOnce&&!retry.herald.speeches);
        retry.vereesa.nearby={&retry.herald};retry.herald.distance=50;retry.herald.sameMap=true;retry.herald.samePhase=true;retry.herald.alive=true;retry.vereesa.instanced=true;retry.vereesa.expired=false;scenarioManager={};retry.Tick();assert(retry.herald.speeches==1);
    }
    {Fixture missing;missing.sender.me=nullptr;missing.Tick();assert(!missing.sender.mEvents[0].runOnce&&!missing.herald.speeches);}
    {Fixture wrong;wrong.herald.AI()->SetData(1,2);wrong.herald.AI()->SetData(2,1);assert(!wrong.herald.speeches&&!wrong.receiver.mEvents[0].runOnce);wrong.Tick();assert(wrong.herald.lastSpeech==0);}
    {Fixture receiverGuard;scenarioManager.scenario.step=3;Unit pet,guardian;Player player;
        for(Unit* source:{&pet,&guardian,static_cast<Unit*>(&player),static_cast<Unit*>(nullptr)})receiverGuard.receiver.ProcessEventsFor(SMART_EVENT_DATA_SET,source,1,1);
        assert(!receiverGuard.herald.speeches&&!receiverGuard.receiver.mEvents[0].runOnce);scenarioManager.scenario.step=4;receiverGuard.Tick();assert(receiverGuard.herald.speeches==1&&!receiverGuard.receiver.mLastInvoker.unit);}
    {Fixture timing;timing.Tick();timing.receiver.TextTick(8000);assert(timing.receiver.mUseTextTimer&&!timing.receiver.mTextTimer&&!timing.vereesa.speeches);timing.receiver.TextTick(1);assert(!timing.receiver.mUseTextTimer&&timing.vereesa.speeches==1&&timing.vereesa.lastSpeech==10);timing.receiver.TextTick(1000);assert(timing.vereesa.speeches==1);}
    std::cout<<"PASS: native timer/repeat/conditions/nearest/SET_DATA/SmartAI/TALK reproduce lost and premature Herald challenge; native972 stage4 and living50yd actor preserve loaded retry and exactly-once speech/8000ms text timer. World/grid/scenario/text hosts are not client traversal or audio verification.\n";
}
