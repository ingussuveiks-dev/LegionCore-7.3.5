#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
using uint32=uint32_t;
using uint64=uint64_t;
using DamageEffectType=uint32;
enum {SMART_EVENT_DAMAGED=9,SMART_EVENT_HEALTH_PCT=2,SMART_EVENT_FLAG_ALLOW_EVENT_IN_COMBAT=32,SMART_ACTION_DIE=37};
#define TC_LOG_INFO(...) ((void)0)
#define TC_LOG_DEBUG(...) ((void)0)
struct Unit;
struct Creature;
struct CreatureAI {
    virtual ~CreatureAI()=default;
    virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
    virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}
};
struct Unit {uint32 kind=0;uint32 Deal(Creature*,uint32);};
struct InstanceScript {void OnCreatureDamageTaken(Creature*,Unit*,uint32&) {}};
struct Creature {
    uint32 entry=106730,map=1609,instance=42;
    uint64 health=100000,targetHealth=1000,maximum=100000;
    float multiplier=0.001f;
    bool alive=true,combat=true,IsAIEnabled=true;
    CreatureAI* primary=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    CreatureAI* AI(){return primary;}
    uint32 GetEntry() const{return entry;}
    uint32 GetMapId() const{return map;}
    uint32 GetInstanceId() const{return instance;}
    bool IsAlive() const{return alive;}
    bool isDead() const{return !alive;}
    bool isInCombat() const{return combat;}
    uint64 GetHealth() const{return health;}
    uint64 GetHealth(Unit*) const{return targetHealth;}
    uint64 GetMaxHealth() const{return maximum;}
    float GetHealthPct() const{return 100.0f*health/maximum;}
    void SetHealth(uint64 value){health=value;targetHealth=value;}
    float GetHealthMultiplierForTarget(Unit*) const{return multiplier;}
    Creature* ToCreature(){return this;}
    InstanceScript* GetInstanceScript(){return nullptr;}
    void Kill(Creature* target){assert(target==this);alive=false;health=targetHealth=0;}
};
struct Scenario {uint32 identity=1099;uint32 GetScenarioId() const{return identity;}};
struct ScenarioMgr {bool present=true;Scenario scenario;Scenario* GetScenario(uint32 identity){return present&&identity==42?&scenario:nullptr;}} scenarios;
auto* sScenarioMgr=&scenarios;
struct SmartAI:CreatureAI {
    Creature* me;uint32 mInvincibilityHpLevel=1,damagedEvents=0;
    explicit SmartAI(Creature* actor):me(actor){}
    SmartAI* GetScript(){return this;}
    void ProcessEventsFor(uint32 event,Unit*,uint32){assert(event==SMART_EVENT_DAMAGED);++damagedEvents;}
    void DamageTaken(Unit*,uint32&,DamageEffectType) override;
};
#include "SmartDamage.inc"
#include "Protection.inc"
#ifdef BASELINE
using npc_titanstrike_final_prustaga=SmartAI;
#else
#include "Wrapper.inc"
#endif
uint32 Unit::Deal(Creature* victim,uint32 damage){
    DamageEffectType damagetype=0;
    if(victim->IsAIEnabled&&victim->AI())victim->AI()->DamageTaken(this,damage,damagetype);
    uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
    if(health<=damage)victim->alive=false;
    return damage;
}
struct HealthEvent {
    struct Event {uint32 event_flags=1;struct Range {uint32 min=0,max=1,repeatMin=0,repeatMax=0;} minMaxRepeat;} event;
    bool runOnce=false;
};
struct HealthProbe {
    Creature* me;uint32 starts=0;HealthEvent event;
    void RecalcTimer(HealthEvent&,uint32,uint32){}
    void ProcessAction(HealthEvent& holder){holder.runOnce=true;++starts;}
    void Tick(){
        if(event.runOnce)return;
        auto& e=event;
        switch(SMART_EVENT_HEALTH_PCT){
#include "HealthEvent.inc"
        }
    }
    void ScriptedDeath(){
        switch(SMART_ACTION_DIE){
#include "DieAction.inc"
        }
    }
};
int main(){
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    for(uint32 kind:{0u,1u,2u,3u})for(float scale:{0.001f,0.01f,1.0f,100.0f})for(bool charmed:{false,true}){
        Creature actor;actor.multiplier=scale;Unit attacker;attacker.kind=kind;
        npc_titanstrike_final_prustaga content(&actor);CreatureAI charm;
        actor.primary=charmed?&charm:&content;actor.i_disabledAI=charmed?&content:nullptr;
        HealthProbe probe{&actor};probe.Tick();assert(!probe.starts);
        uint32 raw=scale<1.0f?2:200000;
        if(scale==0.01f)raw=20;
        assert(attacker.Deal(&actor,raw)==0&&actor.alive&&actor.health==1);
        probe.Tick();probe.Tick();assert(probe.starts==1);
        assert(attacker.Deal(&actor,200000)==0&&actor.alive);
        probe.ScriptedDeath();assert(!actor.alive);
    }
    {
        Creature actor;Unit attacker;npc_titanstrike_final_prustaga content(&actor);actor.primary=&content;content.mInvincibilityHpLevel=0;
        assert(attacker.Deal(&actor,200000)==0&&actor.health==1&&actor.alive);
    }
    for(uint32 failure=0;failure<7;++failure){
        Creature actor;Unit attacker;scenarios={};uint32 damage=2000;
        switch(failure){case 0:actor.entry=104949;break;case 1:actor.map=1495;break;case 2:actor.alive=false;break;
            case 3:scenarios.present=false;break;case 4:scenarios.scenario.identity=1068;break;case 5:actor.instance=43;break;case 6:damage=0;break;}
        uint32 before=damage;TitanstrikePrustaga::Protect(&actor,&attacker,damage);assert(damage==before&&actor.health==100000);
    }
    scenarios={};Creature actor;Unit attacker;uint32 damage=2;
    TitanstrikePrustaga::Protect(&actor,nullptr,damage);assert(damage==2);
    TitanstrikePrustaga::Protect(nullptr,&attacker,damage);assert(damage==2);
    actor.primary=new npc_titanstrike_final_prustaga(&actor);actor.multiplier=1;
    assert(attacker.Deal(&actor,2)==2&&actor.health==100000);delete actor.primary;
    std::cout<<"PASS: production Prustaga wrapper, native SmartAI raw hook and Unit post-scaling/disabled-AI bridge preserve one-shot surrender for player/pet/guardian/NPC; native health predicate remains reachable once in the controlled scheduler, direct scripted death still works. Shared IDs/maps/scenarios and ordinary nonlethal hits are isolated.\n";
}
