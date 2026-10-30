#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t;
using int32=int32_t; using DamageEffectType=uint32;
using Milliseconds=std::chrono::milliseconds;
enum {EVENT_1=1,EVENT_2,EVENT_3,UNIT_STATE_STUNNED=1,UNIT_STATE_CASTING=2,UNIT_MOD_HEALTH=0,TOTAL_PCT=0};
template<class V,class P> V CalculatePct(V base,P percent){return V(base*static_cast<float>(percent)/100.0f);}
#include "EventMap.inc"
#include "EventMapMethods.inc"
struct WorldObject {virtual ~WorldObject()=default;};
struct Creature; struct Unit;
struct CreatureAI {
    virtual ~CreatureAI()=default;
    virtual void Reset(){} virtual void EnterCombat(Unit*){} virtual void JustDied(Unit*){}
    virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
    virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}
    virtual void UpdateLifecycle(uint32){} virtual void UpdateAI(uint32){}
    virtual void Talk(uint32 line){assert(line==1);++speech;if(onTalk)onTalk();}
    uint32 speech=0;
    std::function<void()> onTalk;
};
struct Unit : WorldObject {
    uint64 currentHealth=1000,maximum=1000;
    uint32 kind=0;
    virtual Creature const* ToCreature() const {return nullptr;}
    uint64 GetHealth() const{return currentHealth;}
    uint64 GetMaxHealth() const{return maximum;}
    uint64 GetHealth(Unit*) const; uint64 GetMaxHealth(Unit*) const;
    float GetHealthPct() const{return 100.0f*float(currentHealth)/float(maximum);}
    uint8 GetLevelForTarget(WorldObject const*) const{return 100;}
    float GetModifierValue(uint32 a,uint32 b) const{assert(a==0&&b==0);return 1.0f;}
    uint32 Deal(Creature*,uint32);
};
struct CreatureLevelStat {uint64 healthMax=1000;};
struct InstanceScript {void OnCreatureDamageTaken(Creature*,Unit*,uint32&) {}};
struct Creature : Unit {
    bool alive=true,scalable=true,statPresent=true,IsAIEnabled=true,m_AI_locked=false;
    uint32 states=0,kills=0,despawns=0;
    CreatureLevelStat stat;
    CreatureAI* primary=nullptr; CreatureAI* i_disabledAI=nullptr;
    std::map<uint32,Creature*> nearby;
    std::function<void()> onKill;
    Creature const* ToCreature() const override{return this;}
    Creature* ToCreature(){return this;}
    CreatureAI* AI(){return primary;}
    bool HasScalableLevels() const{return scalable;}
    uint8 getLevel() const{return 110;}
    CreatureLevelStat const* GetScaleLevelStat(uint8){return statPresent?&stat:nullptr;}
    uint64 GetMaxHealthByLevel(uint8) const{return stat.healthMax;}
    uint64 GetCreateHealth() const{return maximum;}
    float GetHealthMultiplierForTarget(WorldObject const*) const;
    bool IsAlive() const{return alive;}
    bool HealthBelowPct(uint32 p) const{return currentHealth<CalculatePct(maximum,p);}
    bool HasUnitState(uint32 state) const{return (states&state)!=0;}
    uint32 GetInstanceId() const{return 1;}
    InstanceScript* GetInstanceScript(){return nullptr;}
    Creature* FindNearestCreature(uint32 entry,float range,bool living=true){assert(range==30.0f&&living);auto i=nearby.find(entry);return i!=nearby.end()&&i->second->alive?i->second:nullptr;}
    void Kill(Creature* target){if(!target->alive)return;target->alive=false;++kills;if(onKill)onKill();}
    void DespawnOrUnsummon(uint32 delay){assert(delay==3000);++despawns;}
    void Tick(uint32 diff){CreatureAI* i_AI=primary;
#include "LifecycleBridge.inc"
    }
};
#include "Health.inc"
#include "MaxHealth.inc"
#include "Multiplier.inc"
struct Scenario {uint32 id=1092,step=5;uint32 GetScenarioId() const{return id;}uint32 GetCurrentStep() const{return step;}};
struct ScenarioMgr {Scenario* value=nullptr;Scenario* GetScenario(uint32){return value;}} manager;
ScenarioMgr* sScenarioMgr=&manager;
struct ScriptedAI : CreatureAI {
    explicit ScriptedAI(Creature* actor):me(actor){}
    Creature* me;
    bool victim=true,castSuccess=true;
    uint32 melee=0;
    std::vector<uint32> casts;
    std::function<void()> onCast;
    bool UpdateVictim(){return victim;}
    void DoCast(uint32 spell){casts.push_back(spell);if(onCast)onCast();}
    void DoMeleeAttackIfReady(){++melee;}
};
#include "Actor.inc"
uint32 Unit::Deal(Creature* victim,uint32 damage){
    DamageEffectType damagetype=0;
    if(victim->IsAIEnabled&&victim->AI())victim->AI()->DamageTaken(this,damage,damagetype);
    uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
    if(damage>=health)victim->alive=false;
    return damage;
}
struct Test {
    Scenario scenario;
    Creature boss,efrin,duval,travard;
    CreatureAI talker,temporary;
    Unit attacker;
    npc_horrific_aberrationAI ai;
    Test():ai(&boss){manager.value=&scenario;boss.primary=&ai;travard.primary=&talker;
        boss.nearby={{106371,&efrin},{106370,&duval},{106676,&travard}};ai.Reset();}
    void Tick(uint32 diff){boss.Tick(diff);if(boss.alive&&boss.IsAIEnabled&&boss.AI())boss.AI()->UpdateAI(diff);}
};
int main(){uint32 cases=0;
    for(uint32 kind:{0u,1u,2u,3u})for(float ratio:{0.015625f,0.25f,1.0f,16.0f})for(uint32 mode:{0u,1u,2u,3u}){
        Test t;t.attacker.kind=kind;t.boss.stat.healthMax=uint64(t.boss.maximum*ratio);
        if(mode==1){t.boss.primary=&t.temporary;t.boss.i_disabledAI=&t.ai;}
        if(mode==2)t.boss.i_disabledAI=&t.ai;
        if(mode==3){t.boss.primary=nullptr;t.boss.i_disabledAI=&t.ai;}
        uint32 damage=t.attacker.Deal(&t.boss,16000000);
#ifdef BASELINE
        assert(!t.boss.alive);assert(t.boss.kills==0);assert(!t.ai.outroDone);
#else
        assert(t.boss.alive);assert(damage==t.boss.GetHealth(&t.attacker)-1);
        assert(t.ai.outroDone&&t.boss.kills==0);
        t.ai.victim=false;t.boss.states=UNIT_STATE_STUNNED|UNIT_STATE_CASTING;
        t.Tick(999);assert(t.boss.kills==0);t.Tick(1);assert(t.boss.kills==0);
        t.boss.states=0;t.Tick(1);
        assert(t.boss.kills==2&&t.efrin.despawns==1&&t.duval.despawns==1&&t.talker.speech==1);
        t.Tick(10000);assert(t.boss.kills==2&&t.talker.speech==1);
        t.attacker.Deal(&t.boss,16000000);assert(!t.boss.alive);
#endif
        ++cases;
    }
#ifndef BASELINE
    {Test t;assert(t.attacker.Deal(&t.boss,500)==500);assert(!t.ai.outroDone);
        assert(t.attacker.Deal(&t.boss,501)==501);assert(t.ai.outroDone);t.Tick(1000);assert(t.boss.kills==2);++cases;}
    {Test t;t.attacker.Deal(&t.boss,10000);t.boss.currentHealth=1;
        assert(t.attacker.Deal(&t.boss,10000)==0);t.Tick(1000);assert(t.attacker.Deal(&t.boss,1)==1&&!t.boss.alive);++cases;}
    {Test t;t.attacker.Deal(&t.boss,10000);t.Tick(500);t.ai.Reset();t.Tick(5000);assert(t.boss.kills==0);
        t.attacker.Deal(&t.boss,10000);t.Tick(999);assert(t.boss.kills==0);t.Tick(1);assert(t.boss.kills==2);++cases;}
    for(uint32 stale:{0u,1u,2u}){Test t;t.attacker.Deal(&t.boss,10000);
        if(stale==0)manager.value=nullptr;if(stale==1)t.scenario.id=991;if(stale==2)t.scenario.step=6;
        t.Tick(1000);assert(t.boss.kills==0);assert(t.attacker.Deal(&t.boss,10000)==10000&&!t.boss.alive);++cases;}
    for(uint32 missing:{106371u,106370u,106676u}){Test t;t.boss.nearby.erase(missing);t.attacker.Deal(&t.boss,10000);t.Tick(1000);
        assert(t.boss.kills==(missing==106676?2u:1u));t.attacker.Deal(&t.boss,10000);assert(!t.boss.alive);++cases;}
    {Test t;t.travard.IsAIEnabled=false;t.attacker.Deal(&t.boss,10000);t.Tick(1000);assert(t.boss.kills==2&&t.talker.speech==0);++cases;}
    {Test t;t.ai.onCast=[&]{assert(t.attacker.Deal(&t.boss,10000)<1000);t.ai.UpdateLifecycle(1000);};
        t.attacker.Deal(&t.boss,10000);t.Tick(1000);assert(t.boss.kills==2&&t.talker.speech==1);++cases;}
    {Test t;t.boss.statPresent=false;t.attacker.Deal(&t.boss,10000);assert(t.boss.alive);t.Tick(1000);assert(t.boss.kills==2);++cases;}
    {Test t;t.boss.scalable=false;t.attacker.Deal(&t.boss,10000);assert(t.boss.alive);t.Tick(1000);assert(t.boss.kills==2);++cases;}
    {Test t;t.ai.onCast=[&]{t.ai.Reset();};t.attacker.Deal(&t.boss,10000);t.Tick(1000);assert(t.boss.kills==0);++cases;}
    {Test t;t.boss.onKill=[&]{t.ai.Reset();};t.attacker.Deal(&t.boss,10000);t.Tick(1000);assert(t.boss.kills==1);++cases;}
    {Test t;t.ai.EnterCombat(&t.attacker);t.Tick(2500);assert(t.ai.casts.size()==1&&t.ai.casts[0]==215690&&t.boss.kills==0);++cases;}
    for(uint64 maximum:{1ull,2ull,3ull,1001ull}){Test t;t.boss.currentHealth=t.boss.maximum=t.boss.stat.healthMax=maximum;
        t.attacker.Deal(&t.boss,10000);assert(t.boss.alive&&t.ai.outroDone);t.Tick(1000);
        assert(t.boss.kills==2);t.attacker.Deal(&t.boss,10000);assert(!t.boss.alive);++cases;}
    {Test t;t.talker.onTalk=[&]{t.ai.Reset();};t.attacker.Deal(&t.boss,10000);t.Tick(1000);
        assert(!t.ai.outroDone&&!t.ai._outroFinished);++cases;}
#endif
    std::cout<<"PASS "<<cases<<" extracted aberration transition cases; baseline is a counterfactual, world services are controlled\n";
}
