// Native event/action/timer sections; map lookup, casts and world mutations
// are controlled services. This does not emulate client rendering or awards.
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;
#include "Constants.inc"
constexpr uint32 UNIT_STATE_CASTING=1;
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS};
struct ObjectGuid {uint32 n=0;static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct SmartScript;struct Creature;struct GameObject;struct Player;
struct Scenario {uint32 id=990,step=6;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}} scenario;
struct Map {uint32 instanceId=1;bool instanceMap=true;Map* ToInstanceMap(){return instanceMap?this:nullptr;}uint32 GetInstanceId()const{return instanceId;}};
using InstanceMap=Map;
struct ScenarioManager {bool available=true;Scenario* GetScenario(uint32 id){return available&&id==1?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct AI {SmartScript* script=nullptr;void SetData(uint32,uint32);void SetCombatMove(bool){}};
struct Unit {
    bool creature=false,player=false,gameobject=false,alive=true,expired=false,combat=false,casting=false,aura=false,applyAura=true,instanced=true;
    uint32 entry=0,flags=768,mapId=1536,casts=0;Map map;SpellCastResult castResult=SPELL_CAST_OK;
    Creature* ToCreature();GameObject* ToGameObject();Player* ToPlayer();Unit* ToUnit(){return this;}
    bool IsUnit()const{return !gameobject;}bool HasAura(uint32 spell)const{return aura&&spell==208720;}
    bool HasAuraEffect(uint32 spell,uint32 effect)const{return HasAura(spell)&&effect==0;}
    bool IsAlive()const{return alive;}uint32 GetMapId()const{return mapId;}bool InInstance()const{return instanced;}Map* GetMap(){return &map;}
    bool isInCombat()const{return combat;}bool HasUnitState(uint32)const{return casting;}
    bool IsQuestObjectExpired()const{return expired;}ObjectGuid GetGUID()const{return {};}
    void GetPosition(float& x,float& y,float& z)const{x=y=z=0;}
    void InterruptNonMeleeSpells(bool){}
    SpellCastResult CastSpell(Unit* target,uint32 spell,bool){++casts;if(castResult==SPELL_CAST_OK&&applyAura&&spell==208720)target->aura=true;return castResult;}
    SpellCastResult CastSpell(float,float,float,uint32,bool){return castResult;}
};
struct Creature:Unit {::AI ai;Creature(){creature=true;}::AI* AI(){return &ai;}uint32 GetEntry()const{return entry;}};
struct Player:Unit {Player(){player=true;}};
struct GameObject:Unit {::AI ai;GameObject(){gameobject=true;}::AI* AI(){return &ai;}SpellCastResult CastSpell(Unit*,uint32){return castResult;}};
Creature* Unit::ToCreature(){return static_cast<Creature*>(this);}GameObject* Unit::ToGameObject(){return static_cast<GameObject*>(this);}Player* Unit::ToPlayer(){return static_cast<Player*>(this);}
bool IsCreature(Unit* o){return o&&o->creature;}bool IsGameObject(Unit* o){return o&&o->gameobject;}bool IsPlayer(Unit* o){return o&&o->player;}bool IsUnit(Unit* o){return o&&o->IsUnit();}
using ObjectList=std::vector<Unit*>;struct SpellInfo{};
struct ConditionSourceInfo {Unit* targets[2];ConditionSourceInfo(Unit* a,Unit* b):targets{a,b}{}};
struct Rule {uint32 entry,group,target,kind,a,b,negative;};
using NativeConditionList=std::vector<Rule>;
using ConditionList=NativeConditionList;
constexpr uint32 CONDITION_AURA=1,CONDITION_MAPID=22,CONDITION_ALIVE=36,CONDITION_SCENARION_STEP=44;
bool Evaluate(Rule const& rule,ConditionSourceInfo const& info)
{
    Unit* object=info.targets[rule.target];if(!object)return false;
    auto ConditionValue1=rule.a,ConditionValue2=rule.b;bool condMeets=false;
    switch(rule.kind) {
#include "AuraCondition.inc"
#include "MapCondition.inc"
#include "AliveCondition.inc"
#include "ScenarioCondition.inc"
    }
    return rule.negative?!condMeets:condMeets;
}
struct Conditions {
    bool allow=true;NativeConditionList rules;
    bool IsObjectMeetingSmartEventConditions(uint32 entry,uint32 event,uint32,Unit* unit,Unit* baseObject)const{
#include "SmartTargetBinding.inc"
        return IsObjectMeetToConditions(sourceInfo,GetConditionsForSmartEvent(entry,event,0));
    }
    NativeConditionList GetConditionsForSmartEvent(uint32 entry,uint32 event,uint32)const{NativeConditionList result;for(auto r:rules)if(r.entry==entry&&r.group==event+1)result.push_back(r);return result;}
    bool IsObjectMeetToConditions(ConditionSourceInfo& info,NativeConditionList const& ruleset)const{if(!allow)return false;for(auto r:ruleset)if(!Evaluate(r,info))return false;return true;}
} conditions;Conditions* sConditionMgr=&conditions;
struct Chat {void SendChat(Creature*,uint8,ObjectGuid){}} chat;Chat* sCreatureTextMgr=&chat;
bool roll_chance_i(uint32){return true;}uint32 urand(uint32 a,uint32){return a;}
struct SmartScriptHolder {
    uint32 entryOrGuid=0,source_type=0,event_id=0,link=0,type=0,actionType=0,parameter=0,targetType=0,targetEntry=0,timer=0;
    bool active=true,runOnce=false,enableTimed=true;
    struct {
        uint32 event_phase_mask=0,event_flags=0,event_chance=100;
        struct {uint32 textGroupID=0,creatureEntry=0;} textOver;
        struct {uint32 pointID=0,pathID=0;} waypoint;
        struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;} dataSet;
        struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;
    } event;
    struct {
        struct {uint32 textGroupID=0,duration=0;} talk;
        struct {uint32 spell=0,flags=0;} cast;
        struct {uint32 field=0,data=0;} setData;
        struct {uint32 phase=0;} setEventPhase;
    } action;
    struct {float x=0,y=0,z=0;} target;
    uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}uint32 GetScriptType()const{return source_type;}
};
struct Call {uint32 time,action,value;};
struct SmartScript {
 // This older scope has no linked-cast opt-in; unexpected use must fail.
 template<class... Args>void QueueLinkedCastRetry(Args const&...){assert(false);}
 template<class... Args>uint32 BeginLinkedCastAttempt(Args const&...){assert(false);return 0;}
 template<class... Args>bool FinishLinkedCastAttempt(Args const&...){assert(false);return false;}
 void UpdateLinkedCastRetries(uint32){}
 void ResetLinkedCastRetries(){}

