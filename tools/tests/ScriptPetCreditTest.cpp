#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t;
constexpr int SMART_ACTION_INVOKER_CAST=85, SMART_ACTION_CALL_KILLEDMONSTER=33, SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205;
constexpr int SPELL_EFFECT_KILL_CREDIT=90, SPELL_EFFECT_KILL_CREDIT2=134;
using CriteriaTypes=int;
#define TC_LOG_DEBUG(...) ((void)0)
#include "CastFlags.inc"
struct Unit; struct Player; struct Creature; struct Vehicle;
struct ObjectGuid {
    Unit* unit=nullptr;
    bool IsPlayer() const;
    static ObjectGuid Empty;
};
ObjectGuid ObjectGuid::Empty;
struct WorldObject {
    virtual ~WorldObject()=default;
    virtual Unit* ToUnit() { return nullptr; }
    virtual Player* ToPlayer() { return nullptr; }
    virtual ObjectGuid GetGUID() { return {ToUnit()}; }
};
struct Unit:WorldObject {
    Unit* owner=nullptr;
    Unit* charmer=nullptr;
    bool available=true, aura=false;
    int casts=0, interrupted=0;
    uint32 lastSpell=0;
    Unit* lastTarget=nullptr;
    Vehicle* vehicle=nullptr;
    std::vector<std::function<void()>> delayed;
    Unit* ToUnit() override { return this; }
    virtual Player* ToPlayer() { return nullptr; }
    virtual Player const* ToPlayer() const { return nullptr; }
    ObjectGuid GetGUID() { return {this}; }
    ObjectGuid GetCharmerOrOwnerGUID() const { return {charmer ? charmer : owner}; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
    Vehicle* GetVehicleKit() { return vehicle; }
    bool HasAura(uint32) { return aura; }
    void InterruptNonMeleeSpells(bool) { ++interrupted; }
    template<class F> void AddDelayedEvent(int delay,F f) { assert(delay==200); delayed.emplace_back(f); }
    void Flush() { auto pending=std::move(delayed); delayed.clear(); for(auto& f:pending) f(); }
    void CastSpell(Unit* target,uint32 spell,bool);
};
struct Player:Unit {
    int credits=0, events=0, criteria=0;
    Unit* criterionInvoker=nullptr;
    WorldObject* eventSource=nullptr;
    Player* ToPlayer() override { return this; }
    Player const* ToPlayer() const override { return this; }
    void KilledMonsterCredit(uint32) { ++credits; }
    void RewardPlayerAndGroupAtEvent(uint32,WorldObject* source) { ++events; eventSource=source; }
    void UpdateAchievementCriteria(CriteriaTypes,uint32,uint32,uint32,Unit* actualInvoker,bool) { ++criteria; criterionInvoker=actualInvoker; }
};
struct Creature:Unit { Player* loot=nullptr; Player* GetLootRecipient() { return loot; } };
struct Vehicle { struct Seat { struct { ObjectGuid Guid; } Passenger; }; std::map<int,Seat> Seats; };
bool ObjectGuid::IsPlayer() const { return unit && unit->ToPlayer(); }
struct ObjectAccessor {
    static Player* GetPlayer(Unit const&,ObjectGuid guid) { return guid.unit && guid.unit->available ? guid.unit->ToPlayer() : nullptr; }
    static Player* FindPlayer(ObjectGuid guid) { return guid.unit && guid.unit->available ? guid.unit->ToPlayer() : nullptr; }
    static Unit* GetUnit(Unit const&,ObjectGuid guid) { return guid.unit && guid.unit->available ? guid.unit : nullptr; }
};
#include "OwnerPlayer.inc"
struct SpellEffectInfo { int Effect=0; bool IsEffect() const { return Effect!=0; } };
struct SpellInfo {
    bool personal=false,group=false;
    SpellEffectInfo credit,extra;
    SpellEffectInfo const* Effects[2];
    SpellInfo(bool p=false,bool g=false,bool mixed=false):personal(p),group(g),credit{p?134:(g?90:0)},extra{mixed?2:0},Effects{&credit,&extra} {}
    bool HasEffect(int effect) const { return (effect==134 && personal) || (effect==90 && group); }
};
struct SpellManager {
    SpellInfo personal{true,false},group{false,true},ordinary{},mixed{true,false,true};
    SpellInfo const* GetSpellInfo(uint32 id) { if(id==198287)return &personal; if(id==900)return &group; if(id==123)return &ordinary; if(id==901)return &mixed; return nullptr; }
} spellManager;
auto sSpellMgr=&spellManager;
void Unit::CastSpell(Unit* target,uint32 spell,bool) {
    ++casts; lastSpell=spell; lastTarget=target;
    // Model the unchanged player-only/self-target requirement of198287.
    if(spell==198287) if(Player* player=ToPlayer()) player->KilledMonsterCredit(100732);
}
#include "CreditHelpers.inc"
using ObjectList=std::list<WorldObject*>;
struct SmartScriptHolder {
    int type=33;
    struct {
        struct { uint32 spell=198287,flags=0; } cast;
        struct { uint32 creature=100732; } killedMonster;
        struct { int type=92; uint32 misc1=50959,misc2=0,misc3=0; } achievementCriteria;
    } action;
};
struct SmartScript {
    Creature* me=nullptr;
    WorldObject* base=nullptr;
    bool trigger=false;
    Unit* invoker=nullptr;
    ObjectList selection;
    WorldObject* GetBaseObject() { return base; }
    Unit* GetLastInvoker() { return invoker; }
    ObjectList* GetTargets(SmartScriptHolder&,Unit*) { return selection.empty() ? nullptr : new ObjectList(selection); }
    bool IsUnit(WorldObject* object) { return object && object->ToUnit(); }
    bool IsPlayer(WorldObject* object) { return object && object->ToUnit() && object->ToUnit()->ToPlayer(); }
    void Run(SmartScriptHolder& e,Unit* unit) { invoker=unit; switch(e.type) {
#include "KillAction.inc"
#include "CriteriaAction.inc"
#include "InvokerCast.inc"
    }}
};
int main() {
    Player owner,other;
    Unit pet,guardian,charmed,npc,ownerless,npcOwned;
    pet.owner=&owner; guardian.owner=&owner; charmed.charmer=&owner; npcOwned.owner=&npc;
    WorldObject gameobject;
    for(Unit* u:{static_cast<Unit*>(&owner),&pet,&guardian,&charmed}) assert(GetScriptCreditPlayer(u)==&owner);
    for(WorldObject* u:std::initializer_list<WorldObject*>{&ownerless,&npcOwned,&gameobject,nullptr}) assert(!GetScriptCreditPlayer(u));
    owner.available=false; assert(!GetScriptCreditPlayer(&pet)); owner.available=true;
    assert(GetScriptCreditPlayer(&owner)==&owner);
    Creature victim; WorldObject base;
    SmartScript script; script.me=&victim; script.base=&base;
    SmartScriptHolder action;
    script.selection={&pet}; action.type=33; script.Run(action,&pet); assert(owner.events==1);
    action.type=205; script.Run(action,&pet); assert(owner.criteria==1);
    owner.events=owner.criteria=0;
    // Player, pets and guardians in the same selection grant one reward per owner.
    script.selection={&owner,&pet,&guardian,&charmed,&gameobject,&npcOwned,&other};
    action.type=33; script.Run(action,&pet); assert(owner.events==1 && other.events==1 && owner.eventSource==&owner);
    action.type=205; script.Run(action,&pet); assert(owner.criteria==1 && other.criteria==1 && owner.criterionInvoker==&pet);
    // Existing loot recipient takes priority, including when somebody else's pet kills.
    victim.loot=&other; script.selection={&pet}; action.type=33; script.Run(action,&pet);
    assert(other.events==2 && owner.events==1); victim.loot=nullptr;
    Vehicle vehicle; vehicle.Seats[0].Passenger.Guid=owner.GetGUID(); pet.vehicle=&vehicle;
    script.selection={&pet,&owner}; script.Run(action,&pet); assert(owner.events==2); pet.vehicle=nullptr;
    // No actor/base, as with a trigger: retain the original event source for group range checks.
    script.me=nullptr; script.base=nullptr; script.trigger=true; script.Run(action,&pet);
    assert(owner.events==3 && owner.eventSource==&pet);
    script.Run(action,nullptr); script.Run(action,&ownerless); assert(owner.events==3);
    // Pet-triggered kill-credit spell: caster and invoker-target both become the player.
    script.me=&victim; script.base=&base; script.trigger=false;
    action.type=85; script.selection={&pet,&owner}; script.Run(action,&pet);
    owner.Flush(); pet.Flush(); assert(owner.casts==1 && pet.casts==0 && owner.lastTarget==&owner && owner.credits==1);
    // Explicit corpse target is retained. Native SELF credit and linked casts still belong to the player.
    script.selection={&victim}; script.Run(action,&guardian); owner.Flush();
    assert(owner.casts==2 && owner.lastTarget==&victim && owner.credits==2);
    action.action.cast.spell=900; script.selection={&pet}; script.Run(action,&pet); owner.Flush(); assert(owner.casts==3);
    // Ordinary spells, including repeated target records, retain their real caster and targets.
    action.action.cast.spell=123; script.selection={&pet,&pet}; script.Run(action,&pet); pet.Flush();
    assert(pet.casts==2 && pet.lastTarget==&pet && owner.casts==3);
    action.action.cast.spell=999; script.selection={&pet}; script.Run(action,&pet); pet.Flush(); assert(pet.casts==3);
    action.action.cast.spell=901; script.Run(action,&pet); pet.Flush(); assert(pet.casts==4 && owner.casts==3);
    // A missing/disconnected owner never redirects credit to an unrelated player.
    owner.available=false; action.action.cast.spell=198287; script.Run(action,&pet); pet.Flush();
    assert(owner.credits==2 && other.credits==0); owner.available=true;
    // Caster normalization does not replace the stored event invoker.
    script.Run(action,&pet); assert(script.GetLastInvoker()==&pet); owner.Flush();
    std::cout<<"PASS: production SmartAI credit/criteria/invoker-cast; pets, guardians, charmers, ownerless/NPC-owned/non-unit/null, dedup, loot/vehicle/trigger source, ordinary spell isolation and raw invoker preservation\n";
}
