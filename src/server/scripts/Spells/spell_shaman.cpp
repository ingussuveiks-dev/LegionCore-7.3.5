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
 * Scripts for spells with SPELLFAMILY_SHAMAN and SPELLFAMILY_GENERIC spells used by shaman players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_sha_".
 */

#include "ScriptMgr.h"
#include "GridNotifiers.h"
#include "Unit.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "ScriptedCreature.h"

// Spirit Link - 98020 : triggered by 98017
// Spirit Link Totem
class spell_sha_spirit_link : public SpellScriptLoader
{
    public:
        spell_sha_spirit_link() : SpellScriptLoader("spell_sha_spirit_link") { }

        class spell_sha_spirit_link_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_spirit_link_SpellScript);

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                std::list<WorldObject*> removeList;
                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    if ((*itr) && (*itr)->GetTypeId() != TYPEID_PLAYER)
                        removeList.push_back(*itr);

                for (std::list<WorldObject*>::iterator iter = removeList.begin(); iter != removeList.end(); ++iter)
                    targets.remove(*iter);
            }
            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    std::vector<TargetInfoPtr>* memberList = GetSpell()->GetUniqueTargetInfo();
                    if(memberList->empty())
                        return;

                    float totalRaidHealthPct = 0;
                    for (std::vector<TargetInfoPtr>::iterator ihit = memberList->begin(); ihit != memberList->end(); ++ihit)
                    {
                        if(Unit* member = ObjectAccessor::GetUnit(*caster, (*ihit)->targetGUID))
                            totalRaidHealthPct += member->GetHealthPct();
                    }
                    totalRaidHealthPct /= memberList->size() * 100.0f;
                    for (std::vector<TargetInfoPtr>::iterator ihit = memberList->begin(); ihit != memberList->end(); ++ihit)
                    {
                        if(Unit* member = ObjectAccessor::GetUnit(*caster, (*ihit)->targetGUID))
                            member->SetHealth(uint32(totalRaidHealthPct * member->GetMaxHealth()));
                    }
                }
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_spirit_link_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                AfterCast += SpellCastFn(spell_sha_spirit_link_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_spirit_link_SpellScript();
        }
};

// Healing Rain - 73920, 215864
class spell_sha_healing_rain : public SpellScriptLoader
{
    public:
        spell_sha_healing_rain() : SpellScriptLoader("spell_sha_healing_rain") { }

        class spell_sha_healing_rain_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_healing_rain_SpellScript);

            void HandleOnCast()
            {
                if(Unit* caster = GetCaster())
                {
                    Position const* sumpos = GetExplTargetDest();
                    if(TempSummon* summon = caster->SummonCreature(73400, *sumpos))
                    {
                        if(caster->m_SummonSlot[17])
                        {
                            if(Creature* tempsummon = caster->GetMap()->GetCreature(caster->m_SummonSlot[17]))
                                tempsummon->DespawnOrUnsummon(500);
                        }
                        caster->m_SummonSlot[17] = summon->GetGUID();
                    }
                }
            }

            void Register() override
            {
                OnCast += SpellCastFn(spell_sha_healing_rain_SpellScript::HandleOnCast);
            }
        };

        class spell_sha_healing_rain_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_healing_rain_AuraScript);

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* caster = GetCaster())
                {
                    if(caster->m_SummonSlot[17])
                    {
                        if(Creature* summon = caster->GetMap()->GetCreature(caster->m_SummonSlot[17]))
                            summon->DespawnOrUnsummon(500);
                    }
                }
            }

            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_healing_rain_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_healing_rain_AuraScript();
        }

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_healing_rain_SpellScript();
        }
};

class EarthenPowerTargetSelector
{
    public:
        EarthenPowerTargetSelector() { }

        bool operator() (WorldObject* target)
        {
            if (!target->ToUnit())
                return true;

            if (!target->ToUnit()->HasAuraWithMechanic(1 << MECHANIC_SNARE))
                return true;

            return false;
        }
};

class spell_sha_bloodlust : public SpellScriptLoader
{
    public:
        spell_sha_bloodlust() : SpellScriptLoader("spell_sha_bloodlust") { }

