#include <limits>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#include <algorithm>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using DamageEffectType=int;
constexpr int DATA_SKOVALD=3,DATA_ODYN=4,IN_PROGRESS=1,DONE=3,UNIT_MOD_HEALTH=0,TOTAL_PCT=0;
constexpr int SAY_DEATH=9,DATA_ACHIEVEMENT=0,UNIT_FIELD_FLAGS=1,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_NOT_ATTACKABLE_1=128,UNIT_FLAG_NON_ATTACKABLE=2;
constexpr int CRITERIA_TYPE_COMPLETE_DUNGEON_ENCOUNTER=0,CRITERIA_TYPE_BE_SPELL_TARGET=1;
constexpr int EVENT_SPEAR_OF_LIGHT=3,EVENT_RADIANT_TEMPEST=4,EVENT_SHATTER_SPEARS=5,EVENT_RUNIC_BRAND=6,EVENT_SUMMON_STORMFORGED=7,DIFFICULTY_NORMAL=1;
#define TC_LOG_DEBUG(...) ((void)0)
template<class T>T CalculatePct(T n,int pct){return T(n*pct/100);}
struct Creature;struct InstanceScript;
struct WorldObject {virtual ~WorldObject()=default;};
struct Unit:WorldObject {
    uint64 health=1000,maximum=1000;
    virtual Creature* ToCreature(){return nullptr;}virtual Creature const* ToCreature()const{return nullptr;}
    uint64 GetHealth()const{return health;}uint64 GetMaxHealth()const{return maximum;}
    uint64 GetHealth(Unit*)const;uint64 GetMaxHealth(Unit*)const;
    float GetHealthPct()const{return maximum?float(health)*100/float(maximum):0;}
    uint8 GetLevelForTarget(WorldObject const*)const{return 100;}
    float GetModifierValue(int,int)const{return 1;}
    uint64 CountPctFromMaxHealth(int n){return CalculatePct(maximum,n);}bool HealthBelowPct(int n){return health<CountPctFromMaxHealth(n);}
    uint32 Deal(Creature*,uint32);
};
struct Player:Unit {unsigned credits=0;void KilledMonsterCredit(uint32 id){assert(id==95676);++credits;}};
struct MapHarness {std::vector<Player*> players;template<class F>void ApplyOnEveryPlayer(F f){for(auto p:players)f(p);}};
struct InstanceScript {
    MapHarness map;MapHarness* instance=&map;int state=IN_PROGRESS,skovald=DONE;unsigned achievements=0,auras=0;
    int GetBossState(int id){assert(id==DATA_ODYN||id==DATA_SKOVALD);return id==DATA_ODYN?state:skovald;}
    void OnCreatureDamageTaken(Creature*,Unit*,uint32&){}
    void DoUpdateAchievementCriteria(int,uint32){++achievements;}
    void DoRemoveAurasDueToSpellOnPlayers(uint32 id){assert(id==200988);++auras;}
};
struct CreatureLevelStat {uint64 healthMax=1000;};
struct AI {virtual void EnterCombat(Unit*){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}};
using CreatureAI=AI;
struct Creature:Unit {
    ::AI* i_disabledAI=nullptr;
    CreatureLevelStat stat;bool scalable=true,IsAIEnabled=true,alive=true,combat=true;AI* ai=nullptr;InstanceScript* instance=nullptr;
    unsigned stopped=0,home=0,despawn=0,flags=0,faction=16;
    Creature* ToCreature()override{return this;}Creature const* ToCreature()const override{return this;}
    bool HasScalableLevels()const{return scalable;}CreatureLevelStat const* GetScaleLevelStat(uint8){return &stat;}
    uint8 getLevel()const{return 110;}uint64 GetCreateHealth()const{return maximum;}uint64 GetMaxHealthByLevel(uint8)const{return stat.healthMax;}
    float GetHealthMultiplierForTarget(WorldObject const*)const;
    InstanceScript* GetInstanceScript(){return instance;}AI* AI(){return ai;}
    uint32 GetEntry(){return 95676;}void SetHealth(uint64 n){health=n;}void StopAttack(){++stopped;}
    void setFaction(int n){faction=n;}void SetFlag(int,uint32 n){flags|=n;}void CombatStop(){combat=false;}
    uint32 getFaction(){return faction;}bool HasFlag(int,uint32 n){return flags&n;}void SetInCombatWith(Unit* attacker){combat=true;ai->EnterCombat(attacker);}
    struct Motion {unsigned* home;void MoveTargetedHome(){++*home;}} motion{&home};Motion* GetMotionMaster(){return &motion;}
    void DespawnOrUnsummon(uint32 n){despawn=n;}
};
#include "NativeHealth.inc"
#include "NativeMultiplier.inc"
struct OdynProbe:AI {
    Creature* me;InstanceScript* instance;bool intro=false,encounterComplete=false,achievement=false;unsigned deaths=0,talks=0,starts=0;
    OdynProbe(Creature& c,InstanceScript& i):me(&c),instance(&i){c.ai=this;c.instance=&i;}
    void Talk(int id){assert(id==SAY_DEATH);++talks;}
    void _JustDied(){++deaths;instance->state=DONE;}uint32 GetData(int){return achievement;}
    void _EnterCombat(){++starts;instance->state=IN_PROGRESS;}
    int GetDifficultyID(){return DIFFICULTY_NORMAL;}
    struct Events {void RescheduleEvent(int,uint32){}} events;
    void DoCast(Creature*,uint32 spell,bool){assert(spell==202366);}
#include "OdynEnterCombat.inc"
#include "OdynDamage.inc"
};
struct OldProbe:OdynProbe {
    using OdynProbe::OdynProbe;
    void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType)override{}
