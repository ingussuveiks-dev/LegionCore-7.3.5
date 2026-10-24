#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <vector>
using uint32=uint32_t;
using int32=int32_t;
using int64=int64_t;
using QuestStatus=uint32;
using ConditionSourceType=uint32;
enum { CONDITION_AURA=1,CONDITION_QUESTREWARDED=8,CONDITION_QUESTTAKEN=9,CONDITION_CLASS=15,CONDITION_MAPID=22,
    CONDITION_QUEST_COMPLETE=28,CONDITION_OBJECT_ENTRY=31,CONDITION_ALIVE=36,CONDITION_SCENARION_STEP=44,
    CONDITION_SOURCE_TYPE_NONE=0,CONDITION_SOURCE_TYPE_SPELL=17,CONDITION_SOURCE_TYPE_MAX=32,
    QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_REWARDED=6,SMART_EVENT_LINK=61,SMART_EVENT_ON_SPELLCLICK=73,
    SMART_EVENT_SPELLHIT=8,SMART_EVENT_FLAG_EVENT_NON_COMBAT=64,SMART_EVENT_FLAG_NOT_REPEATABLE=1,
    SMART_ACTION_REMOVE_NPC_FLAG=83,UNIT_FIELD_NPC_FLAGS=0 };
using SMART_EVENT=uint32;
#define TC_LOG_DEBUG(...) ((void)0)
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_CASTER_AURASTATE,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_DONT_REPORT};
struct Player;
struct InstanceMap {uint32 identity=42;uint32 GetInstanceId() const{return identity;}};
struct Map {bool instanced=true;InstanceMap instance;InstanceMap* ToInstanceMap(){return instanced?&instance:nullptr;}} world,otherWorld;
struct Scenario {uint32 identity=1099,step=3;uint32 GetScenarioId() const{return identity;}uint32 GetCurrentStep() const{return step;}};
struct ScenarioMgr {bool present=true;Scenario scenario;Scenario* GetScenario(uint32 identity){return present&&identity==42?&scenario:nullptr;}} scenarios;
auto* sScenarioMgr=&scenarios;
struct Unit {
    virtual ~Unit()=default;
    virtual Player* ToPlayer(){return nullptr;}
    Unit* ToUnit(){return this;}
    uint32 entry=103154,mapId=1609,classMask=4;
    bool alive=true,bindingAura=false,clickFlag=true;
    Map* map=&world;
    uint32 GetTypeId(){return ToPlayer()?4:3;}
    uint32 GetEntry() const{return entry;}
    bool IsCreature(){return !ToPlayer();}
    bool isPet() const{return false;}
    Unit* GetOwner() const{return nullptr;}
    struct Threat {Unit* target=nullptr;Unit* getHostilTarget() const{return target;}} threat;
    Threat& getThreatManager(){return threat;}
    uint32 getClassMask() const{return classMask;}
    uint32 GetMapId() const{return mapId;}
    Map* GetMap() const{return map;}
    bool InInstance() const{return map->instanced;}
    bool IsAlive() const{return alive;}
    bool isInCombat() const{return false;}
    bool IsQuestObjectExpired() const{return false;}
    bool HasAuraEffect(uint32 spell,uint32 effect) const{assert(spell==211967&&effect==0);return bindingAura;}
    void RemoveFlag(uint32 field,uint32 flag){assert(field==UNIT_FIELD_NPC_FLAGS&&flag==16777216);clickFlag=false;}
};
using WorldObject=Unit;
struct Player:Unit {
    uint32 quest=3;
    bool rewarded=false;
    Player* ToPlayer() override{return this;}
    uint32 GetQuestStatus(uint32 identity) const{assert(identity==42185);return rewarded?6:quest;}
    bool GetQuestRewardStatus(uint32 identity) const{assert(identity==42185);return rewarded;}
    struct Session {void SendNotification(uint32){assert(false);}} session;
    Session* GetSession(){return &session;}
};
struct Condition;
struct ConditionSourceInfo {
    Unit* mConditionTargets[2];Condition* mLastFailedCondition=nullptr;
    ConditionSourceInfo(Unit* first,Unit* second=nullptr):mConditionTargets{first,second}{}
};
struct Condition {
    uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0,ErrorTextId=0;
    bool NegativeCondition=false;
    bool isLoaded() const{return true;}
    bool Meets(ConditionSourceInfo& sourceInfo){
        Unit* object=sourceInfo.mConditionTargets[ConditionTarget];
        if(!object)return false;
        bool condMeets=false;
        switch(ConditionType){
#include "Conditions.inc"
            default:assert(false);
        }
        bool result=NegativeCondition?!condMeets:condMeets;
        if(!result)sourceInfo.mLastFailedCondition=this;
        return result;
    }
};
using ConditionList=std::vector<Condition*>;
using ConditionReferenceContainer=std::map<uint32,ConditionList>;
using ConditionTypeContainer=std::map<uint32,ConditionList>;
using SmartEventConditionContainer=std::map<std::pair<int32,uint32>,ConditionTypeContainer>;
using ConditionContainer=std::map<ConditionSourceType,ConditionTypeContainer>;
struct ConditionMgr {
    ConditionReferenceContainer ConditionReferenceStore;
    SmartEventConditionContainer SmartEventConditionStore;
    ConditionContainer ConditionStore;
    ConditionList GetConditionsForNotGroupedEntry(ConditionSourceType,uint32);
    ConditionList GetConditionsForSmartEvent(int64,uint32,uint32);
    bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&) const;
    bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*) const;
    bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& rows) const{return rows.empty()||IsObjectMeetToConditionList(info,rows);}
} conditionManager;
auto* sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
#include "SmartConditions.inc"
#include "SmartLookup.inc"
#include "CastLookup.inc"
struct SpellInfo {uint32 Id=211967;struct Misc {struct Data {uint32 SchoolMask=1;} MiscData;} misc;Misc const* GetMisc() const{return &misc;}};
struct Spell {
    Unit* m_caster;SpellInfo info;SpellInfo* m_spellInfo=&info;
    struct Targets {Unit* target;Unit* GetObjectTarget() const{return target;}} m_targets;
    SpellCastResult Check(){
#include "CastConditions.inc"
        return SPELL_CAST_OK;
    }
};
struct GameObject {};
struct SmartScriptHolder {
    int64 entryOrGuid=103154;uint32 source_type=0,event_id=4,type=73;
    bool active=true,runOnce=false;
    uint32 GetEventType() const{return type;}
    struct Event {uint32 event_flags=0,event_phase_mask=0;struct SpellHit {uint32 spell=211967,school=0,cooldownMin=0,cooldownMax=0;} spellHit;struct Click {uint32 cooldownMin=0,cooldownMax=0;} spellclick;} event;
    struct Action {struct Flag {uint32 flag=16777216;} unitFlag;} action;
};
using ObjectList=std::list<WorldObject*>;
bool IsUnit(WorldObject* object){return object&&object->ToUnit();}
struct SmartScript {
    Unit actor;Unit* me=&actor;
    std::vector<SmartScriptHolder> mEvents;
    uint32 queued=0,started=0;
    bool& click=actor.clickFlag;
    WorldObject* GetBaseObject(){return me;}
    bool IsInPhase(uint32) const{return true;}
    void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
    ObjectList* GetTargets(SmartScriptHolder&,Unit*){return new ObjectList{me};}
    void RemoveClicks(SmartScriptHolder& e,Unit* unit){
        switch(SMART_ACTION_REMOVE_NPC_FLAG){
#include "RemoveFlag.inc"
        }
    }
    void ProcessEventsFor(SMART_EVENT,Unit*,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessEvent(SmartScriptHolder& holder,Unit* invoker,uint32 first,uint32 second,bool boolean,SpellInfo const* spell,GameObject* object){
        auto& e=holder;auto* unit=invoker;auto var0=first;auto var1=second;auto bvar=boolean;auto* gob=object;
#include "EventHead.inc"
        switch(e.GetEventType()){
#include "ClickEvent.inc"
#include "HitEvent.inc"
            default:assert(false);
        }
    }
    void ProcessAction(SmartScriptHolder& e,Unit*,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr){
#include "RunOnce.inc"
        if(e.event_id==4)++queued;
        else{assert(e.event_id==5);++started;RemoveClicks(e,nullptr);}
    }
    explicit SmartScript(bool baseline=false){
        SmartScriptHolder root;root.event.event_flags=baseline?1:0;mEvents.push_back(root);
        root.event_id=5;root.type=8;root.event.event_flags=1;mEvents.push_back(root);
    }
};
#include "EventDispatch.inc"
bool Admit(Unit* player,Unit* actor,uint32 event){return conditionManager.IsObjectMeetingSmartEventConditions(103154,event,0,player,actor);}
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);
    otherWorld.instance.identity=43;
    Player owner;Unit pet;SpellInfo binding;
    SmartScript baseline(true);baseline.ProcessEventsFor(73,&pet);baseline.ProcessEventsFor(73,&owner);assert(baseline.queued==1&&baseline.mEvents[0].runOnce);
    std::ifstream input(argv[1]);std::vector<Condition> rows;Condition row;
    while(input>>row.ElseGroup>>row.ConditionType>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2>>row.ConditionValue3>>row.NegativeCondition)rows.push_back(row);
    assert(input.eof()&&rows.size()==24);
    for(auto& condition:rows){conditionManager.SmartEventConditionStore[{103154,0}][5].push_back(&condition);if(condition.ConditionType!=CONDITION_AURA)conditionManager.SmartEventConditionStore[{103154,0}][6].push_back(&condition);conditionManager.ConditionStore[17][211967].push_back(&condition);}
    for(uint32 failure=0;failure<18;++failure){
        Player player;SmartScript script;scenarios={};Unit* caster=&player;Unit* target=&script.actor;
        switch(failure){
            case 0:player.quest=0;break;case 1:player.rewarded=true;break;case 2:player.classMask=128;break;
            case 3:player.mapId=1220;break;case 4:player.alive=false;break;case 5:script.actor.alive=false;break;
            case 6:scenarios.scenario.step=2;break;case 7:scenarios.scenario.step=4;break;
            case 8:scenarios.scenario.identity=1068;break;case 9:scenarios.present=false;break;
            case 10:script.actor.entry=106745;break;case 11:script.actor.mapId=1220;break;
            case 12:caster=&pet;break;case 13:caster=nullptr;break;case 14:script.actor.map=&otherWorld;break;
            case 15:pet.threat.target=&owner;caster=&pet;break;
            case 16:script.me=nullptr;target=nullptr;break;
            case 17:script.me=&owner;target=&owner;break;
        }
        assert(!Admit(caster,target,4)&&!Admit(caster,target,5));
        script.ProcessEventsFor(73,caster);script.ProcessEventsFor(8,caster,0,0,false,&binding);
        assert(!script.queued&&!script.started&&!script.mEvents[0].runOnce&&!script.mEvents[1].runOnce);
        if(caster){Spell spell{caster,{},nullptr,{target}};spell.m_spellInfo=&spell.info;assert(spell.Check()!=SPELL_CAST_OK);}
    }
    scenarios={};
    for(uint32 quest:{3u,1u}){
        Player player;player.quest=quest;SmartScript script;
        script.ProcessEventsFor(73,&pet);assert(!script.queued&&!script.mEvents[0].runOnce);
        script.ProcessEventsFor(73,&player);assert(script.queued==1&&script.click);
        script.ProcessEventsFor(73,&player);assert(script.queued==2&&script.click&&!script.started);
        SpellInfo unrelated;unrelated.Id=211966;
        script.ProcessEventsFor(8,&player,0,0,false,&unrelated);assert(!script.started&&script.click);
        script.ProcessEventsFor(8,&player);assert(!script.started&&script.click);
        Spell spell{&player,{},nullptr,{&script.actor}};spell.m_spellInfo=&spell.info;assert(spell.Check()==SPELL_CAST_OK);
        player.quest=0;assert(spell.Check()!=SPELL_CAST_OK);script.ProcessEventsFor(8,&player,0,0,false,&binding);assert(!script.started&&script.click);
        player.quest=quest;assert(spell.Check()==SPELL_CAST_OK);script.actor.bindingAura=true;
        script.ProcessEventsFor(8,&player,0,0,false,&binding);assert(script.started==1&&!script.click);
        script.ProcessEventsFor(73,&player);assert(script.queued==2&&spell.Check()!=SPELL_CAST_OK);
        script.ProcessEventsFor(8,&player,0,0,false,&binding);assert(script.started==1);
    }
    std::cout<<"PASS: actual condition cases/ElseGroups, source22 event dispatch/one-shot guard and source17 CheckCast block reject wrong claimant/actor/step; invalid clicks preserve the event, failed-cast click retry remains admitted and repeated valid hit starts one controlled list sink. No native spell effect/timer/world traversal claim.\n";
}