        class spell_sha_bloodlust_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_bloodlust_SpellScript);

            bool Validate(SpellInfo const* /*SpellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(57724))
                    return false;
                return true;
            }

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                std::list<WorldObject*> removeList;

                for (auto itr : targets)
                {
                    if (Unit* unit = itr->ToUnit())
                    {
                        if (unit->HasAura(95809) || unit->HasAura(57724) || unit->HasAura(80354) || unit->HasAura(160455))
                        {
                            removeList.push_back(itr);
                            continue;
                        }
                    }
                }

                if (!removeList.empty())
                {
                    for (auto itr : removeList)
                        targets.remove(itr);
                }
            }

            void ApplyDebuff()
            {
                if (Unit* target = GetHitUnit())
                    target->CastSpell(target, 57724, true);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_bloodlust_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_bloodlust_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_CASTER_AREA_RAID);
                AfterHit += SpellHitFn(spell_sha_bloodlust_SpellScript::ApplyDebuff);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_bloodlust_SpellScript();
        }
};

class spell_sha_heroism : public SpellScriptLoader
{
    public:
        spell_sha_heroism() : SpellScriptLoader("spell_sha_heroism") { }

        class spell_sha_heroism_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_heroism_SpellScript);

            bool Validate(SpellInfo const* /*SpellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(57723))
                    return false;
                return true;
            }

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                std::list<WorldObject*> removeList;

                for (auto itr : targets)
                {
                    if (Unit* unit = itr->ToUnit())
                    {
                        if (unit->HasAura(95809) || unit->HasAura(57723) || unit->HasAura(80354) || unit->HasAura(160455))
                        {
                            removeList.push_back(itr);
                            continue;
                        }
                    }
                }

                if (!removeList.empty())
                {
                    for (auto itr : removeList)
                        targets.remove(itr);
                }
            }

            void ApplyDebuff()
            {
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target, 57723, true);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_heroism_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_heroism_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_CASTER_AREA_RAID);
                AfterHit += SpellHitFn(spell_sha_heroism_SpellScript::ApplyDebuff);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_heroism_SpellScript();
        }
};

// Chain Heal - 1064
class spell_sha_chain_heal : public SpellScript
{
    PrepareSpellScript(spell_sha_chain_heal);

    void HandleHeal(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (Unit* target = GetHitUnit())
        {
            int32 _heal = GetHitHeal();
            if (AuraEffect const* aurEff = caster->GetAuraEffect(200076, EFFECT_0)) // Deluge
            {
                if (target->HasAura(200075, caster->GetGUID()) || target->HasAura(61295, caster->GetGUID()))
                    _heal += CalculatePct(_heal, aurEff->GetAmount());
            }
            SetHitHeal(_heal);
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_sha_chain_heal::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
    }
};

// Astral Recall - 556
class spell_sha_astral_recall : public SpellScriptLoader
{
    public:
        spell_sha_astral_recall() : SpellScriptLoader("spell_sha_astral_recall") { }

        class spell_sha_astral_recall_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_astral_recall_SpellScript);

            void HandleAfterCast()
            {
                Unit* caster = GetCaster();
                if(!caster)
                    return;

                if (caster->HasAura(147787)) //Glyph of Astral Fixation
                {
                    if (Player::TeamForRace(caster->getRace()) == HORDE)
                        caster->CastSpell(caster, 147902, true);
                    else
                        caster->CastSpell(caster, 147901, true);
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_sha_astral_recall_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_astral_recall_SpellScript();
        }
};

// Glyph of Elemental Familiars - 148118
class spell_sha_elemental_familiars : public SpellScriptLoader
{
    public:
        spell_sha_elemental_familiars() : SpellScriptLoader("spell_sha_elemental_familiars") { }

        class spell_sha_elemental_familiars_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_elemental_familiars_SpellScript);

            void HandleAfterCast()
            {
                if(Unit* caster = GetCaster())
                {
                    Position const* sumpos = GetExplTargetDest();
                    uint32 entry[] = { 73556, 73559, 73560 };
                    SummonPropertiesEntry const* properties = sSummonPropertiesStore.LookupEntry(3221);
                    if(TempSummon* summon = caster->GetMap()->SummonCreature(entry[urand(0, 2)], *sumpos, properties, 600000, caster))
                    {
                        if(caster->m_SummonSlot[17])
                        {
                            if(Creature* tempsummon = caster->GetMap()->GetCreature(caster->m_SummonSlot[17]))
                                tempsummon->DespawnOrUnsummon(500);
                        }
                        caster->m_SummonSlot[17] = summon->GetGUID();
                    }
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_sha_elemental_familiars_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_elemental_familiars_SpellScript();
        }
};

// Cloudburst Totem - 157504
class spell_sha_cloudburst_totem : public SpellScriptLoader
{
    public:
        spell_sha_cloudburst_totem() : SpellScriptLoader("spell_sha_cloudburst_totem") { }

        class spell_sha_cloudburst_totem_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_cloudburst_totem_AuraScript);

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetUnitOwner())
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(157503);
                    if(!spellInfo || caster->ToTotem())
                        return;
                    float heal = CalculatePct(aurEff->GetAmount(), spellInfo->Effects[EFFECT_1]->CalcValue(caster));
                    caster->CastCustomSpell(caster, 157503, &heal, NULL, NULL, true);
                }
            }

            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_cloudburst_totem_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_cloudburst_totem_AuraScript();
        }
};

// Earth Shock - 8042
class spell_sha_earth_shock : public SpellScriptLoader
{
    public:
        spell_sha_earth_shock() : SpellScriptLoader("spell_sha_earth_shock") { }

        class spell_sha_earth_shock_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_earth_shock_SpellScript)

            void HandleDamage(SpellEffIndex /*eff*/)
            {
                SetHitDamage(CalculatePct(GetHitDamage(), GetSpell()->GetPowerCost(POWER_MAELSTROM)));
                if (Unit* target = GetHitUnit())
                    if(Unit* caster = GetCaster())
                        if (caster->HasAura(204398)) // Earthfury (Honor Talent)
                            if (GetSpell()->GetPowerCost(POWER_MAELSTROM) >= 100)
                                caster->CastSpell(target, 204399, true);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_earth_shock_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript *GetSpellScript() const override
        {
            return new spell_sha_earth_shock_SpellScript();
        }
};

// 188389 - Flame Shock
class spell_sha_flame_shock : public SpellScriptLoader
{
    public:
        spell_sha_flame_shock() : SpellScriptLoader("spell_sha_flame_shock") { }

        class spell_sha_flame_shock_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_flame_shock_AuraScript);

            void CalculateMaxDuration(int32 & duration)
            {
                duration += (duration / 20) * GetAura()->GetPowerCost(POWER_MAELSTROM);
            }

            void Register() override
            {
                DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_sha_flame_shock_AuraScript::CalculateMaxDuration);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_flame_shock_AuraScript();
        }
};

// 196840 - Frost Shock
class spell_sha_frost_shock : public SpellScriptLoader
{
    public:
        spell_sha_frost_shock() : SpellScriptLoader("spell_sha_frost_shock") { }

        class spell_sha_frost_shock_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_frost_shock_AuraScript);

            void CalculateMaxDuration(int32 & duration)
            {
                duration += (duration / 20) * GetAura()->GetPowerCost(POWER_MAELSTROM);
            }

            void Register() override
            {
                DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_sha_frost_shock_AuraScript::CalculateMaxDuration);
            }
        };

        class spell_sha_frost_shock_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_frost_shock_SpellScript)

            void HandleDamage(SpellEffIndex /*eff*/)
            {
                SetHitDamage(GetHitDamage() + CalculatePct(GetHitDamage(), GetSpell()->GetPowerCost(POWER_MAELSTROM) * 5));
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_frost_shock_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript *GetSpellScript() const override
        {
            return new spell_sha_frost_shock_SpellScript();
        }

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_frost_shock_AuraScript();
        }
};

// 187837 - Lightning Bolt
class spell_sha_lightning_bolt : public SpellScriptLoader
{
    public:
        spell_sha_lightning_bolt() : SpellScriptLoader("spell_sha_lightning_bolt") { }

