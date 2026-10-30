#include <algorithm>
#include <limits>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint32 = uint32_t;
using uint64 = uint64_t;
using DamageEffectType = uint32;
enum { EVENT_1=1, UNIT_FIELD_FLAGS=1, UNIT_FLAG_IMMUNE_TO_NPC=1, UNIT_FLAG_IMMUNE_TO_PC=2,
    UNIT_FLAG_NOT_ATTACKABLE_1=4, UNIT_FLAG_NON_ATTACKABLE=8 };
struct Unit;
struct CreatureAI
{
    virtual ~CreatureAI() = default;
    virtual void DamageTaken(Unit*,uint32&,DamageEffectType) { }
    virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType) { }
};
struct Unit { uint32 kind=0; uint32 Deal(struct Creature*,uint32); };
struct InstanceScript { void OnCreatureDamageTaken(struct Creature*,Unit*,uint32&) { } };
struct Creature
{
    uint64 health=100000, maximum=100000, targetHealth=1000;
    float multiplier=0.01f;
    bool alive=true;
    uint32 flags=0, faction=14, heals=0, clears=0, homes=0;
    CreatureAI* primary=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    bool IsAIEnabled=true;
    CreatureAI* AI() { return primary; }
    Creature* ToCreature() { return this; }
    InstanceScript* GetInstanceScript() { return nullptr; }
    uint64 GetHealth() const { return health; }
    uint64 GetHealth(Unit* attacker) const { return attacker ? targetHealth : health; }
    uint64 GetMaxHealth() const { return maximum; }
    bool HealthBelowPct(uint32 percent) const { return health*100 < maximum*percent; }
    float GetHealthMultiplierForTarget(Unit*) const { return multiplier; }
    void SetHealth(uint64 value) { health=value; ++heals; }
    void RemoveAllAuras() { ++clears; }
    uint32 GetPosition() const { return 7; }
    void SetHomePosition(uint32 value) { assert(value==7); ++homes; }
    void setFaction(uint32 value) { faction=value; }
    void SetFlag(uint32 field,uint32 value) { assert(field==UNIT_FIELD_FLAGS); flags|=value; }
};
struct EventMap
{
    std::map<uint32,uint32> events;
    uint32 resets=0, schedules=0;
    void Reset() { ++resets; events.clear(); }
    void RescheduleEvent(uint32 identity,uint32 delay) { ++schedules; events[identity]=delay; }
};
struct Yrgrim : CreatureAI
{
    Creature* me;
    bool fightComplete=false;
    EventMap events;
    uint32 speech=0, evades=0;
    void Talk(uint32 text) { assert(text==2 && fightComplete); ++speech; }
    void EnterEvadeMode() { assert(fightComplete); ++evades; }
#include "Surrender.inc"
};
uint32 Unit::Deal(Creature* victim,uint32 damage)
{
    DamageEffectType damagetype=0;
    if(victim->primary) victim->primary->DamageTaken(this,damage,damagetype);
    if(victim->i_disabledAI && victim->i_disabledAI!=victim->primary) victim->i_disabledAI->DamageTaken(this,damage,damagetype);
    uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
    if(health<=damage) victim->alive=false;
    return damage;
}
int main()
{
#ifdef BASELINE
    Creature actor; Unit attacker; Yrgrim ai{ }; ai.me=&actor; actor.primary=&ai;
    assert(attacker.Deal(&actor,10)==1000 && !actor.alive && !ai.fightComplete);
    Creature early; early.multiplier=100; early.targetHealth=10000000;
    Yrgrim earlyAI{ }; earlyAI.me=&early; early.primary=&earlyAI;
    assert(attacker.Deal(&early,200000)==0 && earlyAI.fightComplete);
    std::cout << "PASS baseline: pinnedb81a0ede raw10/scale0.01 bypasses surrender and kills; raw200000/scale100 prematurely surrenders despite final2000 below target health10000000. Native extracted final scaling/primary-suspended hooks, controlled actor services.\n";
#else
    for(uint32 kind : {0u,1u,2u,3u})
        for(float scale : {0.001f,0.01f,1.0f,100.0f})
            for(bool suspended : {false,true})
            {
                Creature actor; actor.multiplier=scale;
                Unit attacker; attacker.kind=kind;
                Yrgrim ai{ }; ai.me=&actor; CreatureAI temporary;
                actor.primary=suspended?&temporary:&ai; actor.i_disabledAI=suspended?&ai:nullptr;
                ai.events.events[7]=9000;
                uint32 raw=uint32(2000*scale);
                assert(attacker.Deal(&actor,raw)==0 && actor.alive && ai.fightComplete);
                assert(actor.heals==1 && actor.clears==1 && actor.homes==1 && actor.faction==35 && actor.flags==15);
                std::map<uint32,uint32> expectedEvents{{EVENT_1,3000}};
                assert(ai.speech==1 && ai.evades==1 && ai.events.resets==1 && ai.events.events==expectedEvents);
                attacker.Deal(&actor,raw);
                assert(ai.speech==1 && actor.heals==1 && ai.events.schedules==1);
            }
    for(uint32 kind : {0u,1u,2u,3u})
        for(bool below : {false,true})
        {
            Creature actor; actor.multiplier=100; actor.targetHealth=10000000;
            Unit attacker; attacker.kind=kind;
            Yrgrim ai{ }; ai.me=&actor; actor.primary=&ai;
            if(below) actor.health=9999;
            uint32 final=attacker.Deal(&actor,200000);
            assert(actor.alive && final==(below?0u:2000u) && ai.fightComplete==below);
        }
    for(uint64 current : {10000ull,9999ull})
    {
        Creature actor; actor.health=current; actor.multiplier=1;
        Unit attacker; Yrgrim ai{ }; ai.me=&actor; actor.primary=&ai;
        assert(attacker.Deal(&actor,1)==(current==9999?0u:1u));
        assert(ai.fightComplete==(current==9999));
    }
    for(uint32 finalDamage : {999u,1000u})
    {
        Creature actor; actor.multiplier=1;
        Unit attacker; Yrgrim ai{ }; ai.me=&actor; actor.primary=&ai;
        assert(attacker.Deal(&actor,finalDamage)==(finalDamage==1000?0u:999u));
        assert(actor.alive && ai.fightComplete==(finalDamage==1000));
    }
    Creature actor; Yrgrim ai{ }; ai.me=&actor; uint32 damage=100000;
    ai.DamageTakenAfterScaling(nullptr,damage,0); assert(!damage && ai.fightComplete);
    std::cout << "PASS:32 killer-label/scale/primary-suspended lethal cases surrender once before native death;8 high-scale nonlethal/low-health cases retain ordinary damage or existing strict10pct threshold. Exact lethal/10pct boundaries and null attacker safe. Original heal/aura/home/faction/Talk2/evade/3000ms event1 retained; no progression from this handler. Health, scheduler and killer types controlled, not full instance/group/client combat.\n";
#endif
}
