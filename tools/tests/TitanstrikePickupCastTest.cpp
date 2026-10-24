#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
using ObjectGuid=uint32;
using ConditionSourceType=uint32;
#ifndef PICKUP_SPELL_ID
#define PICKUP_SPELL_ID 212010
#endif
constexpr uint32 pickupSpellId=PICKUP_SPELL_ID;
enum { CONDITION_SOURCE_TYPE_NONE=0, CONDITION_SOURCE_TYPE_SPELL=17, CONDITION_SOURCE_TYPE_MAX=32 };
enum SpellCastResult { SPELL_CAST_OK, SPELL_FAILED_CASTER_AURASTATE, SPELL_FAILED_BAD_TARGETS, SPELL_FAILED_DONT_REPORT };
#define TC_LOG_DEBUG(...) ((void)0)
struct Player;
struct Unit {
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    std::vector<std::function<void()>> pending;
    bool targetPresent=true;
    uint32 casts=0,effects=0;
    SpellCastResult last=SPELL_CAST_OK;
    ObjectGuid GetGUID() const { return 19; }
    bool IsInWorld() const { return true; }
    int GetMap() const { return 1; }
    bool InSamePhase(Unit*) const { return true; }
    bool IsWithinDistInMap(Unit*,float) const { return true; }
    bool HasQuestObjectScope() const { return false; }
    bool CanTargetQuestObject(Unit*) const { return true; }
    void AddDelayedEvent(uint32 delay,std::function<void()> callback) { assert(delay==200);pending.push_back(callback); }
    void Flush() { auto callbacks=std::move(pending);pending.clear();for(auto& callback:callbacks)callback(); }
    SpellCastResult CastSpell(Unit*,uint32,bool);
};
using WorldObject=Unit;
struct Player : Unit {
    bool allowed=true;
    Player* ToPlayer() override { return this; }
    struct Session { void SendNotification(uint32) { assert(false); } } session;
    Session* GetSession() { return &session; }
};
struct Condition { uint32 ErrorTextId=0,ConditionTarget=0; } condition;
using ConditionList=std::vector<Condition*>;
using ConditionTypeContainer=std::map<uint32,ConditionList>;
using ConditionContainer=std::map<ConditionSourceType,ConditionTypeContainer>;
struct ConditionSourceInfo { Unit* mConditionTargets[2];Condition* mLastFailedCondition=nullptr;explicit ConditionSourceInfo(Unit* unit):mConditionTargets{unit,nullptr} {} };
struct ConditionMgr {
    ConditionContainer ConditionStore;
    ConditionList GetConditionsForNotGroupedEntry(ConditionSourceType,uint32);
    bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& rows) {
        assert(info.mConditionTargets[0]==info.mConditionTargets[1] && rows.size()==1);
        Player* player=info.mConditionTargets[0]->ToPlayer();
        if(player && player->allowed)return true;
        info.mLastFailedCondition=rows.front();return false;
    }
} conditionManager;
auto* sConditionMgr=&conditionManager;
#include "NotGroupedConditions.inc"
struct SpellInfo { uint32 Id=pickupSpellId; } spellInfo;
struct Spell {
    Unit* m_caster;
    SpellInfo* m_spellInfo=&spellInfo;
    struct Targets { Unit* target;Unit* GetObjectTarget() const { return target; } } m_targets;
    SpellCastResult Check() {
#include "CastConditions.inc"
        return SPELL_CAST_OK;
    }
};
SpellCastResult Unit::CastSpell(Unit* target,uint32 identity,bool triggered) {
    assert(target==this && identity==pickupSpellId && !triggered);++casts;
    Spell spell{this,&spellInfo,{target}};last=spell.Check();
    if(last==SPELL_CAST_OK)++effects;
    return last;
}
struct ObjectAccessor {
    static Unit* GetUnit(Unit& context,ObjectGuid identity) { return context.targetPresent && identity==context.GetGUID()?&context:nullptr; }
    static WorldObject* GetWorldObject(Unit&,ObjectGuid) { assert(false);return nullptr; }
};
void Queue(Player& player) {
    Unit* tempLastInvoker=&player;
    ObjectGuid targetGUID=player.GetGUID(),castSource=0;
    uint32 spellID=pickupSpellId,playerCastDistance=0;
    bool isTrigger=false;
#include "DeferredInvoker.inc"
}
int main() {
    Player baseline;Queue(baseline);baseline.allowed=false;baseline.Flush();
    assert(baseline.casts==1 && baseline.effects==1);
    std::cout<<"BASELINE: native200ms pickup queue reaches an empty source17 condition list after context changes and still accepts the presentation\n";
    conditionManager.ConditionStore[CONDITION_SOURCE_TYPE_SPELL][pickupSpellId]={&condition};
    for(bool allowed:{false,true}) {
        Player player;Queue(player);assert(player.casts==0 && player.pending.size()==1);
        player.allowed=allowed;player.Flush();player.Flush();
        assert(player.casts==1 && player.effects==uint32(allowed));
        assert(player.last==(allowed?SPELL_CAST_OK:SPELL_FAILED_CASTER_AURASTATE));
    }
    Player retry;Queue(retry);retry.allowed=false;retry.Flush();assert(retry.effects==0);
    retry.allowed=true;Queue(retry);retry.Flush();assert(retry.casts==2 && retry.effects==1);
    Player missing;Queue(missing);missing.targetPresent=false;missing.Flush();assert(missing.casts==0 && missing.effects==0);
    Unit pet;pet.CastSpell(&pet,pickupSpellId,false);assert(pet.casts==1 && pet.effects==0 && pet.last==SPELL_FAILED_CASTER_AURASTATE);
    assert(conditionManager.GetConditionsForNotGroupedEntry(17,pickupSpellId+1).empty());
    assert(conditionManager.GetConditionsForNotGroupedEntry(18,pickupSpellId).empty());
    std::cout<<"PASS: actual native200ms invoker payload -> native spell source17 lookup/check branch revalidates at execution, reports native caster-state failure before effect sink, accepts a later valid retry and leaves other spells/source types separate. Condition decision and spell effect dispatch are controlled hosts.\n";
}
