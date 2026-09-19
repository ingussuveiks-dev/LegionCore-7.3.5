/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Scripts for spells with SPELLFAMILY_WARRIOR and SPELLFAMILY_GENERIC spells used by warrior players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_warr_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "ScriptUtils.h"
#include "SpellAuraEffects.h"
#include "PathGenerator.h"

namespace
{
enum WarriorChargeSpells
{
    SPELL_WARRIOR_CHARGE_EFFECT               = 218104,
    SPELL_WARRIOR_CHARGE_EFFECT_BLAZING_TRAIL = 198337,
    SPELL_WARRIOR_CHARGE_PAUSE_RAGE_DECAY     = 109128,
    SPELL_WARRIOR_CHARGE_ROOT_EFFECT          = 105771,
    SPELL_WARRIOR_CHARGE_SLOW_EFFECT          = 236027,
    SPELL_WARRIOR_GLYPH_BLAZING_TRAIL         = 123779
};

enum WarriorCommonSpells
{
    SPELL_WARRIOR_IMPENDING_VICTORY_HEAL = 202166,
    SPELL_WARRIOR_VICTORIOUS_STATE       = 32216,
    SPELL_WARRIOR_VICTORY_RUSH_HEAL      = 118779,
    SPELL_WARRIOR_COMMANDING_SHOUT_AURA  = 97463,
    SPELL_WARRIOR_SHOCKWAVE_STUN         = 132168,
    SPELL_WARRIOR_STORM_BOLT_STUN        = 132169,
    SPELL_WARRIOR_REVENGE                 = 6572
};

enum WarriorArmsSpells
{
    SPELL_WARRIOR_ARMS_SLAM                    = 1464,
    SPELL_WARRIOR_ARMS_WHIRLWIND               = 1680,
    SPELL_WARRIOR_OVERPOWER                    = 7384,
    SPELL_WARRIOR_MORTAL_STRIKE                = 12294,
    SPELL_WARRIOR_EXECUTE                      = 163201,
    SPELL_WARRIOR_COLOSSUS_SMASH               = 167105,
    SPELL_WARRIOR_PRECISE_STRIKES_BUFF         = 248195,
    SPELL_WARRIOR_MORTAL_WOUNDS                = 115804,
    SPELL_WARRIOR_OVERPOWER_PASSIVE            = 119938,
    SPELL_WARRIOR_TRAUMA_BLEED                 = 215537,
    SPELL_WARRIOR_OVERPOWER_ACTIVATED          = 60503,
    SPELL_WARRIOR_CORRUPTED_BLOOD_DOT          = 209569,
    SPELL_WARRIOR_HEROIC_LEAP_JUMP             = 94954,
    SPELL_WARRIOR_ARMS_SPECIALIZATION          = 137048,
    SPELL_WARRIOR_RAVAGER_ARMS                 = 152277,
    SPELL_WARRIOR_RAVAGER_PROTECTION           = 228920,
    SPELL_WARRIOR_RAVAGER_PARRY                = 227744,
    SPELL_WARRIOR_RAVAGER_SUMMON               = 227876
};

enum WarriorFurySpells
{
    SPELL_WARRIOR_WAR_MACHINE_BUFF = 215562
};
}

// Avatar - 107574
class spell_warr_avatar : public SpellScript
{
    PrepareSpellScript(spell_warr_avatar);

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->RemoveMovementImpairingEffects();
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warr_avatar::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// Impending Victory - 202168
class spell_warr_impending_victory : public SpellScript
{
    PrepareSpellScript(spell_warr_impending_victory);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_IMPENDING_VICTORY_HEAL });
    }

    void HandleAfterCast()
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_WARRIOR_IMPENDING_VICTORY_HEAL, true);
        GetCaster()->RemoveAurasDueToSpell(SPELL_WARRIOR_VICTORIOUS_STATE);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_warr_impending_victory::HandleAfterCast);
    }
};

// Victory Rush - 34428
class spell_warr_victory_rush : public SpellScript
{
    PrepareSpellScript(spell_warr_victory_rush);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_VICTORY_RUSH_HEAL });
    }

    void HandleAfterCast()
    {
        GetCaster()->CastSpell(GetCaster(), SPELL_WARRIOR_VICTORY_RUSH_HEAL, true);
        GetCaster()->RemoveAurasDueToSpell(SPELL_WARRIOR_VICTORIOUS_STATE);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_warr_victory_rush::HandleAfterCast);
    }
};

// Commanding Shout - 97462
class spell_warr_commanding_shout : public SpellScript
{
    PrepareSpellScript(spell_warr_commanding_shout);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_COMMANDING_SHOUT_AURA });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* target = GetHitUnit();
        if (!target)
            return;

        int32 health = CalculatePct(target->GetMaxHealth(), GetEffectValue());
        GetCaster()->CastCustomSpell(SPELL_WARRIOR_COMMANDING_SHOUT_AURA, SPELLVALUE_BASE_POINT0, health, target, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warr_commanding_shout::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Intimidating Shout - 5246. The primary target gets the cower effects;
// remove it from all three secondary area-fear effect target lists.
class spell_warr_intimidating_shout : public SpellScript
{
    PrepareSpellScript(spell_warr_intimidating_shout);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targets.remove(GetExplTargetWorldObject());
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warr_intimidating_shout::FilterTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warr_intimidating_shout::FilterTargets, EFFECT_3, TARGET_UNIT_SRC_AREA_ENEMY);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warr_intimidating_shout::FilterTargets, EFFECT_5, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

// Shockwave - 46968
class spell_warr_shockwave : public SpellScript
{
    PrepareSpellScript(spell_warr_shockwave);

    uint32 TargetCount = 0;

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_SHOCKWAVE_STUN });
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
        {
            GetCaster()->CastSpell(target, SPELL_WARRIOR_SHOCKWAVE_STUN, true);
            ++TargetCount;
        }
    }

    void HandleAfterCast()
    {
        if (TargetCount < uint32(GetSpellInfo()->Effects[EFFECT_0]->BasePoints))
            return;

        if (Player* player = GetCaster()->ToPlayer())
            player->ModifySpellCooldown(GetSpellInfo()->Id,
                -int32(GetSpellInfo()->Effects[EFFECT_3]->BasePoints) * int32(IN_MILLISECONDS));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warr_shockwave::HandleDamage, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
        AfterCast += SpellCastFn(spell_warr_shockwave::HandleAfterCast);
    }
};

