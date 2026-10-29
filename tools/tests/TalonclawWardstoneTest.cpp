#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <memory>
using uint64=uint64_t;using CriteriaTypes=uint32_t;using SpellCastResult=uint32_t;
constexpr uint32_t SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_FAILED_SPELL_IN_PROGRESS=2;
using uint32 = uint32_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using int8 = int8_t;
using ObjectGuid = uint32;
using QuestStatus = uint32;
enum { CONDITION_ITEM=2, CONDITION_QUESTREWARDED=8, CONDITION_QUESTTAKEN=9, CONDITION_CLASS=15, CONDITION_MAPID=22,
    CONDITION_QUEST_COMPLETE=28, CONDITION_OBJECT_ENTRY=31, CONDITION_ALIVE=36, CONDITION_SCENARION_STEP=44,
    QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1, QUEST_STATUS_REWARDED=6,
    UNIT_FIELD_NPC_FLAGS=0, UNIT_NPC_FLAG_SPELLCLICK=0x1000000,
    NPC_CLICK_CAST_CASTER_CLICKER=1, NPC_CLICK_CAST_TARGET_CLICKER=2, NPC_CLICK_CAST_ORIG_CASTER_OWNER=4,
    MAX_SPELL_EFFECTS=32, SPELL_AURA_CONTROL_VEHICLE=236, SPELLVALUE_BASE_POINT0=0, MAX_EFFECT_MASK=0xffffffff };
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
#define ASSERT(value) assert(value)
uint32 SpellValueMod(uint32 value) { return value; }
struct Unit;struct Player;struct Creature;struct GameObject;
void EmitCriteria(Player*,uint32,uint32,Unit*);
bool CastGate(Unit*,Unit*);
Unit* pendingCaster=nullptr;Unit* pendingTarget=nullptr;bool rejectActionCast=false;
struct SpellInfo {uint32 Id=199728;struct Misc {struct {uint32 SchoolMask=127;} MiscData;} misc;Misc const* GetMisc()const{return &misc;} uint32 EffectMask=0; struct Effect { uint32 ApplyAuraName=0; } effect; Effect const* GetEffect(uint32,uint32) const { return &effect; } };
struct InstanceMap { uint32 identity=42; uint32 GetInstanceId() const { return identity; } };
struct Map { bool instanced=true; InstanceMap instance; InstanceMap* ToInstanceMap() { return instanced ? &instance : nullptr; } } finalMap, outdoorMap;
struct AchievementCache {uint32 asset=0;};using AchievementCachePtr=std::shared_ptr<AchievementCache>;
struct CriteriaTreeEntry {uint32 ID,CriteriaID;};
struct DB2Manager {std::map<uint32,std::vector<CriteriaTreeEntry const*>> lists;std::vector<CriteriaTreeEntry const*> const* GetCriteriaTreeList(uint32 root)const{auto i=lists.find(root);return i==lists.end()?nullptr:&i->second;}} sDB2Manager;
struct CriteriaEndpoint;
struct Scenario {uint32 identity=973,step=2,currentTree=46184,_criteriaTreeForUpdate=0;std::map<uint32,uint32> counters;uint32 GetScenarioId()const{return identity;}uint32 GetCurrentStep()const{return step;}
 void SetStep(uint32 value){step=value;currentTree=value==3?46483:value==4?46186:value==2?46184:99999;}
 bool CanUpdateCriteria(uint32,uint32=0)const;void UpdateAchievementCriteria(AchievementCachePtr);CriteriaEndpoint& GetAchievementMgr();
};
struct ScenarioMgr {bool present=true;Scenario scenario;Scenario* GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}} scenarios;

