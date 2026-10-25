#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
using uint8=uint8_t;
using DamageEffectType=uint32;
enum { UNIT_STATE_STUNNED=8, UNIT_MOD_HEALTH=0, TOTAL_PCT=0 };
template<typename ValueType,typename PercentType> ValueType CalculatePct(ValueType base,PercentType percent){return ValueType(base*static_cast<float>(percent)/100.0f);}
struct WorldObject { virtual ~WorldObject()=default; };
struct Unit;
struct Creature;
struct CreatureAI
{
    virtual ~CreatureAI()=default;
    virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
    virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}
    virtual void HealReceived(Unit*,uint32&){}
};
struct Unit : WorldObject
{
    uint64 currentHealth=64000,maximum=64000;
    uint32 kind=0;
    virtual Creature const* ToCreature() const {return nullptr;}
    uint64 GetHealth() const {return currentHealth;}
    uint64 GetHealth(Unit*) const;
    float GetHealthPct() const {return 100.0f*float(currentHealth)/float(maximum);}
    uint8 GetLevelForTarget(WorldObject const*) const {return 100;}
    float GetModifierValue(uint32 unitMod,uint32 modifier) const {assert(unitMod==UNIT_MOD_HEALTH && modifier==TOTAL_PCT);return 1.0f;}
    uint32 Deal(Creature*,uint32);
};
struct CreatureLevelStat {uint64 healthMax=1000;};
struct InstanceScript {void OnCreatureDamageTaken(Creature*,Unit*,uint32&) {}};
struct Creature : Unit
{
    bool scalable=true,statPresent=true,alive=true,auraSucceeds=true,IsAIEnabled=true;
    uint32 auraAttempts=0,auraRemovals=0,states=0;
    CreatureLevelStat stat;
    CreatureAI* primary=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    Creature const* ToCreature() const override {return this;}
    Creature* ToCreature() {return this;}
    CreatureAI* AI(){return primary;}
    InstanceScript* GetInstanceScript(){return nullptr;}
    bool HasScalableLevels() const {return scalable;}
    uint8 getLevel() const {return 110;}
    CreatureLevelStat const* GetScaleLevelStat(uint8 level){assert(level==100);return statPresent?&stat:nullptr;}
    uint64 GetMaxHealthByLevel(uint8 level) const {assert(level==100);return stat.healthMax;}
    uint64 GetCreateHealth() const {return maximum;}
    float GetHealthMultiplierForTarget(WorldObject const*) const;
    bool AddAura(uint32 spell,Creature* target){assert(spell==210985 && target==this);++auraAttempts;return auraSucceeds;}
    void RemoveAura(uint32 spell){assert(spell==210985);++auraRemovals;}
    void AddUnitState(uint32 state){assert(state==UNIT_STATE_STUNNED);states|=state;}
    void ClearUnitState(uint32 state){assert(state==UNIT_STATE_STUNNED);states&=~state;}
};
#include "Health.inc"
#include "Multiplier.inc"
struct Companion : CreatureAI
{
    Creature* me=nullptr;
    bool wait_heal=false;
    std::vector<uint32> speech;
    void Talk(uint32 text){speech.push_back(text);}
};
struct First : Companion
{
#include "FirstDamage.inc"
#include "FirstHeal.inc"
};
struct Second : Companion
{
#include "SecondDamage.inc"
#include "SecondHeal.inc"
};
struct Third : Companion
{
#include "ThirdDamage.inc"
#include "ThirdHeal.inc"
};
struct Allies : Companion
{
#include "AlliesDamage.inc"
#include "AlliesHeal.inc"
};
uint32 Unit::Deal(Creature* victim,uint32 damage)
{
    DamageEffectType damagetype=0;
    if(victim->IsAIEnabled && victim->AI())victim->AI()->DamageTaken(this,damage,damagetype);
    uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
    if(health<=damage)victim->alive=false;
    return damage;
}
template<typename ActorAI> uint32 Check(std::vector<uint32> expectedSpeech)
{
    uint32 cases=0;
    for(uint32 kind : {0u,1u,2u,3u})
        for(float scale : {0.015625f,0.25f,1.0f,16.0f})
            for(uint32 mode : {0u,1u,2u,3u})
                for(uint32 intensity : {0u,1u,2u})
                    for(bool auraSucceeds : {false,true})
                    {
                        Creature actor;actor.stat.healthMax=uint64(actor.maximum*scale);actor.auraSucceeds=auraSucceeds;
                        Unit attacker;attacker.kind=kind;ActorAI companion;companion.me=&actor;CreatureAI temporary;
                        actor.primary=mode==1?&temporary:mode==3?nullptr:&companion;actor.i_disabledAI=mode==0?nullptr:&companion;
                        uint32 raw=uint32(std::ceil(actor.GetHealth(&attacker)*scale*intensity/2.0f));
                        uint32 finalDamage=uint32(raw/actor.GetHealthMultiplierForTarget(&attacker));
                        bool lethal=finalDamage>=actor.GetHealth(&attacker);
#ifdef SILVER_HAND_ALLY_BASELINE
                        bool rawClaim=(mode==0 || mode==2) && raw>=actor.GetHealth();
                        uint32 result=attacker.Deal(&actor,raw);
                        assert(companion.wait_heal==rawClaim && result==(rawClaim?0u:finalDamage));
                        assert(actor.alive==(rawClaim || !lethal));
#else
                        assert(attacker.Deal(&actor,raw)==(lethal?0u:finalDamage) && actor.alive);
                        assert(companion.wait_heal==lethal && actor.auraAttempts==uint32(lethal));
                        assert(companion.speech==(lethal?expectedSpeech:std::vector<uint32>{}));
                        if(lethal)
                        {
                            assert(actor.states==UNIT_STATE_STUNNED);
                            assert(attacker.Deal(&actor,raw)==0 && actor.auraAttempts==1 && companion.speech==expectedSpeech);
                            uint32 incomingHeal=1000000;actor.currentHealth=24960;companion.HealReceived(&attacker,incomingHeal);
                            assert(incomingHeal==1000000 && companion.wait_heal && actor.states==UNIT_STATE_STUNNED);
                            actor.currentHealth=25600;companion.HealReceived(&attacker,incomingHeal);
                            assert(!companion.wait_heal && !actor.states && actor.auraRemovals==1);
                            actor.currentHealth=actor.maximum;assert(attacker.Deal(&actor,raw)==0 && actor.auraAttempts==2);
                            assert(companion.speech.size()==expectedSpeech.size()*2);
                        }
#endif
                        ++cases;
                    }
    ActorAI missing;Creature actor;missing.me=&actor;uint32 damage=64000;
#ifndef SILVER_HAND_ALLY_BASELINE
    missing.DamageTakenAfterScaling(nullptr,damage,0);assert(!damage && missing.wait_heal && actor.auraAttempts==1);
#else
    (void)damage;(void)expectedSpeech;
#endif
    return cases;
}
int main()
{
    _set_error_mode(_OUT_TO_STDERR);
    uint32 cases=Check<First>({9,10})+Check<Second>({1,2})+Check<Third>({2,3})+Check<Allies>({0,1});
    Unit attacker;Creature fallback;fallback.scalable=false;assert(fallback.GetHealth(&attacker)==64000 && fallback.GetHealthMultiplierForTarget(&attacker)==1);
    fallback.scalable=true;fallback.statPresent=false;assert(fallback.GetHealth(&attacker)==64000);
    fallback.statPresent=true;fallback.currentHealth=1;fallback.stat.healthMax=1;assert(fallback.GetHealth(&attacker)==1);
    std::cout<<"PASS: "<<cases<<" four native ally guard/heal, native target-health/multiplier and post-scaling-dispatch cases; ";
#ifdef SILVER_HAND_ALLY_BASELINE
    std::cout<<"pinnedd49e77d2 raw guard loses scaled lethal hits and can wound prematurely on nonlethal high-scale hits.\n";
#else
    std::cout<<"final lethal damage zeroes before death, wounds once, preserves exact speech/stun/aura and current pre-heal40pct release/re-wound behavior. Primary/suspended/alias and killer labels controlled; no credit, actual pet/group/client/VFX proof. Failed aura presentation remains OPEN.\n";
#endif
}
