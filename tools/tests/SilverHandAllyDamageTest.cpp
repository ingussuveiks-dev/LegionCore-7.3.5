#include <algorithm>
#include <limits>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
using uint8=uint8_t;
using int32=int32_t;
using int64=int64_t;
using DamageEffectType=uint32;
enum { UNIT_STATE_STUNNED=8, UNIT_MOD_HEALTH=0, TOTAL_PCT=0 };
template<typename ValueType,typename PercentType> ValueType CalculatePct(ValueType base,PercentType percent){return ValueType(base*static_cast<float>(percent)/100.0f);}
struct WorldObject { virtual ~WorldObject()=default; };
struct Unit;
struct Creature;
struct InstanceScript;
struct Player {template<class Packet>void SendDirectMessage(Packet const&) {}};
namespace WorldPackets::Combat {struct HealthUpdate {uint64 Guid=0,Health=0;int Write(){return 0;}};}
struct CreatureAI
{
    virtual ~CreatureAI()=default;
    virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
    virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}
    virtual void HealReceived(Unit*,uint32&){}
    virtual void HealDone(Unit*,uint32&){}
    virtual void Reset(){}
    virtual void UpdateLifecycle(uint32){}
};
struct Unit : WorldObject
{
    uint64 currentHealth=64000,maximum=64000;
    uint32 kind=0;
    bool IsAIEnabled=false;
    CreatureAI* healAI=nullptr;
    virtual Creature const* ToCreature() const {return nullptr;}
    virtual Creature* ToCreature() {return nullptr;}
    uint64 GetHealth() const {return currentHealth;}
    uint64 GetHealth(Unit*) const;
    float GetHealthPct() const {return maximum?100.0f*float(currentHealth)/float(maximum):0;}
    uint64 GetMaxHealth(Unit* =nullptr) const {return maximum;}
    void SetHealth(uint64 value,uint32){currentHealth=value;}
    void SetHealthScal(uint64 value,Unit*,uint32){currentHealth=value;}
    uint64 GetGUID() const{return 0;}
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const{return nullptr;}
    CreatureAI* GetAI();
    InstanceScript* GetInstanceScript(){return nullptr;}
    int64 ModifyHealth(int64,Unit* =nullptr,uint32=0);
    int32 DealHeal(Unit*,uint32);
    uint8 GetLevelForTarget(WorldObject const*) const {return 100;}
    float GetModifierValue(uint32 unitMod,uint32 modifier) const {assert(unitMod==UNIT_MOD_HEALTH && modifier==TOTAL_PCT);return 1.0f;}
    uint32 Deal(Creature*,uint32);
};
struct CreatureLevelStat {uint64 healthMax=1000;};
struct InstanceScript
{
    void OnCreatureDamageTaken(Creature*,Unit*,uint32&) {}
    bool CheckDamageManager() const{return false;}
    void UpdateDamageManager(uint64,int32,bool){}
};
struct Creature : Unit
{
    bool scalable=true,statPresent=true,alive=true,auraSucceeds=true,world=true,preDelete=false,m_AI_locked=false;
    uint32 auraAttempts=0,auraRemovals=0,states=0;
    CreatureLevelStat stat;
    CreatureAI* primary=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    std::function<void()> onRemove;
    Creature(){IsAIEnabled=true;}
    bool IsAlive() const{return alive;}
    bool IsInWorld() const{return world;}
    bool IsPreDelete() const{return preDelete;}
    Creature const* ToCreature() const override {return this;}
    Creature* ToCreature() override {return this;}
    // This fixture certifies wound/lifecycle behavior; dispatch has its own native regression.
    void NotifyHealApplied(Unit*,uint32,int32) {}
    CreatureAI* AI(){return primary;}
    InstanceScript* GetInstanceScript(){return nullptr;}
    bool HasScalableLevels() const {return scalable;}
    uint8 getLevel() const {return 110;}
    CreatureLevelStat const* GetScaleLevelStat(uint8 level){assert(level==100);return statPresent?&stat:nullptr;}
    uint64 GetMaxHealthByLevel(uint8 level) const {assert(level==100);return stat.healthMax;}
    uint64 GetCreateHealth() const {return maximum;}
    float GetHealthMultiplierForTarget(WorldObject const*) const;
    bool AddAura(uint32 spell,Creature* target){assert(spell==210985 && target==this);++auraAttempts;return auraSucceeds;}
    void RemoveAura(uint32 spell){assert(spell==210985);++auraRemovals;if(onRemove)onRemove();}
    void AddUnitState(uint32 state){assert(state==UNIT_STATE_STUNNED);states|=state;}
    void ClearUnitState(uint32 state){assert(state==UNIT_STATE_STUNNED);states&=~state;}
    void TickLifecycle(uint32 diff)
    {
        CreatureAI* i_AI=primary;
#include "LifecycleBridge.inc"
    }
};
CreatureAI* Unit::GetAI(){auto creature=const_cast<Creature*>(ToCreature());return creature?creature->AI():healAI;}
#include "ModifyHealth.inc"
#include "DealHealPrefix.inc"
#include "Health.inc"
#include "Multiplier.inc"
#include "Recovery.inc"
struct EventMap {void Reset(){}};
struct FunctionProcessor {void Update(uint32){}};
struct Companion : CreatureAI
{
    Creature* me=nullptr;
    bool wait_heal=false;
    bool _pendingHealRecovery=false;
    EventMap events;
    FunctionProcessor _entryFunctions;
    uint64 _entryGeneration=0;
    bool _entryPending=false;
    bool _introDone=false;
    std::vector<uint32> speech;
    void Talk(uint32 text){speech.push_back(text);}
};
struct First : Companion
{
    struct FirstPathClock {void Update(uint32){}} _firstPath; // Dedicated first-path regression owns this service.
    struct RoomClock {void Update(uint32){}} _firstRoom; // Dedicated first-room policy regression.
    void CompleteFirstFight(uint32) {} // Dedicated first-fight regression owns this service.
#include "FirstDamage.inc"
#include "FirstHeal.inc"
#include "FirstLifecycle.inc"
#include "FirstReset.inc"
};
struct Second : Companion
{
    struct ProcessionClock {void Update(uint32){}} _processionState;void CompleteProcession(){} // Dedicated recovery host owns progression.
#include "SecondDamage.inc"
#include "SecondHeal.inc"
#include "SecondLifecycle.inc"
#include "SecondReset.inc"
};
struct Third : Companion
{
    void CompleteRitualArrival(){}
    void CompletePrisonWaypoints(){}
#include "ThirdDamage.inc"
#include "ThirdHeal.inc"
#include "ThirdLifecycle.inc"
#include "ThirdReset.inc"
};
struct Allies : Companion
{
#include "AlliesDamage.inc"
#include "AlliesHeal.inc"
#include "AlliesLifecycle.inc"
#include "AlliesReset.inc"
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
                            actor.TickLifecycle(0);
                            assert(incomingHeal==1000000 && companion.wait_heal && actor.states==UNIT_STATE_STUNNED);
                            actor.currentHealth=25600;companion.HealReceived(&attacker,incomingHeal);
                            actor.TickLifecycle(0);
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
struct HealerAI : CreatureAI
{
    bool cancel=false;
    void HealDone(Unit*,uint32& amount) override{if(cancel)amount=0;}
};
template<typename ActorAI> void CheckHealing()
{
#ifndef SILVER_HAND_ALLY_BASELINE
    for(uint32 kind : {0u,1u,2u,3u})
        for(uint32 mode : {0u,1u,2u,3u})
        {
            Creature actor;actor.currentHealth=1;actor.maximum=1000;
            ActorAI companion;companion.me=&actor;companion.wait_heal=true;
            actor.primary=&companion;actor.states=UNIT_STATE_STUNNED;
            Unit healer;healer.kind=kind;
            assert(healer.DealHeal(&actor,399)==399 && actor.currentHealth==400 && companion.wait_heal);
#ifdef SILVER_HAND_HEAL_BASELINE
            actor.TickLifecycle(0);
            assert(companion.wait_heal && actor.states==UNIT_STATE_STUNNED && !actor.auraRemovals);
            assert(healer.DealHeal(&actor,1)==1 && !companion.wait_heal && actor.auraRemovals==1);
#else
            CreatureAI temporary;
            if(mode==1 || mode==3){actor.primary=mode==1?&temporary:nullptr;actor.i_disabledAI=&companion;}
            if(mode==2)actor.i_disabledAI=&companion;
            actor.onRemove=[&]{assert(actor.m_AI_locked && !companion.wait_heal);uint32 heal=1;companion.HealReceived(&healer,heal);};
            actor.TickLifecycle(0);
            assert(!companion.wait_heal && !actor.states && actor.auraRemovals==1 && !companion._pendingHealRecovery);
            actor.TickLifecycle(10);assert(actor.auraRemovals==1);
#endif
        }
#if !defined(SILVER_HAND_HEAL_BASELINE)
    for(uint32 invalid : {0u,1u,2u,3u,4u,5u,6u,7u,8u})
    {
        Creature actor;actor.currentHealth=1;actor.maximum=1000;
        ActorAI companion;companion.me=&actor;companion.wait_heal=true;
        actor.primary=&companion;actor.states=UNIT_STATE_STUNNED;
        Unit healer;HealerAI healerAI;healer.IsAIEnabled=true;healer.healAI=&healerAI;
        if(invalid==0)healerAI.cancel=true;
        assert(healer.DealHeal(&actor,invalid==1?0:invalid==2?398:399)==(invalid==0 || invalid==1?0:invalid==2?398:399));
        if(invalid==3)actor.currentHealth=399;
        if(invalid==4)actor.alive=false;
        if(invalid==5)actor.world=false;
        if(invalid==6)actor.preDelete=true;
        if(invalid==7)companion.Reset();
        if(invalid==8)actor.maximum=0;
        actor.TickLifecycle(0);
        assert(companion.wait_heal && actor.states==UNIT_STATE_STUNNED && !actor.auraRemovals && !companion._pendingHealRecovery);
        actor.currentHealth=400;actor.maximum=1000;actor.alive=actor.world=true;actor.preDelete=false;healerAI.cancel=false;
        actor.TickLifecycle(0);assert(companion.wait_heal && !actor.auraRemovals);
        healer.DealHeal(&actor,1);actor.TickLifecycle(0);
        assert(!companion.wait_heal && actor.auraRemovals==1);
    }
    {
        Creature actor;actor.currentHealth=1;actor.maximum=1000;actor.states=UNIT_STATE_STUNNED;
        ActorAI companion;companion.me=&actor;actor.primary=&companion;
        Unit healer;
        healer.DealHeal(&actor,399);assert(!companion._pendingHealRecovery);
        companion.wait_heal=true;actor.TickLifecycle(0);assert(companion.wait_heal && !actor.auraRemovals);
        healer.DealHeal(&actor,1);healer.DealHeal(&actor,1);actor.TickLifecycle(0);
        assert(!companion.wait_heal && actor.auraRemovals==1);
        companion._pendingHealRecovery=true;uint32 lethal=1000;
        companion.DamageTakenAfterScaling(&healer,lethal,0);
        assert(companion.wait_heal && !companion._pendingHealRecovery && lethal==0);
        actor.TickLifecycle(0);assert(companion.wait_heal && actor.auraRemovals==1);
    }
    {
        Creature actor;actor.currentHealth=1;actor.maximum=1000;actor.states=UNIT_STATE_STUNNED;
        auto original=std::make_unique<ActorAI>();original->me=&actor;original->wait_heal=true;actor.primary=original.get();
        Unit healer;healer.DealHeal(&actor,399);assert(original->_pendingHealRecovery);
        original.reset();ActorAI replacement;replacement.me=&actor;replacement.wait_heal=true;actor.primary=&replacement;
        actor.TickLifecycle(0);assert(replacement.wait_heal && !actor.auraRemovals);
        healer.DealHeal(&actor,1);actor.TickLifecycle(0);assert(!replacement.wait_heal && actor.auraRemovals==1);
    }
#endif
#endif
}
int main()
{
    _set_error_mode(_OUT_TO_STDERR);
    uint32 cases=Check<First>({9,10})+Check<Second>({1,2})+Check<Third>({2,3})+Check<Allies>({0,1});
    CheckHealing<First>();CheckHealing<Second>();CheckHealing<Third>();CheckHealing<Allies>();
    Unit attacker;Creature fallback;fallback.scalable=false;assert(fallback.GetHealth(&attacker)==64000 && fallback.GetHealthMultiplierForTarget(&attacker)==1);
    fallback.scalable=true;fallback.statPresent=false;assert(fallback.GetHealth(&attacker)==64000);
    fallback.statPresent=true;fallback.currentHealth=1;fallback.stat.healthMax=1;assert(fallback.GetHealth(&attacker)==1);
    std::cout<<"PASS: "<<cases<<" four native ally guard/heal, native target-health/multiplier and post-scaling-dispatch cases; ";
#ifdef SILVER_HAND_ALLY_BASELINE
    std::cout<<"pinnedd49e77d2 raw guard loses scaled lethal hits and can wound prematurely on nonlethal high-scale hits.\n";
#elif defined(SILVER_HAND_HEAL_BASELINE)
    std::cout<<"pinned877ad851 native DealHeal first reaches40pct without releasing wound; an extra heal is required. No client reproduction claimed.\n";
#else
    std::cout<<"final lethal damage zeroes before death, wounds once, preserves exact speech/stun/aura. Native DealHeal prefix/ModifyHealth and locked lifecycle release after40pct crossing; cancel/zero/subthreshold/damage/dead/unloaded/predelete/Reset/re-wound/replacement/reentrant/primary/disabled/alias paths retained. Healer/killer labels and health services controlled; no credit, actual spell targeting/scaling/group/client/VFX proof. Failed aura presentation remains OPEN.\n";
#endif
}
