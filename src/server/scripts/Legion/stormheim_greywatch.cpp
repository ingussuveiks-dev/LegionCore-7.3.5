#include "ScriptMgr.h"
#include "SpellScript.h"
#include "Player.h"
#include "Creature.h"
#include <memory>

namespace StormheimGreywatch
{
constexpr uint32 QuestId=39060;
bool Active(Player* p)
{
    return p && p->IsInWorld() && p->IsAlive() && p->GetMapId()==1220 && p->GetZoneId()==7541 && !p->GetInstanceId() &&
        (p->GetQuestStatus(QuestId)==QUEST_STATUS_INCOMPLETE || p->GetQuestStatus(QuestId)==QUEST_STATUS_COMPLETE);
}
bool Cache(Player* p,Unit* unit)
{
    auto c=unit?unit->ToCreature():nullptr;
    if(!Active(p) || !c || !c->IsInWorld() || !c->IsAlive() || c->GetMap()!=p->GetMap() || !p->InSamePhase(c))return false;
    // These are stationary native credit actors, not every reuse of their entry.
    auto entry=c->GetEntry();auto spawn=c->GetDBTableGUIDLow();
    return (entry==94340 && spawn==292386) || (entry==94341 && spawn==292849) || (entry==94342 && spawn==292844);
}
}

// Keep the existing SpellHit -> SmartAI33 credit independent of cosmetic casts.
class spell_stormheim_blight_torch : public SpellScript
{
    PrepareSpellScript(spell_stormheim_blight_torch);
    void Hit(SpellEffIndex)
    {
        auto p=GetCaster()->ToPlayer();auto target=GetHitUnit();
        if(!StormheimGreywatch::Cache(p,target) || p->GetQuestStatus(StormheimGreywatch::QuestId)!=QUEST_STATUS_INCOMPLETE ||
            p->GetQuestObjectiveData(StormheimGreywatch::QuestId,target->GetEntry()))return;
        p->CastSpell(target,186746,true);
    }
    void Register() override {OnEffectHitTarget+=SpellEffectFn(spell_stormheim_blight_torch::Hit,EFFECT_0,SPELL_EFFECT_DUMMY);}
};

class spell_stormheim_blight_throw : public SpellScript
{
    PrepareSpellScript(spell_stormheim_blight_throw);
    ObjectGuid targetGuid;
    std::weak_ptr<uint8> lifetime;
    bool scoped=false;
    SpellCastResult Check()
    {
        auto p=GetCaster()->ToPlayer();auto target=GetExplTargetUnit();
        if(!StormheimGreywatch::Cache(p,target))return SPELL_FAILED_BAD_TARGETS;
        // Native target38 searches nearby; the effect must retain the exact
        // actor hit by186745, including when other players stand closer.
        targetGuid=target->GetGUID();p->AddQuestDelayedEvent(StormheimGreywatch::QuestId,0,[]{});
        lifetime=p->getQuestStatus(StormheimGreywatch::QuestId)->ScriptLifetime;scoped=true;
        return SPELL_CAST_OK;
    }
    bool Valid(Unit* target)
    {
        auto p=GetCaster()->ToPlayer();
        return scoped && !lifetime.expired() && StormheimGreywatch::Cache(p,target) && target->GetGUID()==targetGuid &&
            lifetime.lock()==p->getQuestStatus(StormheimGreywatch::QuestId)->ScriptLifetime;
    }
    void Select(WorldObject*& target)
    {
        auto exact=GetExplTargetUnit();target=Valid(exact)?exact:nullptr;
    }
    void Hit(SpellEffIndex)
    {
        auto target=GetHitUnit();
        if(!Valid(target))return;
        // Add the native dummy aura to the exact impact actor. Casting747's
        // nearby-entry selector could instead burn the player or a bystander.
        // Its native600000ms duration expires normally; no progression uses it.
        if(!target->HasAura(186747))GetCaster()->AddAura(186747,target);
    }
    void Register() override
    {
        OnCheckCast+=SpellCheckCastFn(spell_stormheim_blight_throw::Check);
        OnObjectTargetSelect+=SpellObjectTargetSelectFn(spell_stormheim_blight_throw::Select,EFFECT_0,TARGET_UNIT_NEARBY_ENTRY);
        OnEffectHitTarget+=SpellEffectFn(spell_stormheim_blight_throw::Hit,EFFECT_0,SPELL_EFFECT_DUMMY);
    }
};

void AddSC_stormheim_greywatch()
{
    // Diagnostic also makes the deployed script implementation distinguishable.
    TC_LOG_INFO("server.loading","Greywatch: native torch throw and exact cache burn");
    RegisterSpellScript(spell_stormheim_blight_torch);
    RegisterSpellScript(spell_stormheim_blight_throw);
}
