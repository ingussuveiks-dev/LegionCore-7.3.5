#ifndef ASHBRINGER_STORAGE_H
#define ASHBRINGER_STORAGE_H

#include "Item.h"
#include "QuestData.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "SpellScript.h"

namespace AshbringerStorage
{
    inline Item* Inventory(Player* player)
    {
        auto item = player ? player->GetItemByEntry(120978) : nullptr;
        return item && item->GetOwnerGUID() == player->GetGUID() ? item : nullptr;
    }

    // Same immutable trigger provenance as native EffectTriggerSpell. Do not
    // retain its temporary Spell pointer in the player's delayed queue.
    struct Trigger
    {
        ObjectGuid cast, originalCaster, item;
        int32 castTime = 0;
        SpellPowerCost cost;

        explicit Trigger(Spell* parent) :
            cast(parent->m_castGuid[1].GetSubType() == SPELL_CAST_TYPE_NORMAL ? parent->m_castGuid[1] : parent->m_castGuid[0]),
            originalCaster(parent->GetOriginalCaster() ? parent->GetOriginalCaster()->GetGUID() : ObjectGuid::Empty),
            item(parent->m_CastItem ? parent->m_CastItem->GetGUID() : ObjectGuid::Empty),
            castTime(parent->GetCastTime()), cost(parent->m_powerCost) { }

        SpellCastResult Cast(Player* player, uint32 id) const
        {
            auto info = sSpellMgr->GetSpellInfo(id);
            if (!info) return SPELL_FAILED_SPELL_UNAVAILABLE;
            SpellCastTargets targets;
            targets.SetCaster(player);
            targets.SetUnitTarget(player);
            TriggerCastData data;
            data.triggerFlags = TRIGGERED_FULL_MASK;
            data.spellGuid = cast;
            data.originalCaster = originalCaster;
            data.casttime = castTime;
            data.SubType = SPELL_CAST_TYPE_MISSILE;
            data.powerCost = cost;
            data.castItem = player->GetItemByGuid(item);
            return player->CastSpell(targets, info, nullptr, data);
        }
    };

    inline bool Receipt(Scenario* scenario)
    {
        if (!scenario) return false;
        for (auto const& row : *scenario->GetAchievementMgr().GetCriteriaProgressMap())
        {
            auto tree = sCriteriaTreeStore.LookupEntry(row.first);
            auto criterion = tree ? sCriteriaStore.LookupEntry(tree->CriteriaID) : nullptr;
            if (criterion && criterion->Type == CRITERIA_TYPE_SCRIPT_EVENT_2 &&
                criterion->Asset == 46130 && row.second.Counter) return true;
        }
        return false;
    }

    struct Delivery
    {
        ObjectGuid owner, weapon;
        std::weak_ptr<uint8> attempt;
        uint32 instance, spell;
        Trigger trigger;

        void Run(Player* player, uint8 tries = 0) const
        {
            auto status = player->getQuestStatus(38376);
            auto item = Inventory(player);
            if (attempt.expired() || !status || status->ScriptLifetime != attempt.lock() ||
                player->GetGUID() != owner || player->GetQuestRewardStatus(38376) ||
                (player->GetQuestStatus(38376) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(38376) != QUEST_STATUS_COMPLETE) ||
                !item || item->GetGUID() != weapon) return;
            auto scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
            if (player->IsInWorld() && player->CanContact() && player->GetMapId() == 1500 &&
                player->GetInstanceId() == instance && scenario && scenario->GetScenarioId() == 775)
            {
                if (spell == 190777 && Receipt(scenario)) return;
                auto result = trigger.Cast(player, spell);
                if (spell == 190777 ? Receipt(scenario) : result == SPELL_CAST_OK) return;
            }
            if (tries < 29)
                player->AddQuestDelayedEvent(38376, 1000, [player, pending = *this, tries]() { pending.Run(player, tries + 1); });
            else
                TC_LOG_WARN("scripts", "Ashbringer pickup: stored weapon's native follow-up %u was not acknowledged; original quest attempt retained without forced progress", spell);
        }
    };
}

class spell_bs_ashbringer_storage : public SpellScript
{
    PrepareSpellScript(spell_bs_ashbringer_storage);
    ObjectGuid _player, _weapon;
    std::weak_ptr<uint8> _attempt;
    bool _issued = false;

