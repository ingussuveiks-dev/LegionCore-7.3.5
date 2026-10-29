#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>
using uint32 = uint32_t;
using uint8 = uint8_t;
using SMART_EVENT = uint32;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
enum { SMART_EVENT_LINK=61, SMART_EVENT_JUST_SUMMONED=54, SMART_EVENT_TEXT_OVER=52, SMART_EVENT_DATA_SET=38,
    SMART_EVENT_RESET=25, SMART_EVENT_FLAG_NOT_REPEATABLE=1, SMART_EVENT_FLAG_EVENT_NON_COMBAT=64,
    SMART_EVENT_FLAG_DONT_RESET=256, SMART_ACTION_TALK=1, SMART_ACTION_SET_DATA=45, SMART_ACTION_WP_START=53 };
struct ObjectGuid { uint32 value=0; static const ObjectGuid Empty; };
const ObjectGuid ObjectGuid::Empty{};
struct SpellInfo { };
struct Player;
struct Creature;
struct GameObject;
struct Unit {
    virtual ~Unit()=default;
    virtual Creature* ToCreature() { return nullptr; }
    virtual Player* ToPlayer() { return nullptr; }
    virtual GameObject* ToGameObject() { return nullptr; }
    ObjectGuid GetGUID() const { return {}; }
};
struct CreatureAI {
    virtual ~CreatureAI()=default;
    virtual void IsSummonedBy(Unit*) { }
    virtual void SetData(uint32,uint32) { }
    virtual void UpdateAI(uint32) { }
};
struct Creature : Unit {
    uint32 entry=104307,map=1539,instance=42;
    bool expired=false;
    CreatureAI* brain=nullptr;
    Creature* ToCreature() override { return this; }
    CreatureAI* AI() { return brain; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return map; }
    uint32 GetInstanceId() const { return instance; }
    bool isInCombat() const { return false; }
    bool IsQuestObjectExpired() const { return expired; }
};
struct Player : Unit { Player* ToPlayer() override { return this; } };
struct GameObject : Unit {
    GameObject* ToGameObject() override { return this; }
    CreatureAI* AI() { assert(false);return nullptr; }
};
bool IsCreature(Unit* unit) { return unit&&unit->ToCreature(); }
bool IsPlayer(Unit* unit) { return unit&&unit->ToPlayer(); }
bool IsGameObject(Unit* unit) { return unit&&unit->ToGameObject(); }
using ObjectList=std::list<Unit*>;
struct Scenario { uint32 identity=1037,step=9;uint32 GetScenarioId() const { return identity; }uint32 GetCurrentStep() const { return step; } };
struct ScenarioMgr {
    Scenario scenario;
    bool present=true;
    Scenario* GetScenario(uint32 instance) { return present&&instance==42?&scenario:nullptr; }
} scenarioManager;
auto* sScenarioMgr=&scenarioManager;
struct SmartScriptHolder {
    uint32 entryOrGuid=104307,event_id=0,source_type=0,link=0,target=1;
    bool runOnce=false,active=true;
    struct Event {
        uint32 type=0,event_flags=1,event_phase_mask=0,event_chance=100;
        union {
            struct { uint32 textGroupID,creatureEntry,padding0,padding1; } textOver;
            struct { uint32 id,value,cooldownMin,cooldownMax; } dataSet;
        };
        Event():dataSet{} { }
    } event;
    struct Action {
        uint32 type=0;
        union {
            struct { uint32 textGroupID,duration; } talk;
            struct { uint32 field,data; } setData;
        };
        Action():setData{} { }
    } action;
    uint32 GetEventType() const { return event.type; }
    uint32 GetActionType() const { return action.type; }
    uint32 GetTargetType() const { return target; }
    uint32 GetScriptType() const { return source_type; }
};
using ConditionList=std::vector<int>;
struct ConditionSourceInfo { ConditionSourceInfo(Unit*,Unit*) { } };
struct ConditionMgr {
    ConditionList GetConditionsForSmartEvent(uint32,uint32,uint32) { return {}; }
    bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&) { return true; }
    bool IsObjectMeetingSmartEventConditions(uint32,uint32,uint32,Unit*,Unit*) { return true; }
} conditionManager;
auto* sConditionMgr=&conditionManager;
bool roll_chance_i(uint32) { assert(false);return false; }
std::vector<uint32> spoken;
struct CreatureTextMgr { void SendChat(Creature*,uint8 group,ObjectGuid const&) { spoken.push_back(group); } } textManager;
auto* sCreatureTextMgr=&textManager;
struct SmartScript {
    Creature* me;
    std::vector<SmartScriptHolder> mEvents;
    uint32 mTalkerEntry=0,mLastTextID=0,mTextTimer=0,pathStarts=0;
    ObjectGuid mTextGUID{},mLastInvoker{};
    bool mUseTextTimer=false;
    explicit SmartScript(Creature* actor):me(actor) { }
    Creature* GetBaseObject() { return me; }
    Unit* GetLastInvoker() { return nullptr; }
    ObjectList* GetTargets(SmartScriptHolder const& event,Unit*) { assert(event.target==1);return new ObjectList{me}; }
    bool IsInPhase(uint32) { assert(false);return false; }
    void RecalcTimer(SmartScriptHolder&,uint32 minimum,uint32 maximum) { assert(!minimum&&!maximum); }
    void SetPhase(uint32 phase) { assert(!phase); }
    void ResetBaseObject() { }
    void InitTimer(SmartScriptHolder& event) { event.active=true; }
    void OnReset();
    SmartScriptHolder FindLinkedEvent(uint32 link);
    void ProcessEventsFor(SMART_EVENT,Unit* = nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* = nullptr,GameObject* = nullptr);
    void ProcessEvent(SmartScriptHolder&,Unit* = nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* = nullptr,GameObject* = nullptr);
    void ProcessAction(SmartScriptHolder&,Unit* = nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* = nullptr,GameObject* = nullptr);
    void Tick(uint32 diff) {
#include "TextTimer.inc"
    }
};
#include "FindLinked.inc"
#include "ProcessEvents.inc"
#include "ProcessEvent.inc"
#include "ProcessAction.inc"
#include "OnReset.inc"
struct SmartAI : CreatureAI {
    Creature* me;
    SmartScript script;
    explicit SmartAI(Creature* actor):me(actor),script(actor) { }
    SmartScript* GetScript() { return &script; }
    void IsSummonedBy(Unit*) override;
    void SetData(uint32,uint32) override;
    void UpdateAI(uint32) override { }
};
#include "Summoned.inc"
#include "SetData.inc"
#ifdef BASELINE
struct npc_stromkar_thoradin : SmartAI { using SmartAI::SmartAI; };
#else
namespace StromkarReturn {
struct State {uint32 signals=0;};
void Signal(Creature*,uint32 id,uint32 value,State& state) {if(id==3&&value==1)++state.signals;}
void Update(Creature*,uint32,State&) { }
}
#include "Thoradin.inc"
#endif
std::vector<SmartScriptHolder> ReadRows(char const* path) {
    std::ifstream input(path);assert(input);
    std::vector<SmartScriptHolder> rows;
    SmartScriptHolder row;
    while(input>>row.event_id>>row.link>>row.event.type>>row.event.event_flags>>row.event.dataSet.id>>row.event.dataSet.value
        >>row.event.dataSet.cooldownMin>>row.event.dataSet.cooldownMax>>row.action.type>>row.action.setData.field>>row.action.setData.data>>row.target)
        rows.push_back(row);
    return rows;
}
void Advance(SmartScript& script,uint32 ticks) { for(uint32 tick=0;tick<ticks;++tick)script.Tick(1); }
int main(int argc,char** argv) {
    assert(argc==2);
    auto rows=ReadRows(argv[1]);
    bool hasReturn=false;for(auto const&row:rows)if(row.event_id==6)hasReturn=true;
    {
        Creature actor;npc_stromkar_thoradin content(&actor);actor.brain=&content;content.script.mEvents=rows;
        content.IsSummonedBy(nullptr);assert(spoken==std::vector<uint32>{0}&&content.script.pathStarts==1);
        content.SetData(1,1);Advance(content.script,30000);
#ifdef BASELINE
        assert((spoken==std::vector<uint32>{0,2,3}));
        std::cout<<"PASS: native baseline TALK timer is overwritten by early1/1; TEXT_OVER0 and take-up-the-blade line1 disappear\n";
        return 0;
#else
        assert((spoken==std::vector<uint32>{0,1,2,3}));
#endif
    }
#ifndef BASELINE
    for(uint32 delay:{0u,1u,5999u,6000u,6001u,6002u,8000u,14000u,14001u,14002u,14003u,20000u})
        for(uint32 invokerKind:{0u,1u,2u}) {
            scenarioManager={};spoken.clear();Creature actor;npc_stromkar_thoradin content(&actor);actor.brain=&content;
            content.script.mEvents=rows;Player player;Unit pet;
            content.IsSummonedBy(invokerKind==0?nullptr:invokerKind==1?static_cast<Unit*>(&player):&pet);
            Advance(content.script,delay);
            if(delay<6001)assert((spoken==std::vector<uint32>{0}));
            for(uint32 repeat=0;repeat<20;++repeat)content.SetData(1,1);
            content.IsSummonedBy(nullptr);
            content.script.OnReset();
            scenarioManager.scenario.step=10;
            Advance(content.script,60000);
            assert((spoken==std::vector<uint32>{0,1,2,3})&&content.script.pathStarts==1);
            assert(content.returnState.signals==(hasReturn?1u:0u));
            content.script.OnReset();
            content.SetData(1,1);content.SetData(2,1);Advance(content.script,60000);
            assert((spoken==std::vector<uint32>{0,1,2,3}));
            assert(content.returnState.signals==(hasReturn?1u:0u));
        }
    for(uint32 variant=0;variant<6;++variant) {
        scenarioManager={};spoken.clear();Creature actor;
        if(variant==0)actor.entry=103144;
        if(variant==1)actor.map=1;
        if(variant==2)actor.instance=43;
        if(variant==3)scenarioManager.present=false;
        if(variant==4)scenarioManager.scenario.identity=991;
        if(variant==5)scenarioManager.scenario.step=8;
        npc_stromkar_thoradin content(&actor);actor.brain=&content;content.script.mEvents=rows;
        content.IsSummonedBy(nullptr);content.SetData(1,1);Advance(content.script,30000);
        assert((spoken==std::vector<uint32>{0,2}||spoken==std::vector<uint32>{0,2,3}));
    }
    scenarioManager={};spoken.clear();Creature expired;npc_stromkar_thoradin content(&expired);expired.brain=&content;
    content.script.mEvents=rows;content.IsSummonedBy(nullptr);content.SetData(1,1);expired.expired=true;
    Advance(content.script,60000);assert((spoken==std::vector<uint32>{0}));
    std::cout<<"PASS: actual scoped wrapper plus native TALK/TEXT_OVER/DATA_SET/timer/dispatch retains0,1,2,3 for early/boundary/late/duplicate final requests and null/player/pet summon contexts; native reset does not replay a delivered finale and expired quest-object dispatch stops. Scene, allocation, chat transport/audio and client traversal remain separate\n";
#endif
}