// Storm Bolt - 107570
class spell_warr_storm_bolt : public SpellScript
{
    PrepareSpellScript(spell_warr_storm_bolt);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_STORM_BOLT_STUN });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_WARRIOR_STORM_BOLT_STUN, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warr_storm_bolt::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};

// Revenge Trigger - 5301
class spell_warr_revenge_trigger : public AuraScript
{
    PrepareAuraScript(spell_warr_revenge_trigger);

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        if (Player* player = GetTarget()->ToPlayer())
            player->RemoveSpellCooldown(SPELL_WARRIOR_REVENGE, true);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warr_revenge_trigger::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// War Machine - 215556. The client proc points at an inert target marker
// (215557); the actual self buff described by the talent is 215562.
class spell_warr_war_machine : public AuraScript
{
    PrepareAuraScript(spell_warr_war_machine);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_WAR_MACHINE_BUFF });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        GetTarget()->CastSpell(GetTarget(), SPELL_WARRIOR_WAR_MACHINE_BUFF, true, nullptr, aurEff);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_warr_war_machine::HandleProc,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Executioner's Precision - 238147. Only Execute may apply the Mortal Strike debuff.
class spell_warr_executioners_precision : public AuraScript
{
    PrepareAuraScript(spell_warr_executioners_precision);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell* procSpell = eventInfo.GetSpell();
        return procSpell && procSpell->GetSpellInfo()->Id == SPELL_WARRIOR_EXECUTE;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_warr_executioners_precision::CheckProc);
    }
};

// Focused Rage - 207982. Its one charge/three stacks may only be consumed by Mortal Strike.
class spell_warr_focused_rage_arms : public AuraScript
{
    PrepareAuraScript(spell_warr_focused_rage_arms);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell* procSpell = eventInfo.GetSpell();
        return procSpell && procSpell->GetSpellInfo()->Id == SPELL_WARRIOR_MORTAL_STRIKE;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_warr_focused_rage_arms::CheckProc);
    }
};

// Overpower passive - 119938, learned by talent spell 7384.
class spell_warr_overpower_passive : public AuraScript
{
    PrepareAuraScript(spell_warr_overpower_passive);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_OVERPOWER_ACTIVATED });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell* procSpell = eventInfo.GetSpell();
        if (!procSpell)
            return false;

        switch (procSpell->GetSpellInfo()->Id)
        {
            case SPELL_WARRIOR_ARMS_WHIRLWIND:
            case SPELL_WARRIOR_COLOSSUS_SMASH:
            case SPELL_WARRIOR_MORTAL_STRIKE:
            case SPELL_WARRIOR_ARMS_SLAM:
                break;
            default:
                return false;
        }

        // The active world DB supplies the final 5% roll through
        // spell_proc_event for passive 119938. This script only narrows the
        // otherwise empty family mask to the four intended Arms attacks.
        return true;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_warr_overpower_passive::CheckProc);
    }
};

// Precise Strikes - 248579
class spell_warr_precise_strikes : public AuraScript
{
    PrepareAuraScript(spell_warr_precise_strikes);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_PRECISE_STRIKES_BUFF });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell* procSpell = eventInfo.GetSpell();
        return procSpell && procSpell->GetSpellInfo()->Id == SPELL_WARRIOR_COLOSSUS_SMASH;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();
        float criticalChance = aurEff->GetAmount();
        GetTarget()->CastCustomSpell(GetTarget(), SPELL_WARRIOR_PRECISE_STRIKES_BUFF,
            &criticalChance, nullptr, nullptr, true, nullptr, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_warr_precise_strikes::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_warr_precise_strikes::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// In for the Kill - 248621. Its native trigger applies haste; restrict it to Colossus Smash.
class spell_warr_in_for_the_kill : public AuraScript
{
    PrepareAuraScript(spell_warr_in_for_the_kill);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell* procSpell = eventInfo.GetSpell();
        return procSpell && procSpell->GetSpellInfo()->Id == SPELL_WARRIOR_COLOSSUS_SMASH;
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_warr_in_for_the_kill::CheckProc);
    }
};

// Trauma - 215538. Add the remaining bleed damage to 20% of the new hit and
// redistribute the result over a fresh six-second, three-tick bleed.
class spell_warr_trauma : public AuraScript
{
    PrepareAuraScript(spell_warr_trauma);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_TRAUMA_BLEED });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Spell* procSpell = eventInfo.GetSpell();
        if (!procSpell || !eventInfo.GetDamageInfo() || !eventInfo.GetActionTarget())
            return false;

        switch (procSpell->GetSpellInfo()->Id)
        {
            case SPELL_WARRIOR_ARMS_SLAM:
            case SPELL_WARRIOR_ARMS_WHIRLWIND:
            case SPELL_WARRIOR_EXECUTE:
                return eventInfo.GetDamageInfo()->GetDamage() > 0;
            default:
                return false;
        }
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        Unit* target = eventInfo.GetActionTarget();
        SpellInfo const* trauma = sSpellMgr->GetSpellInfo(SPELL_WARRIOR_TRAUMA_BLEED);
        if (!caster || !target || !trauma)
            return;

        uint32 totalTicks = trauma->GetMaxTicks();
        if (!totalTicks)
            return;

        float totalDamage = CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), aurEff->GetAmount());
        if (Aura* current = target->GetAura(SPELL_WARRIOR_TRAUMA_BLEED, caster->GetGUID()))
            if (AuraEffect* currentEffect = current->GetEffect(EFFECT_0))
                totalDamage += currentEffect->GetAmount() *
                    (currentEffect->GetTotalTicks() - currentEffect->GetTickNumber());

        float damagePerTick = totalDamage / totalTicks;
        caster->CastCustomSpell(target, SPELL_WARRIOR_TRAUMA_BLEED, &damagePerTick,
            nullptr, nullptr, true, nullptr, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_warr_trauma::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_warr_trauma::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// Corrupted Blood of Zakajz - 209567. Each damaging attack adds 20% of its
// damage to the remaining six-second Shadow bleed and redistributes it over
// the full three ticks.
class spell_warr_corrupted_blood_of_zakajz : public AuraScript
{
    PrepareAuraScript(spell_warr_corrupted_blood_of_zakajz);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_CORRUPTED_BLOOD_DOT });
    }

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        if (!eventInfo.GetDamageInfo() || !eventInfo.GetActionTarget() ||
            eventInfo.GetDamageInfo()->GetDamage() <= 0)
            return false;

        Spell* procSpell = eventInfo.GetSpell();
        return !procSpell || procSpell->GetSpellInfo()->Id != SPELL_WARRIOR_CORRUPTED_BLOOD_DOT;
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* caster = GetTarget();
        Unit* target = eventInfo.GetActionTarget();
        SpellInfo const* dot = sSpellMgr->GetSpellInfo(SPELL_WARRIOR_CORRUPTED_BLOOD_DOT);
        if (!caster || !target || !dot)
            return;

        uint32 totalTicks = dot->GetMaxTicks();
        if (!totalTicks)
            return;

        float totalDamage = CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), aurEff->GetAmount());
        if (Aura* current = target->GetAura(SPELL_WARRIOR_CORRUPTED_BLOOD_DOT, caster->GetGUID()))
            if (AuraEffect* currentEffect = current->GetEffect(EFFECT_0))
                totalDamage += currentEffect->GetAmount() *
                    (currentEffect->GetTotalTicks() - currentEffect->GetTickNumber());

        float damagePerTick = totalDamage / totalTicks;
        caster->CastCustomSpell(target, SPELL_WARRIOR_CORRUPTED_BLOOD_DOT, &damagePerTick,
            nullptr, nullptr, true, nullptr, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_warr_corrupted_blood_of_zakajz::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_warr_corrupted_blood_of_zakajz::HandleProc,
            EFFECT_0, SPELL_AURA_DUMMY);
    }
};