    bool Validate(SpellInfo const* info) override
    {
        auto child = sSpellMgr->GetSpellInfo(199827);
        auto event = sSpellMgr->GetSpellInfo(190777);
        return info->Id == 180850 && info->GetEffect(EFFECT_2, DIFFICULTY_NONE)->Effect == SPELL_EFFECT_TRIGGER_SPELL &&
            info->GetEffect(EFFECT_2, DIFFICULTY_NONE)->TriggerSpell == 199827 &&
            info->GetEffect(EFFECT_2, DIFFICULTY_NONE)->MiscValue == 0 &&
            info->GetEffect(EFFECT_3, DIFFICULTY_NONE)->TriggerSpell == 186987 &&
            info->GetEffect(EFFECT_4, DIFFICULTY_NONE)->TriggerSpell == 192145 &&
            info->GetEffect(EFFECT_5, DIFFICULTY_NONE)->TriggerSpell == 190777 &&
            info->GetEffect(EFFECT_6, DIFFICULTY_NONE)->ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
            info->GetEffect(EFFECT_6, DIFFICULTY_NONE)->TriggerSpell == 181257 &&
            event && event->GetEffect(EFFECT_0, DIFFICULTY_NONE)->Effect == SPELL_EFFECT_SEND_EVENT &&
            event->GetEffect(EFFECT_0, DIFFICULTY_NONE)->MiscValue == 46130 &&
            child && !child->CalcCastTime() && child->GetEffect(EFFECT_0, DIFFICULTY_NONE)->Effect == SPELL_EFFECT_CREATE_ITEM_2 &&
            !child->GetEffect(EFFECT_0, DIFFICULTY_NONE)->ItemType;
    }

    SpellCastResult Check()
    {
        auto player = GetCaster()->ToPlayer();
        if (!player) return SPELL_FAILED_BAD_TARGETS;
        std::list<GameObject*> sources;
        player->GetGameObjectListWithEntryInGrid(sources, 247318, player->GetVisibilityRange());
        bool admitted = false;
        for (auto source : sources)
            if (AshbringerAcquisition::CanRetrieve(player, source)) { admitted = true; break; }
        if (!admitted) return SPELL_FAILED_BAD_TARGETS;
        if (player->HasAura(180850)) return SPELL_FAILED_SPELL_IN_PROGRESS;
        player->AddQuestDelayedEvent(38376, 0, []() { });
        auto status = player->getQuestStatus(38376);
        if (!status || !status->ScriptLifetime) return SPELL_FAILED_BAD_TARGETS;
        _player = player->GetGUID();
        _attempt = status->ScriptLifetime;
        return SPELL_CAST_OK;
    }

    bool Stored() const
    {
        auto player = GetCaster()->ToPlayer();
        auto status = player ? player->getQuestStatus(38376) : nullptr;
        auto item = AshbringerStorage::Inventory(player);
        return _issued && !_attempt.expired() && status && status->ScriptLifetime == _attempt.lock() &&
            player->GetGUID() == _player && item && item->GetGUID() == _weapon;
    }

    void Create(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        if (_issued) return;
        _issued = true;
        auto player = GetCaster()->ToPlayer();
        if (!player) return;
        // Only this native inventory child is synchronous. The generic core's
        //10ms callback otherwise lets credit and scene effects escape failure.
        if (!AshbringerStorage::Inventory(player))
            AshbringerStorage::Trigger(GetSpell()).Cast(player, 199827);
        if (auto item = AshbringerStorage::Inventory(player)) _weapon = item->GetGUID();
    }

    void Launch(SpellEffIndex index)
    {
        if (index >= EFFECT_3 && index <= EFFECT_5)
        {
            // Preserve its native initial delay, but own it by the original
            // quest attempt and check the real stored item before native61.
            PreventHitDefaultEffect(index);
        }
        else if (index >= EFFECT_3 && !Stored()) PreventHitDefaultEffect(index);
    }

    void Hit(SpellEffIndex index)
    {
        if (!Stored()) PreventHitDefaultEffect(index);
    }

