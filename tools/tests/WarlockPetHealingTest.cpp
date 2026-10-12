#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint64=uint64_t;
using uint32=uint32_t;
using SpellEffIndex=int;
constexpr int EFFECT_0=0, EFFECT_1=1, EFFECT_2=2;
constexpr int RATE_HEALTH=0, SPELL_AURA_MOD_HEALTH_REGEN_PERCENT=1, SPELL_AURA_MOD_REGEN=2;
constexpr int SPELL_AURA_MOD_ABSORB_AMOUNT=3, CURRENT_CHANNELED_SPELL=4;
constexpr int IN_MILLISECONDS=1000, CREATURE_REGEN_INTERVAL=5000;
template<class T,class U> T CalculatePct(T v,U pct){return T(v*pct/100.0);}
template<class T,class U> void AddPct(T& v,U pct){v+=CalculatePct(v,pct);}
struct World {double rate=1; double getRate(int)const{return rate;}} world;
World* sWorld=&world;
struct Aura {int refresh=0; void RefreshDuration(){++refresh;}};
struct AuraEffect {
    float amount=0; int index=0; Aura base;
    float GetAmount()const{return amount;} int GetEffIndex()const{return index;}
    void SetAmount(float v){amount=v;} Aura* GetBase(){return &base;}
};
struct SpellInfo {
    struct Effect {int BasePoints=0;} effects[3];
    Effect* Effects[3]{&effects[0],&effects[1],&effects[2]};
    bool area=false; bool IsTargetingArea()const{return area;}
};
struct Unit;
using Player=Unit;
using Pet=Unit;
struct Unit {
    uint64 health=50000, maximum=100000;
    bool player=true, alive=true, interrupted=false, immune=false;
    Unit* owner=nullptr; Unit* pet=nullptr;
    std::map<std::pair<int,int>,AuraEffect> effects;
    std::vector<AuraEffect*> modifiers;
    std::vector<std::pair<float,float>> heals;
    bool IsAlive()const{return alive;} bool IsFullHealth()const{return health==maximum;}
    uint64 GetMaxHealth(Unit* =nullptr)const{return maximum;}
    uint64 GetHealth()const{return health;}
    uint64 CountPctFromMaxHealth(int pct,Unit* =nullptr)const{return CalculatePct(maximum,pct);}
    Player* ToPlayer(){return player?this:nullptr;}
    Unit* GetOwner(){return owner;} Pet* GetPet(){return pet;}
    AuraEffect* GetAuraEffect(int id,int i){auto it=effects.find({id,i});return it==effects.end()?nullptr:&it->second;}
    std::vector<AuraEffect*> const& GetAuraEffectsByType(int)const{return modifiers;}
    float GetTotalAuraMultiplier(int)const {float n=1;for(auto m:modifiers)AddPct(n,m->amount);return n;}
    void CastCustomSpell(Unit*,int id,float const* bp0,float const* bp1,void*,bool){
        if(id==108366 && !immune) effects[{id,0}].amount=*bp0*1.5f; // simulate generic absorb modifiers
        if(id==108447) heals.emplace_back(*bp0,*bp1);
    }
    void InterruptSpell(int,bool){interrupted=true;}
    float AddSoulLeechAbsorb(float,float);
    void HealFromSoulLeech(float,float);
    bool Proc(float damage,Unit* victim,SpellInfo const* procSpell=nullptr,int index=0){
        AuraEffect driver; driver.index=index; AuraEffect* triggeredByAura=&driver;
        float triggerAmount=10;
        switch(108370){
#include "SoulLeechProc.inc"
        }
        return false;
    }
};
#include "SoulLeechAbsorb.inc"
#include "SoulLinkHeal.inc"
struct Creature : Unit {
    bool regen=true, controlled=true; float multiplier=1; int flat=0;
    bool isRegeneratingHealth()const{return regen;}
    bool GetCharmerOrOwnerGUID()const{return controlled;}
    float GetTotalAuraMultiplier(int)const{return multiplier;}
    int GetTotalAuraModifier(int)const{return flat;}
    void ModifyHealth(uint64 v){health=std::min(maximum,health+v);}
    void RegenerateHealth();
};
#include "PetRegen.inc"
struct Funnel {
    Unit* caster=nullptr; Unit* hit=nullptr; SpellInfo info;
    uint64 heal=0,damage=0;
    Unit* GetHitUnit(){return hit;} Unit* GetCaster(){return caster;}
    SpellInfo const* GetSpellInfo(){return &info;}
    void SetHitHeal(uint64 v){heal=v;} void SetHitDamage(uint64 v){damage=v;}
#include "FunnelDamage.inc"
#include "FunnelHeal.inc"
#include "FunnelStop.inc"
};
struct Skin {
    Unit* caster=nullptr; SpellInfo info;
    Unit* GetCaster(){return caster;} SpellInfo const* GetSpellInfo(){return &info;}
#include "DemonSkin.inc"
};
float shield(Unit& u){auto a=u.GetAuraEffect(108366,0);return a?a->amount:0;}
void near(float actual,float expected){assert(std::abs(actual-expected)<0.1f);}
int main(){
    Creature c; c.maximum=2000000;c.health=1000000;
    c.RegenerateHealth();assert(c.health==1030000);
    world.rate=2;c.RegenerateHealth();assert(c.health==1090000);
    world.rate=1;c.multiplier=0.5;c.RegenerateHealth();assert(c.health==1105000);
    c.regen=false;c.RegenerateHealth();assert(c.health==1105000);
    c.regen=true;c.health=c.maximum-1;c.RegenerateHealth();assert(c.health==c.maximum);
    c.RegenerateHealth();assert(c.health==c.maximum);

    Unit owner,pet,enemy;owner.pet=&pet;pet.player=false;pet.owner=&owner;
    owner.maximum=100000;pet.maximum=400000;
    Funnel funnel;funnel.caster=&owner;funnel.hit=&owner;
    funnel.info.effects[0].BasePoints=4;funnel.info.effects[1].BasePoints=8;
    funnel.HandleDamage(0);funnel.hit=&pet;funnel.HandleHeal(1);
    assert(funnel.damage==4000 && funnel.heal==8000);
    assert(funnel.damage*6==24000 && funnel.heal*6==48000);
    pet.maximum=20000;funnel.HandleHeal(1);assert(funnel.heal==8000);
    funnel.HandleCast();assert(!owner.interrupted);
    pet.health=pet.maximum;funnel.HandleCast();assert(owner.interrupted);
    funnel.caster=nullptr;funnel.HandleHeal(1);funnel.HandleCast();

    pet.maximum=400000;pet.health=100000;
    assert(owner.Proc(10000,&enemy));near(shield(owner),1000);near(shield(pet),1000);
    assert(pet.Proc(20000,&enemy));near(shield(owner),3000);near(shield(pet),3000);
    assert(owner.Proc(1000000,&enemy));near(shield(owner),15000);near(shield(pet),60000);
    assert(owner.heals.empty());
    SpellInfo aoe;aoe.area=true;assert(!owner.Proc(1000,&enemy,&aoe));
    assert(!owner.Proc(1000,&owner));assert(!owner.Proc(0,&enemy));assert(!owner.Proc(1000,&enemy,nullptr,1));
    Unit orphan;orphan.player=false;assert(!orphan.Proc(1000,&enemy));

    owner.effects[{108446,1}].amount=25;owner.effects[{108446,2}].amount=50;
    owner.effects[{108366,0}].amount=0;pet.effects[{108366,0}].amount=0;
    owner.Proc(10000,&enemy);near(owner.heals.back().first,250);near(owner.heals.back().second,500);
    owner.effects[{108366,0}].amount=14900;pet.effects[{108366,0}].amount=59950;
    owner.Proc(10000,&enemy);near(owner.heals.back().first,25);near(owner.heals.back().second,25);
    auto n=owner.heals.size();owner.Proc(10000,&enemy);assert(owner.heals.size()==n);
    owner.effects[{219272,1}].amount=20;
    owner.Proc(1000000,&enemy);near(shield(owner),20000);near(shield(pet),80000);

    owner.effects.erase({108366,0});pet.effects.erase({108366,0});
    Skin skin;skin.caster=&owner;skin.info.effects[1].BasePoints=20;
    AuraEffect tick;tick.amount=1;
    skin.OnTick(&tick);near(shield(owner),1000);near(shield(pet),4000);
    near(owner.heals.back().first,250);near(owner.heals.back().second,2000);
    for(int i=0;i<30;++i)skin.OnTick(&tick);
    near(shield(owner),20000);near(shield(pet),80000);
    pet.alive=false;n=owner.heals.size();owner.effects[{108366,0}].amount=0;
    owner.Proc(10000,&enemy);assert(owner.heals.size()==n);near(shield(pet),80000);
    owner.pet=nullptr;owner.Proc(10000,&enemy);assert(owner.heals.size()==n);
    Unit immune;immune.immune=true;near(immune.AddSoulLeechAbsorb(100,15),0);
    std::cout << "PASS: extracted pet regeneration, Health Funnel, Soul Leech, Demon Skin and Soul Link handlers\n";
}
