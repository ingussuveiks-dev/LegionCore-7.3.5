#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
using uint32=uint32_t;
using int32=int32_t;
using QuestStatus=uint32;
using TempSummonType=uint32;
#define ASSERT(value) assert(value)
#define TC_LOG_DEBUG(...) ((void)0)
enum ComparisionType { COMP_TYPE_EQ,COMP_TYPE_HIGH,COMP_TYPE_LOW,COMP_TYPE_HIGH_EQ,COMP_TYPE_LOW_EQ,COMP_TYPE_MAX };
#include "Compare.inc"
enum { CONDITION_AURA=1,CONDITION_ITEM_EQUIPPED=3,CONDITION_QUESTREWARDED=8,CONDITION_QUESTTAKEN=9,CONDITION_CLASS=15,
    CONDITION_MAPID=22,CONDITION_QUEST_COMPLETE=28,CONDITION_NEAR_CREATURE=29,CONDITION_OBJECT_ENTRY=31,CONDITION_ALIVE=36,CONDITION_HP_PCT=38,CONDITION_SCENARION_STEP=44,
    QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_REWARDED=6,
    SMART_EVENT_LINK=61,SMART_EVENT_UPDATE=60,SMART_EVENT_FLAG_EVENT_NON_COMBAT=0x40,SMART_EVENT_FLAG_NOT_REPEATABLE=1,SMART_TARGET_POSITION=8 };