    void After()
    {
        if (!Stored())
        {
            // Aura allocation precedes the per-effect application hooks.
            // Remove this cast's unapplied shell as well, so a failed store
            // cannot block a legitimate retry or later tick181257.
            if (auto aura = GetHitAura()) aura->Remove();
            return;
        }
        auto player = GetCaster()->ToPlayer();
        for (uint8 index : {uint8(EFFECT_3), uint8(EFFECT_4), uint8(EFFECT_5)})
        {
            auto effect = GetSpellInfo()->GetEffect(index, DIFFICULTY_NONE);
            AshbringerStorage::Delivery pending{_player, _weapon, _attempt, player->GetInstanceId(), effect->TriggerSpell, AshbringerStorage::Trigger(GetSpell())};
            player->AddQuestDelayedEvent(38376, effect->MiscValue ? effect->MiscValue : 10, [player, pending]() { pending.Run(player); });
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_bs_ashbringer_storage::Check);
        OnEffectLaunch += SpellEffectFn(spell_bs_ashbringer_storage::Create, EFFECT_2, SPELL_EFFECT_TRIGGER_SPELL);
        OnEffectLaunchTarget += SpellEffectFn(spell_bs_ashbringer_storage::Create, EFFECT_2, SPELL_EFFECT_TRIGGER_SPELL);
        OnEffectLaunch += SpellEffectFn(spell_bs_ashbringer_storage::Launch, EFFECT_ALL, SPELL_EFFECT_ANY);
        OnEffectLaunchTarget += SpellEffectFn(spell_bs_ashbringer_storage::Launch, EFFECT_ALL, SPELL_EFFECT_ANY);
        OnEffectHit += SpellEffectFn(spell_bs_ashbringer_storage::Hit, EFFECT_ALL, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_bs_ashbringer_storage::Hit, EFFECT_ALL, SPELL_EFFECT_ANY);
        AfterHit += SpellHitFn(spell_bs_ashbringer_storage::After);
    }
};

class aura_bs_ashbringer_storage : public AuraScript
{
    PrepareAuraScript(aura_bs_ashbringer_storage);
    ObjectGuid _owner, _weapon;
    std::weak_ptr<uint8> _attempt;
    uint32 _instance = 0;

    bool Load() override
    {
        auto player = GetUnitOwner()->ToPlayer();
        auto status = player ? player->getQuestStatus(38376) : nullptr;
        if (!status || !status->ScriptLifetime || player->GetMapId() != 1500) return false;
        _owner = player->GetGUID();
        _attempt = status->ScriptLifetime;
        _instance = player->GetInstanceId();
        return true;
    }

    void Applied(AuraEffect const*, AuraEffectHandleModes)
    {
        if (auto item = AshbringerStorage::Inventory(GetUnitOwner()->ToPlayer())) _weapon = item->GetGUID();
    }

    bool Valid() const
    {
        auto player = GetUnitOwner()->ToPlayer();
        auto status = player ? player->getQuestStatus(38376) : nullptr;
        auto item = AshbringerStorage::Inventory(player);
        return !_attempt.expired() && status && status->ScriptLifetime == _attempt.lock() &&
            player->GetGUID() == _owner && player->GetMapId() == 1500 && player->GetInstanceId() == _instance &&
            !player->GetQuestRewardStatus(38376) &&
            (player->GetQuestStatus(38376) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(38376) == QUEST_STATUS_COMPLETE) &&
            item && item->GetGUID() == _weapon;
    }

    void Cleanup()
    {
        // Only this retrieval's self-cast presentation. Normal finite expiry
        // retains the original scene's independent completion handling.
        auto player = GetUnitOwner()->ToPlayer();
        if (player && player->GetGUID() == _owner)
        {
            player->RemoveAurasDueToSpell(186987, _owner);
            player->RemoveAurasDueToSpell(192145, _owner);
        }
    }

    void Update(uint32)
    {
        if (!Valid()) { Cleanup(); GetAura()->Remove(); }
    }

    void Removed(AuraEffect const*, AuraEffectHandleModes)
    {
        if (!Valid()) Cleanup();
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(aura_bs_ashbringer_storage::Applied, EFFECT_6, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove += AuraEffectRemoveFn(aura_bs_ashbringer_storage::Removed, EFFECT_6, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        OnAuraUpdate += AuraUpdateFn(aura_bs_ashbringer_storage::Update);
    }
};
#endif