#include "OldOdynDamage.inc"
};
uint32 Unit::Deal(Creature* victim,uint32 damage)
{
    DamageEffectType damagetype=0;
    victim->AI()->DamageTaken(this,damage,damagetype);
    uint64 health=victim->GetHealth(this);
#include "NativeScaledHook.inc"
    if(health<=damage)victim->alive=false;
    return damage;
}
int main()
{
    {Player p;Creature c;InstanceScript instance;instance.map.players={&p};OldProbe old(c,instance);
     assert(!p.Deal(&c,2000)&&c.health==800&&!old.encounterComplete&&!p.credits&&instance.state==IN_PROGRESS);}
    for(bool pet:{false,true})for(bool achievement:{false,true})for(uint64 scale:{1000ull,1000000ull}){
        Player owner,party;Unit guardian;Creature c;c.maximum=c.health=1000000000;c.stat.healthMax=scale;
        InstanceScript instance;instance.map.players={&owner,&party};OdynProbe probe(c,instance);probe.achievement=achievement;
        auto& attacker=pet?guardian:static_cast<Unit&>(owner);
        assert(c.GetHealth(&attacker)==scale&&c.GetMaxHealth(&attacker)==scale);
        uint32 raw=std::max<uint32>(1,uint32(2.0*scale*c.GetHealthMultiplierForTarget(&attacker)));
        assert(!attacker.Deal(&c,raw)&&c.alive&&c.GetHealthPct()==80&&probe.encounterComplete&&instance.state==DONE);
        assert(owner.credits==1&&party.credits==1&&probe.deaths==1&&probe.talks==1&&instance.auras==1&&instance.achievements==1+unsigned(achievement));
        assert(c.faction==35&&c.despawn==5000&&c.home==1&&!c.combat);
        for(int i=0;i<4;++i)assert(!attacker.Deal(&c,1000));assert(owner.credits==1&&probe.deaths==1);
    }
    {Player p;Creature c;c.scalable=false;InstanceScript i;i.map.players={&p};OdynProbe probe(c,i);
     assert(p.Deal(&c,190)==190&&!probe.encounterComplete);assert(!p.Deal(&c,191)&&probe.encounterComplete&&p.credits==1);}
    {Player p;Creature c;c.maximum=c.health=1000;c.stat.healthMax=1000000;InstanceScript i;OdynProbe probe(c,i);
     assert(p.Deal(&c,1000000)==1000&&!probe.encounterComplete&&c.alive);}
    for(int state:{0,2,DONE}){Player p;Creature c;InstanceScript i;i.state=state;OdynProbe probe(c,i);probe.intro=true;uint32 damage=2000;probe.DamageTakenAfterScaling(&p,damage,0);assert(damage==2000&&!probe.encounterComplete);}
    for(int state:{0,2}){Player p;Creature c;c.scalable=false;InstanceScript i;i.state=state;i.map.players={&p};OdynProbe probe(c,i);assert(!p.Deal(&c,2000)&&probe.encounterComplete&&probe.starts==1&&p.credits==1);}
    for(int mode=0;mode<4;++mode){Player p;Creature c;InstanceScript i;i.state=0;OdynProbe probe(c,i);if(mode==0)probe.intro=true;if(mode==1)c.faction=35;if(mode==2)i.skovald=0;if(mode==3)c.flags=UNIT_FLAG_IMMUNE_TO_PC;uint32 d=2000;probe.DamageTakenAfterScaling(&p,d,0);assert(!probe.starts&&!probe.encounterComplete&&d==2000);}
    {Creature c;InstanceScript i;OdynProbe probe(c,i);uint32 damage=0;probe.DamageTakenAfterScaling(nullptr,damage,0);assert(!probe.encounterComplete);}
    std::cout<<"PASS: old first-hit80% stall reproduced; real post-scaling dispatcher, native target health/max/multiplier getters and production Odyn surrender. Normal/scaled lethal, large raw but small final damage,81% boundary, player/pet attackers, repeat/idle/achievement/party-credit branches passed. Encounter persistence, map/players and death comparison are fixture services; client pending.\n";
}