//Arms Execute - 163201
class spell_warr_execute : public SpellScriptLoader
{
public:
    spell_warr_execute() : SpellScriptLoader("spell_warr_execute") { }

    class spell_warr_execute_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warr_execute_SpellScript);

        uint32 mod;

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                int32 damage = GetHitDamage();

                if (mod)
                    AddPct(damage, mod);

                SetHitDamage(damage);
            }
        }

        void HandleEnergy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                if (Unit* target = GetOriginalTarget())
                {
                    if (target->IsAlive())
                        SetEffectValue(CalculatePct(caster->GetPowerCost(POWER_RAGE), GetSpellInfo()->Effects[EFFECT_4]->BasePoints));
                }
            }
        }

        void TakePower(Powers power, int32 &amount)
        {
            mod = 0;

            if (Unit* caster = GetCaster())
            {
                if (Player* plr = caster->ToPlayer())
                {
                    SpellInfo const* spellInfo = GetSpellInfo();
                    int32 bonusDmgPct = RoundingFloatValue(spellInfo->Effects[EFFECT_4]->BasePoints * 10.f);

                    if (!bonusDmgPct)
                        return;

                    int32 bonusDmgPctConst = bonusDmgPct;

                    plr->ApplySpellMod(spellInfo->Id, SPELLMOD_COST, bonusDmgPct);

					bool ignoreCost = false;

					if (bonusDmgPct == 0)
					{
						bonusDmgPctConst = 300;
						bonusDmgPct = 270;
						ignoreCost = true;
					}

                    int32 coef = bonusDmgPctConst / bonusDmgPct;
                    int32 rage = caster->GetPower(POWER_RAGE);

                    if (rage >= (bonusDmgPct + amount))
                    {
                        mod = bonusDmgPctConst;
                    }
                    else
                    {
                        bonusDmgPct = rage - amount;
                        mod = bonusDmgPct * coef;
                    }

					if (!ignoreCost)
					{
						amount += bonusDmgPct;
						caster->m_powerCost[POWER_RAGE] = amount;
					}
                }
            }
        }

        void HandleOnCast()
        {
            if (Unit* caster = GetCaster())
            {
                if (AuraEffect const* aurEff = caster->GetAuraEffect(202161, EFFECT_0))
                {
                    if (Unit* target = GetOriginalTarget())
                    {
                        if (uint8 jumpTargets = aurEff->GetAmount())
                        {
                            std::list<Unit*> targetList = MS::ScriptUtils::SelectNearestUnits(caster, 0, NOMINAL_MELEE_RANGE);

                            if (targetList.empty())
                                return;

                            targetList.remove(target);
                            targetList.remove_if(Trinity::AnyUnfriendlyAttackableVisibleUnitInObjectRangeCheck(caster, 10.f, false));

                            if (targetList.size() > jumpTargets)
                                targetList.resize(jumpTargets);

                            if (Spell* spell = GetSpell())
                            {
                                for (auto itr : targetList)
                                    spell->AddUnitTarget(itr, 3, false);
                            }
                        }
                    }
                }
            }
        }

        void Register() override
        {
            OnTakePower += TakePowertFn(spell_warr_execute_SpellScript::TakePower);
            OnEffectHitTarget += SpellEffectFn(spell_warr_execute_SpellScript::HandleDamage, EFFECT_1, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
            OnEffectHitTarget += SpellEffectFn(spell_warr_execute_SpellScript::HandleEnergy, EFFECT_2, SPELL_EFFECT_ENERGIZE);
            OnCast += SpellCastFn(spell_warr_execute_SpellScript::HandleOnCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warr_execute_SpellScript();
    }
};

// Shield Block - 2565
class spell_warr_shield_block : public SpellScriptLoader
{
    public:
        spell_warr_shield_block() : SpellScriptLoader("spell_warr_shield_block") { }

        class spell_warr_shield_block_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_shield_block_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Aura* aura = caster->GetAura(132404))
                        aura->SetDuration(aura->GetDuration() + 6000);
                    else
                        caster->CastSpell(caster, 132404, true);
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_warr_shield_block_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_shield_block_SpellScript();
        }
};

// Heroic leap - 6544
class spell_warr_heroic_leap : public SpellScriptLoader
{
    public:
        spell_warr_heroic_leap() : SpellScriptLoader("spell_warr_heroic_leap") { }

