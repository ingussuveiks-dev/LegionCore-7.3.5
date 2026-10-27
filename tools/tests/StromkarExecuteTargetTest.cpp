#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <map>
#include <vector>
#include <iostream>
using uint32 = uint32_t;
using uint8 = uint8_t;
using int32 = int32_t;
using SpellEffIndex = uint32;
using QuestStatus = uint32;
#define ASSERT(value) assert(value)
#define TC_LOG_DEBUG(...) ((void)0)
enum ComparisionType { COMP_TYPE_EQ, COMP_TYPE_HIGH, COMP_TYPE_LOW, COMP_TYPE_HIGH_EQ, COMP_TYPE_LOW_EQ, COMP_TYPE_MAX };
#include "Compare.inc"
#include "MaskEnums.inc"
enum { QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1, QUEST_STATUS_REWARDED=6 };
struct InstanceMap { uint32 identity=42; uint32 GetInstanceId() const { return identity; } } instance;
struct Map { bool instanced=true; InstanceMap* ToInstanceMap() { return instanced?&instance:nullptr; } } world;
struct Scenario { uint32 identity=1037,step=9; uint32 GetScenarioId() const { return identity; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioMgr { Scenario scenario; bool present=true; Scenario* GetScenario(uint32 identity) { return present&&identity==42?&scenario:nullptr; } } scenarioManager;
auto* sScenarioMgr=&scenarioManager;
struct GameObject {};
struct Player;
struct Unit {
    uint32 type=3,entry=104276,map=1539;
    float hp=1,distance=10;
    bool alive=true,visible=true,phase=true;
    uint32 classMask=1;
    bool aura=true;
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    uint32 getClassMask() const { return classMask; }
    bool HasAuraEffect(uint32 identity,uint32 effect) const { assert(identity==206448&&effect==0); return aura; }
    bool IsCreature() const { return type==3; }
    bool isPet() const { return false; }
    Unit* GetOwner() const { return nullptr; }
    struct Threat { Unit* getHostilTarget() const { return nullptr; } } threat;
    Threat& getThreatManager() { return threat; }
    Unit* ToUnit() { return this; }
    GameObject* ToGameObject() { return nullptr; }
    bool isDead() const { return !alive; }
    bool IsAlive() const { return alive; }
    float GetHealthPct() const { return hp; }
    uint32 GetTypeId() const { return type; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return map; }
    bool InInstance() const { return world.instanced; }
    Map* GetMap() { return &world; }
};
struct Player : Unit {
    uint32 quest=41105,status=3;
    bool rewarded=false,equipped=true;
    Player() { type=4;entry=0;distance=0; }
    Player* ToPlayer() override { return this; }
    uint32 GetQuestStatus(uint32 identity) const { return identity==quest?status:0; }
    bool GetQuestRewardStatus(uint32 identity) const { return identity==quest&&rewarded; }
    bool HasItemOrGemWithIdEquipped(uint32 identity,uint32 count) const { assert(identity==128910&&count==1);return equipped; }
};
using WorldObject=Unit;
struct ConditionSourceInfo { Unit* objects[2]; ConditionSourceInfo(Unit* target,Unit* caster):objects{target,caster} {} };
struct Condition {
    uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;
    bool NegativeCondition=false;
    bool isLoaded() const { return true; }
    uint32 GetSearcherTypeMaskForCondition();
    bool Meets(ConditionSourceInfo& sourceInfo) {
        Unit* object=sourceInfo.objects[ConditionTarget]; if(!object) return false;
        bool condMeets=false;
        switch(ConditionType) {
#include "Conditions.inc"
            default: assert(false);
        }
        return NegativeCondition?!condMeets:condMeets;
    }
};
using ConditionList=std::vector<Condition*>;
using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {
    ConditionReferenceContainer ConditionReferenceStore;
    bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&) const;
    uint32 GetSearcherTypeMaskForConditionList(ConditionList const&);
} conditionManager;
#include "ElseGroups.inc"
#include "Mask.inc"
auto* sConditionMgr=&conditionManager;
using SpellTargetObjectTypes=uint32;
enum { TARGET_REFERENCE_TYPE_CASTER,TARGET_REFERENCE_TYPE_DEST,TARGET_CHECK_ENEMY,TARGET_CHECK_ALLY,TARGET_CHECK_PARTY,TARGET_CHECK_RAID,TARGET_CHECK_RAID_CLASS,TARGET_CHECK_SUMMON,TARGET_CHECK_ENTRY,TARGET_CHECK_DEFAULT,
    TARGET_OBJECT_TYPE_UNIT,TARGET_OBJECT_TYPE_GOBJ,TARGET_OBJECT_TYPE_DEST,TARGET_OBJECT_TYPE_OBJ_AND_DEST,TARGET_OBJECT_TYPE_CORPSE,
    TARGET_OBJECT_TYPE_UNIT_AND_DEST,TARGET_OBJECT_TYPE_CORPSE_ENEMY,TARGET_OBJECT_TYPE_CORPSE_ALLY,TARGET_OBJECT_TYPE_PLAYER,TARGET_OBJECT_TYPE_GOBJ_ITEM,
    SPELL_ATTR2_CAN_TARGET_DEAD,SPELL_ATTR3_ONLY_TARGET_PLAYERS,SPELL_EFFECT_TELEPORT_L,SPELL_EFFECT_TELEPORT,SPELL_FAILED_BAD_IMPLICIT_TARGETS };
struct SpellImplicitTargetInfo {
    uint32 GetReferenceType() const { return TARGET_REFERENCE_TYPE_CASTER; }
    uint32 GetCheckType() const { return TARGET_CHECK_ENTRY; }
    uint32 GetObjectType() const { return TARGET_OBJECT_TYPE_DEST; }
};
struct SpellTargetPosition { float target_X=0,target_Y=0,target_Z=0,target_Orientation=0; uint32 target_mapId=1539; };
struct SpellMgr { SpellTargetPosition const* GetSpellTargetPosition(uint32) const { return nullptr; } } spellManager;
auto* sSpellMgr=&spellManager;
struct Spell;
struct SpellInfo {
    uint32 Id=206455;
    struct { bool RequiresSpellFocus=false; } CastingReq;
    struct Effect { ConditionList* ImplicitTargetConditions=nullptr; } effect;
    Effect* Effects[1]{&effect};
    bool HasEffect(uint32) const { return false; }
    bool IsPositive() const { return true; }
    bool HasAttribute(uint32) const { return false; }
    bool IsRequiringDeadTarget() const { return false; }
    float GetMaxRange(bool,Unit*,Spell*) const { return 40; }
};
struct Targets {
    Unit* referer=nullptr;
    Unit* destination=nullptr;
    Unit* GetUnitTarget() const { return referer; }
    void SetUnitTarget(Unit* actor) { referer=actor; }
    void SetDst(Unit& actor) { destination=&actor; }
    void SetDst(float,float,float,float=0,int32=0) { assert(false); }
};
struct Spell {
    Unit* m_caster;
    Unit* focusObject=nullptr;
    SpellInfo* m_spellInfo;
    Targets m_targets;
    std::vector<Unit*> candidates;
    uint32 searches=0,errors=0,failed=0,chains=0;
    void AddGOTarget(GameObject*,uint32) { assert(false); }
    void AddGOTarget(Unit*,uint32) { assert(false); }
    void AddUnitTarget(Unit*,uint32,bool,bool) { assert(false); }
    void SendCastResult(uint32 value) { assert(value==SPELL_FAILED_BAD_IMPLICIT_TARGETS); ++errors; }
    void finish(bool value) { assert(!value); ++failed; }
    void CallScriptObjectTargetSelectHandlers(WorldObject*&,SpellEffIndex) { }
    void SelectImplicitChainTargets(SpellEffIndex,SpellImplicitTargetInfo const&,WorldObject*,uint32) { ++chains; }
    WorldObject* SearchNearbyTarget(float range,uint32,uint32,ConditionList* conditions) {
        ++searches; Unit* selected=nullptr;
        // Apply the actual native condition container mask before evaluating candidates.
        // This is the grid-admission step omitted by the original fixture.
        uint32 mask=GetSearcherTypeMask(TARGET_OBJECT_TYPE_DEST,conditions);
        for(Unit* actor:candidates) {
            uint32 actorMask=actor->ToPlayer()?GRID_MAP_TYPE_MASK_PLAYER:GRID_MAP_TYPE_MASK_CREATURE;
            if(!(mask&actorMask)) continue;
            ConditionSourceInfo context(actor,m_caster);
            if(actor->visible&&actor->phase&&actor->distance<range&&conditions&&conditionManager.IsObjectMeetToConditionList(context,*conditions)) {
                selected=actor; range=actor->distance;
            }
        }
        return selected;
    }
    void SelectImplicitNearbyTargets(SpellEffIndex,SpellImplicitTargetInfo const&,uint32);
    uint32 GetSearcherTypeMask(SpellTargetObjectTypes,ConditionList*);
    void Resolve() { SpellImplicitTargetInfo target; SelectImplicitNearbyTargets(0,target,1); }
};
#include "NearbySelector.inc"
#include "SpellMask.inc"
int main(int argc,char** argv) {
    _set_abort_behavior(0,_CALL_REPORTFAULT); assert(argc==2);
    std::ifstream input(argv[1]); std::vector<Condition> rows(22); ConditionList conditions;
    for(Condition& row:rows) {
        assert(input>>row.ElseGroup>>row.ConditionType>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2>>row.ConditionValue3>>row.NegativeCondition);
        assert(row.ConditionTarget<2); conditions.push_back(&row);
    }
    Player caster;
    Unit boss,decoy; decoy.entry=123; decoy.distance=1;
    SpellInfo native; Spell baseline{&caster,nullptr,&native,{&caster}}; baseline.candidates={&decoy,&boss}; baseline.Resolve();
    assert(baseline.m_targets.destination==&caster&&baseline.searches==0);
    native.effect.ImplicitTargetConditions=&conditions;
#ifdef MASK_BASELINE
    // Old rows can mask the missing scenario-mask case by reaching zero first.
    std::stable_sort(conditions.begin(),conditions.end(),[](Condition* a,Condition* b){return a->ConditionType==CONDITION_ITEM_EQUIPPED&&b->ConditionType!=CONDITION_ITEM_EQUIPPED;});
    uint32 mask=conditionManager.GetSearcherTypeMaskForConditionList(conditions);
    assert(mask==0);
    Spell blocked{&caster,nullptr,&native,{&caster}};blocked.candidates={&boss};blocked.Resolve();
    assert(!blocked.m_targets.destination&&blocked.failed==1);
    std::cout<<"EXPECTED COUNTERFACTUAL: original native22-row mask is zero and rejects the valid equipped caster's living Zakajz before predicate evaluation\n";
    return 0;
#else
    uint32 mask=conditionManager.GetSearcherTypeMaskForConditionList(conditions);
    assert(mask==GRID_MAP_TYPE_MASK_CREATURE);
    // References keep each leaf's real target index; OR groups retain alternatives.
    Condition reference;reference.ReferenceId=100;
    conditionManager.ConditionReferenceStore[100]=conditions;
    assert(conditionManager.GetSearcherTypeMaskForConditionList({&reference})==mask);
    Condition playerOnly;playerOnly.ConditionType=CONDITION_ITEM_EQUIPPED;
    assert(playerOnly.GetSearcherTypeMaskForCondition()==GRID_MAP_TYPE_MASK_PLAYER);
    playerOnly.ConditionTarget=1;
    assert(playerOnly.GetSearcherTypeMaskForCondition()==GRID_MAP_TYPE_MASK_ALL);
    playerOnly.ConditionTarget=2;
    assert(playerOnly.GetSearcherTypeMaskForCondition()==GRID_MAP_TYPE_MASK_ALL);
    playerOnly.ConditionTarget=0;playerOnly.NegativeCondition=true;
    assert(playerOnly.GetSearcherTypeMaskForCondition()==GRID_MAP_TYPE_MASK_ALL);
    playerOnly.NegativeCondition=false;playerOnly.ElseGroup=2;
    assert(conditionManager.GetSearcherTypeMaskForConditionList({&reference,&playerOnly})==(mask|GRID_MAP_TYPE_MASK_PLAYER));
    assert(conditionManager.GetSearcherTypeMaskForConditionList({})==GRID_MAP_TYPE_MASK_ALL);
    for(uint32 stage=0;stage<=11;++stage) for(float hp:{0.001f,1.0f,1.001f,100.0f}) for(uint32 status:{0u,1u,3u,5u,6u}) {
        scenarioManager={}; scenarioManager.scenario.step=stage; boss.hp=hp;
        caster.status=status;
        Spell cast{&caster,nullptr,&native,{&caster}}; cast.candidates={&decoy,&boss}; cast.Resolve();
        bool allowed=stage==9&&hp<=1&&(status==1||status==3);
        assert(cast.m_targets.destination==(allowed?&boss:nullptr)&&cast.errors==uint32(!allowed)&&cast.failed==uint32(!allowed));
        assert(cast.searches==1&&cast.chains==uint32(allowed));
    }
    for(uint32 invalid=0;invalid<10;++invalid) {
        scenarioManager={};world={};instance={};boss={};caster={};
        if(invalid==0) boss.alive=false;
        if(invalid==1) boss.map=1;
        if(invalid==2) scenarioManager.scenario.identity=991;
        if(invalid==3) scenarioManager.scenario.identity=1092;
        if(invalid==4) scenarioManager.present=false;
        if(invalid==5) world.instanced=false;
        if(invalid==6) instance.identity=43;
        if(invalid==7) boss.visible=false;
        if(invalid==8) boss.phase=false;
        if(invalid==9) boss.distance=41;
        Spell cast{&caster,nullptr,&native,{&caster}};cast.candidates={&caster,&decoy,&boss};cast.Resolve();
        assert(!cast.m_targets.destination&&cast.errors==1&&cast.failed==1);
    }
    for(uint32 invalid=0;invalid<6;++invalid) {
        scenarioManager={};world={};instance={};boss={};caster={};
        if(invalid==0) caster.classMask=16;
        if(invalid==1) caster.quest=40710;
        if(invalid==2) { caster.rewarded=true;caster.status=QUEST_STATUS_COMPLETE; }
        if(invalid==3) caster.equipped=false;
        if(invalid==4) caster.aura=false;
        if(invalid==5) caster.alive=false;
        Spell cast{&caster,nullptr,&native,{&caster}};cast.candidates={&boss};cast.Resolve();
        if(cast.m_targets.destination||cast.failed!=1) std::cerr<<"Unexpected caster admission case "<<invalid<<'\n';
        assert(!cast.m_targets.destination&&cast.failed==1);
    }
    scenarioManager={};world={};instance={};boss={};caster={};
    Unit pet; Spell petCast{&pet,nullptr,&native,{&pet}};petCast.candidates={&boss};petCast.Resolve();assert(petCast.failed==1);
    Spell missing{&caster,nullptr,&native,{&caster}}; missing.Resolve(); assert(!missing.m_targets.destination&&missing.failed==1);
    Spell retry{&caster,nullptr,&native,{&caster}}; retry.candidates={&boss};retry.Resolve();assert(retry.m_targets.destination==&boss);
    std::cout<<"PASS: actual native nearby selector reproduces self fallback without conditions; actual22 target/caster rules select living104276/1539/1037/9/HP<=1 only for active living equipped Warrior41105 with native bar. Failed-target diagnostics/retry retained; grid/range/visibility and spell effects are controlled, not client jump/credit\n";
#endif
}