        class spell_sha_lightning_bolt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_lightning_bolt_SpellScript)

            void HandleDamage(SpellEffIndex /*eff*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(210727))
                    {
                        float dmgMod = 1.f + CalculatePct(1.f, GetSpell()->GetPowerCost(POWER_MAELSTROM) * (spellInfo->Effects[EFFECT_1]->BasePoints / spellInfo->Effects[EFFECT_0]->BasePoints));
                        SetHitDamage(GetHitDamage() * dmgMod);
                    }
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_lightning_bolt_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript *GetSpellScript() const override
        {
            return new spell_sha_lightning_bolt_SpellScript();
        }
};

// 188196 - Lightning bolt, 188443 - Chain Lightning, 114074 - Lava Beam
/*class spell_sha_static_overload : public SpellScriptLoader
{
public:
	spell_sha_static_overload() : SpellScriptLoader("spell_sha_static_overload") { }

	class spell_sha_static_overload_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_sha_static_overload_SpellScript)

			void HandleDamage()
		{
			if (Unit * caster = GetCaster())
			{
				if (Unit * hitTarget = GetHitUnit())
				{
					if (Aura * aur = caster->GetAura(205495)) //Stormkeeper
					{
						if (caster->HasAura(205495))
						{
							if (aur->GetStackAmount() >= 2)
							{
								std::list<Unit*> targets;
								hitTarget->GetAttackableUnitListInRange(targets, 10);

								for (auto target : targets)
									if (target != hitTarget)
									{
										switch (GetSpellInfo()->Id)
										{
										case 188196:
											caster->CastSpellDelay(target, 45284, true, 500);
											break;
										case 188443:
											caster->CastSpellDelay(target, 45297, true, 500);
											break;
										case 114074:
											caster->CastSpellDelay(target, 114738, true, 500);
											break;
										}
									}
							}
						}
					}
				}
			}
		}

		void Register() override
		{
			AfterHit += SpellHitFn(spell_sha_static_overload_SpellScript::HandleDamage);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_sha_static_overload_SpellScript();
	}
};*/

// Mastery: Elemental Overload - 168534
// 188196 - Lightning Bolt, 188443 - Chain Lightning, 114074 - Lava Beam,
// 51505 - Lava Burst, 117014 - Elemental Blast, 210714 - Icefury
class spell_sha_elemental_overload : public SpellScriptLoader
{
public:
	spell_sha_elemental_overload() : SpellScriptLoader("spell_sha_elemental_overload") { }

	class spell_sha_elemental_overload_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_sha_elemental_overload_SpellScript)

		void HandleCast()
		{
			Unit* caster = GetCaster();
			Unit* target = GetExplTargetUnit();
			SpellInfo const* spellInfo = GetSpellInfo();
			if (!caster || !target || !spellInfo)
				return;

			AuraEffect const* mastery = caster->GetAuraEffect(168534, EFFECT_0);
			if (!mastery)
				return;

			uint32 overloadSpell = 0;
			switch (spellInfo->Id)
			{
				case 188196: overloadSpell = 45284;  break; // Lightning Bolt Overload
				case 188443: overloadSpell = 45297;  break; // Chain Lightning Overload
				case 114074: overloadSpell = 114738; break; // Lava Beam Overload
				case 51505:  overloadSpell = 77451;  break; // Lava Burst Overload
				case 117014: overloadSpell = 120588; break; // Elemental Blast Overload
				case 210714: overloadSpell = 219271; break; // Icefury Overload
				default: return;
			}

			// Static Overload supplies the guaranteed first Stormkeeper overload.
			// Do not also roll the normal mastery proc for that same cast.
			if (caster->HasAura(191634) &&
				(spellInfo->Id == 188196 || spellInfo->Id == 188443 || spellInfo->Id == 114074))
				return;

			float chance = mastery->GetAmount();
			if (spellInfo->Id == 188196 || spellInfo->Id == 188443 || spellInfo->Id == 114074)
				if (AuraEffect const* stormTotem = caster->GetAuraEffect(210652, EFFECT_0))
					chance += stormTotem->GetAmount();

			// Chain Lightning and its Ascendance replacement use one third of the
			// normal overload chance. The resulting overload spell performs its own chain.
			if (spellInfo->Id == 188443 || spellInfo->Id == 114074)
				chance /= 3.0f;

			if (roll_chance_f(chance))
				caster->CastSpellDelay(target, overloadSpell, true, 500);
		}

		void Register() override
		{
			OnCast += SpellCastFn(spell_sha_elemental_overload_SpellScript::HandleCast);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_sha_elemental_overload_SpellScript();
	}
};

// 188196 - Lightning Bolt, 45284 - Lightning Bolt Overload, 188443 - Chain Lightning, 45297 - Chain lightning Overload, 114074 - Lava Beam, 114738 - Lava Beam Overload
class spell_sha_lightning_rod : public SpellScriptLoader
{
public:
	spell_sha_lightning_rod() : SpellScriptLoader("spell_sha_lightning_rod") { }

	class spell_sha_lightning_rod_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_sha_lightning_rod_SpellScript)

		void HandleDamage()
		{
			Unit* caster = GetCaster();
			Unit* hitTarget = GetHitUnit();
			SpellInfo const* spellInfo = GetSpellInfo();
			uint32 damage = GetHitDamage();
			if (!caster || !hitTarget || !spellInfo || !damage)
				return;

			AuraEffect const* rodDamage = caster->GetAuraEffect(210689, EFFECT_1);
			if (!rodDamage)
				return;

			bool isOriginalCast = spellInfo->Id == 188196 || spellInfo->Id == 188443 || spellInfo->Id == 114074;
			bool isPrimaryTarget = spellInfo->Id == 188196;
			if (Unit* explicitTarget = GetExplTargetUnit())
				isPrimaryTarget = explicitTarget->GetGUID() == hitTarget->GetGUID();

			// Lightning Bolt, Chain Lightning, and Lava Beam only create a rod on
			// the primary target. Overload payloads can deal rod damage but cannot
			// create another rod themselves.
			if (isOriginalCast && isPrimaryTarget)
				if (AuraEffect const* rodChance = caster->GetAuraEffect(210689, EFFECT_0))
					if (roll_chance_f(rodChance->GetAmount()))
						caster->CastSpell(hitTarget, 197209, true);

			std::list<Unit*> targets;
			std::vector<uint32> aura = { 197209 };
			caster->TargetsWhoHasMyAuras(targets, aura);

			float sharedDamage = CalculatePct(damage, rodDamage->GetAmount());
			for (Unit* target : targets)
				if (target && target->IsAlive())
					caster->CastCustomSpell(target, 197568, &sharedDamage, nullptr, nullptr, true);
		}

		void Register() override
		{
			AfterHit += SpellHitFn(spell_sha_lightning_rod_SpellScript::HandleDamage);
		}
	};

	SpellScript *GetSpellScript() const override
	{
		return new spell_sha_lightning_rod_SpellScript();
	}
};

