#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <functional>
#include <iostream>
#include <list>
#include <set>
#include <vector>
using uint32 = uint32_t;
using AuraEffectHandleModes = int;
constexpr int DIFFICULTY_MYTHIC_DUNGEON=23, DIFFICULTY_MYTHIC_KEYSTONE=8;
constexpr int REACT_PASSIVE=0, UNIT_STATE_CASTING=1, SELECT_TARGET_RANDOM=0;
constexpr int AURA_REMOVE_BY_EXPIRE=1, EFFECT_0=0, SPELL_AURA_PERIODIC_DAMAGE=3;
constexpr int SPELL_AURA_PERIODIC_TRIGGER_SPELL=23, AURA_EFFECT_HANDLE_REAL=1;
constexpr int SPELL_AURA_CREATE_AREATRIGGER=395;
struct Position { float x=0; };
struct ObjectGuid { int value=0; void Clear(){value=0;} };
struct Map { int difficulty=1; int GetDifficultyID()const{return difficulty;} };
struct Unit; struct Creature; struct Player; struct ScriptedAI;
struct Cast { Unit* caster; Unit* target; int spell; bool triggered; };
std::vector<Cast> casts;
std::vector<Player*> worldPlayers;
std::vector<Creature*> worldKeepers;
std::vector<Creature*> creatures;
Player* selected=nullptr;
Creature* spawnWarning=nullptr;
bool lastPlayersOnly=false;
struct Unit : Position
{
    bool alive=true, gm=false, attackable=true;
    virtual ~Unit()=default;
    virtual bool IsCreature(){return false;}
    virtual Creature* ToCreature(){return nullptr;}
    virtual int GetEntry(){return 0;}
    virtual Map* GetMap(){return nullptr;}
    bool IsAlive(){return alive;}
    bool isGameMaster(){return gm;}
    void CastSpell(Unit*,uint32,bool);
};
struct Player : Unit { };
struct Creature : Unit
{
    int entry=0, id=0, react=1, removals=0, despawn=0;
    bool combat=true, casting=false;
    Map map;
    ScriptedAI* ai=nullptr;
    std::set<int> auras;
    std::function<void()> removed;
    bool IsCreature()override{return true;}
    Creature* ToCreature()override{return this;}
    ScriptedAI* AI(){return ai;}
    int GetEntry()override{return entry;}
    ObjectGuid GetGUID(){return {id};}
    Map* GetMap()override{return &map;}
    void RemoveAllAreaObjects(){++removals;if(removed)removed();}
    void RemoveAurasDueToSpell(int spell){auras.erase(spell);}
    bool HasAura(int spell){return auras.count(spell)!=0;}
    bool HasUnitState(int){return casting;}
    void SetReactState(int state){react=state;}
    bool IsValidAttackTarget(Unit* u){return u->attackable;}
    void DespawnOrUnsummon(int delay){despawn=delay;}
    void GetCreatureListWithEntryInGrid(std::list<Creature*>& result,int templateEntry,float distance)
    {for(auto k:worldKeepers)if(k->entry==templateEntry&&std::abs(k->x-x)<=distance)result.push_back(k);}
};
void GetPlayerListInGrid(std::list<Player*>& result,Creature* me,float distance)
{for(auto p:worldPlayers)if(std::abs(p->x-me->x)<=distance)result.push_back(p);}
struct EventMap
{
    uint32 left=0; bool active=false;
    void Reset(){active=false;}
    void RescheduleEvent(int,uint32 time){left=time;active=true;}
    void Update(uint32 diff){if(active)left=diff>=left?0:left-diff;}
    int ExecuteEvent(){if(!active||left)return 0;active=false;return 1;}
};
struct SummonList
{
    std::vector<Creature*> list;
    explicit SummonList(Creature*){}
    void Summon(Creature* summon){list.push_back(summon);}
    void DespawnAll(){for(auto c:list)c->despawn=1;list.clear();}
};
struct ScriptedAI
{
    Creature* me;bool movement=true;
    explicit ScriptedAI(Creature* c):me(c){c->ai=this;}
    virtual ~ScriptedAI()=default;
    virtual void Reset(){} virtual void EnterCombat(Unit*){} virtual void JustDied(Unit*){}
    virtual void JustSummoned(Creature*){} virtual void UpdateAI(uint32){}
    virtual void OnAreaTriggerDespawn(uint32,Position,bool){}
    void SetCombatMovement(bool v){movement=v;}
    bool UpdateVictim(){return me->combat;}
    void DoCast(Unit* target,int spell,bool triggered=false){me->CastSpell(target,spell,triggered);}
    void DoMeleeAttackIfReady(){}
    Unit* SelectTarget(int,int,float,bool onlyPlayers){lastPlayersOnly=onlyPlayers;return selected;}
};
void Unit::CastSpell(Unit* target,uint32 spell,bool triggered)
{
    casts.push_back({this,target,int(spell),triggered});
    if(IsCreature()&&(spell==225497||spell==225503))ToCreature()->auras.insert(spell);
    // Summon callback transport only. Native DB2 separately checks the actual
    // summon effect, target kind, duration and spawnWarning/damage spell effects.
    if(spell==201129&&spawnWarning){spawnWarning->x=target->x;ToCreature()->AI()->JustSummoned(spawnWarning);}
}
namespace ObjectAccessor
{
Creature* GetCreature(Creature&,ObjectGuid id){for(auto c:creatures)if(c->id==id.value)return c;return nullptr;}
}
uint32 urand(uint32 min,uint32){return min;}
struct AuraEffect{};
struct Application {int mode=0;int GetRemoveMode(){return mode;}};
struct Hook {template<class T>void operator+=(T){}};
struct AuraScript
{
    Unit* auraCaster=nullptr;Unit* auraTarget=nullptr;Application app;bool prevented=false;
    Hook AfterEffectRemove,OnEffectPeriodic;
    virtual void Register(){}
    Unit* GetCaster(){return auraCaster;} Unit* GetTarget(){return auraTarget;}
    Application* GetTargetApplication(){return &app;}
    void PreventDefaultAction(){prevented=true;}
};
#define PrepareAuraScript(name) public:
#define AuraEffectRemoveFn(fn,...) &fn
#define AuraEffectPeriodicFn(fn,...) &fn
struct SpellScript
{
    Unit* source=nullptr;Unit* explicitTarget=nullptr;Hook OnCast;
    virtual void Register(){}
    Unit* GetCaster(){return source;}Unit* GetExplTargetUnit(){return explicitTarget;}
};
#define PrepareSpellScript(name) public:
#define SpellCastFn(fn) &fn
#include "Trash.inc"
struct Trail
{
    Creature* me;uint32 trailTimer=1000;
    void Update(uint32 diff)
    {
#include "Trail.inc"
    }
};
int Count(int id){return int(std::count_if(casts.begin(),casts.end(),[id](Cast const& c){return c.spell==id;}));}
int main()
{
    Player player, dead, gm, far;dead.alive=false;gm.gm=true;far.x=7;
    Creature keeper;keeper.entry=99359;
    npc_darkheart_rotheart_keeper k(&keeper);
    for(int difficulty:{1,2,23,8})
    {
        casts.clear();keeper.map.difficulty=difficulty;k.Reset();
        assert(Count(225497)==((difficulty==23||difficulty==8)?1:0));
    }
    casts.clear();k.EnterCombat(&player);k.UpdateAI(5999);assert(!Count(198799));
    k.UpdateAI(1);assert(Count(198799)==1);keeper.casting=true;k.UpdateAI(15000);assert(Count(198799)==1);
    keeper.casting=false;k.UpdateAI(0);assert(Count(198799)==2);
    Creature mushroom;mushroom.entry=100960;
    npc_darkheart_vile_mushroom m(&mushroom);m.Reset();
    assert(Count(198937)==1&&Count(198915)==1&&mushroom.react==REACT_PASSIVE);
    worldPlayers={&dead,&gm,&far};m.UpdateAI(3000);assert(!Count(198916));
    worldPlayers={&player};mushroom.removed=[&]{m.OnAreaTriggerDespawn(198915,{},true);};
    m.UpdateAI(1);assert(Count(198916)==1&&mushroom.despawn==100);
    m.UpdateAI(30000);m.OnAreaTriggerDespawn(198915,{},true);assert(Count(198916)==1);
    mushroom.removed=nullptr;casts.clear();worldPlayers.clear();m.Reset();m.UpdateAI(29999);assert(!Count(198916));
    m.UpdateAI(1);assert(Count(198916)==1);
    casts.clear();m.Reset();m.OnAreaTriggerDespawn(198915,{},false);m.OnAreaTriggerDespawn(1,{},true);assert(!Count(198916));
    m.OnAreaTriggerDespawn(198915,{},true);assert(Count(198916)==1);
    casts.clear();m.Reset();worldPlayers={&player};m.UpdateAI(2999);assert(!Count(198916));m.UpdateAI(1);assert(Count(198916)==1);
    // Owner cleanup removes helpers without running their proximity/expiry cast.
    k.JustSummoned(&mushroom);casts.clear();k.JustDied(&player);
    assert(mushroom.despawn&&!Count(198916)&&!keeper.HasAura(225497));

    Creature root;root.entry=101958;root.id=42;
    npc_darkheart_root_burst r(&root);casts.clear();r.Reset();assert(Count(201145)==1&&!Count(201123));
    Creature flower;npc_darkheart_vilethorn_blossom f(&flower);assert(!f.movement);
    selected=&player;spawnWarning=&root;creatures={&root};player.x=10;f.Reset();casts.clear();
    f.EnterCombat(&player);f.UpdateAI(20999);assert(!Count(201129));
    f.UpdateAI(1);assert(Count(201129)==1&&Count(201123)==1&&lastPlayersOnly);
    assert(casts.back().target==&root&&root.x==10);player.x=30;assert(root.x==10);
    f.Reset();assert(root.despawn);spawnWarning=nullptr;casts.clear();f.EnterCombat(&player);f.UpdateAI(21000);
    assert(Count(201129)==1&&!Count(201123)); // Failed summon cannot create an invisible hit.
    selected=nullptr;casts.clear();f.EnterCombat(&player);f.UpdateAI(21000);assert(!Count(201129));
    Creature second;second.entry=99359;second.x=20;second.auras.insert(225497);
    worldKeepers={&keeper,&second};keeper.x=20;keeper.alive=true;keeper.auras.insert(225497);
    flower.map.difficulty=23;f.UpdateInfusion(500);assert(flower.HasAura(225503));
    keeper.alive=false;f.UpdateInfusion(500);assert(flower.HasAura(225503)); // overlapping worldKeepers
    second.x=26;f.UpdateInfusion(500);assert(!flower.HasAura(225503));
    second.x=20;f.UpdateInfusion(500);assert(flower.HasAura(225503));
    flower.map.difficulty=1;f.UpdateInfusion(500);assert(!flower.HasAura(225503));

    spell_darkheart_tormenting_eye eye;eye.auraCaster=&keeper;eye.auraTarget=&player;
    for(int mode:{0,2,3}){casts.clear();eye.app.mode=mode;eye.OnRemove(nullptr,0);assert(!Count(204246));}
    eye.app.mode=AURA_REMOVE_BY_EXPIRE;eye.OnRemove(nullptr,0);assert(Count(204246)==1);
    eye.auraCaster=nullptr;eye.OnRemove(nullptr,0);assert(Count(204246)==1);
    spell_darkheart_noxious_spit spit;spit.auraCaster=&keeper;selected=&player;casts.clear();spit.Tick(nullptr);
    assert(spit.prevented&&Count(204973)==1&&lastPlayersOnly&&casts.back().target==&player);
    selected=nullptr;spit.Tick(nullptr);assert(Count(204973)==1);
    spit.auraCaster=nullptr;spit.Tick(nullptr);assert(Count(204973)==1);
    spell_darkheart_mythic_nightfall nightfall;nightfall.source=&keeper;nightfall.explicitTarget=&player;
    keeper.entry=96512;
    for(int difficulty:{1,2,23,8}){casts.clear();keeper.map.difficulty=difficulty;nightfall.Launch();assert(Count(198432)==((difficulty==23||difficulty==8)?1:0));}
    casts.clear();nightfall.explicitTarget=nullptr;nightfall.Launch();assert(!Count(198432));
    nightfall.source=nullptr;nightfall.Launch();assert(!Count(198432));
    nightfall.source=&keeper;nightfall.explicitTarget=&player;keeper.entry=99359;nightfall.Launch();assert(!Count(198432));
    spell_darkheart_growing_paranoia paranoia;paranoia.auraTarget=&player;
    for(int mode:{0,2,3}){casts.clear();paranoia.app.mode=mode;paranoia.OnRemove(nullptr,0);assert(!Count(200243));}
    paranoia.app.mode=AURA_REMOVE_BY_EXPIRE;paranoia.OnRemove(nullptr,0);
    assert(Count(200243)==1&&casts.back().caster==&player&&casts.back().target==&player);
    casts.clear();Trail trail{&keeper};trail.Update(999);assert(!Count(198464));trail.Update(1);assert(Count(198464)==1);
    keeper.x=35;trail.Update(1000);assert(Count(198464)==2&&casts.back().target==&keeper);
    std::cout<<"PASS: extracted production trash AI, native-helper handoff, arming/one-shot/reentry/cleanup, fixed warning target, pet-safe player selection, multi-keeper/difficulty healing, interrupted fear and missing-trigger fallback.\n";
}