auto* sScenarioMgr=&scenarios;
struct Unit {
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual Creature* ToCreature() { return nullptr; }
    Unit* ToUnit() { return this; }
    uint32 entry=0, mapId=1609,classMask=4,casts=0;
    bool alive=true,expired=false;std::vector<uint32> actionCasts;Unit* owner=nullptr;
    Map* map=&finalMap;
    struct Vehicle { uint32 GetCreatureEntry() const { return 106854; } };
    Vehicle* GetVehicleKit() { return nullptr; }
    uint32 GetEntry() const { return entry; }
    uint32 GetTypeId() { return ToPlayer()?4u:ToCreature()?3u:0u; }
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
    bool IsQuestObjectExpired()const{return expired;}bool isInCombat()const{return false;}bool IsUnit()const{return true;}bool HasAura(uint32)const{return false;}void InterruptNonMeleeSpells(bool){}
    void GetPosition(float& x,float& y,float& z){x=y=z=0;}
    Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():owner?owner->ToPlayer():nullptr;}
    ObjectGuid GetGUID() const { return 17; }
    ObjectGuid GetOwnerGUID() const { return 0; }
    uint32 GetSpawnMode() const { return 0; }
    bool IsInMap(Unit* other) const { return map==other->map; }
    bool HandleSpellClick(Unit*,int8=-1);
    void CastSpell(Unit* target,SpellInfo const*,bool triggered,void*,void*,ObjectGuid){assert(!triggered);if(CastGate(this,target)){++casts;pendingCaster=this;pendingTarget=target;}}
    SpellCastResult CastSpell(Unit*,uint32 id,bool){actionCasts.push_back(id);return rejectActionCast?SPELL_FAILED_BAD_TARGETS:SPELL_CAST_OK;}
    void CastSpell(float,float,float,uint32,bool){assert(false);}
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
    void UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32 v2,uint32 v3,Unit* unit,bool ignore){assert(type==92&&!v2&&!v3&&ignore);EmitCriteria(this,type,asset,unit);}
    OutdoorPvP* GetOutdoorPvP() { return nullptr; }
    bool canSeeSpellClickOn(Creature const*) const;
};
struct Aura { template<class... Values> static Aura* TryRefreshStackOrCreate(Values...) { assert(false); return nullptr; } void ApplyForTargets() { assert(false); } };
struct Creature : Unit {
    struct Delegate { uint32 calls=0; Unit* invoker=nullptr; void SetCombatMove(bool){}void OnSpellClick(Unit* unit) { ++calls; invoker=unit; } } ai;
    bool IsAIEnabled=true,clickFlag=true;
    Creature() { entry=106854; }
    Creature* ToCreature() override { return this; }
    bool HasFlag(uint32,uint32 flag) const { assert(flag==UNIT_NPC_FLAG_SPELLCLICK); return clickFlag; }
    Delegate* AI() { return &ai; }
};
using WorldObject=Unit;
struct ConditionSourceInfo { union {Unit* targets[2];Unit* mConditionTargets[2];}; ConditionSourceInfo(Unit* first,Unit* second=nullptr):targets{first,second} {} };
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
    ConditionList GetConditionsForSmartEvent(uint32 entry,uint32 id,uint32 source)const{auto i=SmartEventConditionStore.find({int32(entry),source});if(i==SmartEventConditionStore.end())return {};auto j=i->second.find(id+1);return j==i->second.end()?ConditionList{}:j->second;}
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
struct SpellMgr { uint32 identity=212060; SpellInfo spell; SpellInfo const* GetSpellInfo(uint32 requested) { assert(requested==identity); return &spell; } } spells;
auto* sSpellMgr=&spells;
#include "ClickVisibility.inc"
#include "HandleSpellClick.inc"
void ReadRows(char const* path,std::vector<Condition>& rows) {
    std::ifstream input(path); Condition row;
    while(input>>row.ElseGroup>>row.ConditionType>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2>>row.ConditionValue3>>row.NegativeCondition) rows.push_back(row);
    assert(input.eof());
}