// Flametongue Attack - 10444
// Legion scales the 20% AP payload by the off-hand weapon's actual speed
// relative to the 2.6 second reference speed.
class spell_sha_flametongue_attack : public SpellScript
{
    PrepareSpellScript(spell_sha_flametongue_attack);

    void ScaleDamage(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            SetHitDamage(int32(GetHitDamage() * caster->GetAttackTime(OFF_ATTACK) / 2600.0f));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_sha_flametongue_attack::ScaleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// Ascendance - 114051
// Entering Enhancement Ascendance resets the shared Stormstrike/Windstrike cooldown.
class spell_sha_ascendance_enhancement : public SpellScript
{
    PrepareSpellScript(spell_sha_ascendance_enhancement);

    void HandleCast()
    {
        if (Player* player = GetCaster()->ToPlayer())
        {
            player->RemoveSpellCooldown(17364, true);
            player->RemoveSpellCooldown(115356, true);
        }
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_sha_ascendance_enhancement::HandleCast);
    }
};

// Frostbrand - 196834 / Hailstorm - 210853
// Frostbrand already applies its slow through effect 1. Hailstorm adds the
// separate 210854 weapon strike to that same successful weapon proc.
class spell_sha_hailstorm : public AuraScript
{
    PrepareAuraScript(spell_sha_hailstorm);

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        Unit* caster = GetTarget();
        Unit* target = eventInfo.GetActionTarget();
        if (caster && target && caster->HasAura(210853))
            caster->CastSpell(target, 210854, true, nullptr, aurEff);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_sha_hailstorm::HandleProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

// Fury of Air - 197211
// SpellPower.db2 build 26972 stores a 3 Maelstrom upkeep cost.
class spell_sha_fury_of_air : public AuraScript
{
    PrepareAuraScript(spell_sha_fury_of_air);

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        if (Unit* caster = GetCaster())
        {
            if (caster->GetPower(POWER_MAELSTROM) >= 3)
                caster->ModifyPower(POWER_MAELSTROM, -3);
            else
                caster->RemoveAurasDueToSpell(197211);
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_fury_of_air::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

// Windfury - 33757
// Doom Winds guarantees Windfury only for auto attacks. It also permits the
// off-hand auto attack, while ordinary off-hand attacks remain ineligible.
class spell_sha_windfury : public AuraScript
{
    PrepareAuraScript(spell_sha_windfury);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        Unit* caster = GetTarget();
        Unit* target = eventInfo.GetActionTarget();
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!caster || !target || !target->IsAlive() || !damageInfo || !damageInfo->GetDamage())
            return false;

        bool autoAttack = eventInfo.GetSpell() == nullptr;
        bool offHand = (eventInfo.GetTypeMask() & PROC_FLAG_DONE_OFFHAND_ATTACK) != 0;
        bool doomWinds = caster->HasAura(204945);

        if (offHand && (!doomWinds || !autoAttack))
            return false;

        if (doomWinds && autoAttack)
            return true;

        float chance = 20.0f;
        if (AuraEffect const* mastery = caster->GetAuraEffect(77223, EFFECT_3))
            chance += mastery->GetAmount();

        return roll_chance_f(chance);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        if (Unit* target = eventInfo.GetActionTarget())
            if (Unit* caster = GetTarget())
                for (uint8 i = 0; i < 2; ++i)
                    caster->CastSpell(target, 25504, true, nullptr, aurEff);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_sha_windfury::CheckProc);
        OnEffectProc += AuraEffectProcFn(spell_sha_windfury::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// Lava Surge - 77756
// Flame Shock's periodic damage supplies the 10% proc chance through the
// client proc data. Restore one Lava Burst charge and apply the instant-cast
// buff after the same short delay used by the original data chain.
class spell_sha_lava_surge : public AuraScript
{
    PrepareAuraScript(spell_sha_lava_surge);

    void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
    {
        PreventDefaultAction();

        if (Player* player = GetTarget()->ToPlayer())
        {
            player->RemoveSpellCooldown(51505, true);
            player->CastSpellDelay(player, 77762, true, 250);
        }
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_sha_lava_surge::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// Healing Surge - 8004, Healing Wave - 77472, talent 200071 - Undulation
class spell_sha_undulation : public SpellScriptLoader
{
    public:
        spell_sha_undulation() : SpellScriptLoader("spell_sha_undulation") { }

        class spell_sha_undulation_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_undulation_SpellScript);

            void HandleAfterCast()
            {
                if(Unit* caster = GetCaster())
                    if(Aura* aura = caster->GetAura(200071)) // Undulation
                    {
                        if(aura->GetCustomData() >= 2)
                        {
                            caster->CastSpell(caster, 216251, true);
                            aura->ModCustomData(-2);
                        }
                        else
                            aura->ModCustomData(1);
                    }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_sha_undulation_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_undulation_SpellScript();
        }
};

// Ancestral Protection - 207498
class spell_sha_ancestral_protection : public AuraScript
{
    PrepareAuraScript(spell_sha_ancestral_protection);

    bool canProc = true;

    void CalculateAmount(AuraEffect const* /*aurEff*/, float& amount, bool& /*canBeRecalculated*/)
    {
        amount = -1;
    }

    uint32 CallSpecialFunction(uint32 /*Num*/) override
    {
        if (!canProc)
            return 0;

        if (Unit* caster = GetCaster())
        {
            if (Creature* cre = caster->ToCreature())
            {
                caster->RemoveAurasDueToSpell(207495, caster->GetGUID());
                cre->DespawnOrUnsummon(500);
            }

            if (Unit* target = GetUnitOwner())
                caster->CastSpell(target, 255234, true);
        }

        return 207553;
    }

    void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, float & /*absorbAmount*/)
    {
        if (!canProc)
            return;

        if (Unit* target = GetTarget())
        {
            uint32 damageLimit = target->GetMaxHealth() * 2;
            uint32 damage = dmgInfo.GetDamage();

            if (damageLimit < damage)
                canProc = false;
        }
    }

    void Register() override
    {
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_sha_ancestral_protection::Absorb, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_ancestral_protection::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
    }
};

// Earthen Rage - 170377
class spell_sha_earthen_rage : public AuraScript
{
    PrepareAuraScript(spell_sha_earthen_rage);

    ObjectGuid targetGUID;

    void OnTick(AuraEffect const* /*aurEff*/)
    {
        if (Unit* owner = GetUnitOwner())
        {
            if (!targetGUID)
            {
                Trinity::AnyDataContainer& cont = owner->GetAnyDataContainer();

                if (!(targetGUID = cont.GetValue<ObjectGuid>("EarthenRageTarget", ObjectGuid::Empty)))
                    return;

                cont.Remove("EarthenRageTarget");
            }

            if (Unit* target = ObjectAccessor::GetUnit(*owner, targetGUID))
                owner->CastSpell(target, 170379, true);
        }
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* owner = GetUnitOwner())
        {
            if (Aura* aura = owner->GetAura(170374))
                aura->SetAuraAttribute(AURA_ATTR_IS_NOT_ACTIVE, false);
        }
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectRemoveFn(spell_sha_earthen_rage::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_earthen_rage::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// Earthen Shield - 201633
class spell_sha_earthen_shield : public SpellScriptLoader
{
    public:
        spell_sha_earthen_shield() : SpellScriptLoader("spell_sha_earthen_shield") { }

        class spell_sha_earthen_shield_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_earthen_shield_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, float& amount, bool& /*canBeRecalculated*/)
            {
                amount = -1;
            }

            void Absorb(AuraEffect* aurEff, DamageInfo & dmgInfo, float & absorbAmount)
            {
                Unit* target = GetTarget();
                Unit* caster = GetCaster();
                if (!caster || !target)
                    return;

                Unit* owner = caster->GetAnyOwner();
                if (caster->IsAlive() && owner)
                {
                    float bp = owner->GetSpellPowerDamage(SPELL_SCHOOL_MASK_NATURE);
                    absorbAmount = bp;

                    if (dmgInfo.GetDamage() < uint32(bp))
                        bp = dmgInfo.GetDamage();

                    target->CastCustomSpell(caster, 201657, &bp, NULL, NULL, true);
                }
                else
                    absorbAmount = 0;
            }

            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_earthen_shield_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_sha_earthen_shield_AuraScript::Absorb, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_earthen_shield_AuraScript();
        }
};

// Recall Cloudburst Totem - 201764
class spell_sha_recall_cloudburst_totem : public SpellScriptLoader
{
    public:
        spell_sha_recall_cloudburst_totem() : SpellScriptLoader("spell_sha_recall_cloudburst_totem") { }

        class spell_sha_recall_cloudburst_totem_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_recall_cloudburst_totem_SpellScript);

            void HandleAfterCast()
            {
                if(Unit* caster = GetCaster())
                {
                    GuidList* summonList = caster->GetSummonList(78001);
                    for (GuidList::const_iterator iter = summonList->begin(); iter != summonList->end(); ++iter)
                        if(Creature* summon = ObjectAccessor::GetCreature(*caster, (*iter)))
                            summon->DespawnOrUnsummon(500);
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_sha_recall_cloudburst_totem_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_recall_cloudburst_totem_SpellScript();
        }
};

//204288
class spell_sha_earth_shield : public SpellScriptLoader
{
    public:
        spell_sha_earth_shield() : SpellScriptLoader("spell_sha_earth_shield") {}

        class spell_sha_earth_shield_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_earth_shield_AuraScript);

            void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                if (!GetCaster() || !GetTarget() || eventInfo.GetActor()->GetGUID() == GetTarget()->GetGUID())
                {
                    PreventDefaultAction();
                    return;
                }

                if (eventInfo.GetDamageInfo()->GetDamage() >= GetTarget()->CountPctFromMaxHealth(GetSpellInfo()->Effects[EFFECT_1]->BasePoints))
                    GetCaster()->CastSpell(GetTarget(), 204290, true); // Heal
                else
                    PreventDefaultAction();
            }

