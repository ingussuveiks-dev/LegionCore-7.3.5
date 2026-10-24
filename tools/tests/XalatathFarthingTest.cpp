#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
using uint32=uint32_t;
using uint64=uint64_t;
using ObjectGuid=uint32;
using DamageEffectType=int;
#ifdef STROMKAR_ZAKAJZ_TEST
constexpr uint32 actorEntry=104276,scenarioIdentity=1037,firstStage=8,lastStage=9;
#define npc_xalatath_farthing npc_stromkar_zakajz
#define XalatathFarthing StromkarZakajz
#else
constexpr uint32 actorEntry=101148,scenarioIdentity=991,firstStage=7,lastStage=8;
#endif
enum { SMART_EVENT_DAMAGED=32, SMART_EVENT_HEALTH_PCT=2, SMART_ACTION_DIE=37, SMART_EVENT_FLAG_ALLOW_EVENT_IN_COMBAT=0x020 };
uint32 messages=0;
#define TC_LOG_INFO(...) (++messages)
#define TC_LOG_DEBUG(...) ((void)0)
struct Unit;
struct Creature;
struct CreatureAI {
    virtual ~CreatureAI()=default;
    virtual void Reset() { }
    virtual void JustSummoned(Creature*) { }
    virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType) { }
};
struct InstanceScript { void OnCreatureDamageTaken(Creature*,Unit*,uint32&) { } };
struct Unit { uint32 kind=0;uint32 Deal(Creature*,uint32); };
struct Creature {
    uint32 entry=actorEntry,map=1539,instance=42;
    uint64 hp=100000,targetHp=1000;
    float multiplier=0.001f;
    bool alive=true;
    bool combat=true;
    bool inWorld=true,phase=true;
    ObjectGuid guid=1;
    CreatureAI* active=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    Creature* ToCreature() { return this; }
    InstanceScript* GetInstanceScript() { return nullptr; }
    CreatureAI* AI() { return active; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return map; }
    uint32 GetInstanceId() const { return instance; }
    ObjectGuid GetGUID() const { return guid; }
    bool IsInWorld() const { return inWorld; }
    bool InSamePhase(Creature const*) const { return phase; }
    uint64 GetHealth(Unit*) const { return targetHp; }
    uint64 GetHealth() const { return hp; }
    uint64 GetMaxHealth() const { return 100000; }
    float GetHealthPct() const { return 100.0f*float(hp)/float(GetMaxHealth()); }
    bool isInCombat() const { return combat; }
    float GetHealthMultiplierForTarget(Unit*) const { return multiplier; }
    bool IsAlive() const { return alive; }
    bool isDead() const { return !alive; }
    void SetHealth(uint64 value) { hp=targetHp=value; }
    void Kill(Creature* target) { assert(target==this);alive=false; }
};
#ifdef STROMKAR_ZAKAJZ_TEST
struct ObjectAccessor {
    static Creature* tracked;
    static Creature* GetCreature(Creature& context,ObjectGuid guid) {
        return tracked&&tracked->guid==guid&&tracked->map==context.map&&tracked->instance==context.instance?tracked:nullptr;
    }
};
Creature* ObjectAccessor::tracked=nullptr;
constexpr uint32 CONDITION_SOURCE_TYPE_SMART_EVENT=22,CONDITION_NEAR_CREATURE=29;
struct ConditionSourceInfo;
struct Condition {
    uint32 SourceType=22,SourceEntry=104276,SourceGroup=10,SourceId=0,ConditionType=29,ConditionTarget=1,ConditionValue1=104307,ConditionValue2=50,ConditionValue3=0;
    bool NegativeCondition=true;
    bool Meets(ConditionSourceInfo&,bool);
};
struct ConditionSourceInfo { Creature* mConditionTargets[2]{nullptr,nullptr};Condition* mLastFailedCondition=nullptr; };
struct ConditionScript {
    explicit ConditionScript(char const*) { }
    virtual ~ConditionScript()=default;
    virtual bool OnConditionCheck(Condition*,ConditionSourceInfo&) { return true; }
};
#endif
struct Scenario {
    uint32 identity=scenarioIdentity,step=firstStage;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioMgr {
    bool present=true;
    Scenario scenario;
    Scenario* GetScenario(uint32 instance) { return present && instance==42?&scenario:nullptr; }
} scenarioManager;
auto* sScenarioMgr=&scenarioManager;
struct Script {
    uint32 damageEvents=0;
    void ProcessEventsFor(int eventType,Unit*,uint32) { assert(eventType==SMART_EVENT_DAMAGED);++damageEvents; }
};
struct SmartAI : CreatureAI {
    Creature* me;
    uint32 mInvincibilityHpLevel=1;
    Script script;
    uint32 summonEvents=0;
    explicit SmartAI(Creature* actor):me(actor) { }
    Script* GetScript() { return &script; }
    void Reset() override { }
    void JustSummoned(Creature*) override { ++summonEvents; }
    void DamageTaken(Unit*,uint32&,DamageEffectType);
};
#include "SmartDamage.inc"
#ifndef BASELINE
#include "Farthing.inc"
#else
struct npc_xalatath_farthing : SmartAI { using SmartAI::SmartAI; };
#endif
#if defined(STROMKAR_ZAKAJZ_TEST) && !defined(BASELINE)
struct TestScriptMgr {
    condition_stromkar_thoradin_absence script;
    bool OnConditionCheck(Condition* condition,ConditionSourceInfo& info) { return script.OnConditionCheck(condition,info); }
} testScriptManager;
auto* sScriptMgr=&testScriptManager;
bool Condition::Meets(ConditionSourceInfo& sourceInfo,bool nativeNearby) {
    bool condMeets=nativeNearby;
#include "StromkarConditionTail.inc"
#endif
uint32 Unit::Deal(Creature* victim,uint32 damage) {
    DamageEffectType damagetype=0;
    SmartAI* original=static_cast<SmartAI*>(victim->i_disabledAI?victim->i_disabledAI:victim->AI());
    original->DamageTaken(this,damage,damagetype);
    uint64 health=victim->GetHealth(this);
#include "NativeScaledHook.inc"
    if(health<=damage)victim->alive=false;
    return damage;
}
void NativeDie(Creature* me) {
    switch(SMART_ACTION_DIE) {
#include "NativeDie.inc"
    }
}
struct HealthProbe {
    Creature* me;
    uint32 actions=0;
    struct Holder {
        struct Event {
            uint32 event_flags=1;
            struct Range { uint32 min=0,max=1,repeatMin=0,repeatMax=0; } minMaxRepeat;
        } event;
    };
    void RecalcTimer(Holder&,uint32,uint32) { }
    void ProcessAction(Holder&) { ++actions; }
    void Check(Holder& event) {
        switch(SMART_EVENT_HEALTH_PCT) {
#define e event
#include "HealthEvent.inc"
#undef e
        }
    }
};
int main() {
    _set_abort_behavior(0,_CALL_REPORTFAULT);
#if defined(STROMKAR_ZAKAJZ_TEST) && !defined(BASELINE)
    {
        Creature owner,ghost;ghost.entry=104307;ghost.guid=99;
        npc_stromkar_zakajz content(&owner);owner.active=&content;
        Condition condition;ConditionSourceInfo source;source.mConditionTargets[1]=&owner;
        condition_stromkar_thoradin_absence gate;
        assert(gate.OnConditionCheck(&condition,source));
        Creature unrelated;unrelated.entry=123;content.JustSummoned(&unrelated);assert(!content.HasThoradin()&&content.summonEvents==1);
        ObjectAccessor::tracked=&ghost;content.JustSummoned(&ghost);
        assert(content.HasThoradin()&&!gate.OnConditionCheck(&condition,source)&&content.summonEvents==2);
        assert(!condition.Meets(source,false)&&!condition.Meets(source,true));
        for(uint32 invalid=0;invalid<6;++invalid) {
            ghost.inWorld=ghost.alive=ghost.phase=true;ghost.map=1539;ghost.instance=42;ghost.entry=104307;
            if(invalid==0)ghost.inWorld=false;
            if(invalid==1)ghost.alive=false;
            if(invalid==2)ghost.phase=false;
            if(invalid==3)ghost.map=1;
            if(invalid==4)ghost.instance=43;
            if(invalid==5)ghost.entry=123;
            assert(!content.HasThoradin()&&gate.OnConditionCheck(&condition,source));
            assert(condition.Meets(source,false));
        }
        ObjectAccessor::tracked=nullptr;assert(gate.OnConditionCheck(&condition,source));
        assert(!condition.Meets(source,true));
        owner.active=nullptr;assert(!gate.OnConditionCheck(&condition,source));
        condition.SourceEntry=1;assert(gate.OnConditionCheck(&condition,source));
        std::cout<<"PASS: actual Stromkar summon callback retains native delegate and caches only104307 GUID; actual condition helper suppresses any live resolved same-phase receiver without a distance dependency and retries removed/dead/uninserted/wrong-map/instance/entry actors. Map lookup is a controlled adapter, not real allocation/visibility proof\n";
    }
#endif
    for(uint32 kind:{0u,1u,2u})for(bool suspended:{false,true})for(uint32 stage:{firstStage,lastStage}) {
        scenarioManager={};scenarioManager.scenario.step=stage;messages=0;
        Unit attacker;attacker.kind=kind;Creature actor;CreatureAI temporary;
        npc_xalatath_farthing content(&actor);
        actor.active=suspended?&temporary:&content;actor.i_disabledAI=suspended?&content:nullptr;
        HealthProbe probe{&actor};HealthProbe::Holder event;
        probe.Check(event);assert(probe.actions==0);
        uint32 result=attacker.Deal(&actor,2);
        assert(result==0 && actor.alive && actor.hp==1 && content.script.damageEvents==1 && messages==1);
        actor.combat=false;probe.Check(event);assert(probe.actions==0);actor.combat=true;
        probe.Check(event);assert(probe.actions==1);
        assert(attacker.Deal(&actor,200000)==0 && actor.alive && actor.hp==1 && messages==1);
        NativeDie(&actor);NativeDie(&actor);assert(!actor.alive);
    }
    for(float multiplier:{0.001f,0.01f,1.0f,100.0f})for(uint32 invincibility:{0u,1u}) {
        scenarioManager={};Creature actor;actor.multiplier=multiplier;
        npc_xalatath_farthing content(&actor);actor.active=&content;content.mInvincibilityHpLevel=invincibility;
        Unit attacker;assert(attacker.Deal(&actor,200000)==0 && actor.alive && actor.hp==1);
    }
    for(uint32 damage:{0u,999u,1000u}) {
        scenarioManager={};Creature actor;actor.multiplier=1;
        npc_xalatath_farthing content(&actor);actor.active=&content;content.mInvincibilityHpLevel=0;
        Unit attacker;uint32 result=attacker.Deal(&actor,damage);
        assert(actor.alive && result==(damage==1000?0:damage) && actor.hp==(damage==1000?1:100000));
    }
    for(uint32 invalid=0;invalid<9;++invalid) {
        scenarioManager={};Creature actor;npc_xalatath_farthing content(&actor);actor.active=&content;
        Unit attacker;
        if(invalid==0)actor.entry=123;
        if(invalid==1)actor.map=1533;
        if(invalid==2)actor.instance=43;
        if(invalid==3)scenarioManager.present=false;
        if(invalid==4)scenarioManager.scenario.identity=scenarioIdentity==1037?991:1037;
        if(invalid==5)scenarioManager.scenario.identity=1092;
        if(invalid==6)scenarioManager.scenario.step=firstStage-1;
        if(invalid==7)scenarioManager.scenario.step=lastStage+1;
        if(invalid==8)actor.alive=false;
        uint32 damage=2000;content.DamageTakenAfterScaling(&attacker,damage,0);
        assert(damage==2000 && actor.hp==100000);
    }
#ifndef BASELINE
    Creature actor;Unit attacker;uint32 damage=2000;scenarioManager={};
    XalatathFarthing::Protect(nullptr,&attacker,damage);XalatathFarthing::Protect(&actor,nullptr,damage);
    assert(damage==2000 && actor.hp==100000);
#endif
    std::cout<<"PASS: native SmartAI damage then actual Unit scaling/post-hook, player/pet/guardian identities, active/suspended content AI, initial/repeated lethal and sublethal hits, missing/wrong stage/map/scenario; native DIE remains possible. Native HP event/casts/summon/credit/visuals are separate.\n";
}
