#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include "../../src/server/game/Movement/SkyholdDeparture.h"
using uint32=uint32_t;
using QuestStatus=uint32;
using ObjectGuid=uint32;
using ConditionSourceType=uint32;
enum { CONDITION_AURA=1, CONDITION_ITEM_EQUIPPED=3, CONDITION_QUESTREWARDED=8, CONDITION_QUESTTAKEN=9,
    CONDITION_CLASS=15, CONDITION_MAPID=22, CONDITION_QUEST_COMPLETE=28, CONDITION_ALIVE=36, CONDITION_SCENARION_STEP=44,
    QUEST_STATUS_COMPLETE=1, QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_REWARDED=6,
    CONDITION_SOURCE_TYPE_NONE=0, CONDITION_SOURCE_TYPE_SPELL=17, CONDITION_SOURCE_TYPE_MAX=32, EFFECT_MOTION_TYPE=3 };
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_CASTER_AURASTATE, SPELL_FAILED_BAD_TARGETS, SPELL_FAILED_DONT_REPORT };
#define TC_LOG_DEBUG(...) ((void)0)
struct InstanceMap { uint32 identity=42; uint32 GetInstanceId() const { return identity; } } instance;
struct Map { bool instanced=true; InstanceMap* ToInstanceMap() { return instanced?&instance:nullptr; } } world;
struct Scenario { uint32 identity=1037,step=9; uint32 GetScenarioId() const { return identity; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioMgr { Scenario scenario; bool present=true; Scenario* GetScenario(uint32 identity) { return present&&identity==42?&scenario:nullptr; } } scenarioManager;
auto* sScenarioMgr=&scenarioManager;
struct Player;
struct Creature;
struct Unit {
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    bool IsPlayer() { return ToPlayer()!=nullptr; }
    Creature* ToCreature() { return nullptr; }
    uint32 map=1539,classMask=1,casts=0,effects=0;
    bool alive=true,aura=true;
    SpellCastResult last=SPELL_CAST_OK;
    uint32 getClassMask() const { return classMask; }
    bool HasAuraEffect(uint32 identity,uint32 effect) const { assert(identity==206448&&effect==0);return aura; }
    bool IsCreature() const { return false; }
    bool isPet() const { return false; }
    Unit* GetOwner() const { return nullptr; }
    struct Threat { Unit* getHostilTarget() const { return nullptr; } } threat;
    Threat& getThreatManager() { return threat; }
    Unit* ToUnit() { return this; }
    bool IsAlive() const { return alive; }
    uint32 GetMapId() const { return map; }
    bool InInstance() const { return world.instanced; }
    Map* GetMap() { return &world; }
    SpellCastResult CastSpell(Unit*,uint32,bool);
};
struct Creature : Unit {
    struct Brain { void MovementInform(uint32,uint32) { assert(false); } } brain;
    Brain* AI() { return &brain; }
};
struct Player : Unit {
    uint32 quest=41105,status=QUEST_STATUS_INCOMPLETE,faction=0;
    bool rewarded=false,equipped=true;
    Player* ToPlayer() override { return this; }
    uint32 GetQuestStatus(uint32 identity) const { return identity==quest?status:0; }
    bool GetQuestRewardStatus(uint32 identity) const { return identity==quest&&rewarded; }
    bool HasItemOrGemWithIdEquipped(uint32 identity,uint32 count) const { assert(identity==128910&&count==1);return equipped; }
    struct Session { void SendNotification(uint32) { assert(false); } } session;
    Session* GetSession() { return &session; }
};
using WorldObject=Unit;
struct Condition;
struct ConditionSourceInfo {
    Unit* mConditionTargets[2]; Condition* mLastFailedCondition=nullptr;
    explicit ConditionSourceInfo(Unit* actor):mConditionTargets{actor,nullptr} { }
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
using ConditionTypeContainer=std::map<uint32,ConditionList>;
using ConditionContainer=std::map<ConditionSourceType,ConditionTypeContainer>;
struct ConditionMgr {
    ConditionReferenceContainer ConditionReferenceStore;
    ConditionContainer ConditionStore;
    ConditionList GetConditionsForNotGroupedEntry(ConditionSourceType,uint32);
    bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&) const;
    bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& rows) { return IsObjectMeetToConditionList(info,rows); }
} conditionManager;
auto* sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
#include "NotGroupedConditions.inc"
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
SpellCastResult Unit::CastSpell(Unit* target,uint32 identity,bool triggered) {
    assert(target==this&&identity==206456&&triggered);++casts;
    Spell spell{this};last=spell.Check();
    if(last==SPELL_CAST_OK) {
        ++effects;
        scenarioManager.scenario.step=10;
        aura=false;
    }
    return last;
}
struct ObjectAccessor { static Unit* GetUnit(Unit& owner,ObjectGuid identity) { assert(identity==1);return &owner; } };
struct EffectMovementGenerator {
    uint32 _arrivalSpellId=206456,_pointId=206455,_arrivalMapId=uint32(-1);
    ObjectGuid _arrivalSpellTargetGuid=1;
    bool _arrived=true;
    void Finalize(Unit&);
    void MovementInform(Unit&);
};
#include "Arrival.inc"
void Reset() { scenarioManager={};world={};instance={}; }
int main(int argc,char** argv) {
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);
    std::ifstream input(argv[1]);std::vector<Condition> rows(16);ConditionList conditions;
    for(Condition& row:rows) {
        assert(input>>row.ElseGroup>>row.ConditionType>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2>>row.ConditionValue3>>row.NegativeCondition);
        assert(row.ConditionTarget==0);conditions.push_back(&row);
    }
    EffectMovementGenerator arrival;
    Player baseline;baseline.equipped=false;arrival.Finalize(baseline);
    assert(baseline.effects==1&&scenarioManager.scenario.step==10&&!baseline.aura);
    conditionManager.ConditionStore[17][206456]=conditions;
    for(uint32 faction:{0u,1u})for(uint32 stage=0;stage<=11;++stage)for(uint32 status:{0u,1u,3u,5u,6u}) {
        Reset();Player player;player.faction=faction;player.status=status;scenarioManager.scenario.step=stage;
        arrival.Finalize(player);
        bool allowed=stage==9&&(status==1||status==3);
        assert(player.casts==1&&player.effects==uint32(allowed));
        assert(player.last==(allowed?SPELL_CAST_OK:SPELL_FAILED_CASTER_AURASTATE));
        assert(player.aura==!allowed);
    }
    for(uint32 invalid=0;invalid<12;++invalid) {
        Reset();Player player;
        if(invalid==0)player.map=1;
        if(invalid==1)player.classMask=16;
        if(invalid==2)player.quest=40710;
        if(invalid==3){player.status=QUEST_STATUS_COMPLETE;player.rewarded=true;}
        if(invalid==4)player.equipped=false;
        if(invalid==5)player.alive=false;
        if(invalid==6)player.aura=false;
        if(invalid==7)scenarioManager.present=false;
        if(invalid==8)scenarioManager.scenario.identity=991;
        if(invalid==9)scenarioManager.scenario.identity=1092;
        if(invalid==10)world.instanced=false;
        if(invalid==11)instance.identity=43;
        arrival.Finalize(player);
        bool beforeCast=invalid==0||invalid==5;
        assert(player.effects==0&&player.casts==uint32(!beforeCast));
        assert(player.last==(beforeCast?SPELL_CAST_OK:SPELL_FAILED_CASTER_AURASTATE));
    }
    Reset();Player retry;retry.equipped=false;arrival.Finalize(retry);assert(retry.effects==0&&retry.aura);
    retry.equipped=true;arrival.Finalize(retry);arrival.Finalize(retry);
    assert(retry.casts==3&&retry.effects==1&&scenarioManager.scenario.step==10&&!retry.aura);
    Reset();Unit pet;arrival.Finalize(pet);assert(pet.effects==0&&pet.last==SPELL_FAILED_CASTER_AURASTATE);
    assert(conditionManager.GetConditionsForNotGroupedEntry(17,206455).empty());
    assert(conditionManager.GetConditionsForNotGroupedEntry(18,206456).empty());
    std::cout<<"PASS: native EffectMovementGenerator arrival uses triggered206456/self target; actual source17 lookup/cast branch and16 native conditions reject changed quest/map/stage/class/equipment/life/bar. Retry and duplicate gating use a controlled stage/aura effect sink, not real scenario credit, movement or client proof.\n";
}