#include "Constants.inc"
using ObjectList=std::vector<Unit*>;
struct GameObject:Unit {SpellCastResult CastSpell(Unit*,uint32){return SPELL_FAILED_BAD_TARGETS;}};
bool IsUnit(Unit* u){return u!=nullptr;}
#include "CreditOwner.inc"
std::vector<Condition> clickRules,castRules,rootRules;
bool CastGate(Unit* m_caster,Unit* object){struct Targets {Unit* object;Unit* GetObjectTarget(){return object;}} m_targets{object};
#include "CastBinding.inc"
 ConditionList rows;for(auto& r:castRules)rows.push_back(&r);return conditionManager.IsObjectMeetToConditions(condInfo,rows);
}
struct CriteriaEndpoint {Scenario* owner=nullptr;void UpdateAchievementCriteria(AchievementCachePtr cache){for(uint32 leaf:{46484u,46187u})if(cache->asset==48172&&owner->CanUpdateCriteria(leaf)){auto& counter=owner->counters[leaf];++counter;uint64 requiredAmount=leaf==46484?0:3;
#include "CriteriaAmount.inc"
 if(counter>=requiredAmount)owner->SetStep(owner->step+1);}}} criteriaEndpoint;
CriteriaEndpoint& Scenario::GetAchievementMgr(){criteriaEndpoint.owner=this;return criteriaEndpoint;}
#include "CriteriaSnapshot.inc"
#include "CriteriaFilter.inc"
void EmitCriteria(Player*,uint32,uint32 asset,Unit*){assert(asset==48172);scenarios.scenario.UpdateAchievementCriteria(std::make_shared<AchievementCache>(AchievementCache{asset}));}
struct SmartScriptHolder {
 uint32 entryOrGuid=101325,source_type=0,event_id=0,link=0,type=0,actionType=0,parameter=0,targetType=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;struct {uint32 spell=0,school=0,cooldownMin=0,cooldownMax=0;} spellHit;} event;
 struct {struct {uint32 spell=0,flags=0;} cast;struct {uint32 type=0,misc1=0,misc2=0,misc3=0;} achievementCriteria;} action;
 struct {float x=0,y=0,z=0;} target;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}uint32 GetScriptType()const{return source_type;}
};
bool roll_chance_i(uint32){return true;}
struct SmartScript {
 std::vector<SmartScriptHolder> mEvents;Creature* me=nullptr;GameObject* go=nullptr;ObjectGuid mLastInvoker=0;unsigned errors=0,retries=0,baseRequests=0;Unit* last=nullptr;
 Unit* GetBaseObject(){return me;}Unit* GetLastInvoker(){return last;}bool IsInPhase(uint32)const{return true;}void RecalcTimer(SmartScriptHolder&,uint32 lo,uint32 hi){assert(lo==hi&&(lo==0||lo==500));if(lo)++retries;}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit* unit){auto l=new ObjectList();if(e.targetType==1&&me)l->push_back(me);if(e.targetType==7&&unit)l->push_back(unit);if(l->empty()){delete l;return nullptr;}return l;}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void Hit(Unit* caster){last=caster;SpellInfo info;ProcessEventsFor(SMART_EVENT_SPELLHIT,caster,0,0,false,&info);}
};
#undef TC_LOG_ERROR
#define TC_LOG_ERROR(...) ++errors
#define ENSURE_AI(type,ai) (ai)
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LinkEvent.inc"
#include "SpellHitEvent.inc"
 default:break;
 }
}
#include "ActionPrefix.inc"
 switch(e.GetActionType()){
#include "CastAction.inc"
#include "CreditAction.inc"
 case 12:assert(e.parameter==101360);++baseRequests;break; // unchanged native base summon endpoint
 default:assert(false);
 }