        class spell_warr_heroic_leap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_heroic_leap_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ SPELL_WARRIOR_HEROIC_LEAP_JUMP });
            }

            SpellCastResult CheckElevation()
            {
                Unit* caster = GetCaster();

                WorldLocation* dest = const_cast<WorldLocation*>(GetExplTargetDest());

                Player* player = caster->ToPlayer();
                if (!player || !dest)
                    return SPELL_FAILED_DONT_REPORT;
                // else if (dest->GetPositionZ() > caster->GetPositionZ() + 10.0f) // If use patch system this not need
                    // return SPELL_FAILED_NOPATH;
                else if (caster->HasAuraType(SPELL_AURA_MOD_ROOT) || caster->HasAuraType(SPELL_AURA_MOD_ROOTED))
                    return SPELL_FAILED_ROOTED;
                else if (player->GetMap()->IsBattlegroundOrArena())
                {
                    if (Battleground* bg = player->GetBattleground())
                    {
                        if (bg->GetStatus() != STATUS_IN_PROGRESS)
                            return SPELL_FAILED_NOT_READY;
                    }
                }

                // battleground <bad> positions
                if (player->GetMap()->IsBattleground())
                {
                    switch (player->GetMapId())
                    {
                    case 489:
                        if ((dest->GetPositionZ() >= 375.0f) || (player->GetPositionZ() <= 361.0f && dest->GetPositionZ() >= 364.0f && dest->GetPositionY() <= 1370.0f)
                            || (dest->GetPositionY() <= 1266.0f) || (dest->GetPositionX() <= 991.0f && dest->GetPositionZ() >= 368.0f)
                            || (dest->GetPositionY() <= 1396.0f && dest->GetPositionZ() >= 353.0f) || (dest->GetPositionY() >= 1652.0f && dest->GetPositionZ() >= 354.0f))
                            return SPELL_FAILED_NOPATH;
                        break;
                    case 529:
                        if (player->GetCurrentAreaID() == 3424)
                            if ((dest->GetPositionX() <= 1199.0f && dest->GetPositionY() <= 1199.0f && dest->GetPositionZ() >= -51.0f) || (dest->GetPositionZ() >= 13.9f))
                                return SPELL_FAILED_NOPATH;
                        if (player->GetCurrentAreaID() == 3421)
                            if ((dest->GetPositionZ() >= -40.0f))
                                return SPELL_FAILED_NOPATH;
                        if (player->GetCurrentAreaID() == 3422)
                            if ((dest->GetPositionZ() >= 18.8f))
                                return SPELL_FAILED_NOPATH;
                        break;
                    case 726:
                        if (((dest->GetPositionX() < 1855.0f && dest->GetPositionX() > 1534.0f) && (dest->GetPositionY() < 183.0f && dest->GetPositionY() > 78.0f) && dest->GetPositionZ() >= -4.8f)
                            || ((dest->GetPositionX() < 1926.0f && dest->GetPositionX() > 1818.0f) && (dest->GetPositionY() < 510.0f && dest->GetPositionY() > 407.0f) && dest->GetPositionZ() >= -10.0f))
                            return SPELL_FAILED_NOPATH;
                        break;
                    case 998:
                        if ((dest->GetPositionX() < 1733.0f && dest->GetPositionY() > 1394.0f && dest->GetPositionZ() >= 23.4f)
                            || (dest->GetPositionX() > 1824.0f && dest->GetPositionZ() > 20.4f)
                            || ((dest->GetPositionX() > 1719.0f && dest->GetPositionX() < 1846.7f) && (dest->GetPositionY() > 1253.0f
                                && dest->GetPositionY() < 1416.0f) && dest->GetPositionZ() >= 11.0f))
                            return SPELL_FAILED_NOPATH;
                        break;
                    case 607:
                        if (((dest->GetPositionX() < 1475.0f && dest->GetPositionX() > 1383.7f) && (dest->GetPositionY() < -177.0f && dest->GetPositionY() > -262.0f) && dest->GetPositionZ() >= 38.0f)
                            || ((dest->GetPositionX() < 1468.0f && dest->GetPositionX() > 1357.7f) && (dest->GetPositionY() < 160.0f && dest->GetPositionY() > 59.0f) && dest->GetPositionZ() >= 36.0f)
                            || ((dest->GetPositionX() < 1262.0f && dest->GetPositionX() > 1158.0f) && (dest->GetPositionY() < 137.0f && dest->GetPositionY() > 29.0f) && dest->GetPositionZ() >= 58.1f)
                            || ((dest->GetPositionX() < 1277.0f && dest->GetPositionX() > 1169.0f) && (dest->GetPositionY() < -149.0f && dest->GetPositionY() > -272.0f) && dest->GetPositionZ() >= 58.0f)
                            || ((dest->GetPositionX() < 1092.0f && dest->GetPositionX() > 1012.0f) && (dest->GetPositionY() < -55.0f && dest->GetPositionY() > -154.0f) && dest->GetPositionZ() >= 89.77f))
                            return SPELL_FAILED_NOPATH;
                        break;
                    }
                }

                bool forceDest = caster->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR);
                if (forceDest)
                {
                    float x, y, z;
                    caster->GetPosition(x, y, z);
                    float ground = caster->GetMap()->GetHeight(caster->GetPhases(), x, y, z, true);
                    if ((z - ground) <= 2.0f)
                        forceDest = false;
                }

                if (!caster->IsInWater() && !forceDest) // don`t check in water
                {
                    PathGenerator patch(player);
                    bool result = patch.CalculatePath(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ());

                    if (!result || patch.GetPathType() & PATHFIND_NOPATH)
                        return SPELL_FAILED_NOPATH;
                }

                //caster->CastSpell(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ(), 178367, true);
                return SPELL_FAILED_SUCCESS;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (WorldLocation const* dest = GetExplTargetDest())
                    GetCaster()->CastSpell(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ(),
                        SPELL_WARRIOR_HEROIC_LEAP_JUMP, true);
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_warr_heroic_leap_SpellScript::CheckElevation);
                OnEffectHit += SpellEffectFn(spell_warr_heroic_leap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_heroic_leap_SpellScript();
        }
};

// Charge - 100. The client spell's first effect is a dummy; it must launch
// the actual movement spell selected by Glyph of the Blazing Trail.
class spell_warr_charge : public SpellScript
{
    PrepareSpellScript(spell_warr_charge);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_CHARGE_EFFECT, SPELL_WARRIOR_CHARGE_EFFECT_BLAZING_TRAIL });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        uint32 chargeSpell = GetCaster()->HasAura(SPELL_WARRIOR_GLYPH_BLAZING_TRAIL)
            ? SPELL_WARRIOR_CHARGE_EFFECT_BLAZING_TRAIL
            : SPELL_WARRIOR_CHARGE_EFFECT;
        GetCaster()->CastSpell(GetHitUnit(), chargeSpell, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_warr_charge::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Charge movement - 198337/218104. Apply the root followed by the slow from
// Charge's 7.3.5 tooltip and pause out-of-combat Rage decay during movement.
class spell_warr_charge_effect : public SpellScript
{
    PrepareSpellScript(spell_warr_charge_effect);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_CHARGE_PAUSE_RAGE_DECAY,
            SPELL_WARRIOR_CHARGE_ROOT_EFFECT, SPELL_WARRIOR_CHARGE_SLOW_EFFECT });
    }

    void HandleCharge(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        caster->CastCustomSpell(SPELL_WARRIOR_CHARGE_PAUSE_RAGE_DECAY, SPELLVALUE_BASE_POINT0, 0, caster, true);
        caster->CastSpell(target, SPELL_WARRIOR_CHARGE_ROOT_EFFECT, true);
        caster->CastSpell(target, SPELL_WARRIOR_CHARGE_SLOW_EFFECT, true);
    }

    void Register() override
    {
        OnEffectLaunchTarget += SpellEffectFn(spell_warr_charge_effect::HandleCharge, EFFECT_0, SPELL_EFFECT_CHARGE);
    }
};