            void Register() override
            {
                OnEffectProc += AuraEffectProcFn(spell_sha_earth_shield_AuraScript::OnProc, EFFECT_1, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_earth_shield_AuraScript();
        }
};

// Sundering - 197214
class spell_sha_sundering : public SpellScriptLoader
{
    public:
        spell_sha_sundering() : SpellScriptLoader("spell_sha_sundering") { }

        class spell_sha_sundering_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_sundering_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return ValidateSpellInfo({ 197619 });
            }

            void HandleDummy()
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();
                if (caster && target)
                {
                    // The 7.3.5 spell knocks each enemy perpendicular to the
                    // line of effect. Keep enemies on their existing side of
                    // the line so a pack is split instead of moved together.
                    float relativeAngle = Position::NormalizeOrientation(caster->GetRelativeAngle(target));
                    if (relativeAngle > float(M_PI))
                        relativeAngle -= 2.0f * float(M_PI);

                    float halfPi = 0.5f * float(M_PI);
                    float sideAngle = caster->GetOrientation() + (relativeAngle >= 0.0f ? halfPi : -halfPi);
                    Position pos = target->GetFirstCollisionPosition(10.0f, sideAngle - target->GetOrientation());
                    target->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 197619, true);
                }
            }

            void Register() override
            {
                BeforeHit += SpellHitFn(spell_sha_sundering_SpellScript::HandleDummy);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_sundering_SpellScript();
        }
};

// Hot Hand - 201900
class spell_sha_hot_hand : public AuraScript
{
    PrepareAuraScript(spell_sha_hot_hand);

    bool CheckProc(ProcEventInfo& eventInfo)
    {
        return eventInfo.GetActor() && eventInfo.GetActor()->HasAura(194084);
    }

    void Register() override
    {
        DoCheckProc += AuraCheckProcFn(spell_sha_hot_hand::CheckProc);
    }
};

// Forked Lightning (Honor Talent) - 204350
class spell_sha_forked_lightning_pvp : public SpellScriptLoader
{
    public:
    spell_sha_forked_lightning_pvp() : SpellScriptLoader("spell_sha_forked_lightning_pvp") {}