#include "LinkTail.inc"
static std::vector<SmartScriptHolder> Actions(std::string const& path){std::ifstream f(path);assert(f);std::string line;std::vector<SmartScriptHolder> rows;while(std::getline(f,line)){std::stringstream ss(line);std::string cell;std::vector<std::string> v;while(std::getline(ss,cell,'\t'))v.push_back(cell);assert(v.size()==31);auto n=[&](size_t i){return uint32(std::stoul(v[i]));};if(n(0)!=101325||n(1))continue;SmartScriptHolder e;e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_flags=n(8);e.event.spellHit={n(9),n(10),n(11),n(12)};e.actionType=n(14);e.parameter=n(15);e.action.cast={n(15),n(16)};e.action.achievementCriteria={n(15),n(16),n(17),n(18)};e.targetType=n(21);rows.push_back(e);}assert(rows.size()==4);return rows;}
static void Rules(std::string const& data,bool repaired){clickRules.clear();castRules.clear();rootRules.clear();auto tag=repaired?"first":"original";ReadRows((data+"/"+tag+"-click.tsv").c_str(),clickRules);ReadRows((data+"/"+tag+"-cast.tsv").c_str(),castRules);ReadRows((data+"/"+tag+"-root.tsv").c_str(),rootRules);
 conditionManager.SpellClickEventConditionStore.clear();conditionManager.SmartEventConditionStore.clear();for(auto& r:clickRules)conditionManager.SpellClickEventConditionStore[101325][199728].push_back(&r);for(auto& r:rootRules)conditionManager.SmartEventConditionStore[{101325,0}][1].push_back(&r);}