// Intervene - 3411
class spell_war_intervene : public SpellScriptLoader
{
    public:
        spell_war_intervene() : SpellScriptLoader("spell_war_intervene") { }

        class spell_war_intervene_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_war_intervene_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.sort(CheckHealthState());
                if (targets.size() > 1)
                    targets.resize(1);
            }

            SpellCastResult CheckCast()
            {
                Player* _player = GetCaster()->ToPlayer();
                if (!_player)
                    return SPELL_FAILED_BAD_TARGETS;

                Unit* target = _player->GetSelectedUnit();
                if (!target)
                    return SPELL_FAILED_BAD_TARGETS;

                if (!_player->IsFriendlyTo(target) || (!_player->IsInPartyWith(target) && !_player->IsInRaidWith(target)))
                    return SPELL_FAILED_BAD_TARGETS;

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_war_intervene_SpellScript::CheckCast);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_war_intervene_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ALLY);
            }

        private:
            class CheckHealthState
            {
                public:
                    CheckHealthState() { }

                    bool operator() (WorldObject* a, WorldObject* b) const
                    {
                        Unit* unita = a->ToUnit();
                        Unit* unitb = b->ToUnit();
                        if(!unita)
                            return true;
                        if(!unitb)
                            return false;
                        return unita->GetHealthPct() < unitb->GetHealthPct();
                    }
            };
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_war_intervene_SpellScript();
        }
};

// Warrior Charge Drop Fire Periodic - 126661
class spell_warr_charge_drop_fire : public SpellScriptLoader
{
    public:
        spell_warr_charge_drop_fire() : SpellScriptLoader("spell_warr_charge_drop_fire") { }

        class spell_warr_charge_drop_fire_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warr_charge_drop_fire_AuraScript);

            Position savePos;
            void OnTick(AuraEffect const* aurEff)
            {
                if(Unit* caster = GetCaster())
                {
                    float distance = caster->GetDistance(savePos);
                    float angle = caster->GetAngle(&savePos);
                    if (uint32 count = uint32(distance))
                    {
                        for(uint32 j = 1; j < count + 1; ++j)
                        {
                            uint32 distanceNext = j;
                            float destx = caster->GetPositionX() + distanceNext * std::cos(angle);
                            float desty = caster->GetPositionY() + distanceNext * std::sin(angle);
                            savePos.Relocate(destx, desty, caster->GetPositionZ());
                            caster->SendSpellCreateVisual(GetSpellInfo(), &savePos);
                        }
                    }
                }
            }

            void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* caster = GetCaster())
                    savePos.Relocate(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ());
            }

            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_warr_charge_drop_fire_AuraScript::HandleApply, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warr_charge_drop_fire_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warr_charge_drop_fire_AuraScript();
        }
};

// Fervor of Battle - 202316, run from 199850, 199658
class spell_warr_fervor_of_battle : public SpellScriptLoader
{
    public:
        spell_warr_fervor_of_battle() : SpellScriptLoader("spell_warr_fervor_of_battle") { }

        class spell_warr_fervor_of_battle_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_fervor_of_battle_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->GetSelectedUnit() == GetHitUnit())
                    {
                        if (AuraEffect* aurEff = _player->GetAuraEffect(202316, EFFECT_0))
                        {
                            int32 damage = GetHitDamage();
                            damage += CalculatePct(damage, aurEff->GetAmount());
                            SetHitDamage(damage);
                        }
                    }
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_warr_fervor_of_battle_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_fervor_of_battle_SpellScript();
        }
};

// Ignore Pain - 190456
class spell_warr_ignore_pain : public AuraScript
{
    PrepareAuraScript(spell_warr_ignore_pain);

    void CalculateAmount(AuraEffect const* aurEff, float & amount, bool & /*canBeRecalculated*/)
    {
        if (Unit* caster = GetCaster())
        {
            int32 powerCost = aurEff->GetBase()->GetPowerCost(POWER_RAGE);
            if (caster->HasAura(202574))
                powerCost *= 2;

            float percAdd = (powerCost - 200) * 0.167f + 33.f;
            float absorbData = GetSpellInfo()->Effects[EFFECT_0]->BonusCoefficientFromAP * caster->GetTotalAttackPowerValue(BASE_ATTACK);

            if (caster->HasAura(202561)) // Never Surrender
                AddPct(percAdd, (100 - int32(caster->GetHealthPct())));
            if (AuraEffect* aurEf = caster->GetAuraEffect(203581, EFFECT_0)) // Dragon Scales
            {
                AddPct(percAdd, aurEf->GetAmount());
                aurEf->GetBase()->Remove();
            }
            if (AuraEffect* aurEf = caster->GetAuraEffect(202095, EFFECT_1)) // Indomitable
                AddPct(percAdd, aurEf->GetAmount());
            if (AuraEffect* aurEf = caster->GetAuraEffect(203225, EFFECT_0)) // Dragon Skin
                AddPct(percAdd, aurEf->GetAmount());
            if (AuraEffect* aurEf = caster->GetAuraEffect(254339, EFFECT_0)) // Item - Warrior T21 Protection 4P Bonus - Blocked Attacks Buff Ignore Pain
            {
                absorbData += aurEf->GetAmount();
                aurEf->GetBase()->Remove();
            }

            amount += CalculatePct(absorbData, percAdd);
            amount += aurEff->GetOldBaseAmount();

            if (amount > caster->GetMaxHealth())
                amount = caster->GetMaxHealth();
        }
    }

    void Absorb(AuraEffect* aurEff, DamageInfo& dmgInfo, float& absorbAmount)
    {    
        if (absorbAmount < dmgInfo.GetDamage())
            absorbAmount = aurEff->GetAmount();
        else
            absorbAmount = CalculatePct(dmgInfo.GetDamage(), GetSpellInfo()->Effects[EFFECT_1]->BasePoints);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warr_ignore_pain::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_warr_ignore_pain::Absorb, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
    }
};