    std::vector<SmartScriptHolder> mEvents,mTimedActionList;std::vector<Call> calls;
    Creature actor;Creature* me=&actor;GameObject* go=nullptr;Creature* visual=nullptr;Player player;Unit* lastInvoker=nullptr;
    uint32 errors=0,now=0,mTalkerEntry=0,mLastTextID=0,mTextTimer=0,mEventPhase=0,pathId=101390;ObjectGuid mTextGUID,mLastInvoker;bool mUseTextTimer=false,playerAvailable=true;
    unsigned castAttempts=0,despawns=0,summons=0,chests=0,credits=0;
    explicit SmartScript(uint32 entry){actor.entry=entry;actor.ai.script=this;}
    Unit* GetBaseObject(){return me;}Unit* GetLastInvoker(){return lastInvoker;}uint32 GetPathId()const{return pathId;}
    bool IsInPhase(uint32)const;
    void SetPhase(uint32);
    ObjectList* GetTargets(SmartScriptHolder const& e,Unit*) {
        if(e.targetType==1)return new ObjectList{me};
        if(e.targetType==19&&visual&&visual->alive)return new ObjectList{visual};
        if(e.targetType==21&&playerAvailable)return new ObjectList{&player};
        return nullptr;
    }
    SmartScriptHolder FindLinkedEvent(uint32);
    void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
    void RecalcTimer(SmartScriptHolder&,uint32,uint32);void InitTimer(SmartScriptHolder&);void UpdateTimer(SmartScriptHolder&,uint32);
    void Tick(uint32 diff);
    void ResetBaseObject(){}
    void OnReset();
    unsigned Count(uint32 action)const{unsigned n=0;for(auto c:calls)if(c.action==action)++n;return n;}
};
void AI::SetData(uint32 field,uint32 data){if(script)script->ProcessEventsFor(SMART_EVENT_DATA_SET,nullptr,field,data);}
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_DEBUG(...) ((void)0)
#define ENSURE_AI(T,p) (p)
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
    switch(e.GetEventType()) {
#include "LinkEvent.inc"
#include "WaypointEvent.inc"
#include "DataSetEvent.inc"
#include "TextOverEvent.inc"
#include "NoParamEvents.inc"
#include "ICEvent.inc"
    }
}
#include "ActionPrefix.inc"
    if(unit)lastInvoker=unit;
    calls.push_back({now,e.actionType,e.parameter});
    switch(e.GetActionType()) {
#include "TalkAction.inc"
#include "CastAction.inc"
#include "DataAction.inc"
#include "PhaseAction.inc"
        case 19: me->flags&=~e.parameter;break;
        case 49: if(playerAvailable)me->combat=true;break;
        case 41: me->alive=false;++despawns;break;
        case 12: ++summons;break;
        case 50: ++chests;break;
        case 205: ++credits;break;
    }
    if(e.actionType==11)++castAttempts;
