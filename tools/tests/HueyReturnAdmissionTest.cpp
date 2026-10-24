#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using uint32 = uint32_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using int8 = int8_t;
using ObjectGuid = uint32;
using QuestStatus = uint32;
enum { CONDITION_ITEM=2, CONDITION_QUESTREWARDED=8, CONDITION_QUESTTAKEN=9, CONDITION_CLASS=15, CONDITION_MAPID=22,
    CONDITION_QUEST_COMPLETE=28, CONDITION_ALIVE=36, CONDITION_SCENARION_STEP=44,
    QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1, QUEST_STATUS_REWARDED=6,
    UNIT_FIELD_NPC_FLAGS=0, UNIT_NPC_FLAG_SPELLCLICK=0x1000000,
    NPC_CLICK_CAST_CASTER_CLICKER=1, NPC_CLICK_CAST_TARGET_CLICKER=2, NPC_CLICK_CAST_ORIG_CASTER_OWNER=4,
    MAX_SPELL_EFFECTS=32, SPELL_AURA_CONTROL_VEHICLE=236, SPELLVALUE_BASE_POINT0=0, MAX_EFFECT_MASK=0xffffffff };
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
#define ASSERT(value) assert(value)
uint32 SpellValueMod(uint32 value) { return value; }
struct Player;
struct Creature;
struct SpellInfo { uint32 EffectMask=0; struct Effect { uint32 ApplyAuraName=0; } effect; Effect const* GetEffect(uint32,uint32) const { return &effect; } };
struct InstanceMap { uint32 identity=42; uint32 GetInstanceId() const { return identity; } };
struct Map { bool instanced=true; InstanceMap instance; InstanceMap* ToInstanceMap() { return instanced ? &instance : nullptr; } } finalMap, outdoorMap;
struct Scenario { uint32 identity=1099,step=5; uint32 GetScenarioId() const { return identity; } uint32 GetCurrentStep() const { return step; } };
struct ScenarioMgr { bool present=true; Scenario scenario; Scenario* GetScenario(uint32 identity) { return present && identity==42 ? &scenario : nullptr; } } scenarios;
auto* sScenarioMgr=&scenarios;
struct Unit {
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual Creature* ToCreature() { return nullptr; }
    Unit* ToUnit() { return this; }
    uint32 entry=0, mapId=1609,classMask=4,casts=0;
    bool alive=true;
    Map* map=&finalMap;
    struct Vehicle { uint32 GetCreatureEntry() const { return 106854; } };
    Vehicle* GetVehicleKit() { return nullptr; }
    uint32 GetEntry() const { return entry; }
    bool IsCreature() { return ToCreature()!=nullptr; }
    bool isPet() const { return false; }
    Unit* GetOwner() const { return nullptr; }
    struct Threat { Unit* getHostilTarget() const { return nullptr; } } threat;
    Threat& getThreatManager() { return threat; }
    uint32 getClassMask() const { return classMask; }
    uint32 GetMapId() const { return mapId; }
    Map* GetMap() const { return map; }
    bool InInstance() const { return map->instanced; }
    bool IsAlive() const { return alive; }
    ObjectGuid GetGUID() const { return 17; }
    ObjectGuid GetOwnerGUID() const { return 0; }
    uint32 GetSpawnMode() const { return 0; }
    bool IsInMap(Unit* other) const { return map==other->map; }
    bool HandleSpellClick(Unit*,int8=-1);
    void CastSpell(Unit*,SpellInfo const*,bool,void*,void*,ObjectGuid) { ++casts; }
    void CastCustomSpell(uint32,uint32,int,Unit*,bool,void*,void*,ObjectGuid) { assert(false); }
};
struct OutdoorPvP { void HandleSpellClick(Player*,Unit*) { assert(false); } };
struct Player : Unit {
    std::map<uint32,uint32> quests{{41574,6},{42185,3}};
    bool owned=true,bank=false,rewarded=false;
    Player* ToPlayer() override { return this; }
    uint32 GetQuestStatus(uint32 identity) const { if(identity==42185 && rewarded) return 6; auto found=quests.find(identity); return found==quests.end()?0:found->second; }
    bool GetQuestRewardStatus(uint32 identity) const { return GetQuestStatus(identity)==6 || (identity==42185 && rewarded); }
    bool HasItemCount(uint32 identity,uint32 amount,bool includeBank) const { assert(identity==128861 && amount==1); return owned || (includeBank && bank); }
    OutdoorPvP* GetOutdoorPvP() { return nullptr; }
    bool canSeeSpellClickOn(Creature const*) const;
};
struct Aura { template<class... Values> static Aura* TryRefreshStackOrCreate(Values...) { assert(false); return nullptr; } void ApplyForTargets() { assert(false); } };
struct Creature : Unit {
    struct Delegate { uint32 calls=0; Unit* invoker=nullptr; void OnSpellClick(Unit* unit) { ++calls; invoker=unit; } } ai;
    bool IsAIEnabled=true,clickFlag=true;
    Creature() { entry=106854; }
    Creature* ToCreature() override { return this; }
    bool HasFlag(uint32,uint32 flag) const { assert(flag==UNIT_NPC_FLAG_SPELLCLICK); return clickFlag; }
    Delegate* AI() { return &ai; }
};
using WorldObject=Unit;
struct ConditionSourceInfo { Unit* targets[2]; ConditionSourceInfo(Unit* first,Unit* second=nullptr):targets{first,second} {} };
struct Condition {
    uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;
    bool NegativeCondition=false;
    bool isLoaded() const { return true; }
    bool Meets(ConditionSourceInfo& sourceInfo) {
        Unit* object=sourceInfo.targets[ConditionTarget];
        if (!object) return false;
        bool condMeets=false;
        switch(ConditionType) {
#include "Conditions.inc"
            default: assert(false);
        }
        return NegativeCondition ? !condMeets : condMeets;
    }
};
using ConditionList=std::vector<Condition*>;
using ConditionReferenceContainer=std::map<uint32,ConditionList>;
using ConditionTypeContainer=std::map<uint32,ConditionList>;
using CreatureSpellConditionContainer=std::map<uint32,ConditionTypeContainer>;
using SmartEventConditionContainer=std::map<std::pair<int32,uint32>,ConditionTypeContainer>;
struct ConditionMgr {
    ConditionReferenceContainer ConditionReferenceStore;
    CreatureSpellConditionContainer SpellClickEventConditionStore;
    SmartEventConditionContainer SmartEventConditionStore;
    ConditionList GetConditionsForSpellClickEvent(uint32,uint32);
    bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&) const;
    bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*) const;
    bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& conditions) const { return conditions.empty() || IsObjectMeetToConditionList(info,conditions); }
} conditionManager;
auto* sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
#include "ClickConditions.inc"
#include "SmartEventConditions.inc"
struct SpellClickInfo { uint32 spellId=212060,castFlags=1; bool IsFitToRequirements(Unit const*,Unit const*) const { return true; } };
using SpellClickInfoContainer=std::multimap<uint32,SpellClickInfo>;
using SpellClickInfoMapBounds=std::pair<SpellClickInfoContainer::const_iterator,SpellClickInfoContainer::const_iterator>;
struct ObjectMgr { SpellClickInfoContainer clicks{{106854,{}}}; SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 identity) { return clicks.equal_range(identity); } } templates;
auto* sObjectMgr=&templates;
struct SpellMgr { SpellInfo spell; SpellInfo const* GetSpellInfo(uint32 identity) { assert(identity==212060); return &spell; } } spells;
auto* sSpellMgr=&spells;
#include "ClickVisibility.inc"
#include "HandleSpellClick.inc"
void ReadRows(char const* path,std::vector<Condition>& rows) {
    std::ifstream input(path); Condition row;
    while(input>>row.ElseGroup>>row.ConditionType>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2>>row.ConditionValue3>>row.NegativeCondition) rows.push_back(row);
    assert(input.eof());
}
int main(int argc,char** argv) {
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    assert(argc==5);
    outdoorMap.instanced=false;
    bool baseline=std::string(argv[4])=="baseline";
    std::vector<Condition> clickRows,rootRows,outgoingRows;
    ReadRows(argv[1],clickRows);ReadRows(argv[2],rootRows);ReadRows(argv[3],outgoingRows);
    assert(clickRows.size()==(baseline?2u:20u) && rootRows.size()==(baseline?1u:16u) && outgoingRows.size()==(baseline?6u:9u));
    for(auto& row:clickRows) conditionManager.SpellClickEventConditionStore[106854][212060].push_back(&row);
    for(auto& row:rootRows) conditionManager.SmartEventConditionStore[{106854,0}][3].push_back(&row);
    for(auto& row:outgoingRows) conditionManager.SmartEventConditionStore[{106854,0}][1].push_back(&row);
    Creature bird;Player owner;
    if(baseline) {
        assert(!owner.canSeeSpellClickOn(&bird));
        assert(!bird.HandleSpellClick(&owner) && owner.casts==0 && bird.ai.calls==1);
        Player unrelated;unrelated.quests.clear();unrelated.owned=false;
        assert(conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,&unrelated,&bird));
        scenarios.scenario.step=6;
        assert(conditionManager.IsObjectMeetingSmartEventConditions(106854,0,0,&owner,&bird));
        std::cout<<"BASELINE: rewarded41574/active42185 owner loses native visible click/cast; AI callback still runs and old actor-only root admits an unrelated player\n";
        return 0;
    }
    assert(owner.canSeeSpellClickOn(&bird) && bird.HandleSpellClick(&owner) && owner.casts==1 && bird.ai.calls==1);
    assert(conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,&owner,&bird));
    scenarios.scenario.step=6;
    assert(!bird.HandleSpellClick(&owner) && owner.casts==1);
    assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,0,0,&owner,&bird));
    assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,&owner,&bird));
    for(uint32 stage=0;stage<8;++stage) for(uint32 state:{0u,1u,3u,5u,6u}) for(uint32 ownership=0;ownership<3;++ownership) {
        scenarios={};scenarios.scenario.step=stage;Player player;Creature actor;
        player.quests[42185]=state;player.owned=ownership==1;player.bank=ownership==2;
        bool eligible=stage==5 && (state==1 || state==3) && ownership!=0;
        assert(player.canSeeSpellClickOn(&actor)==eligible);
        assert(actor.HandleSpellClick(&player)==eligible && player.casts==uint32(eligible) && actor.ai.calls==1);
        assert(conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,&player,&actor)==eligible);
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,0,0,&player,&actor));
    }
    for(uint32 invalid=0;invalid<9;++invalid) {
        scenarios={};finalMap.instanced=true;finalMap.instance.identity=42;Player player;Creature actor;
        if(invalid==0) scenarios.present=false;
        if(invalid==1) scenarios.scenario.identity=1068;
        if(invalid==2) player.mapId=1220;
        if(invalid==3) player.classMask=1;
        if(invalid==4) player.rewarded=true;
        if(invalid==5) player.alive=false;
        if(invalid==6) actor.map=&outdoorMap;
        if(invalid==7) finalMap.instance.identity=43;
        if(invalid==8) finalMap.instanced=false;
        bool visible=player.canSeeSpellClickOn(&actor),handled=actor.HandleSpellClick(&player);
        if(visible || handled || player.casts) { std::cerr<<"FAIL: Huey invalid context "<<invalid<<" visible="<<visible<<" handled="<<handled<<" casts="<<player.casts<<'\n';return 1; }
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,&player,&actor));
    }
    scenarios={};finalMap.instanced=true;finalMap.instance.identity=42;
    for(uint32 state:{0u,1u,3u,5u,6u}) {
        Player outgoing;Creature actor;outgoing.quests.clear();outgoing.quests[41574]=state;
        outgoing.mapId=actor.mapId=1220;outgoing.map=actor.map=&outdoorMap;
        bool admitted=state==1 || state==3;
        assert(outgoing.canSeeSpellClickOn(&actor)==admitted && actor.HandleSpellClick(&outgoing)==admitted);
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,&outgoing,&actor));
        assert(conditionManager.IsObjectMeetingSmartEventConditions(106854,0,0,&outgoing,&actor)==(state==1 || state==3 || state==6));
    }
    for(uint32 state:{0u,1u,3u,5u,6u}) {
        Player wrongQuest;Creature actor;wrongQuest.quests.clear();wrongQuest.quests[41574]=state;wrongQuest.owned=false;
        scenarios.scenario.step=4;
        assert(!wrongQuest.canSeeSpellClickOn(&actor) && !actor.HandleSpellClick(&wrongQuest));
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,0,0,&wrongQuest,&actor));
    }
    for(uint32 state:{0u,1u,3u,5u,6u}) for(uint32 stage:{4u,5u,6u}) {
        Player claimant;Creature actor;claimant.quests[41574]=state;scenarios.scenario.step=stage;
        assert(claimant.canSeeSpellClickOn(&actor)==(stage==5));
        assert(conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,&claimant,&actor)==(stage==5));
        assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,0,0,&claimant,&actor));
    }
    scenarios.scenario.step=5;
    Unit pet;Creature actor;Player player;
    assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,&pet,&actor));
    assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,nullptr,&actor));
    assert(!conditionManager.IsObjectMeetingSmartEventConditions(106854,2,0,&player,nullptr));
    actor.clickFlag=false;assert(!player.canSeeSpellClickOn(&actor));
    std::cout<<"PASS: native source18 lookup/visibility/HandleSpellClick plus source22 lookup/ElseGroups gate final owned Hunter42185 at1099/5; initial41574 branches preserved, early/failed/rewarded/abandoned/dead/missing/wrong contexts and pet/null return recipients rejected\n";
}
