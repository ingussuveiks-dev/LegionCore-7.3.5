#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>
using uint32 = uint32_t;
using int32 = int32_t;
using int8 = int8_t;
constexpr int EFFECT_0=0, EFFECT_1=1, EFFECT_2=2, EFFECT_3=3, IN_MILLISECONDS=1000;
constexpr int SPEC_WARLOCK_AFFLICTION=265, SPEC_WARLOCK_DEMONOLOGY=266, SPEC_WARLOCK_DESTRUCTION=267;
constexpr int POWER_SOUL_SHARDS=7, SPELL_CAST_OK=0, SPELL_FAILED_CUSTOM_ERROR=1;
constexpr int SPELL_CUSTOM_ERROR_ULTHALESH_HAS_NO_POWER_WITHOUT_SOULS=1;
using SpellCastResult = int;
float chanceSeen=-1;
bool procSucceeds=true;
bool roll_chance_f(float chance) { chanceSeen=chance; return procSucceeds; }
bool roll_chance_i(float chance) { return roll_chance_f(chance); }
struct SpellInfo {
    struct Effect { int BasePoints; } effects[4]{{9},{3},{6},{8}};
    Effect* Effects[4]{&effects[0],&effects[1],&effects[2],&effects[3]};
};
struct AuraEffect {
    float amount=0; SpellInfo info;
    float GetAmount() const { return amount; }
    SpellInfo const* GetSpellInfo() const { return &info; }
};
struct Aura { int32 duration=0; int32 GetDuration() const {return duration;} void SetDuration(int32 v){duration=v;} };
struct AreaTrigger {
    bool active=true;
    float GetPositionX() const{return 0;} float GetPositionY() const{return 0;} float GetPositionZ() const{return 0;}
    void Despawn(){active=false;}
};
struct Unit {
    int spec=265, guid=1;
    std::map<int,AuraEffect> effects;
    std::map<int,Aura> auras;
    std::set<std::pair<int,int>> targetAuras;
    std::vector<AreaTrigger*> souls;
    std::vector<int> casts;
    int GetSpecializationId() const{return spec;}
    int GetGUID() const{return guid;}
    bool HasAura(int spell, int owner) const {return targetAuras.count({spell,owner})!=0;}
    bool HasAura(int spell) const {return effects.count(spell)!=0;}
    template<class F> void AddDelayedEvent(int,F callback){callback();}
    AuraEffect* GetAuraEffect(int id,int){auto i=effects.find(id);return i==effects.end()?nullptr:&i->second;}
    Aura* GetAura(int id){auto i=auras.find(id);return i==auras.end()?nullptr:&i->second;}
    void CastSpellDuration(Unit*,int id,bool,int duration){casts.push_back(id);auras[id].duration=duration;}
    void CastSpell(Unit*,int id,bool){casts.push_back(id);}
    void CastSpell(float,float,float,int id,bool){casts.push_back(id);}
    void GetAreaObjectList(std::list<AreaTrigger*>& out,std::vector<uint32> const&) {
        for(auto soul:souls)if(soul->active)out.push_back(soul);
    }
};
using Player=Unit;
#include "MasterHarvester.inc"
struct Payment : Unit {
    void Notify(int power,int32 oldVal,int32 modVal,SpellInfo const* spellInfo,bool generate=false) {
        Player* player=this;
        switch(power) {
#include "ShardPayment.inc"
        }
    }
};
bool LegacyHarvesterProc() {
    switch(248113) {
#include "LegacyHarvester.inc"
    }
    return true;
}
struct Spell {bool triggered=false;int cost=10; bool IsTriggered() const{return triggered;}int GetPowerCost(int)const{return cost;}};
struct Script {
    Unit* caster=nullptr; Unit* target=nullptr; Spell spell;
    Unit* GetCaster(){return caster;} Unit* GetHitUnit(){return target;} Spell* GetSpell(){return &spell;}
    void SetCustomCastResultMessage(int){}
};
struct Belt : Script {
#include "PowerCord.inc"
};
struct Reap : Script {
#include "ReapSouls.inc"
};
int main() {
    SpellInfo spendingSpell;
    Payment payment;payment.spec=266;payment.effects[248113];
    payment.Notify(POWER_SOUL_SHARDS,40,-40,&spendingSpell);assert(chanceSeen==12&&payment.auras[196098].duration==8000);
    assert(!LegacyHarvesterProc()); // no second cast/hit proc for the same spend
    chanceSeen=-1;
    payment.Notify(POWER_SOUL_SHARDS,40,0,&spendingSpell);
    payment.Notify(POWER_SOUL_SHARDS,40,10,&spendingSpell);
    payment.Notify(POWER_SOUL_SHARDS,40,-10,nullptr);
    payment.Notify(POWER_SOUL_SHARDS,40,-10,&spendingSpell,true);
    payment.Notify(POWER_SOUL_SHARDS,0,-40,&spendingSpell);
    assert(chanceSeen==-1);
    payment.Notify(POWER_SOUL_SHARDS,20,-40,&spendingSpell);assert(chanceSeen==6);
    // Paid costs, not static DB2 prices: Hand of Gul'dan can spend 1..4 shards.
    for(int spec : {265,266,267}) for(int shards : {1,2,3,4,5}) {
        Player p;p.spec=spec;p.effects[248113];chanceSeen=-1;
        TriggerMasterHarvester(&p,shards*10);
        assert(chanceSeen==float((spec==265?9:spec==266?3:6)*shards));
        assert(p.auras[196098].duration==8000 && p.casts.size()==1);
        TriggerMasterHarvester(&p,10);
        assert(p.auras[196098].duration==16000 && p.casts.size()==1);
    }
    Player p;p.effects[248113];
    for(int cost:{-10,0,9}){chanceSeen=-1;TriggerMasterHarvester(&p,cost);assert(chanceSeen==-1&&p.casts.empty());}
    procSucceeds=false;TriggerMasterHarvester(&p,10);assert(p.casts.empty());procSucceeds=true;
    p.spec=0;chanceSeen=-1;TriggerMasterHarvester(&p,10);assert(chanceSeen==-1);
    p.spec=265;p.effects.clear();TriggerMasterHarvester(&p,10);assert(chanceSeen==-1);

    Unit caster,target;caster.effects[205753].amount=25;
    Belt belt;belt.caster=&caster;belt.target=&target;
    belt.HandleHit();assert(caster.casts==std::vector<int>{205756}&&chanceSeen==25);
    for(int dot:{233490,233496,233497,233498,233499}) {
        caster.casts.clear();target.targetAuras={{dot,1}};belt.HandleHit();assert(caster.casts.empty());
        target.targetAuras={{dot,2}};belt.HandleHit();assert(caster.casts.size()==1); // another warlock's DoT
    }
    target.targetAuras.clear();caster.casts.clear();belt.spell.cost=0;belt.HandleHit();assert(caster.casts.empty());
    belt.spell.cost=10;belt.spell.triggered=true;belt.HandleHit();assert(caster.casts.empty());
    belt.spell.triggered=false;procSucceeds=false;belt.HandleHit();assert(caster.casts.empty());procSucceeds=true;
    caster.effects.clear();belt.HandleHit();assert(caster.casts.empty());
    belt.target=nullptr;belt.HandleHit();belt.caster=nullptr;belt.HandleHit();

    // Repeated permission checks and an abandoned cast must not consume souls.
    Unit warlock; AreaTrigger a,b;warlock.souls={&a,&b};Reap reap;reap.caster=&warlock;
    for(int i=0;i<3;++i)assert(reap.CheckCast()==SPELL_CAST_OK);
    assert(a.active&&b.active&&warlock.casts.empty());
    warlock.effects[236114].amount=1500;
    reap.HandleAfterCast();assert(!a.active&&!b.active&&warlock.auras[216708].duration==13000);
    auto castCount=warlock.casts.size();reap.HandleAfterCast();assert(warlock.casts.size()==castCount);
    assert(reap.CheckCast()==SPELL_FAILED_CUSTOM_ERROR);
    AreaTrigger c;warlock.souls={&c};warlock.effects.clear();
    reap.HandleAfterCast();assert(warlock.auras[216708].duration==18000); // unequipped cloak
    reap.caster=nullptr;assert(reap.CheckCast()==SPELL_FAILED_CUSTOM_ERROR);reap.HandleAfterCast();
    std::cout<<"PASS: Master Harvester paid shards/specs/zero spend; Power Cord all five own DoTs/free casts/unequip; Reap Souls read-only checks and cloak duration.\n";
}
