#include "../VantusRuneLifecycle.h"
#include "the_emerald_nightmare.h"
#include "QuestData.h"

enum Says
{
    //SAY_1           = ,
};

enum Spells
{
    //SPELL_    = ,
    //SPELL_    = ,
    //SPELL_    = ,
    //SPELL_    = ,
};

enum eEvents
{
    //EVENT_    = 1,
    //EVENT_    = 2,
    //EVENT_    = 3,
    //EVENT_    = 4,
};

class spell_vantus_rune_the_emerald_nightmare : public SpellScriptLoader
{
public:
    spell_vantus_rune_the_emerald_nightmare() : SpellScriptLoader("spell_vantus_rune_the_emerald_nightmare") {}

    class spell_vantus_rune_the_emerald_nightmare_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_vantus_rune_the_emerald_nightmare_AuraScript);

        uint32 checkTimer = 1000;

        static std::array<LegionVantus::Encounter, 7> const& Encounters()
        {
            static std::array<LegionVantus::Encounter, 7> const entries = {{
                { 192761, 208844, DATA_NYTHENDRA },
                { 192765, 208848, DATA_RENFERAL },
                { 192762, 208845, DATA_ILGYNOTH },
                { 191464, 208843, DATA_URSOC },
                { 192763, 208846, DATA_DRAGON_NIGHTMARE },
                { 192766, 208849, DATA_CENARIUS },
                { 192764, 208847, DATA_XAVIUS },
            }};
            return entries;
        }

        void OnUpdate(uint32 diff, AuraEffect* /*aurEff*/)
        {
            LegionVantus::Update(GetUnitOwner(), GetId(), 1520, diff, checkTimer, Encounters());
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            LegionVantus::Cleanup(GetUnitOwner(), GetId(), Encounters());
        }

        void Register() override
        {
            OnEffectUpdate += AuraEffectUpdateFn(spell_vantus_rune_the_emerald_nightmare_AuraScript::OnUpdate, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            AfterEffectRemove += AuraEffectRemoveFn(spell_vantus_rune_the_emerald_nightmare_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_vantus_rune_the_emerald_nightmare_AuraScript();
    }

    class spell_vantus_rune_the_emerald_nightmare_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_vantus_rune_the_emerald_nightmare_SpellScript);

        SpellCastResult CheckCast()
        {
            if (auto player = GetCaster()->ToPlayer())
                if (!player->GetQuestRewardStatus(39695))
                    return SPELL_CAST_OK;

            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_YOU_ALREADY_USED_VANTUS_RUNE);
            return SPELL_FAILED_CUSTOM_ERROR;
        }

        void HandleOnHit()
        {
            if (auto player = GetCaster()->ToPlayer())
                if (Quest const* quest = sQuestDataStore->GetQuestTemplate(39695))
                    if (player->CanTakeQuest(quest, false))
                        player->CompleteQuest(39695);
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_vantus_rune_the_emerald_nightmare_SpellScript::CheckCast);
            OnHit += SpellHitFn(spell_vantus_rune_the_emerald_nightmare_SpellScript::HandleOnHit);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_vantus_rune_the_emerald_nightmare_SpellScript();
    }
};

//1000
class eventobject_en_into : public EventObjectScript
{
public:
    eventobject_en_into() : EventObjectScript("eventobject_en_into") {}

    bool event_elerethe_Done = false;

    bool OnTrigger(Player* plr, EventObject* eo, bool enter) override
    {
        if (!enter)
            return true;

        InstanceScript* instance = plr->GetInstanceScript();
        if (!instance)
            return false;

        switch (eo->GetEntry())
        {
            case 1000:
            {
                if (!event_elerethe_Done)
                {
                    event_elerethe_Done = true;
                    plr->CreateConversation(3626);
                    plr->AddDelayedEvent(6000, [plr]() -> void
                    {
                        plr->CreateConversation(3635);
                    });
                }
                break;
            }
        }
        return true;
    }
};

void AddSC_the_emerald_nightmare()
{
    new spell_vantus_rune_the_emerald_nightmare();
    new (eventobject_en_into);
}