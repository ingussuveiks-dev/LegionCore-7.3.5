#pragma once

namespace MonkKiteTravel
{
    bool Eligible(Player* player)
    {
        return player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
            !player->HasPendingTeleport() && player->getClass() == CLASS_MONK && player->GetMapId() == 1514 &&
            !player->GetQuestRewardStatus(40633) &&
            (player->GetQuestStatus(40633) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40633) == QUEST_STATUS_COMPLETE);
    }
}

// Native click202475 owns personal90/102531 and force-cast202474. The former
// unconditional ON_SPELLCLICK also granted group credit and teleported after
// rejected conditions/casts. Preserve the native teleport only after real hit.
class spell_monk_kite_travel : public SpellScript
{
    PrepareSpellScript(spell_monk_kite_travel);
    std::weak_ptr<uint8> lifetime;
    Map* origin = nullptr; // identity only, never dereferenced after a callback
    uint32 instance = 0;
    bool bound = false, delivered = false;

    bool Current(Player* player) const
    {
        QuestStatusData* quest = player ? player->getQuestStatus(40633) : nullptr;
        return bound && MonkKiteTravel::Eligible(player) && quest && !lifetime.expired() &&
            quest->ScriptLifetime == lifetime.lock() && player->GetMap() == origin && player->GetInstanceId() == instance;
    }

    bool Validate(SpellInfo const* info) override
    {
        return ValidateSpellInfo({202474,202481}) && info && info->Id == 202475 &&
            info->Effects[EFFECT_0] && info->Effects[EFFECT_0]->Effect == SPELL_EFFECT_FORCE_CAST &&
            info->Effects[EFFECT_0]->TriggerSpell == 202474 &&
            info->Effects[EFFECT_2] && info->Effects[EFFECT_2]->Effect == SPELL_EFFECT_KILL_CREDIT &&
            info->Effects[EFFECT_2]->MiscValue == 102531;
    }

    SpellCastResult Check()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!MonkKiteTravel::Eligible(player)) return SPELL_FAILED_BAD_TARGETS;
        QuestStatusData* quest = player->getQuestStatus(40633);
        if (!quest) return SPELL_FAILED_BAD_TARGETS;
        if (bound)
        {
            if (!Current(player)) return SPELL_FAILED_BAD_TARGETS;
        }
        else
        {
            if (!quest->ScriptLifetime) quest->ScriptLifetime = std::make_shared<uint8>();
            lifetime = quest->ScriptLifetime;
            origin = player->GetMap(); instance = player->GetInstanceId(); bound = true;
        }
        return SPELL_CAST_OK;
    }

    void Guard(SpellEffIndex effect)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!Current(player) || GetHitUnit() != player) PreventHitDefaultEffect(effect);
    }

    void After()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (delivered || !Current(player) || GetHitUnit() != player || player->GetQuestObjectiveData(40633,102531) <= 0) return;
        delivered = true;
        SpellCastResult result = player->CastSpell(player,202481,true);
        if (result != SPELL_CAST_OK)
            TC_LOG_WARN("scripts", "Monk kite: native202481 rejected (%u) after personal102531; retain native credit and allow a new kite click", uint32(result));
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_monk_kite_travel::Check);
        OnEffectHitTarget += SpellEffectFn(spell_monk_kite_travel::Guard,EFFECT_ALL,SPELL_EFFECT_ANY);
        AfterHit += SpellHitFn(spell_monk_kite_travel::After);
    }
};
