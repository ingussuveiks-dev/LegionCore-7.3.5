#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <vector>
using uint32=uint32_t;using int64=int64_t;using uint64=uint64_t;
enum SMART_EVENT {SMART_EVENT_OOC_LOS=10,SMART_EVENT_LINK=61};
enum {SMART_TARGET_ACTION_INVOKER=7,SMART_EVENT_FLAG_NOT_REPEATABLE=1,CONDITION_TYPE_MASK=32,
    CONDITION_SCENARION_STEP=44,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205,SMART_ACTION_INVOKER_CAST=85,
    SPELL_EFFECT_KILL_CREDIT=90,SPELL_EFFECT_KILL_CREDIT2=134};
using CriteriaTypes=uint32;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
#include "CastFlags.inc"
struct Unit;struct Player;struct Creature;
enum class HighGuid {Player};
struct ObjectGuid {
    Unit* unit=nullptr;bool IsPlayer()const;static ObjectGuid Empty;
    uint64 GetLowPart()const{return reinterpret_cast<uint64>(unit);}uint64 GetHighPart()const{return 1;}
    template<HighGuid>static ObjectGuid Create(uint64){return {};}
};ObjectGuid ObjectGuid::Empty;
struct InstanceMap {uint32 GetInstanceId(){return 42;}} instanceMap;
struct Map {InstanceMap* ToInstanceMap(){return &instanceMap;}} worldMap;
struct WorldObject {
    bool expired=false,instanced=true;
    std::map<uint32,Creature*> nearby;
    virtual ~WorldObject()=default;
    virtual Unit* ToUnit(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}
    bool IsQuestObjectExpired(){return expired;}bool InInstance(){return instanced;}Map* GetMap(){return &worldMap;}
    bool isType(uint32 mask){return ToPlayer()&&(mask&16);}
    Creature* FindNearestCreature(uint32 identity,float){auto found=nearby.find(identity);return found==nearby.end()?nullptr:found->second;}
    float GetVisibilityRange(){return 100;}
};
struct Unit:WorldObject {
    Unit* owner=nullptr;Unit* charmer=nullptr;float distance=1;bool visible=true,hostile=false,available=true,castAccepted=true;
    uint32 casts=0,presentations=0;Unit* lastTarget=nullptr;std::vector<std::function<void()>> delayed;
    Unit* ToUnit()override{return this;}Player* ToPlayer()override{return nullptr;}virtual Player const* ToPlayer()const{return nullptr;}
    ObjectGuid GetGUID(){return {this};}ObjectGuid GetCharmerOrOwnerGUID()const{return {charmer?charmer:owner};}
    Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;
    bool HasAura(uint32){return false;}void InterruptNonMeleeSpells(bool){}
    template<class Callback>void AddDelayedEvent(uint32 delay,Callback callback){assert(delay==200);delayed.emplace_back(callback);}
    void Flush(){auto pending=std::move(delayed);delayed.clear();for(auto& callback:pending)callback();}
    void CastSpell(Unit* target,uint32 spell,bool triggered){assert(spell==200848&&!triggered);++casts;lastTarget=target;if(castAccepted&&ToPlayer())++presentations;}
};
struct Player:Unit {
    uint32 faction=0,criteria=0,accepted=0;Unit* actualInvoker=nullptr;
    Player* ToPlayer()override{return this;}Player const* ToPlayer()const override{return this;}
    void UpdateAchievementCriteria(CriteriaTypes,uint32,uint32,uint32,Unit*,bool);
};
struct Creature:Unit {
    bool combat=false;
    bool isInCombat(){return combat;}bool IsWithinDistInMap(Unit* unit,float range){return unit&&unit->distance<=range;}
    bool IsWithinLOSInMap(Unit* unit){return unit&&unit->visible;}bool IsHostileTo(Unit* unit){return unit&&unit->hostile;}
};
bool ObjectGuid::IsPlayer()const{return unit&&unit->ToPlayer();}
struct ObjectAccessor {
    static Player* GetPlayer(Unit const&,ObjectGuid guid){return guid.unit&&guid.unit->available?guid.unit->ToPlayer():nullptr;}
    static Unit* GetUnit(Unit const&,ObjectGuid guid){return guid.unit&&guid.unit->available?guid.unit:nullptr;}
};
#include "OwnerPlayer.inc"
struct Scenario {uint32 id=972,step=2;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct ScenarioManager {bool present=true;Scenario scenario;Scenario* GetScenario(uint32 identity){return present&&identity==42?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
void Player::UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32 misc2,uint32 misc3,Unit* invoker,bool fromScript){
    assert(type==92&&asset==49592&&!misc2&&!misc3&&fromScript);++criteria;actualInvoker=invoker;
    if(scenarioManager.scenario.step==2){++accepted;scenarioManager.scenario.step=3;}
}
struct ConditionSourceInfo {WorldObject* mConditionTargets[2];ConditionSourceInfo(WorldObject* first,WorldObject* second):mConditionTargets{first,second}{}};
struct Condition {
    uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;bool NegativeCondition=false;
    bool isLoaded(){return true;}
    bool Meets(ConditionSourceInfo& sourceInfo){WorldObject* object=sourceInfo.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;
        switch(ConditionType){
#include "Conditions.inc"
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
struct SpellEffectInfo {uint32 Effect=219;bool IsEffect()const{return true;}};
struct SpellInfo {SpellEffectInfo effect;SpellEffectInfo const* Effects[1]{&effect};bool HasEffect(uint32 value)const{return value==219;}};
struct SpellManager {SpellInfo spell;SpellInfo const* GetSpellInfo(uint32 identity){assert(identity==200848);return &spell;}} spellManager;
auto sSpellMgr=&spellManager;
#include "CreditHelpers.inc"
#include "ConversationStructs.inc"
struct ConversationStore {
    std::vector<ConversationData> lines;std::vector<ConversationCreature> creatures;std::vector<ConversationActor> actors;
    auto GetConversationData(uint32){return &lines;}auto GetConversationCreature(uint32){return &creatures;}auto GetConversationActor(uint32){return &actors;}
} conversationStore;
auto sConversationDataStore=&conversationStore;
struct ConversationDynamicFieldActors {uint32 actorId,creatureId,displayId,unk1,unk2,unk3;};
struct ConversationDynamicFieldCreatures {uint64 GuidLow,GuidHight;uint32 unk1,unk2;};
struct ConversationDynamicFieldLines {uint32 id,textId,unk1,unk2;};
struct Conversation {
    std::map<uint32,ConversationDynamicFieldCreatures> creatures;std::map<uint32,ConversationDynamicFieldLines> lines;
    void SetActor(ConversationDynamicFieldActors*,uint32,bool){assert(false);}
    void SetCreature(ConversationDynamicFieldCreatures* value,uint32 index,bool){creatures[index]=*value;}
    void SetLine(ConversationDynamicFieldLines* value,uint32 index,bool){lines[index]=*value;}
    bool GenerateDynamicFieldData(uint32,WorldObject*,uint32&);
};
#include "ConversationData.inc"
struct SmartScriptHolder {
    int64 entryOrGuid=100398;uint32 source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,targetRange=0;bool runOnce=false;
    struct {uint32 event_flags=1,event_phase_mask=0;struct{uint32 noHostile=1,maxDist=5,cooldownMin=0,cooldownMax=0;}los;}event;
    struct {struct{uint32 type=92,misc1=49592,misc2=0,misc3=0;}achievementCriteria;struct{uint32 spell=200848,flags=0;}cast;}action;
    uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}uint32 GetScriptType()const{return source_type;}
};
using SmartAIEventList=std::vector<SmartScriptHolder>;using ObjectList=std::list<WorldObject*>;
struct GameObject{};
struct SmartScript {
    Creature* me=nullptr;Player* closest=nullptr;SmartAIEventList mEvents;ObjectGuid mLastInvoker;
    WorldObject* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}
    bool IsInPhase(uint32){return true;}bool IsUnit(WorldObject* object){return object&&object->ToUnit();}
    void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
    ObjectList* GetTargets(SmartScriptHolder const& row,Unit* unit){Unit* trigger=unit?unit:GetLastInvoker();auto* l=new ObjectList;
        switch(row.GetTargetType()){
#include "InvokerTarget.inc"
            case 21:if(closest&&closest->distance<=row.targetRange)l->push_back(closest);break;
        }return l;
    }
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessEvent(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "RepeatGuard.inc"
#include "EventGate.inc"
        switch(e.GetEventType()){
#include "Los.inc"
            case SMART_EVENT_LINK:ProcessAction(e,unit,var0,var1,bvar,spell,gob);break;
        }
    }
    void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "ActionStart.inc"
        switch(e.GetActionType()){
#include "CriteriaAction.inc"
#include "InvokerCast.inc"
        }
#include "LinkTail.inc"
};
#include "Dispatch.inc"
#include "FindLink.inc"
SmartAIEventList definitions;
struct Fixture {
    Creature actor,pet,guardian,npc;Player finder,peer;SmartScript script;
    Fixture(){scenarioManager={};pet.owner=&finder;guardian.owner=&finder;script.me=&actor;script.closest=&peer;script.mEvents=definitions;}
    void See(Unit* unit){script.ProcessEventsFor(SMART_EVENT_OOC_LOS,unit);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);std::vector<Condition> rows(2);std::ifstream conditions(argv[2]);
    for(auto& row:rows){assert(conditions>>row.ElseGroup>>row.ConditionType>>row.ConditionTarget>>row.ConditionValue1>>row.ConditionValue2>>row.ConditionValue3>>row.NegativeCondition);conditionManager.SmartEventConditionStore[{100398,0}][4].push_back(&row);}
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<2;++index){SmartScriptHolder row;uint32 first,second,third,fourth;
        assert(events>>row.event_id>>row.link>>row.type>>row.event.event_flags>>row.event.los.noHostile>>row.event.los.maxDist>>row.actionType>>first>>second>>third>>fourth>>row.targetType>>row.targetRange);
        row.action.achievementCriteria={first,second,third,fourth};row.action.cast={first,second};definitions.push_back(row);
    }
    assert(definitions[0].event_id==3&&definitions[0].targetType==7&&!definitions[0].targetRange);
    assert(definitions[1].event_id==4&&definitions[1].action.cast.spell==200848);
    std::ifstream conversation(argv[3]);std::string payload;
    while(std::getline(conversation,payload)){std::istringstream row(payload);std::string kind;row>>kind;
        if(kind=="creature"){ConversationCreature actor{};assert(row>>actor.entry>>actor.id>>actor.creatureId>>actor.creatureGuid>>actor.unk1>>actor.unk2>>actor.duration);conversationStore.creatures.push_back(actor);}
        else if(kind=="line"){ConversationData line{};assert(row>>line.entry>>line.id>>line.idx>>line.textId>>line.unk1>>line.unk2);conversationStore.lines.push_back(line);}
        else assert(false);
    }
    assert(conversationStore.creatures.size()==3&&conversationStore.lines.size()==3);
    {Fixture actors;actors.finder.nearby={{100398,&actors.actor},{100749,&actors.npc},{100397,&actors.guardian}};
        Conversation built;uint32 duration=0;assert(built.GenerateDynamicFieldData(1073,&actors.finder,duration));
        assert(duration==14400&&built.creatures.size()==3&&built.lines.size()==3);
        assert(built.creatures[0].GuidLow==actors.actor.GetGUID().GetLowPart()&&built.creatures[1].GuidLow==actors.npc.GetGUID().GetLowPart()&&built.creatures[2].GuidLow==actors.guardian.GetGUID().GetLowPart());
        assert(built.lines[0].id==3364&&built.lines[0].textId==0&&built.lines[1].id==2632&&built.lines[1].textId==4800&&built.lines[2].id==2633&&built.lines[2].textId==9200);
        for(uint32 missing:{100398u,100749u,100397u}){auto savedActor=actors.finder.nearby[missing];actors.finder.nearby.erase(missing);Conversation rejected;duration=0;assert(!rejected.GenerateDynamicFieldData(1073,&actors.finder,duration));actors.finder.nearby[missing]=savedActor;}
    }
    auto saved=conditionManager.SmartEventConditionStore;
    for(bool premature:{false,true}){
        conditionManager.SmartEventConditionStore.clear();Fixture baseline;baseline.script.mEvents[0].targetType=21;baseline.script.mEvents[0].targetRange=100;
        scenarioManager.scenario.step=premature?1:2;baseline.See(&baseline.pet);baseline.pet.Flush();
        assert(baseline.peer.criteria==1&&baseline.finder.criteria==0&&baseline.pet.casts==1&&!baseline.pet.presentations&&baseline.script.mEvents[0].runOnce);
        scenarioManager.scenario.step=2;baseline.See(&baseline.finder);assert(baseline.finder.criteria==0&&baseline.finder.delayed.empty());
    }
    conditionManager.SmartEventConditionStore=saved;
    for(uint32 faction=0;faction<2;++faction){Fixture fixed;fixed.finder.faction=faction;
        for(Unit* nonplayer:{static_cast<Unit*>(&fixed.pet),static_cast<Unit*>(&fixed.guardian),static_cast<Unit*>(&fixed.npc),static_cast<Unit*>(nullptr)})fixed.See(nonplayer);
        assert(!fixed.script.mEvents[0].runOnce&&!fixed.finder.criteria&&!fixed.peer.criteria);
        fixed.finder.distance=5;fixed.See(&fixed.finder);assert(fixed.finder.accepted==1&&scenarioManager.scenario.step==3&&!fixed.peer.criteria);
        assert(fixed.finder.actualInvoker==&fixed.finder&&fixed.finder.delayed.size()==1&&!fixed.finder.casts);
        fixed.finder.Flush();assert(fixed.finder.casts==1&&fixed.finder.presentations==1&&fixed.finder.lastTarget==&fixed.finder);
        for(uint32 repeat=0;repeat<20;++repeat)fixed.See(&fixed.finder);fixed.finder.Flush();assert(fixed.finder.criteria==1&&fixed.finder.casts==1);
    }
    for(uint32 stage=0;stage<15;++stage){if(stage==2)continue;Fixture retry;scenarioManager.scenario.step=stage;retry.See(&retry.finder);
        assert(!retry.script.mEvents[0].runOnce&&!retry.finder.criteria);scenarioManager.scenario.step=2;retry.See(&retry.finder);assert(retry.finder.accepted==1);
    }
    for(uint32 blocked=0;blocked<9;++blocked){Fixture retry;
        switch(blocked){case 0:retry.finder.distance=5.01f;break;case 1:retry.finder.visible=false;break;case 2:retry.finder.hostile=true;break;case 3:retry.actor.combat=true;break;case 4:scenarioManager.present=false;break;case 5:scenarioManager.scenario.id=973;break;case 6:retry.actor.instanced=false;break;case 7:retry.actor.expired=true;break;case 8:retry.script.me=nullptr;break;}
        retry.See(&retry.finder);assert(!retry.script.mEvents[0].runOnce&&!retry.finder.criteria);
        retry.finder.distance=1;retry.finder.visible=true;retry.finder.hostile=false;retry.actor.combat=false;retry.actor.instanced=true;retry.actor.expired=false;retry.script.me=&retry.actor;scenarioManager={};retry.See(&retry.finder);assert(retry.finder.accepted==1);
    }
    {Fixture missing;missing.See(&missing.finder);missing.finder.available=false;missing.finder.Flush();assert(!missing.finder.casts&&missing.finder.accepted==1);}
    {Fixture failed;failed.finder.castAccepted=false;failed.See(&failed.finder);failed.finder.Flush();failed.See(&failed.finder);assert(failed.finder.casts==1&&!failed.finder.presentations&&failed.finder.accepted==1);}
    {Fixture fresh;fresh.See(&fresh.finder);assert(fresh.finder.accepted==1);Fixture reentry;reentry.See(&reentry.finder);assert(reentry.finder.accepted==1);}
    std::cout<<"PASS: native conditions/LOS/one-shot/LINK/criteria/invoker-cast/200ms callback reject premature/pet discovery, retain actual finder and Player self-cast after stage advance, both faction hosts, nonplayers, retries, dedup and missing target. Failed-cast presentation retry remains unsupported; world/scenario/cast hosts are not client traversal.\n";
}