static void Reset(uint32 step){scenarios={};scenarios.scenario.SetStep(step);finalMap.instanced=true;finalMap.instance.identity=42;outdoorMap.instanced=false;pendingCaster=pendingTarget=nullptr;rejectActionCast=false;templates.clicks.clear();templates.clicks.emplace(101325,SpellClickInfo{199728,1});spells.identity=199728;}
static void Actors(Player& owner,Creature& stone){owner.mapId=stone.mapId=1511;owner.classMask=4;owner.quests.clear();owner.quests[40385]=3;stone.entry=101325;}
static SmartScript Script(std::vector<SmartScriptHolder> const& rows,Creature& stone){SmartScript script;script.mEvents=rows;script.me=&stone;return script;}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::string data=argv[1];auto rows=Actions(data+"/original.tsv");CriteriaTreeEntry first{46484,1},more{46187,1};sDB2Manager.lists[46483]={&first};sDB2Manager.lists[46186]={&more};sDB2Manager.lists[46184]={};
 // Original early actual hit consumes the once event despite the native
 // scenario filter rejecting both48172 trees. Later eligible hit cannot recover.
 Reset(2);Rules(data,false);Player oldOwner;Creature oldStone;Actors(oldOwner,oldStone);auto before=Script(rows,oldStone);assert(oldOwner.canSeeSpellClickOn(&oldStone)&&oldStone.HandleSpellClick(&oldOwner)&&oldOwner.casts==1);before.Hit(pendingCaster);assert(before.mEvents[0].runOnce&&before.errors==1&&scenarios.scenario.counters.empty());scenarios.scenario.SetStep(3);before.Hit(&oldOwner);assert(scenarios.scenario.counters.empty());
 Reset(2);Rules(data,true);Player owner;Creature stone;Actors(owner,stone);auto after=Script(rows,stone);assert(owner.canSeeSpellClickOn(&stone)&&stone.HandleSpellClick(&owner)&&!owner.casts&&stone.ai.calls==1&&!pendingCaster);after.Hit(&owner);assert(!after.mEvents[0].runOnce&&!after.errors&&!after.baseRequests);
 scenarios.scenario.SetStep(3);assert(owner.canSeeSpellClickOn(&stone)&&stone.HandleSpellClick(&owner)&&owner.casts==1);assert(scenarios.scenario.counters.empty());after.Hit(pendingCaster);assert(scenarios.scenario.counters[46484]==1&&scenarios.scenario.counters[46187]==0&&scenarios.scenario.step==4&&after.baseRequests==1&&after.errors==1);after.Hit(&owner);assert(scenarios.scenario.counters[46187]==0&&after.baseRequests==1);
 for(unsigned i=0;i<3;++i){Creature next;next.entry=101325;next.mapId=1511;auto s=Script(rows,next);s.Hit(&owner);assert(scenarios.scenario.counters[46187]==i+1&&s.errors==1);s.Hit(&owner);assert(scenarios.scenario.counters[46187]==i+1);}assert(scenarios.scenario.step==5);
 unsigned cases=0;for(uint32 stage=0;stage<=13;++stage)for(uint32 status:{0u,1u,3u,5u,6u}){Reset(stage);Rules(data,true);Player p;Creature a;Actors(p,a);p.quests[40385]=status;auto s=Script(rows,a);bool active=status==3;bool eligible=active&&(stage==3||stage==4);assert(p.canSeeSpellClickOn(&a)==active);assert(CastGate(&p,&a)==eligible);assert(conditionManager.IsObjectMeetingSmartEventConditions(101325,0,0,&p,&a)==eligible);s.Hit(&p);assert(s.mEvents[0].runOnce==eligible&&s.errors==unsigned(eligible)&&s.baseRequests==unsigned(eligible));++cases;}
 for(uint32 mode=0;mode<11;++mode){Reset(3);Rules(data,true);Player p;Creature a;Actors(p,a);auto s=Script(rows,a);switch(mode){case 0:p.classMask=1;break;case 1:p.mapId=1220;break;case 2:a.mapId=1220;break;case 3:p.alive=false;break;case 4:a.alive=false;break;case 5:a.entry=101360;break;case 6:scenarios.present=false;break;case 7:scenarios.scenario.identity=991;break;case 8:a.map=&outdoorMap;break;case 9:finalMap.instance.identity=43;break;case 10:finalMap.instanced=false;break;}assert(!CastGate(&p,&a));s.Hit(&p);assert(!s.mEvents[0].runOnce&&!s.errors&&!s.baseRequests);++cases;}
 Reset(3);Rules(data,true);Unit pet;pet.mapId=1511;pet.classMask=4;Creature a;a.entry=101325;a.mapId=1511;assert(!CastGate(&pet,&a)&&!CastGate(nullptr,&a)&&!CastGate(&owner,nullptr));auto invalid=Script(rows,a);invalid.Hit(&pet);invalid.Hit(nullptr);assert(!invalid.mEvents[0].runOnce&&!invalid.errors);
 // Accepted cast can become ineligible before delivery. Actual source22 is
 // checked again and must preserve the future legitimate attempt.
 for(uint32 mode=0;mode<4;++mode){Reset(3);Rules(data,true);Player p;Creature target;Actors(p,target);assert(target.HandleSpellClick(&p)&&p.casts==1);auto s=Script(rows,target);if(mode==0)p.quests.clear();if(mode==1)p.alive=false;if(mode==2)scenarios.scenario.SetStep(5);if(mode==3)target.expired=true;s.Hit(&p);assert(!s.mEvents[0].runOnce&&!s.errors);}
 // Optional old visuals can fail after real credit; the repair doesn't invent
 // success, cleanup or a retry. Preserve that remaining visual/base limitation.
 Reset(3);Rules(data,true);Player p;Creature target;Actors(p,target);auto failure=Script(rows,target);rejectActionCast=true;failure.Hit(&p);assert(scenarios.scenario.counters[46484]==1&&failure.retries==1&&!failure.baseRequests&&!failure.errors);
 std::cout<<"PASS native Wardstone source18 visibility/HandleSpellClick, actual source17 target binding, source22 conditions/event/once/LINK/CAST/205 plus native scenario event-tree snapshot/filter and amount0 normalization: old early-hit consumed/stalled counterfactual; fixed retry3 then three distinct4 nodes once, "<<cases<<" stage/status/invalid matrices, pet/null/late state/cast failures. KnownLINK4 remains expected for valid full tail; cast/summon/scenario completion endpoints controlled; client and visual/base failure recovery separate.\n";
}