// 254339 - Stand Your Ground
class spell_war_t21_prot_4p : public AuraScript
{
    PrepareAuraScript(spell_war_t21_prot_4p);

    void CalculateDummy(AuraEffect const* aurEff, float & amount, bool & canBeRecalculated)
    {
        if (Unit* caster = GetCaster())
            amount = int32(GetSpellInfo()->Effects[EFFECT_0]->BasePoints * caster->GetTotalAttackPowerValue(BASE_ATTACK));
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_war_t21_prot_4p::CalculateDummy, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// Intercept - 198304
class spell_warr_intercept : public SpellScriptLoader
{
    public:
        spell_warr_intercept() : SpellScriptLoader("spell_warr_intercept") { }

        class spell_warr_intercept_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_intercept_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();
                if (!target)
                    return;

                if (target->IsFriendlyTo(caster))
                {
                    caster->CastSpell(target, 147833, true); // charge

                    if (caster->HasAura(223657)) // Safeguard
                        caster->CastSpell(target, 223658, true);
                    if (caster->HasAura(199037)) // Leave No Man Behind	(PvP Talent)
                        caster->CastSpell(target, 199038, true);
                }
                else
                {
                    uint32 chargeSpell = caster->HasAura(SPELL_WARRIOR_GLYPH_BLAZING_TRAIL)
                        ? SPELL_WARRIOR_CHARGE_EFFECT_BLAZING_TRAIL
                        : SPELL_WARRIOR_CHARGE_EFFECT;
                    caster->CastSpell(target, chargeSpell, true);

                    if (caster->HasAura(103828)) // Warbringer
                        caster->CastSpell(target, 7922, true);
                }
            }

            SpellCastResult CheckElevation()
            {
                Unit* caster = GetCaster();
                Unit* target = GetExplTargetUnit();

                if (!caster || !target || caster == target || !caster->HasInArc(static_cast<float>(M_PI), target))
                    return SPELL_FAILED_BAD_TARGETS;

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_warr_intercept_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast += SpellCheckCastFn(spell_warr_intercept_SpellScript::CheckElevation);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_intercept_SpellScript();
        }
};

// Charge - 100, Intercept - 198304
class spell_warr_charge_check_cast : public SpellScriptLoader
{
    public:
        spell_warr_charge_check_cast() : SpellScriptLoader("spell_warr_charge_check_cast") { }

        class spell_warr_charge_check_cast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_charge_check_cast_SpellScript);

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                Unit* target = GetExplTargetUnit();

                if (!caster || !target)
                    return SPELL_FAILED_BAD_TARGETS;

                if (caster->HasUnitState(UNIT_STATE_ROOT))
                    return SPELL_FAILED_ROOTED;

                if (caster->IsInWater()) // don`t check in water
                    return SPELL_CAST_OK;

                float angle = target->GetRelativeAngle(caster);
                Position pos = target->GetFirstCollisionPosition(target->GetObjectSize(), angle);
                float limit = GetSpellInfo()->GetMaxRange(true, caster) + 1.0f;
                PathGenerator* m_path = new PathGenerator(caster);
                bool result = m_path->CalculatePath(pos.m_positionX, pos.m_positionY, pos.m_positionZ, false);
                PathType _pathType = m_path->GetPathType();
                float _totalLength = m_path->GetTotalLength();
                delete m_path;

                if (_pathType & PATHFIND_SHORT)
                    return SPELL_FAILED_OUT_OF_RANGE;
                if (!result || _pathType & PATHFIND_NOPATH)
                    return SPELL_FAILED_NOPATH;
                if (_totalLength > (limit * 1.5f))
                    return SPELL_FAILED_OUT_OF_RANGE;

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_warr_charge_check_cast_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_charge_check_cast_SpellScript();
        }
};

// Ravager cast - 152277 (Arms) / 228920 (Protection). Both client spells
// contain a destination dummy and require the actual 227876 summon spell.
class spell_warr_ravager_cast : public SpellScript
{
    PrepareSpellScript(spell_warr_ravager_cast);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_WARRIOR_RAVAGER_SUMMON, SPELL_WARRIOR_RAVAGER_PARRY });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (WorldLocation const* dest = GetExplTargetDest())
            GetCaster()->CastSpell(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ(),
                SPELL_WARRIOR_RAVAGER_SUMMON, true);

        if (GetSpellInfo()->Id == SPELL_WARRIOR_RAVAGER_ARMS &&
            GetCaster()->HasAura(SPELL_WARRIOR_ARMS_SPECIALIZATION))
            GetCaster()->CastSpell(GetCaster(), SPELL_WARRIOR_RAVAGER_PARRY, true);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_warr_ravager_cast::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};

// Ravager - 227876
class spell_warr_ravager : public SpellScriptLoader
{
    public:
        spell_warr_ravager() : SpellScriptLoader("spell_warr_ravager") { }

        class spell_warr_ravager_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warr_ravager_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if(Unit* caster = GetCaster())
                    if (GuidList* summonList = caster->GetSummonList(76168))
                    {
                        GuidList _summonList(*summonList);
                        for (GuidList::const_iterator iter = _summonList.begin(); iter != _summonList.end(); ++iter)
                            if(Unit* summon = ObjectAccessor::GetUnit(*caster, (*iter)))
                            {
                                summon->CastSpell(summon, 156287, true, nullptr, aurEff, caster->GetGUID());
                                summon->SendSpellCreateVisual(GetSpellInfo(), summon, NULL, 2);
                            }
                    }
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warr_ravager_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warr_ravager_AuraScript();
        }
};

// Ravager - 177466
class spell_warr_ravager_visual : public SpellScriptLoader
{
    public:
        spell_warr_ravager_visual() : SpellScriptLoader("spell_warr_ravager_visual") { }

        class spell_warr_ravager_visual_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warr_ravager_visual_AuraScript);

            void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* target = GetUnitOwner())
                    target->SetDisplayId(55644);
            }

            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_warr_ravager_visual_AuraScript::HandleApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warr_ravager_visual_AuraScript();
        }
};

// Revenge - 6572
class spell_warr_revenge : public SpellScriptLoader
{
    public:
        spell_warr_revenge() : SpellScriptLoader("spell_warr_revenge") { }

