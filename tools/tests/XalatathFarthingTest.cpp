#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
using uint32=uint32_t;
using uint64=uint64_t;
using DamageEffectType=int;
enum { SMART_EVENT_DAMAGED=32, SMART_EVENT_HEALTH_PCT=2, SMART_ACTION_DIE=37, SMART_EVENT_FLAG_ALLOW_EVENT_IN_COMBAT=0x020 };
uint32 messages=0;
#define TC_LOG_INFO(...) (++messages)
#define TC_LOG_DEBUG(...) ((void)0)
struct Unit;
struct Creature;
struct CreatureAI {
    virtual ~CreatureAI()=default;
    virtual void Reset() { }
    virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType) { }
};
struct InstanceScript { void OnCreatureDamageTaken(Creature*,Unit*,uint32&) { } };
struct Unit { uint32 kind=0;uint32 Deal(Creature*,uint32); };
struct Creature {
    uint32 entry=101148,map=1539,instance=42;
    uint64 hp=100000,targetHp=1000;
    float multiplier=0.001f;
    bool alive=true;
    bool combat=true;
    CreatureAI* active=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    Creature* ToCreature() { return this; }
    InstanceScript* GetInstanceScript() { return nullptr; }
    CreatureAI* AI() { return active; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return map; }
    uint32 GetInstanceId() const { return instance; }
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
struct Scenario {
    uint32 identity=991,step=7;
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
    explicit SmartAI(Creature* actor):me(actor) { }
    Script* GetScript() { return &script; }
    void Reset() override { }
    void DamageTaken(Unit*,uint32&,DamageEffectType);
};
#include "SmartDamage.inc"
#ifndef BASELINE
#include "Farthing.inc"
#else
struct npc_xalatath_farthing : SmartAI { using SmartAI::SmartAI; };
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
    for(uint32 kind:{0u,1u,2u})for(bool suspended:{false,true})for(uint32 stage:{7u,8u}) {
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
        if(invalid==4)scenarioManager.scenario.identity=1037;
        if(invalid==5)scenarioManager.scenario.identity=1092;
        if(invalid==6)scenarioManager.scenario.step=6;
        if(invalid==7)scenarioManager.scenario.step=9;
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
