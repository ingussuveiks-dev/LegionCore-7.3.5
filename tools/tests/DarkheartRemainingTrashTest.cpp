#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <vector>
using uint32=uint32_t;
constexpr int UNIT_STATE_CASTING=1,SELECT_TARGET_RANDOM=0;
constexpr int EFFECT_0=0,SPELL_AURA_PERIODIC_DAMAGE=3,AURA_EFFECT_HANDLE_REAL=1,AURA_REMOVE_BY_EXPIRE=1;
using AuraEffectHandleModes=int;
struct Creature;struct ScriptedAI;
struct Unit {
    bool alive=true;Unit* victim=nullptr;int entry=0;
    virtual ~Unit()=default;
    virtual Creature* ToCreature(){return nullptr;}
    bool IsAlive(){return alive;}Unit* getVictim(){return victim;}
    int GetEntry(){return entry;}
    void CastSpell(Unit*,int,bool);
};
struct Cast{Unit* caster;Unit* target;int spell;bool triggered;};
std::vector<Cast> casts;
constexpr int SPELL_FAILED_CASTER_DEAD=1,SPELL_ATTR0_CASTABLE_WHILE_DEAD=1;
struct Info{bool IsPassive(){return false;}bool HasAttribute(int){return false;}};
struct NativeCasterCheck{
    Unit* m_caster;Info info;Info* m_spellInfo=&info;bool triggered;void* m_triggeredByAuraSpell=nullptr;
    bool IsTriggered(){return triggered;}
    int Check(){
#include "NativeCasterDeath.inc"
        return 0;
    }
};
void Unit::CastSpell(Unit* t,int s,bool triggered){NativeCasterCheck check;check.m_caster=this;check.triggered=triggered;if(!check.Check())casts.push_back({this,t,s,triggered});}
struct Map{int difficulty=1;int GetDifficultyID()const{return difficulty;}};
namespace DarkheartTrash{bool IsMythic(Map const* m){return m->difficulty==23||m->difficulty==8;}}
struct HostileReference{Unit* target;float threat;Unit* getTarget(){return target;}float getThreat(){return threat;}};
struct ThreatManager{std::list<HostileReference*> refs;auto const& getThreatList(){return refs;}};
struct Creature:Unit {
    Map map;float hp=100;bool combat=true,casting=false,IsAIEnabled=true;int despawn=0,zone=0;
    ScriptedAI* ai=nullptr;ThreatManager threats;std::vector<std::pair<Unit*,float>> added;
    Creature* ToCreature()override{return this;}Map* GetMap(){return &map;}
    float GetHealthPct(){return hp;}bool isInCombat(){return combat;}
    bool HasUnitState(int){return casting;}ScriptedAI* AI(){return ai;}
    ThreatManager& getThreatManager(){return threats;}
    void AddThreat(Unit* u,float v){added.push_back({u,v});}
    void DespawnOrUnsummon(int v){despawn=v;}
};
struct ScriptedAI {
    Creature* me;Unit* selected=nullptr;Unit* attacked=nullptr;bool playersOnly=false;
    explicit ScriptedAI(Creature* c):me(c){c->ai=this;}
    virtual void Reset(){}virtual void IsSummonedBy(Unit*){}virtual void JustSummoned(Creature*){}virtual void UpdateAI(uint32){}
    bool UpdateVictim(){return me->combat;}void AttackStart(Unit* t){attacked=t;}
    void DoZoneInCombat(){++me->zone;}void DoMeleeAttackIfReady(){}
    void DoCast(Unit* u,int s){me->CastSpell(u,s,false);}
    Unit* SelectTarget(int,int,float,bool p){playersOnly=p;return selected;}
};
struct AuraEffect{};struct Application{int mode=0;int GetRemoveMode(){return mode;}};
struct Hook{template<class T>void operator+=(T){}};
struct AuraScript{
    Unit* auraCaster=nullptr;Unit* target=nullptr;Application app;Hook OnEffectRemove;
    virtual void Register(){}Unit* GetCaster(){return auraCaster;}Unit* GetTarget(){return target;}
    Application* GetTargetApplication(){return &app;}
};
#define PrepareAuraScript(name) public:
#define AuraEffectRemoveFn(fn,...) &fn
#include "Burster.inc"
#include "Toxin.inc"
int Count(int s){return int(std::count_if(casts.begin(),casts.end(),[=](Cast const& c){return c.spell==s;}));}
int main(){
    Unit player,pet,dead;dead.alive=false;
    Creature burster;burster.entry=100532;burster.victim=&pet;
    npc_darkheart_bloodtainted_burster ai(&burster);ai.selected=&player;
    for(int difficulty:{1,2,23,8}){
        burster.map.difficulty=difficulty;burster.hp=100;casts.clear();ai.Reset();
        ai.UpdateAI(1000);assert(Count(201298)==1&&ai.playersOnly&&casts.back().target==&player);
        burster.hp=20;ai.UpdateAI(1);assert(Count(225562)==((difficulty==23||difficulty==8)?1:0));
        if(difficulty==23||difficulty==8)assert(!casts.back().triggered&&casts.back().target==&burster);
    }
    burster.map.difficulty=23;burster.hp=50;casts.clear();ai.Reset();
    burster.casting=true;ai.UpdateAI(6000);assert(casts.empty());
    burster.casting=false;ai.UpdateAI(0);assert(Count(225562)==1);
    ai.UpdateAI(5999);assert(Count(225562)==1);
    ai.UpdateAI(1);assert(Count(225562)==2&&burster.despawn==0); // Interrupted or failed summon retries.
    Creature fury,other;fury.entry=113398;other.entry=100531;ScriptedAI furyAI(&fury);
    ai.JustSummoned(&other);assert(!ai.transformed&&!burster.despawn);
    HostileReference tank{&pet,120},p{&player,40},d{&dead,9};burster.threats.refs={&tank,&p,&d};
    ai.JustSummoned(&fury);assert(ai.transformed&&burster.despawn==100&&furyAI.attacked==&pet);
    assert(fury.added.size()==2&&fury.added[0].first==&pet&&fury.added[0].second==120);
    ai.JustSummoned(&fury);ai.UpdateAI(60000);assert(fury.added.size()==2&&Count(225562)==2);
    ai.Reset();burster.despawn=0;ai.JustSummoned(&fury);assert(!ai.transformed&&!burster.despawn);
    ai.IsSummonedBy(&burster);assert(ai.attacked==&pet);ai.IsSummonedBy(nullptr);assert(burster.zone==1);
    ai.selected=nullptr;burster.hp=100;casts.clear();ai.UpdateAI(1000);assert(casts.empty());
    for(bool combat:{false,true})for(bool alive:{false,true}){
        ai.Reset();ai.requested=true;burster.combat=combat;burster.alive=alive;ai.JustSummoned(&fury);
        assert(ai.transformed==(combat&&alive));
    }
    spell_nightmare_toxin toxin;toxin.target=&player;
    Creature original,nearby;original.alive=false;toxin.auraCaster=&original;
    casts.clear();original.CastSpell(&player,200686,false);assert(casts.empty()); // Native dead-caster admission rejects an ordinary cast.
    casts.clear();toxin.app.mode=AURA_REMOVE_BY_EXPIRE;toxin.OnRemove(nullptr,0);
    assert(Count(200686)==1&&casts[0].caster==&original&&casts[0].target==&player);
    for(int removal:{0,2,3,4}){casts.clear();toxin.app.mode=removal;toxin.OnRemove(nullptr,0);assert(casts.empty());}
    toxin.app.mode=AURA_REMOVE_BY_EXPIRE;toxin.auraCaster=nullptr;toxin.OnRemove(nullptr,0);assert(casts.empty());
    toxin.auraCaster=&nearby;player.alive=false;toxin.OnRemove(nullptr,0);assert(casts.empty());
    toxin.target=nullptr;toxin.OnRemove(nullptr,0);assert(casts.empty());
    std::cout<<"PASS: actual Burster AI difficulty, threshold, interrupted/failed cast, real replacement, threat/pet and original-caster toxin expiry handlers\n";
}