        class spell_warr_revenge_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_revenge_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (AuraEffect* aurEff = _player->GetAuraEffect(202560, EFFECT_0))
                    {
                        int32 targetCount = GetSpell()->GetTargetCount();
                        if (targetCount > aurEff->GetSpellInfo()->Effects[EFFECT_1]->BasePoints)
                            targetCount = aurEff->GetSpellInfo()->Effects[EFFECT_1]->BasePoints;

                        int32 percAdd = aurEff->GetAmount() * targetCount;
                        int32 damage = GetHitDamage();
                        damage += CalculatePct(damage, percAdd);
                        SetHitDamage(damage);
                    }
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_warr_revenge_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_revenge_SpellScript();
        }
};

// Bloodthirst - 23881
class spell_warr_bloodthirst : public SpellScriptLoader
{
    public:
        spell_warr_bloodthirst() : SpellScriptLoader("spell_warr_bloodthirst") { }

        class spell_warr_bloodthirst_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_bloodthirst_SpellScript);

            void HandleDamage(SpellEffIndex effIndex)
            {
                Unit* target = GetHitUnit();
                Unit* caster = GetCaster();
                if (!target || !caster)
                    return;

                if (AuraEffect* aurEff = caster->GetAuraEffect(199204, EFFECT_0))
                {
                    int32 delay = -int32((100.0f - target->GetHealthPct()) * 1000.0f / aurEff->GetSpellInfo()->Effects[EFFECT_2]->BasePoints);

                    int32 damage = GetHitDamage();
                    damage += CalculatePct(damage, aurEff->GetAmount());
                    SetHitDamage(damage);

                    if (Player* _player = caster->ToPlayer())
                    {
                        _player->AddDelayedEvent(100, [delay, _player]() -> void
                        {
                            if (_player)
                                _player->ModifySpellCooldown(23881, delay);
                        });
                    }
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_warr_bloodthirst_SpellScript::HandleDamage, EFFECT_1, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_bloodthirst_SpellScript();
        }
};

// Bodyguard (Honor Talent) - 213871
class spell_warr_bodyguard : public SpellScriptLoader
{
    public:
        spell_warr_bodyguard() : SpellScriptLoader("spell_warr_bodyguard") { }

        class spell_warr_bodyguard_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warr_bodyguard_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if(Unit* target = GetUnitOwner())
                    if(Unit* caster = GetCaster())
                        if (caster->GetDistance(target) > aurEff->GetAmount())
                            GetAura()->Remove();
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warr_bodyguard_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warr_bodyguard_AuraScript();
        }
};

// Shield Bash - 198912
class spell_warr_shield_bash : public SpellScriptLoader
{
    public:
        spell_warr_shield_bash() : SpellScriptLoader("spell_warr_shield_bash") { }

        class spell_warr_shield_bash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_shield_bash_SpellScript);

            void HandleDamage(SpellEffIndex effIndex)
            {
                Unit* target = GetHitUnit();
                Unit* caster = GetCaster();
                if (!target || !caster)
                    return;

                for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_AUTOREPEAT_SPELL; ++i)
                {
                    if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
                    {
                        SpellInfo const* curSpellInfo = spell->m_spellInfo;
                        // check if we can interrupt spell
                        if ((spell->getState() == SPELL_STATE_CASTING
                            || (spell->getState() == SPELL_STATE_PREPARING && spell->GetCastTime() > 0.0f))
                            && (curSpellInfo->Categories.PreventionType & SPELL_PREVENTION_TYPE_SILENCE || curSpellInfo->Categories.PreventionType & SPELL_PREVENTION_TYPE_PACIFY)
                            && ((i == CURRENT_GENERIC_SPELL && curSpellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_INTERRUPT)
                            || (i == CURRENT_CHANNELED_SPELL && curSpellInfo->HasChannelInterruptFlag(CHANNEL_INTERRUPT_FLAG_INTERRUPT))))
                        {
                            if (Player* _player = caster->ToPlayer())
                            {
                                _player->AddDelayedEvent(100, [_player]() -> void
                                {
                                    if (_player)
                                        _player->RemoveSpellCooldown(198912, true);
                                });
                            }
                        }
                    }
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_warr_shield_bash_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_shield_bash_SpellScript();
        }
};

// Bloodthirst Heal - 117313
class spell_warr_bloodthirst_heal : public SpellScriptLoader
{
public:
    spell_warr_bloodthirst_heal() : SpellScriptLoader("spell_warr_bloodthirst_heal") { }

    class spell_warr_bloodthirst_heal_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warr_bloodthirst_heal_SpellScript);

        void HandleHeal(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                if (caster->HealthBelowPct(20))
                    if (AuraEffect* aurEff = caster->GetAuraEffect(200859, EFFECT_0)) // Bloodcraze
                        SetEffectValue(GetEffectValue() + aurEff->GetAmount());
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warr_bloodthirst_heal_SpellScript::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL_PCT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warr_bloodthirst_heal_SpellScript();
    }
};

// Mortal Strike - 12294
class spell_warr_mortal_strike : public SpellScriptLoader
{
    public:
    spell_warr_mortal_strike() : SpellScriptLoader("spell_warr_mortal_strike") {}

    class spell_warr_mortal_strike_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warr_mortal_strike_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_WARRIOR_MORTAL_WOUNDS });
        }

        void HandleMortalWounds(SpellEffIndex /*effIndex*/)
        {
            GetCaster()->CastSpell(GetHitUnit(), SPELL_WARRIOR_MORTAL_WOUNDS, true);
        }

        void HandleJump(int32& AddJumpTarget)
        {
            if (AuraEffect const* eff = GetTriggeredAuraEff())
                AddJumpTarget = 0;
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_warr_mortal_strike_SpellScript::HandleMortalWounds, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnObjectJumpTarget += SpellObjectJumpTargetFn(spell_warr_mortal_strike_SpellScript::HandleJump, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
            OnObjectJumpTarget += SpellObjectJumpTargetFn(spell_warr_mortal_strike_SpellScript::HandleJump, EFFECT_1, TARGET_UNIT_TARGET_ENEMY);
            OnObjectJumpTarget += SpellObjectJumpTargetFn(spell_warr_mortal_strike_SpellScript::HandleJump, EFFECT_2, TARGET_UNIT_TARGET_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_warr_mortal_strike_SpellScript();
    }
};

// Bladestorm - 50622 
class spell_warr_bladestorm : public SpellScriptLoader
{
    public:
        spell_warr_bladestorm() : SpellScriptLoader("spell_warr_bladestorm") { }

        class spell_warr_bladestorm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_bladestorm_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(242297)) // Item - Warrior T20 Arms 4P Bonus
                    {
                        if (AuraEffect const* eff = GetTriggeredAuraEff())
                        {
                            uint8 tickNum = eff->GetTickNumber();

                            if (tickNum != 1 && tickNum != 6)
                                return;

                            if (WorldObject* object = Trinity::Containers::SelectRandomContainerElement(targets))
                            {
                                if (Unit* unit = object->ToUnit())
                                    caster->CastSpell(unit, 12294, true, nullptr, eff);
                            }
                        }
                    }
                }
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warr_bladestorm_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_bladestorm_SpellScript();
        }
};