#include "LinkTail.inc"
#include "InitTimer.inc"
#include "UpdateTimer.inc"
#include "PhaseMethods.inc"
#include "Reset.inc"
void SmartScript::Tick(uint32 diff)
{
    now+=diff;
    for(auto& e:mEvents)UpdateTimer(e,diff);
#include "TextTimer.inc"
static std::vector<SmartScriptHolder> Read(char const* path,uint32 entry)
{
    std::ifstream file(path);std::string line;std::vector<SmartScriptHolder> result;
    while(std::getline(file,line)) {
        std::stringstream stream(line);std::string field;std::vector<std::string> f;
        while(std::getline(stream,field,'\t'))f.push_back(field);
        if(f.size()!=31)return {};
        auto n=[&](size_t i){return uint32(std::stoul(f[i]));};
        if(n(0)!=entry)continue;
        SmartScriptHolder e;e.entryOrGuid=n(0);e.source_type=n(1);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
        e.event.textOver={n(9),n(10)};e.event.waypoint={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};
        e.actionType=n(14);e.parameter=n(15);e.action.talk={n(15),n(16)};e.action.cast={n(15),n(16)};e.action.setData={n(15),n(16)};e.action.setEventPhase={n(15)};e.targetType=n(21);e.targetEntry=n(22);
        result.push_back(e);
    }
    return result;
}
static void Load(SmartScript& s,std::vector<SmartScriptHolder> const& rows){s.mEvents=rows;for(auto& e:s.mEvents)s.InitTimer(e);}
int main(int argc,char** argv)
{
    if(argc!=4)return 1;
    auto old=Read(argv[1],101390),fixed=Read(argv[2],101390),visualRows=Read(argv[2],105331);
    if(old.size()!=11||fixed.size()!=13||visualRows.size()!=1)return 2;
    std::ifstream ruleFile(argv[3]);std::string line;
    while(std::getline(ruleFile,line)) {
        std::stringstream stream(line);std::string field;std::vector<std::string> f;
        while(std::getline(stream,field,'\t'))f.push_back(field);
        if(f.size()!=14)return 3;
        auto n=[&](size_t i){return uint32(std::stoul(f[i]));};
        if(n(0)!=22||n(3)||n(4)||n(6)!=1)return 4;
        conditions.rules.push_back({n(2),n(1),n(6),n(5),n(7),n(8),n(10)});
    }
    if(conditions.rules.size()!=11)return 5;
    {   // Expected failing old graph, not an ordinary successful run.
        SmartScript boss(101390),claws(105331);Load(boss,old);Load(claws,visualRows);boss.visual=&claws.actor;
        boss.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);
        if(boss.errors!=1||claws.despawns||boss.actor.flags)return 6;
        boss.Tick(5000);if(boss.Count(49))return 7;
        boss.Tick(1);boss.Tick(1);if(claws.despawns!=1)return 8;
    }
    for(auto failure:{SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS}) {
        SmartScript before(101390);Load(before,old);before.actor.castResult=failure;
        before.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);
        if(failure==SPELL_FAILED_BAD_TARGETS){before.Tick(1000);if(before.Count(1)||before.actor.casts!=1)return 9;}
        else if(before.actor.aura||before.Count(1)!=1||before.actor.flags!=0)return 10;
    }
    Player player;Unit pet,guardian;
    for(Unit* killer:{static_cast<Unit*>(nullptr),static_cast<Unit*>(&player),&pet,&guardian}) {
        SmartScript boss(101390),claws(105331);Load(boss,fixed);Load(claws,visualRows);boss.visual=&claws.actor;
        boss.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,killer,2,101390);
        if(boss.mEventPhase!=1||boss.actor.casts||boss.Count(1)||claws.despawns||boss.actor.flags!=768)return 11;
        boss.Tick(1);
        if(boss.mEventPhase!=2||!boss.actor.aura||boss.Count(1)!=1||boss.Count(19)!=1||claws.despawns!=1||boss.errors)return 12;
        boss.Tick(4999);if(boss.Count(49))return 13;
        boss.Tick(1);if(boss.Count(49)!=1)return 14;
        boss.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);
        if(boss.Count(1)!=1||claws.despawns!=1)return 15;
        boss.ProcessEventsFor(SMART_EVENT_DEATH,killer);
        if(boss.credits!=1||boss.summons!=1||boss.chests!=1||boss.errors)return 16;
        // A fresh death visual must not inherit the old independent IC cleanup.
        SmartScript rewardVisual(105331);Load(rewardVisual,visualRows);boss.visual=&rewardVisual.actor;boss.actor.combat=true;boss.Tick(1);
        for(unsigned i=0;i<100;++i)boss.Tick(501);
        if(rewardVisual.despawns||boss.credits!=1||boss.summons!=1||boss.chests!=1)return 17;
    }
    for(auto failure:{SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS}) {
        SmartScript boss(101390),claws(105331);Load(boss,fixed);Load(claws,visualRows);boss.visual=&claws.actor;boss.actor.castResult=failure;
        boss.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);boss.Tick(1);
        for(unsigned i=0;i<4;++i)boss.Tick(501);
        if(boss.actor.casts!=5||boss.actor.aura||boss.Count(1)||claws.despawns||boss.actor.flags!=768||boss.errors)return 18;
        boss.actor.castResult=SPELL_CAST_OK;boss.Tick(501);
        if(boss.actor.casts!=6||boss.Count(1)!=1||boss.mEventPhase!=2||claws.despawns!=1||boss.actor.flags)return 19;
    }
    {   // Accepted cast is insufficient until its real effect arrives.
        SmartScript boss(101390);Load(boss,fixed);boss.actor.applyAura=false;
        boss.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);boss.Tick(1);boss.actor.casting=true;boss.Tick(5001);
        if(boss.actor.casts!=1||boss.Count(1)||boss.actor.flags!=768)return 20;
        boss.actor.aura=true;boss.actor.casting=false;boss.Tick(101);
        if(boss.actor.casts!=1||boss.Count(1)!=1||boss.actor.flags)return 21;
    }
    SmartScript missing(101390);Load(missing,fixed);missing.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);
    missing.Tick(1);if(missing.errors||missing.credits||missing.chests||missing.Count(19)!=1)return 22;
    for(unsigned mode=0;mode<12;++mode) {
        SmartScript s(101390);Load(s,fixed);conditions.allow=mode!=0;s.actor.expired=mode==1;
        if(mode==2)s.actor.mapId=1;if(mode==3)s.actor.instanced=false;if(mode==4)s.actor.map.instanceId=0;
        if(mode==5)scenario.id=991;if(mode==6)scenario.step=7;if(mode==7)s.actor.alive=false;if(mode==8)s.me=nullptr;
        if(mode==9)s.actor.map.instanceMap=false;if(mode==10)scenarioManager.available=false;if(mode==11)s.pathId=999;
        s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);s.Tick(1000);
        if(s.actor.casts||s.Count(1)||s.Count(19)||s.credits||s.chests||s.errors)return 23;
        scenario.id=990;scenario.step=6;scenarioManager.available=true;
    }
    conditions.allow=true;
    {
        SmartScript s(101390);Load(s,fixed);s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,1,101390);s.Tick(1000);
        if(s.actor.casts||s.Count(1))return 24;
        s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);s.actor.combat=true;s.Tick(1000);
        if(s.actor.casts||s.Count(1))return 25;s.actor.combat=false;s.Tick(501);if(s.Count(1)!=1)return 26;
    }
    {
        SmartScript s(101390);Load(s,fixed);s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);s.OnReset();s.Tick(1000);
        if(s.mEventPhase||s.actor.casts||s.Count(1))return 27;
        s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);s.Tick(1);if(s.Count(1)!=1)return 28;
    }
    for(uint32 step=0;step<11;++step){if(step==6)continue;SmartScript s(101390);Load(s,fixed);scenario.step=step;s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);s.Tick(1000);if(s.actor.casts||s.Count(1))return 29;}scenario.step=6;
    {
        SmartScript s(101390);Load(s,fixed);s.actor.aura=true;s.Tick(1000);if(s.Count(1)||s.actor.casts)return 30;
        s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);s.Tick(1);if(s.Count(1)!=1||s.actor.casts)return 31;
    }
    {
        SmartScript s(101390);Load(s,fixed);s.actor.castResult=SPELL_FAILED_BAD_TARGETS;
        s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);s.Tick(1);scenario.step=7;s.actor.castResult=SPELL_CAST_OK;s.Tick(1000);
        if(s.actor.casts!=1||s.Count(1)||s.credits||s.chests)return 32;
        scenario.step=6;s.Tick(501);if(s.Count(1)!=1)return 33;
    }
    {
        SmartScript s(101390);Load(s,fixed);s.playerAvailable=false;
        s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2,101390);s.Tick(1);s.Tick(5000);
        if(s.Count(49)!=1||s.actor.combat||s.credits||s.chests||s.summons)return 34;
    }
    std::puts("PASS native old LINK/one-shot failure/false SPELL_IN_PROGRESS counterfactuals; scoped pending cast retries and actual-aura acknowledgement,5s text/native cleanup, four invokers/death requests/reward isolation, deferred effect/combat/missing visual/foreign context/reset/fresh WP gates; actual client traversal remains separate");
}
