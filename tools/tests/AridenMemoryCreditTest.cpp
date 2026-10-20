#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <list>
#include <vector>
using uint32 = uint32_t;
constexpr int SMART_ACTION_CAST=11, SMART_ACTION_INVOKER_CAST=85;
constexpr int SMART_TARGET_OWNER_OR_SUMMONER=23, SMART_TARGET_SELF=1, SMART_TARGET_POSITION=8;
constexpr int CONDITION_NEAR_CREATURE=29, SPELL_EFFECT_HANDLE_HIT_TARGET=1;
constexpr int SPELL_CAST_OK=0, SPELL_FAILED_BAD_TARGETS=1, SPELL_FAILED_SPELL_IN_PROGRESS=2;
using SpellCastResult=int;
using SpellEffIndex=int;
#define TC_LOG_DEBUG(...) ((void)0)
#define ENSURE_AI(type, ai) (ai)
#include "CastFlags.inc"
struct Unit;
struct ObjectAccessor { static Unit* GetUnit(Unit&, Unit* guid) { return guid; } };
using ObjectGuid=Unit*;
struct FakeAI { void SetCombatMove(bool) {} };
struct Unit {
    bool player=false, alive=true, castWorks=true, temporary=false;
    int entry=0, credits=0, conversations=0, casts=0;
    float x=0;
    Unit* owner=nullptr;
    Unit* castTarget=nullptr;
    FakeAI ai;
    std::vector<std::function<void()>> delayed;
    Unit* ToUnit() { return this; }
    Unit* ToPlayer() { return player ? this : nullptr; }
    bool IsPlayer() { return player; }
    bool IsUnit() { return true; }
    FakeAI* AI() { return &ai; }
    bool GetCharmerGUID() const { return false; }
    Unit* GetCharmer() const { return nullptr; }
    Unit const* ToTempSummon() const { return temporary ? this : nullptr; }
    Unit* GetSummoner() const { return owner; }
    Unit* GetOwner() const { return temporary ? nullptr : owner; }
    Unit* GetAnyOwner() const;
    ObjectGuid GetGUID() { return this; }
    bool HasAura(uint32) { return false; }
    void InterruptNonMeleeSpells(bool) {}
    void GetPosition(float& px,float& py,float& pz) { px=x; py=pz=0; }
    void KilledMonsterCredit(int id) { assert(id==100732); ++credits; }
    template<class F> void AddDelayedEvent(int, F f) { delayed.emplace_back(f); }
    void Flush() { for(auto& f:delayed) f(); delayed.clear(); }
    int CastSpell(Unit*,uint32,bool);
    int CastSpell(float,float,float,uint32,bool) { return SPELL_FAILED_BAD_TARGETS; }
};
#include "AnyOwner.inc"
struct Effect { int MiscValue=100732; };
struct Info { Effect effect; Effect* GetEffect(int,int) { return &effect; } };
struct Spell {
    int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET, m_diffMode=0;
    Unit* unitTarget;
    Info* m_spellInfo;
    void EffectKillCreditPersonal(SpellEffIndex);
};
#include "PersonalCredit.inc"
int Unit::CastSpell(Unit* target,uint32 spell,bool) {
    assert(spell==198287);
    ++casts; castTarget=target;
    if(!castWorks) return SPELL_FAILED_BAD_TARGETS;
    Info info;
    Spell credit{SPELL_EFFECT_HANDLE_HIT_TARGET,0,this,&info}; // Native target1 is caster SELF.
    credit.EffectKillCreditPersonal(0);
    ++conversations; // Linked visuals cannot undo the preceding personal credit.
    return SPELL_CAST_OK;
}
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder {
    int actionType=11,targetType=23;
    struct { struct { uint32 spell=198287,flags=18; } cast; } action;
    struct { float x=0,y=0,z=0; } target;
    int GetTargetType() { return targetType; }
};
struct FakeGO { int CastSpell(Unit*,uint32) { return SPELL_FAILED_BAD_TARGETS; } };
struct SmartScript {
    Unit* me;
    Unit* invoker=nullptr;
    FakeGO* go=nullptr;
    Unit* GetLastInvoker() { return invoker; }
    bool IsUnit(Unit* u) { return u!=nullptr; }
    ObjectList* GetTargets(SmartScriptHolder& e,Unit*) {
        auto l=new ObjectList;
        switch(e.targetType) {
#include "OwnerTarget.inc"
        case SMART_TARGET_SELF: l->push_back(me); break;
        }
        return l;
    }
    void RecalcTimer(SmartScriptHolder&,int,int) {}
    void Run(SmartScriptHolder& e,Unit* unit) { invoker=unit; switch(e.actionType) {
#include "CastAction.inc"
#include "InvokerCast.inc"
    }}
};
ObjectList world;
Unit* GetClosestCreatureWithEntry(Unit* object,uint32 entry,float range) {
    for(auto u:world) if(u->alive && u->entry==int(entry) && std::abs(u->x-object->x)<=range) return u;
    return nullptr;
}
bool Near(Unit* object,uint32 ConditionValue1,uint32 ConditionValue2) {
    bool condMeets=false; switch(CONDITION_NEAR_CREATURE) {
#include "NearCondition.inc"
    } return condMeets;
}
int main(int argc,char** argv) {
    assert(argc==5);
    SmartScriptHolder fixed;
    fixed.actionType=std::atoi(argv[1]); fixed.action.cast.flags=std::atoi(argv[2]);
    fixed.targetType=std::atoi(argv[3]); int conditionTarget=std::atoi(argv[4]);
    assert(fixed.actionType==11 && fixed.action.cast.flags==18 && fixed.targetType==23 && conditionTarget==1);
    Unit owner; owner.player=true;
    Unit pet; pet.owner=&owner; pet.x=100;
    Unit memories[2]; for(auto& m:memories) { m.entry=100735; m.owner=&owner; m.temporary=true; assert(!m.GetOwner()); }
    memories[1].x=10;
    Unit outsider; outsider.player=true;
    for(int order:{0,1}) for(Unit* killer:{&owner,&pet,static_cast<Unit*>(nullptr)}) {
        owner.credits=owner.casts=owner.conversations=0;
        for(auto& m:memories) m.alive=true;
        world={&memories[0],&memories[1],&owner,&pet,&outsider};
        auto& first=memories[order]; auto& last=memories[1-order];
        first.alive=false;
        assert(Near(&first,100735,30)); // First death must not finish the quest.
        assert(!Near(&pet,100735,30)); // Old killer-origin condition could finish early.
        last.alive=false; assert(!Near(&last,100735,30));
        SmartScript script{&last}; script.Run(fixed,killer);
        assert(owner.credits==1 && owner.casts==1 && owner.castTarget==&owner);
        assert(!pet.credits && !outsider.credits && !outsider.casts);
    }
    // Reproduce old action85: the pet self-target loses native player-only credit.
    owner.credits=0; memories[0].alive=false;
    SmartScriptHolder old; old.actionType=85; old.targetType=1; old.action.cast.flags=0;
    SmartScript script{&memories[0]}; script.Run(old,&pet); pet.Flush();
    assert(!owner.credits && pet.castTarget==&memories[0]);
    // No summoner means no accidental fallback to an unrelated killer/bystander.
    memories[0].owner=nullptr; script.Run(fixed,&outsider); assert(!outsider.casts);
    std::cout << "PASS: native SmartAI owner selection/cast, player-only credit, last-death condition; both orders and player/pet/null killers; old pet defect reproduced\n";
}