// Ravager - 156287
class spell_warr_ravager_t20 : public SpellScriptLoader
{
    public:
        spell_warr_ravager_t20() : SpellScriptLoader("spell_warr_ravager_t20") { }

        class spell_warr_ravager_t20_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_ravager_t20_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (Unit* caster = GetCaster())
                {
                    Unit* owner = caster->GetAnyOwner();
                    if (!owner)
                        owner = caster;

                    if (owner->HasAura(242297)) // Item - Warrior T20 Arms 4P Bonus
                    {
                        if (AuraEffect const* eff = GetTriggeredAuraEff())
                        {
                            uint8 tickNum = eff->GetTickNumber();

                            if (tickNum != 1)
                                return;

                            if (WorldObject* object = Trinity::Containers::SelectRandomContainerElement(targets))
                            {
                                if (targets.empty())
                                    return;

                                if (Unit* unit = object->ToUnit())
                                    owner->CastSpell(unit, 12294, true, nullptr, eff);
                            }
                        }
                    }
                }
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warr_ravager_t20_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_ravager_t20_SpellScript();
        }
};

// 23881, 218617, 184707, 184709, 201364, 201363 
class spell_warr_meat_cleaver_damage : public SpellScriptLoader
{
    public:
        spell_warr_meat_cleaver_damage() : SpellScriptLoader("spell_warr_meat_cleaver_damage") { }

        class spell_warr_meat_cleaver_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_meat_cleaver_damage_SpellScript);

            void HandleOnHit()
            {
                if (Player* _plr = GetCaster()->ToPlayer())
                {
                    if (_plr->GetSelectedUnit() != GetHitUnit())
                    {
                        if (AuraEffect* aurEff = _plr->GetAuraEffect(85739, EFFECT_2))
                        {
                            int32 damage = GetHitDamage();
                            damage = CalculatePct(damage, aurEff->GetAmount());
                            SetHitDamage(damage);
                        }
                    }
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_warr_meat_cleaver_damage_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_meat_cleaver_damage_SpellScript();
        }
};

// Rampage - 218617, 184707, 184709, 201364, 201363 
class spell_warr_rampage : public SpellScriptLoader
{
    public:
        spell_warr_rampage() : SpellScriptLoader("spell_warr_rampage") { }

        class spell_warr_rampage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_rampage_SpellScript);

            void HandleOnHit()
            {
                if (Player* plr = GetCaster()->ToPlayer())
                {
                    if (plr->GetSelectedUnit() == GetHitUnit())
                    {
                        if (plr->HasAura(248120)) // Valarjar Berserkers
                            if (GetSpell()->IsCritForTarget(GetHitUnit()))
                                plr->CastSpell(plr, 248179, true);
                    }
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_warr_rampage_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_rampage_SpellScript();
        }
};

// 197690 - Call Visual for Defensive Stance
class spell_warr_shield_visual : public SpellScriptLoader
{
    public:
        spell_warr_shield_visual() : SpellScriptLoader("spell_warr_shield_visual") { }

        class spell_warr_shield_visual_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_shield_visual_SpellScript);

            void HandleBeforeCast()
            {
                if (!GetCaster())
                    return;
                Player* caster = GetCaster()->ToPlayer();
                if (!caster)
                    return;

                if (caster->GetTeam() == HORDE)
                    caster->CastSpell(caster, 146127, true);
                else
                    caster->CastSpell(caster, 147925, true);
            }

            void Register()
            {
                BeforeCast += SpellCastFn(spell_warr_shield_visual_SpellScript::HandleBeforeCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warr_shield_visual_SpellScript();
        }
};

void AddSC_warrior_spell_scripts()
{
    RegisterSpellScript(spell_warr_avatar);
    RegisterSpellScript(spell_warr_impending_victory);
    RegisterSpellScript(spell_warr_victory_rush);
    RegisterSpellScript(spell_warr_commanding_shout);
    RegisterSpellScript(spell_warr_intimidating_shout);
    RegisterSpellScript(spell_warr_shockwave);
    RegisterSpellScript(spell_warr_storm_bolt);
    RegisterAuraScript(spell_warr_revenge_trigger);
    RegisterAuraScript(spell_warr_war_machine);
    RegisterAuraScript(spell_warr_executioners_precision);
    RegisterAuraScript(spell_warr_focused_rage_arms);
    RegisterAuraScript(spell_warr_overpower_passive);
    RegisterAuraScript(spell_warr_precise_strikes);
    RegisterAuraScript(spell_warr_in_for_the_kill);
    RegisterAuraScript(spell_warr_trauma);
    RegisterAuraScript(spell_warr_corrupted_blood_of_zakajz);
    new spell_warr_shield_block();
    new spell_warr_heroic_leap();
    RegisterSpellScript(spell_warr_charge);
    RegisterSpellScript(spell_warr_charge_effect);
    new spell_war_intervene();
    new spell_warr_charge_drop_fire();
    new spell_warr_execute();
    new spell_warr_fervor_of_battle();
    RegisterAuraScript(spell_warr_ignore_pain);
    RegisterAuraScript(spell_war_t21_prot_4p);
    new spell_warr_intercept();
    new spell_warr_charge_check_cast();
    RegisterSpellScript(spell_warr_ravager_cast);
    new spell_warr_ravager();
    new spell_warr_ravager_visual();
    new spell_warr_revenge();
    new spell_warr_bodyguard();
    new spell_warr_bloodthirst();
    new spell_warr_shield_bash();
    new spell_warr_bloodthirst_heal();
    new spell_warr_bladestorm();
    new spell_warr_ravager_t20();
    new spell_warr_meat_cleaver_damage();
    new spell_warr_rampage();
    new spell_warr_shield_visual();
    new spell_warr_mortal_strike();
}