enum SpellCastResult { SPELL_CAST_OK,SPELL_FAILED_CASTER_AURASTATE,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_DONT_REPORT };
struct InstanceMap { uint32 identity=42;uint32 GetInstanceId() const { return identity; } } instance;
struct Map { bool instanced=true;InstanceMap* ToInstanceMap() { return instanced?&instance:nullptr; } } world;
struct Scenario { uint32 identity=1037,step=9;uint32 GetScenarioId() const { return identity; }uint32 GetCurrentStep() const { return step; } };
struct ScenarioMgr { Scenario scenario;bool present=true;Scenario* GetScenario(uint32 identity) { return present&&identity==42?&scenario:nullptr; } } scenarioManager;
auto* sScenarioMgr=&scenarioManager;
struct Player;
struct Creature;
struct Unit {
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    uint32 type=3,entry=104276,map=1539,classMask=1;
    bool alive=true,aura=true,combat=false;
    float hp=1;
    uint32 getClassMask() const { return classMask; }
    bool HasAuraEffect(uint32 identity,uint32 effect) const { assert(identity==206448&&effect==0);return aura; }
    bool IsCreature() const { return type==3; }
    bool isPet() const { return false; }
    Unit* GetOwner() const { return nullptr; }
    struct Threat { Unit* getHostilTarget() const { return nullptr; } } threat;
    Threat& getThreatManager() { return threat; }
    Unit* ToUnit() { return this; }
    bool IsAlive() const { return alive; }
    float GetHealthPct() const { return hp; }
    uint32 GetTypeId() const { return type; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return map; }
    bool InInstance() const { return world.instanced; }
    Map* GetMap() { return &world; }
    bool isInCombat() const { return combat; }
    void GetPosition(float& positionX,float& positionY) const { positionX=1883.61f;positionY=2181.69f; }
    float GetPositionZ() const { return 35.8446f; }
    Creature* SummonCreature(uint32,float,float,float,float,TempSummonType,uint32);
};
using WorldObject=Unit;
struct Creature : Unit {
    struct Brain { void AttackStart(Unit*) { assert(false); } } brain;
    Brain* AI() { return &brain; }
};
struct Player : Unit {
    uint32 quest=41105,status=QUEST_STATUS_INCOMPLETE;
    bool rewarded=false,equipped=true;
    Player() { type=4;entry=0; }
    Player* ToPlayer() override { return this; }
    uint32 GetQuestStatus(uint32 identity) const { return identity==quest?status:0; }
    bool GetQuestRewardStatus(uint32 identity) const { return identity==quest&&rewarded; }
    bool HasItemOrGemWithIdEquipped(uint32 identity,uint32 count) const { assert(identity==128910&&count==1);return equipped; }
    struct Session { void SendNotification(uint32) { assert(false); } } session;
    Session* GetSession() { return &session; }
};
Creature thoradin;
bool actorPresent=false,actorInserted=true,actorPhase=true,summonAvailable=true;
float actorDistance=13;
uint32 summonAttempts=0,summons=0;
Creature* GetClosestCreatureWithEntry(Unit* source,uint32 entry,float range) {
    assert(source&&entry==104307&&range==50);
    return actorPresent&&actorInserted&&actorPhase&&thoradin.alive&&actorDistance<=range?&thoradin:nullptr;
}
Creature* Unit::SummonCreature(uint32 identity,float positionX,float positionY,float positionZ,float orientation,TempSummonType summonType,uint32 duration) {
    assert(identity==104307&&summonType==8&&duration==0&&orientation==0);
    assert(std::abs(positionX-1886.91f)<0.001f&&std::abs(positionY-2194.33f)<0.001f&&std::abs(positionZ-37.1166f)<0.001f);
    ++summonAttempts;if(!summonAvailable)return nullptr;
    ++summons;actorPresent=true;thoradin.alive=true;return &thoradin;
}
struct Condition;
struct ConditionSourceInfo {
    Unit* mConditionTargets[2];Condition* mLastFailedCondition=nullptr;
    explicit ConditionSourceInfo(Unit* first,Unit* second=nullptr):mConditionTargets{first,second} { }
};
struct Condition {
    uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0,ErrorTextId=0;
    bool NegativeCondition=false;
    bool isLoaded() const { return true; }
    bool Meets(ConditionSourceInfo& sourceInfo) {
        Unit* object=sourceInfo.mConditionTargets[ConditionTarget];if(!object)return false;
        bool condMeets=false;
        switch(ConditionType) {
#include "Conditions.inc"
            default: assert(false);
        }
        if(NegativeCondition)condMeets=!condMeets;
        if(!condMeets)sourceInfo.mLastFailedCondition=this;
        return condMeets;
    }
};
using ConditionList=std::vector<Condition*>;
using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {
    ConditionReferenceContainer ConditionReferenceStore;
    ConditionList root,cast;
    bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&) const;
    bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& rows) { return IsObjectMeetToConditionList(info,rows); }
    ConditionList GetConditionsForSmartEvent(uint32 entry,uint32 identity,uint32 source) { assert(entry==104276&&identity==9&&source==0);return root; }
    ConditionList GetConditionsForNotGroupedEntry(uint32 source,uint32 spell) { assert(source==17&&spell==206456);return cast; }
} conditionManager;
auto* sConditionMgr=&conditionManager;
constexpr uint32 CONDITION_SOURCE_TYPE_SPELL=17;
#include "ElseGroups.inc"
struct SpellInfo { uint32 Id=206456; } spellInfo;
struct Spell {
    Unit* m_caster;
    SpellInfo* m_spellInfo=&spellInfo;
    struct Targets { Unit* GetObjectTarget() const { return nullptr; } } m_targets;
    SpellCastResult Check() {
#include "CastConditions.inc"
        return SPELL_CAST_OK;
    }
};
struct GameObject { };
struct SmartScriptHolder {
    uint32 entryOrGuid=104276,source_type=0,event_id=9,link=0,type=60,actionType=12,targetType=8,timer=0;
    bool active=true,runOnce=false;
    struct Event { uint32 event_phase_mask=0,event_flags=0;struct { uint32 min=0,max=0,repeatMin=500,repeatMax=500; } minMaxRepeat; } event;
    struct Action { struct { uint32 creature=104307,type=8,duration=0,storageID=0,attackInvoker=0; } summonCreature; } action;
    struct Target { float x=1886.91f,y=2194.33f,z=37.1166f,o=0; } target;
    uint32 GetEventType() const { return type; }
    uint32 GetTargetType() const { return targetType; }
};
uint32 urand(uint32 minimum,uint32 maximum) { assert(minimum==maximum);return minimum; }
struct SmartScript {
    Creature* me;
    bool IsInPhase(uint32) const { assert(false);return false; }
    Unit* GetBaseObject() { return me; }
    void RecalcTimer(SmartScriptHolder&,uint32,uint32);
    void ProcessAction(SmartScriptHolder& event,Unit* unit=nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* = nullptr,GameObject* = nullptr) {
#define e event
#include "FixedCreatureSummon.inc"
#undef e
    }
    void ProcessEvent(SmartScriptHolder&,Unit* = nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* = nullptr,GameObject* = nullptr);
};
#include "RecalcTimer.inc"
#include "UpdateEvent.inc"
void Reset() {
    scenarioManager={};world={};instance={};thoradin={};thoradin.entry=104307;
    actorPresent=false;actorInserted=true;actorPhase=true;summonAvailable=true;actorDistance=13;summonAttempts=0;summons=0;
}
void Tick(SmartScript& script,SmartScriptHolder& row,uint32 elapsed) {
    if(row.timer>=elapsed){row.timer-=elapsed;return;}
    row.active=true;script.ProcessEvent(row);
}
int main(int argc,char** argv) {
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);
    std::ifstream events(argv[1]);SmartScriptHolder definition;
    assert(events>>definition.entryOrGuid>>definition.source_type>>definition.event_id>>definition.link>>definition.type>>definition.event.event_phase_mask>>definition.event.event_flags
        >>definition.event.minMaxRepeat.min>>definition.event.minMaxRepeat.max>>definition.event.minMaxRepeat.repeatMin>>definition.event.minMaxRepeat.repeatMax>>definition.actionType
        >>definition.action.summonCreature.creature>>definition.action.summonCreature.type>>definition.action.summonCreature.duration>>definition.targetType>>definition.target.x>>definition.target.y>>definition.target.z>>definition.target.o);
    assert(definition.entryOrGuid==104276&&definition.source_type==0&&definition.event_id==9&&definition.type==60&&definition.actionType==12&&!definition.event.event_flags&&!definition.event.event_phase_mask&&!definition.link);
    std::vector<Condition> roots(6),casts(18);
    for(uint32 kind=0;kind<2;++kind) {
        std::ifstream input(argv[kind+2]);auto& rows=kind?casts:roots;auto& list=kind?conditionManager.cast:conditionManager.root;
        for(auto& row:rows) {
            assert(input>>row.ElseGroup>>row.ConditionType>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2>>row.ConditionValue3>>row.NegativeCondition);
            assert(row.ConditionTarget==(kind?0u:1u));list.push_back(&row);
        }
    }
    Reset();Player baseline;auto completeRules=conditionManager.cast;
    conditionManager.cast.erase(std::remove_if(conditionManager.cast.begin(),conditionManager.cast.end(),[](Condition* row){return row->ConditionType==29;}),conditionManager.cast.end());
    assert(conditionManager.cast.size()==16&&Spell{&baseline}.Check()==SPELL_CAST_OK&&!actorPresent);
    conditionManager.cast=completeRules;
    std::cout<<"BASELINE: actual sixteen pre51 source17 rules admit the final cast without its missing104307 dialogue receiver\n";
    for(uint32 stage=0;stage<=11;++stage)for(float hp:{0.001f,1.0f,1.001f,100.0f}) {
        Reset();Creature boss;boss.hp=hp;scenarioManager.scenario.step=stage;SmartScript script{&boss};auto row=definition;
        Tick(script,row,1);assert(summons==uint32(stage==9&&hp<=1));
    }
    Reset();Creature boss;SmartScript script{&boss};auto row=definition;Player player;
    Spell missing{&player};assert(missing.Check()==SPELL_FAILED_CASTER_AURASTATE&&player.aura);
    summonAvailable=false;for(uint32 retry=0;retry<20;++retry)Tick(script,row,501);
    assert(summonAttempts==20&&summons==0&&missing.Check()==SPELL_FAILED_CASTER_AURASTATE&&player.aura);
    summonAvailable=true;Tick(script,row,501);assert(summons==1&&missing.Check()==SPELL_CAST_OK);
    for(uint32 repeat=0;repeat<20;++repeat)Tick(script,row,501);
    assert(summonAttempts==21&&summons==1);
    actorPresent=false;Tick(script,row,501);assert(summons==2&&missing.Check()==SPELL_CAST_OK);
    for(uint32 invalid=0;invalid<8;++invalid) {
        Reset();actorPresent=true;Player changed;
        if(invalid==0)changed.classMask=16;
        if(invalid==1)changed.quest=40710;
        if(invalid==2)changed.status=0;
        if(invalid==3){changed.status=QUEST_STATUS_COMPLETE;changed.rewarded=true;}
        if(invalid==4)changed.equipped=false;
        if(invalid==5)changed.aura=false;
        if(invalid==6)changed.alive=false;
        if(invalid==7)changed.map=1;
        assert(Spell{&changed}.Check()==SPELL_FAILED_CASTER_AURASTATE);
    }
    for(uint32 invalid=0;invalid<6;++invalid) {
        Reset();Creature scopedBoss;SmartScript scoped{&scopedBoss};auto scopedRow=definition;
        if(invalid==0)scopedBoss.alive=false;
        if(invalid==1)scopedBoss.map=1;
        if(invalid==2)scopedBoss.entry=102039;
        if(invalid==3)scenarioManager.present=false;
        if(invalid==4)scenarioManager.scenario.identity=991;
        if(invalid==5)world.instanced=false;
        Tick(scoped,scopedRow,1);assert(!summonAttempts);
    }
    for(uint32 absent=0;absent<4;++absent) {
        Reset();actorPresent=true;
        if(absent==0)actorInserted=false;
        if(absent==1)actorPhase=false;
        if(absent==2)thoradin.alive=false;
        if(absent==3)actorDistance=51;
        Spell blocked{&player};assert(blocked.Check()==SPELL_FAILED_CASTER_AURASTATE);
    }
    Reset();actorInserted=false;SmartScript uninserted{&boss};auto uninsertedRow=definition;Tick(uninserted,uninsertedRow,1);
    assert(summons==1&&missing.Check()==SPELL_FAILED_CASTER_AURASTATE);Tick(uninserted,uninsertedRow,501);assert(summonAttempts==2);
    actorInserted=true;assert(missing.Check()==SPELL_CAST_OK);
    Reset();Unit pet;SmartScript petContext{&boss};auto petRow=definition;petContext.ProcessEvent(petRow,&pet);
    assert(summons==1&&Spell{&pet}.Check()==SPELL_FAILED_CASTER_AURASTATE);
    Reset();actorPresent=true;scenarioManager.scenario.step=10;SmartScript finished{&boss};auto finishedRow=definition;Tick(finished,finishedRow,1);assert(!summonAttempts);
    std::cout<<"PASS: actual source22 base-actor conditions, native UPDATE/recalc/fixed summon and full18 source17 predicates retry missing104307 only in1539/1037/9 at HP<=1; no early spawn, duplicates or Execute before a live searchable receiver. Tick/grid/map insertion/cast/effect/actor callbacks are controlled, not client or dialogue/cleanup proof.\n";
}