    class spell_sha_forked_lightning_pvp_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_sha_forked_lightning_pvp_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (Unit* caster = GetCaster())
            {
                targets.remove_if(Trinity::UnitCheckInLos(false, caster));
                targets.sort(Trinity::UnitSortDistance(false, caster));

                uint8 targetCount = irand(1, 2);

                if (targets.size() > targetCount)
                    targets.resize(targetCount);
            }
            
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_forked_lightning_pvp_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_sha_forked_lightning_pvp_SpellScript();
    }
};

// Spirit Link (Honor Talent) - 204293
class spell_sha_spirit_link_pvp : public SpellScriptLoader
{
    public:
        spell_sha_spirit_link_pvp() : SpellScriptLoader("spell_sha_spirit_link_pvp") { }

        class spell_sha_spirit_link_pvp_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_spirit_link_pvp_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                targets.sort(Trinity::UnitSortDistance(true, caster));
                if(targets.size() > 3)
                    targets.resize(3);

                GuidList targetList;
                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    if(WorldObject* object = (*itr))
                        targetList.push_back(object->GetGUID());

                GetSpell()->SetEffectTargets(targetList);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_spirit_link_pvp_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_spirit_link_pvp_SpellScript();
        }

        class spell_sha_spirit_link_pvp_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_spirit_link_pvp_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, float& amount, bool& /*canBeRecalculated*/)
            {
                amount = -1;
            }

            void Absorb(AuraEffect* aurEff, DamageInfo & dmgInfo, float & absorbAmount)
            {
                absorbAmount = 0;
                Unit* caster = GetTarget();
                if (!caster)
                    return;

                GuidList targets = aurEff->GetBase()->GetEffectTargets();
                if(targets.size() <= 1)
                    return;

                targets.remove(caster->GetGUID());
                float damage = CalculatePct(dmgInfo.GetDamage(), GetSpellInfo()->Effects[EFFECT_1]->CalcValue(GetCaster()));
                absorbAmount = dmgInfo.GetDamage() - damage;
                damage /= targets.size();

                for (GuidList::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    if (Unit* target = ObjectAccessor::GetUnit(*caster, (*itr)))
                        caster->CastCustomSpell(target, 204314, &damage, NULL, NULL, true);
            }

            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_spirit_link_pvp_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_sha_spirit_link_pvp_AuraScript::Absorb, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_spirit_link_pvp_AuraScript();
        }
};

// Purge - 370
class spell_sha_purge : public SpellScriptLoader
{
    public:
        spell_sha_purge() : SpellScriptLoader("spell_sha_purge") { }

        class spell_sha_purge_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_purge_SpellScript);

            void OnSuccessfulDispel(SpellEffIndex /*effIndex*/)
            {
                if(Unit* caster = GetCaster())
                    if (Unit* target = GetHitUnit())
                        if (caster->HasAura(206642)) // Electrocute (Honor Talent)
                            caster->CastSpell(target, 206647, true);
            }

            void Register() override
            {
                OnEffectSuccessfulDispel += SpellEffectFn(spell_sha_purge_SpellScript::OnSuccessfulDispel, EFFECT_0, SPELL_EFFECT_DISPEL);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_purge_SpellScript();
        }
};

// Grounding Totem (Honor Talent) - 242900
class spell_sha_grounding_totem : public SpellScriptLoader
{
    public:
        spell_sha_grounding_totem() : SpellScriptLoader("spell_sha_grounding_totem") { }

        class spell_sha_grounding_totem_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_grounding_totem_AuraScript);

            void CalculateAmount(AuraEffect const* aurEff, float& amount, bool& /*canBeRecalculated*/)
            {
                amount = -1;
            }

            void Absorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, float& absorbAmount)
            {
                absorbAmount = dmgInfo.GetDamage();
            }

            void Register() override
            {
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_sha_grounding_totem_AuraScript::Absorb, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_grounding_totem_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_grounding_totem_AuraScript();
        }
};

// Shamanism (Honor Talent) - 193876
class spell_sha_shamanism : public SpellScriptLoader
{
    public:
        spell_sha_shamanism() : SpellScriptLoader("spell_sha_shamanism") { }

        class spell_sha_shamanism_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_shamanism_AuraScript);

            void CalculateAmount(AuraEffect const* aurEff, float& amount, bool& /*canBeRecalculated*/)
            {
                amount = 0;
                if (!GetCaster())
                    return;

                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->GetTeamId() == TEAM_HORDE)
                        amount = 204361;
                    else
                        amount = 204362;
                }
            }

            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_shamanism_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_shamanism_AuraScript();
        }
};

// Stormstrike(17364), Windstrike(115356)
class spell_sha_stormflurry : public SpellScriptLoader
{
    public:
    spell_sha_stormflurry() : SpellScriptLoader("spell_sha_stormflurry") { }

        class spell_sha_stormflurry_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_stormflurry_SpellScript);

            void HandleOnHit(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Aura* aura = caster->GetAura(198367))
                    {
                        uint8 castCount = 0;
                        float bp0 = 0;

                        if (AuraEffect* eff = aura->GetEffect(EFFECT_0))
                            bp0 = eff->GetAmount();

                        // Every repeat can itself repeat. Keep a generous safety cap for
                        // the theoretically unbounded geometric chain.
                        while (castCount < 100 && roll_chance_f(bp0))
                            ++castCount;

                        if (!castCount)
                            return;

                        uint32 spellId0 = 32175;
                        uint32 spellId1 = 32176;

                        if (GetSpellInfo()->Id == 115356)
                        {
                            spellId0 = 115357;
                            spellId1 = 115360;
                        }

                        if (Unit* target = GetHitUnit())
                        {
                            if (AuraEffect* eff = aura->GetEffect(EFFECT_1))
                            {
                                for (uint8 i = 0; i < castCount; i++)
                                {
                                    caster->CastSpell(target, spellId0, true, NULL, eff);
                                    caster->CastSpell(target, spellId1, true, NULL, eff);
                                }
                            }
                        }
                    }
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_stormflurry_SpellScript::HandleOnHit, EFFECT_0, SPELL_EFFECT_TRIGGER_SPELL);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_stormflurry_SpellScript();
        }
};

// Stormstrike/Windstrike weapon payloads repeated by Stormflurry.
// Effect 1 of artifact aura 198367 stores the 40% repeat damage modifier.
class spell_sha_stormflurry_damage : public SpellScript
{
    PrepareSpellScript(spell_sha_stormflurry_damage);

