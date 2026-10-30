#include <algorithm>
#include <limits>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
using uint8=uint8_t;
using uint16=uint16_t;
using uint32=uint32_t;
using uint64=uint64_t;
using DamageEffectType=uint32;
using Milliseconds=std::chrono::milliseconds;
enum {UNIT_FIELD_FLAGS=1,UNIT_FLAG_NOT_SELECTABLE=1,UNIT_FLAG_IMMUNE_TO_PC=2,UNIT_FLAG_NOT_ATTACKABLE_1=4,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
#include "Constants.inc"
struct Creature;
struct Unit {uint32 kind=0;uint32 Deal(Creature*,uint32);};
struct Player
{
    std::vector<uint32> credit;
    void UpdateAchievementCriteria(uint32 type,uint32 event,uint32,uint32,Unit*){assert(type==92 && event==46149);credit.push_back(event);}
};
struct InstanceMap
{
    std::vector<Player*> players;
    template<class Callback>void ApplyOnEveryPlayer(Callback callback){for(Player* player:players)callback(player);}
};
struct InstanceScript
{
    InstanceMap* instance=nullptr;
    uint32 step=5;
    uint32 getScenarionStep(){return step;}
    void OnCreatureDamageTaken(Creature*,Unit*,uint32&){}
    void DoUpdateAchievementCriteria(uint32 type,uint32 miscValue1=0,uint32 miscValue2=0,uint32 miscValue3=0,Unit* unit=nullptr);
};
#include "InstanceCredit.inc"
struct CreatureAI
{
    virtual ~CreatureAI()=default;
    virtual void Reset(){}
    virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
    virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}
};
struct Creature
{
    uint64 health=100000,maximum=100000,targetHealth=1000,targetMaximum=1000;
    float multiplier=0.01f;
    bool alive=true;
    uint32 flags=0,stops=0,despawns=0;
    std::vector<uint32> kits;
    CreatureAI* primary=nullptr;
    CreatureAI* i_disabledAI=nullptr;
    CreatureAI* AI(){return primary;}
    Creature* ToCreature(){return this;}
    InstanceScript* GetInstanceScript(){return nullptr;}
    uint64 GetHealth()const{return health;}
    uint64 GetHealth(Unit* attacker)const{return attacker?targetHealth:health;}
    bool HealthBelowPct(uint32 percent)const{return health*100<maximum*percent;}
    float GetHealthMultiplierForTarget(Unit*)const{return multiplier;}
    void SetFlag(uint32 field,uint32 mask){assert(field==UNIT_FIELD_FLAGS);flags|=mask;}
    void StopAttack(){++stops;}
    void SendPlaySpellVisualKit(uint32 kit,uint32 parameter){assert(parameter==0);kits.push_back(kit);}
    void DespawnOrUnsummon(uint32 delay){assert(delay==3000);++despawns;}
};
struct SummonList {uint32 clears=0;void DespawnAll(){++clears;}};
struct Balnazzar:CreatureAI
{
    Creature* me;
    InstanceScript* instance;
    SummonList summons;
    EventMap events;
    bool fightEnd=false;
    std::vector<uint32> casts;
    std::function<void()> onCast;
    void DoCast(Creature* target,uint32 spell,bool triggered)
    {
        assert(target==me && triggered && fightEnd);casts.push_back(spell);if(onCast)onCast();
    }
#include "Surrender.inc"
};
uint32 Unit::Deal(Creature* victim,uint32 damage)
{
    DamageEffectType damagetype=0;
    if(victim->primary)victim->primary->DamageTaken(this,damage,damagetype);
    if(victim->i_disabledAI && victim->i_disabledAI!=victim->primary)victim->i_disabledAI->DamageTaken(this,damage,damagetype);
    if(!damage)return 0;
    uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
    if(health<=damage)victim->alive=false;
    else{victim->targetHealth-=damage;victim->health=victim->maximum*victim->targetHealth/victim->targetMaximum;}
    return damage;
}
int main()
{
    Player first,second;InstanceMap map{{&first,&second}};InstanceScript instance{&map};Unit attacker;
#ifdef BASELINE
    Creature actor;Balnazzar script;script.me=&actor;script.instance=&instance;actor.primary=&script;
    assert(attacker.Deal(&actor,10)==1000 && !actor.alive && !script.fightEnd && first.credit.empty());
    Creature immune;immune.multiplier=100;immune.targetHealth=immune.targetMaximum=10000000;
    Balnazzar stuck;stuck.me=&immune;stuck.instance=&instance;immune.primary=&stuck;
    assert(attacker.Deal(&immune,200000)==0 && immune.health==100000 && !stuck.fightEnd);
    Creature exact;exact.multiplier=1;exact.targetHealth=exact.targetMaximum=100000;
    Balnazzar stalled;stalled.me=&exact;stalled.instance=&instance;exact.primary=&stalled;
    assert(attacker.Deal(&exact,100000)==0 && !stalled.fightEnd);
    std::cout<<"PASS pinned803e4734: raw10/scale0.01 kills before finale, raw200000/scale100 wrongly absorbs nonlethal damage, unscaled fresh one-shot remains stuck without surrender; actual extracted post-scaling bridge\n";
#else
    for(uint32 kind=0;kind<6;++kind)for(float scale:{0.001f,0.01f,1.0f,100.0f})for(bool suspended:{false,true})
    {
        Creature actor;actor.multiplier=scale;CreatureAI temporary;Balnazzar script;script.me=&actor;script.instance=&instance;
        actor.primary=suspended?&temporary:&script;actor.i_disabledAI=suspended?&script:nullptr;attacker.kind=kind;
        script.events.ScheduleEvent(EVENT_7,30000);first.credit.clear();second.credit.clear();
        uint32 raw=uint32(2000*scale);bool entered=false;
        script.onCast=[&](){if(!entered){entered=true;assert(script.fightEnd);assert(attacker.Deal(&actor,raw)==0);}};
        assert(attacker.Deal(&actor,raw)==0 && actor.alive && script.fightEnd);
        assert(script.casts==std::vector<uint32>({227853,152821}) && actor.kits==std::vector<uint32>{61036});
        assert(actor.flags==7 && actor.stops==1 && actor.despawns==1 && script.summons.clears==1 && script.events.Empty());
        assert(first.credit==std::vector<uint32>{46149} && second.credit==first.credit);
        assert(attacker.Deal(&actor,raw)==0 && script.casts.size()==2 && first.credit.size()==1 && actor.despawns==1);
    }
    for(uint32 step=0;step<8;++step)
    {
        instance.step=step;first.credit.clear();second.credit.clear();Creature actor;Balnazzar script;script.me=&actor;script.instance=&instance;actor.primary=&script;
        assert(attacker.Deal(&actor,10)==0 && actor.alive && script.fightEnd);
        assert(first.credit.size()==(step==5?1u:0u) && second.credit==first.credit);
    }
    instance.step=5;
    {
        Creature actor;actor.multiplier=100;actor.targetHealth=actor.targetMaximum=10000000;
        Balnazzar script;script.me=&actor;script.instance=&instance;actor.primary=&script;
        assert(attacker.Deal(&actor,200000)==2000 && actor.alive && !script.fightEnd && script.casts.empty());
    }
    for(uint64 health:{15000ull,14999ull})
    {
        Creature actor;actor.multiplier=1;actor.health=actor.targetHealth=health;actor.targetMaximum=100000;
        Balnazzar script;script.me=&actor;script.instance=&instance;actor.primary=&script;
        assert(attacker.Deal(&actor,1)==1 && actor.alive && script.fightEnd==(health<15000));
        if(health==15000){assert(actor.health==14999 && !script.fightEnd);assert(attacker.Deal(&actor,1)==1 && script.fightEnd && actor.alive);}
    }
    {
        Creature actor;actor.health=14999;Balnazzar script;script.me=&actor;script.instance=&instance;actor.primary=&script;
        assert(attacker.Deal(&actor,0)==0 && !script.fightEnd && script.casts.empty());
    }
    {
        Creature actor;Balnazzar script;script.me=&actor;script.instance=&instance;uint32 damage=100000;
        script.DamageTakenAfterScaling(nullptr,damage,0);assert(!damage && script.fightEnd && actor.alive);
    }
    {
        Creature actor;Balnazzar script;script.me=&actor;script.instance=&instance;actor.primary=&script;
        script.fightEnd=true;script.events.ScheduleEvent(EVENT_7,30000);script.Reset();
        assert(!script.fightEnd && script.events.Empty() && script.summons.clears==1);
        assert(attacker.Deal(&actor,10)==0 && script.fightEnd && script.summons.clears==2);
    }
    {
        Creature actor;Balnazzar script;script.me=&actor;script.instance=&instance;actor.primary=&script;instance.instance=nullptr;
        assert(attacker.Deal(&actor,10)==0 && script.fightEnd && actor.alive);instance.instance=&map;
    }
    std::cout<<"PASS48 killer-label/scale/primary-suspended lethal and reentrant cases: zero before native death, once-only original227853/morph/kit/cleanup/3000ms and native instance Player fanout at stage5;8 stage gates,nonlethal high-scale/strict15pct/null-attacker. Health/labels/casts/Player eligibility controlled,not full engine group credit or client combat/VFX\n";
#endif
}