    void ScaleDamage(SpellEffIndex /*effIndex*/)
    {
        if (AuraEffect const* trigger = GetTriggeredAuraEff())
            if (trigger->GetId() == 198367)
                SetHitDamage(CalculatePct(GetHitDamage(), trigger->GetAmount()));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_sha_stormflurry_damage::ScaleDamage, EFFECT_0, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
    }
};

// Stormlash - 195222 / damage - 213307
// A Stormlash buff owns a fixed pool based on the granting shaman's AP. Each
// eligible attack releases the fraction accumulated since the previous proc.
class spell_sha_stormlash_buff : public AuraScript
{
    PrepareAuraScript(spell_sha_stormlash_buff);

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        _lastProcTime = getMSTime();
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        Unit* shaman = GetCaster();
        Unit* actor = eventInfo.GetActor();
        Unit* target = eventInfo.GetActionTarget();
        if (!shaman || !actor || !target || !target->IsAlive())
            return;

        uint32 now = getMSTime();
        uint32 elapsed = getMSTimeDiff(_lastProcTime, now);
        if (elapsed <= 100)
            return;

        _lastProcTime = now;
        uint32 duration = std::max(1, GetAura()->GetMaxDuration());
        elapsed = std::min(elapsed, duration);

        SpellInfo const* damageSpell = sSpellMgr->GetSpellInfo(213307);
        if (!damageSpell)
            return;

        float damage = shaman->GetTotalAttackPowerValue(BASE_ATTACK)
            * damageSpell->Effects[EFFECT_0]->BonusCoefficientFromAP
            * elapsed / duration;

        if (AuraEffect const* empowered = shaman->GetAuraEffect(210731, EFFECT_1))
            AddPct(damage, empowered->GetAmount());

        if (damage > 0.0f)
            actor->CastCustomSpell(target, 213307, &damage, nullptr, nullptr, true, nullptr, aurEff, shaman->GetGUID());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_sha_stormlash_buff::HandleApply, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
        OnEffectProc += AuraEffectProcFn(spell_sha_stormlash_buff::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
    }

private:
    uint32 _lastProcTime = 0;
};

// The custom base point passed by spell_sha_stormlash_buff already contains
// the complete time-weighted pool share, so do not add the holder's AP again.
class spell_sha_stormlash_damage : public SpellScript
{
    PrepareSpellScript(spell_sha_stormlash_damage);

    void OverrideDamage(SpellEffIndex /*effIndex*/)
    {
        if (AuraEffect const* trigger = GetTriggeredAuraEff())
            if (trigger->GetId() == 195222)
                SetHitDamage(int32(GetSpellValue()->EffectBasePoints[EFFECT_0]));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_sha_stormlash_damage::OverrideDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// Alpha Wolf - 198486
class spell_sha_alpha_wolf : public SpellScriptLoader
{
    public:
        spell_sha_alpha_wolf() : SpellScriptLoader("spell_sha_alpha_wolf") { }

        class spell_sha_alpha_wolf_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_alpha_wolf_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetTarget())
                {
                    switch (caster->GetDisplayId())
                    {
                        case 55290:
                            caster->CastSpell(caster, 198455, true);
                            break;
                        case 66843:
                            caster->CastSpell(caster, 198480, true);
                            break;
                        case 66844:
                            caster->CastSpell(caster, 198483, true);
                            break;
                        case 66845:
                            if (Unit* target = caster->getVictim())
                                caster->CastSpell(target, 198485, true);
                            break;
                    }
                }
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_alpha_wolf_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_alpha_wolf_AuraScript();
        }
};

// Crash Lightning - 187874
class spell_sha_crash_lightning : public SpellScriptLoader
{
    public:
        spell_sha_crash_lightning() : SpellScriptLoader("spell_sha_crash_lightning") { }

        class spell_sha_crash_lightning_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_crash_lightning_SpellScript);

            void HandleAfterCast()
            {
                Unit* caster = GetCaster();
                if(!caster)
                    return;

                if (AuraEffect const* aurEff = caster->GetAuraEffect(198299, EFFECT_0)) // Gathering Storms
                {
                    float bp0 = aurEff->GetAmount() * GetSpell()->GetTargetCount();
                    caster->CastCustomSpell(caster, 198300, &bp0, NULL, NULL, true);
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_sha_crash_lightning_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_crash_lightning_SpellScript();
        }
};

// Sense of Urgency - 2825, 32182, 80353, 90355, 146555, 160452, 178207
class spell_sha_sense_of_urgency : public SpellScriptLoader
{
    public:
        spell_sha_sense_of_urgency() : SpellScriptLoader("spell_sha_sense_of_urgency") { }

        class spell_sha_sense_of_urgency_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_sense_of_urgency_AuraScript);

            void CalculateMaxDuration(int32 & duration)
            {
                if (Unit* caster = GetUnitOwner())
                {
                    if (AuraEffect const* aurEff = caster->GetAuraEffect(234814, EFFECT_0)) // Uncertain Reminder
                        duration += aurEff->GetAmount() * 1000;
                    if (caster->HasAura(207355)) // Sense of Urgency
                        caster->CastSpellDuration(caster, 208416, true, duration);
                }
            }

            void Register() override
            {
                DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_sha_sense_of_urgency_AuraScript::CalculateMaxDuration);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_sense_of_urgency_AuraScript();
        }
};

// 207778
class spell_monk_gift_of_queen : public SpellScriptLoader
{
    public:
        spell_monk_gift_of_queen() : SpellScriptLoader("spell_monk_gift_of_queen") { }

        class spell_monk_gift_of_queen_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_gift_of_queen_SpellScript);

            void HandleDummy()
            {
                if (auto caster = GetCaster())
                    if (caster->HasAura(238143))
                        if (WorldLocation const* pos = GetExplTargetDest())
                        {
                            Position posit = pos->GetPosition();
                            caster->CastSpellDelay(posit, 255227, true, 3000);
                        }
            }

            void Register() override
            {
                OnCast += SpellCastFn(spell_monk_gift_of_queen_SpellScript::HandleDummy);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_monk_gift_of_queen_SpellScript();
        }
};

//196884
class spell_sha_feral_lunge : public SpellScript
{
    PrepareSpellScript(spell_sha_feral_lunge);

    SpellCastResult CheckElevation()
    {
        auto caster = GetCaster();
        auto target = GetExplTargetUnit();

        if (!caster || !target)
            return SPELL_FAILED_BAD_TARGETS;

        float delta_z = fabs(target->GetPositionZ()) - fabs(caster->GetPositionZ());
        if (caster->GetMap()->IsBattleground())
        {
            switch (caster->GetMapId())
            {
            case 607:
                if ((target->GetEntry() == 27894) || (target->FindNearestCreature(27894, 11.0f) && delta_z > 4.0f))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                break;
            case 726:
                if (((target->GetPositionX() < 1843.0f && target->GetPositionX() > 1798.0f) && (target->GetPositionY() < 173.0f && target->GetPositionY() > 134.0f) && target->GetPositionZ() >= -3.0f)
                    || ((target->GetPositionX() < 1896.0f && target->GetPositionX() > 1860.0f) && (target->GetPositionY() < 461.0f && target->GetPositionY() > 427.0f) && target->GetPositionZ() >= -10.0f))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                break;
            default:
                break;
            }
        }
        return SPELL_CAST_OK;
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
            GetCaster()->CastSpell(target, 215802, true);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_sha_feral_lunge::CheckElevation);
        OnEffectHitTarget += SpellEffectFn(spell_sha_feral_lunge::HandleDamage, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Feral Spirit - creature 29264
// Rank 2 (231723) grants the owner 5 Maelstrom through 190185 for every
// successful wolf melee swing, but not for Alpha Wolf spell damage.
struct npc_sha_feral_spirit : public ScriptedAI
{
    explicit npc_sha_feral_spirit(Creature* creature) : ScriptedAI(creature) { }

    void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType damageType) override
    {
        if (!damage || damageType != DIRECT_DAMAGE)
            return;

        if (Unit* owner = me->GetOwner())
            if (owner->HasAura(231723))
                me->CastSpell(owner, 190185, true);
    }
};

// Hex - 51514, 210873, 211004, 211010, 211015 
class spell_sha_hex : public AuraScript
{
    PrepareAuraScript(spell_sha_hex);

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
            if (Unit* target = GetUnitOwner())
                caster->CastSpell(target, 93958, true);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* target = GetUnitOwner())
            target->RemoveAurasDueToSpell(93958, GetCasterGUID());
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_sha_hex::OnApply, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_sha_hex::OnRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
    }
};

// Lightning Rod - 197209
/*class spell_sha_lightning_rod : public AuraScript
{
    PrepareAuraScript(spell_sha_lightning_rod);

    void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        if (DamageInfo* dmgInfo = eventInfo.GetDamageInfo())
        {
            if (Unit* attacker = eventInfo.GetActor())
            {
                if (uint32 dmg0 = dmgInfo->GetDamage())
                {
                    float dmg = CalculatePct(dmg0, aurEff->GetAmount() * 2);

                    if (GetCasterGUID() != attacker->GetGUID())
                    {
                        PreventDefaultAction();
                        return;
                    }
					else if(Unit* target = GetTarget())
						attacker->CastCustomSpell(target, 197568, &dmg, nullptr, nullptr, true);
					else 
					{
						std::list<Unit*> targets;
						std::vector<uint32> aura = { 197209 };

						attacker->TargetsWhoHasMyAuras(targets, aura);

						for (auto target : targets)
							attacker->CastCustomSpell(target, 197568, &dmg, nullptr, nullptr, true);
					}
                }
            }
        }
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_sha_lightning_rod::OnProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};*/

// 117014 - Elemental Blast, 120588 - Elemental Blast Overload
class spell_sha_elem_blast : public SpellScript
{
    PrepareSpellScript(spell_sha_elem_blast);

    void HandleLaunch(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
        {
            uint32 const buffs[] = { 118522, 173183, 173184 };
            caster->CastSpell(caster, buffs[urand(0, 2)], true);
        }
    }

    void Register() override
    {
        OnEffectLaunch += SpellEffectFn(spell_sha_elem_blast::HandleLaunch, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

void AddSC_shaman_spell_scripts()
{
    new spell_sha_spirit_link();
    new spell_sha_healing_rain();
    new spell_sha_bloodlust();
    new spell_sha_heroism();
    RegisterSpellScript(spell_sha_chain_heal);
    new spell_sha_astral_recall();
    new spell_sha_elemental_familiars();
    new spell_sha_cloudburst_totem();
    new spell_sha_earth_shock();
    new spell_sha_flame_shock();
    new spell_sha_frost_shock();
    new spell_sha_lightning_bolt();
	RegisterSpellScript(spell_sha_flametongue_attack);
	RegisterSpellScript(spell_sha_ascendance_enhancement);
	RegisterAuraScript(spell_sha_hailstorm);
	RegisterAuraScript(spell_sha_fury_of_air);
	RegisterAuraScript(spell_sha_windfury);
	//new spell_sha_static_overload();
	new spell_sha_elemental_overload();
	new spell_sha_lightning_rod();
    RegisterAuraScript(spell_sha_lava_surge);
    new spell_sha_undulation();
    new spell_sha_earthen_shield();
    new spell_sha_recall_cloudburst_totem();
    new spell_sha_earth_shield();
    new spell_sha_sundering();
    RegisterAuraScript(spell_sha_hot_hand);
    new spell_sha_spirit_link_pvp();
    new spell_sha_purge();
    new spell_sha_grounding_totem();
    new spell_sha_shamanism();
    new spell_sha_stormflurry();
    RegisterSpellScript(spell_sha_stormflurry_damage);
    RegisterAuraScript(spell_sha_stormlash_buff);
    RegisterSpellScript(spell_sha_stormlash_damage);
    new spell_sha_alpha_wolf();
    new spell_sha_crash_lightning();
    new spell_sha_sense_of_urgency();
    new spell_sha_forked_lightning_pvp();
    new spell_monk_gift_of_queen();
    RegisterAuraScript(spell_sha_ancestral_protection);
    RegisterAuraScript(spell_sha_earthen_rage);
    RegisterSpellScript(spell_sha_feral_lunge);
    RegisterCreatureAI(npc_sha_feral_spirit);
    RegisterAuraScript(spell_sha_hex);
    //RegisterAuraScript(spell_sha_lightning_rod);
    RegisterSpellScript(spell_sha_elem_blast);
}
