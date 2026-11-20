#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "AreaTriggerAI.h"
#include "Log.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SmartAI.h"
#include "SpellScript.h"
#include "TemporarySummon.h"

namespace Feltotem
{
constexpr uint32 Tugar = 117230;
constexpr uint32 TotemMissile = 243224;
constexpr uint32 TotemSummon = 237903;
constexpr uint32 FelSurge = 242496;

struct TugarAI : SmartAI
{
    explicit TugarAI(Creature* creature) : SmartAI(creature) {}

    void Reset() override
    {
        ++_attempt;
        _totemTimer = 0;
        SmartAI::Reset();
    }

    void EnterCombat(Unit* enemy) override
    {
        ++_attempt;
        // Legion DBM observed the first Fel Surge cast near 62s. The original
        // 15-yard missile travels at 2 yards/sec before the totem begins it.
        _totemTimer = 54500;
        SmartAI::EnterCombat(enemy);
    }

    uint64 ActiveAttempt() const
    {
        if (!me->IsAlive() || !me->isInCombat() || me->GetMapId() != 1702 || me->HasUnitState(UNIT_STATE_EVADE))
            return 0;

        Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1266 || scenario->GetCurrentStep() != 1 || scenario->IsCompleted(false))
            return 0;

        Unit* victim = me->getVictim();
        Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!player || !player->IsAlive() || !player->IsInWorld() || player->HasPendingTeleport() ||
            !me->IsInMap(player) || !me->InSamePhase(player))
            return 0;

        return _attempt;
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!_totemTimer || !ActiveAttempt())
            return;
        if (diff < _totemTimer)
        {
            _totemTimer -= diff;
            return;
        }

        _totemTimer = urand(25000, 33000);
        if (me->CastSpell(me, TotemMissile, true) != SPELL_CAST_OK)
            TC_LOG_WARN("scripts", "Feltotem's Fall: Tugar's Fel Surge Totem missile cast failed in instance %u", me->GetInstanceId());
    }

private:
    uint64 _attempt = 0;
    uint32 _totemTimer = 0;
};

uint64 ActiveTugarAttempt(Unit* caster)
{
    Creature* boss = caster ? caster->ToCreature() : nullptr;
    if (!boss || boss->GetEntry() != Tugar || !boss->IsAIEnabled)
        return 0;
    auto* ai = dynamic_cast<TugarAI*>(boss->AI());
    return ai ? ai->ActiveAttempt() : 0;
}
}

class npc_mage_tower_tugar : public CreatureScript
{
public:
    npc_mage_tower_tugar() : CreatureScript("npc_mage_tower_tugar") {}
    CreatureAI* GetAI(Creature* creature) const override { return new Feltotem::TugarAI(creature); }
};

class npc_mage_tower_fel_surge_totem : public CreatureScript
{
public:
    npc_mage_tower_fel_surge_totem() : CreatureScript("npc_mage_tower_fel_surge_totem") {}

    struct TotemAI : ScriptedAI
    {
        explicit TotemAI(Creature* creature) : ScriptedAI(creature) {}

        void InitializeAI() override
        {
            ScriptedAI::InitializeAI();
            if (me->isTotem() && me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == Feltotem::TotemSummon)
                _attempt = Feltotem::ActiveTugarAttempt(me->ToTempSummon()->GetSummoner());
            if (_attempt)
            {
                me->SetCreateHealth(1);
                me->SetMaxHealth(1);
                me->SetHealth(1);
            }
        }

        bool IsActive() const
        {
            return _attempt && me->IsAlive() &&
                _attempt == Feltotem::ActiveTugarAttempt(me->ToTempSummon()->GetSummoner());
        }

        void UpdateAI(uint32 diff) override
        {
            if (!IsActive())
            {
                me->DespawnOrUnsummon();
                return;
            }
            if (_started)
                return;
            if (_retryTimer > diff)
            {
                _retryTimer -= diff;
                return;
            }

            _started = me->CastSpell(me, Feltotem::FelSurge, false) == SPELL_CAST_OK;
            if (!_started && ++_failedCasts == 3)
            {
                TC_LOG_WARN("scripts", "Feltotem's Fall: Fel Surge Totem could not start its cast in instance %u", me->GetInstanceId());
                me->DespawnOrUnsummon();
            }
            _retryTimer = 250;
        }

        void SpellFinishCast(SpellInfo const* spell) override
        {
            if (spell->Id == Feltotem::FelSurge)
                me->DespawnOrUnsummon(500);
        }

    private:
        uint64 _attempt = 0;
        uint32 _retryTimer = 0;
        uint8 _failedCasts = 0;
        bool _started = false;
    };

    CreatureAI* GetAI(Creature* creature) const override { return new TotemAI(creature); }
};

class spell_mage_tower_fel_surge_missile : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_fel_surge_missile);

    bool Load() override
    {
        _attempt = Feltotem::ActiveTugarAttempt(GetCaster());
        return true;
    }

    void Land(SpellEffIndex effect)
    {
        if (!_attempt || _attempt != Feltotem::ActiveTugarAttempt(GetCaster()))
            PreventHitDefaultEffect(effect);
    }

    void Register() override { OnEffectHit += SpellEffectFn(spell_mage_tower_fel_surge_missile::Land, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE); }

    uint64 _attempt = 0;
};

class spell_mage_tower_fel_surge_summon : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_fel_surge_summon);

    void Summon(SpellEffIndex effect)
    {
        if (!Feltotem::ActiveTugarAttempt(GetCaster()))
            PreventHitDefaultEffect(effect);
        else
            SetEffectValue(1);
    }

    void Register() override { OnEffectHit += SpellEffectFn(spell_mage_tower_fel_surge_summon::Summon, EFFECT_0, SPELL_EFFECT_SUMMON); }
};

class spell_mage_tower_fel_surge : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_fel_surge);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Creature* caster = GetCaster()->ToCreature();
        auto* ai = caster && caster->IsAIEnabled ? dynamic_cast<npc_mage_tower_fel_surge_totem::TotemAI*>(caster->AI()) : nullptr;
        if (!ai || !ai->IsActive())
            targets.clear();
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_tower_fel_surge::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_tower_fel_surge::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

class npc_mage_tower_agatha : public CreatureScript
{
public:
    npc_mage_tower_agatha() : CreatureScript("npc_mage_tower_agatha") {}

    struct AgathaAI : SmartAI
    {
        explicit AgathaAI(Creature* creature) : SmartAI(creature) {}

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (!summon || summon->IsAlive() || !me->IsAlive() || !me->isInCombat() || me->GetMapId() != 1616)
                return;

            switch (summon->GetEntry())
            {
                case 115640: // Fuming Imp
                case 115641: // Smoldering Imp
                case 115642: // Umbral Imp
                case 115643: // Imp Servant
                case 115719: // Imp Servant, opposite side
                    break;
                default:
                    return;
            }

            TempSummon* imp = summon->ToTempSummon();
            if (!imp || imp->GetSummonerGUID() != me->GetGUID() || !me->IsInMap(imp))
                return;

            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1221 || scenario->IsCompleted(false))
                return;

            Unit* victim = me->getVictim();
            Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (!player || !player->IsAlive() || !player->IsInWorld() || player->HasPendingTeleport() ||
                !me->IsInMap(player) || !me->InSamePhase(player))
                return;

            me->CastSpell(player->GetPosition(), 242879, true); // Agatha's Fury at the player's feet.
        }
    };

    CreatureAI* GetAI(Creature* creature) const override { return new AgathaAI(creature); }
};

// 117470
class boss_sivash : public CreatureScript
{
public:
    boss_sivash() : CreatureScript("boss_sivash") {}

    struct boss_sivashAI : public ScriptedAI
    {
        boss_sivashAI(Creature* creature) : ScriptedAI(creature), summons(me) {}

        EventMap events;
        SummonList summons;

        void Reset() override
        {
            events.Reset();
            me->RemoveAllAreaObjects();
            summons.DespawnAll();
        }

        void EnterCombat(Unit* unit) override
        {
            events.RescheduleEvent(EVENT_1, 25000); // summons 233968
            events.RescheduleEvent(EVENT_2, 13000); // 241433
            events.RescheduleEvent(EVENT_3, 23000); // 233996 sea with help 233977
        }

        void JustDied(Unit* who) override
        {
            summons.DespawnAll();
        }

        void JustSummoned(Creature* summon) override
        {
            if (summon->GetEntry() != 117487)
                return;

            summons.Summon(summon);
            if (auto target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                summon->AI()->AttackStart(target);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        me->SummonCreature(117487, me->GetPositionX() + irand(-35, 35), me->GetPositionY() + irand(-35, 35), me->GetPositionZ());
                        me->SummonCreature(117487, me->GetPositionX() + irand(-35, 35), me->GetPositionY() + irand(-35, 35), me->GetPositionZ());
                        events.RescheduleEvent(EVENT_1, 25000);
                        break;
                    case EVENT_2:
                        DoCast(241433);
                        events.RescheduleEvent(EVENT_2, 13000);
                        break;
                    case EVENT_3:
                        DoCast(233996);
                        for (uint8 i = 0; i < urand(1, 2); ++i)
                            me->CastSpell(me->GetPositionX() + irand(-35, 35), me->GetPositionY() + irand(-35, 35), me->GetPositionZ(), 233970, true);
                        events.RescheduleEvent(EVENT_3, 23000);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_sivashAI(creature);
    }
};

// 13481
class areatrigger_at_tidal_wave : public AreaTriggerScript
{
    public:
        areatrigger_at_tidal_wave() : AreaTriggerScript("areatrigger_at_tidal_wave") {}

    struct areatrigger_at_tidal_waveAI : AreaTriggerAI
    {
        areatrigger_at_tidal_waveAI(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) {}

        bool CalculateSpline(Position const* pos, Position& startPos, Position& endPos, std::vector<Position>& path) override
        {
            endPos.m_positionX = pos->GetPositionX() + irand(-30, 30);
            endPos.m_positionY = pos->GetPositionY() + 50;
            endPos.m_positionZ = pos->GetPositionZ() - 5;
            if (Unit* caster = at->GetCaster())
            {
                startPos.m_positionX = caster->GetPositionX() + irand(-55, 55);
                startPos.m_positionY = caster->GetPositionY() - 90;
                startPos.m_positionZ = caster->GetPositionZ() - 7;
                return true;
            }
            else
                return false;
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areatrigger) const override
    {
        return new areatrigger_at_tidal_waveAI(areatrigger);
    }
};

// 121124
class boss_apocron : public CreatureScript
{
public:
    boss_apocron() : CreatureScript("boss_apocron") {}

    struct boss_apocronAI : public ScriptedAI
    {
        boss_apocronAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* unit) override
        {
            events.RescheduleEvent(EVENT_1, 23000); // 241458
            events.RescheduleEvent(EVENT_2, 35000); // 241518 (random)
            events.RescheduleEvent(EVENT_3, 11000); // 241498 script (241507)
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (auto victim = me->getVictim())
            {
                if (abs(victim->GetPositionZ() - me->GetPositionZ()) >= 4.0f)
                {
                    EnterEvadeMode();
                    return;
                }
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(241458);
                        events.RescheduleEvent(EVENT_1, 23000); // 241458
                        break;
                    case EVENT_2:
                        DoCast(241518);
                        events.RescheduleEvent(EVENT_2, 24000); // 241518 (random)
                        break;
                    case EVENT_3:
                        DoCast(241498);
                        me->AddDelayedEvent(2100, [actor = me] () -> void
                        {
                            uint8 r = urand(5, 11);
                            for (uint8 i = 0; i < r; ++i)
                                { if (auto ai = actor->AI()) ai->DoCast(241507); }
                        });

                        me->AddDelayedEvent(3100, [actor = me] () -> void
                        {
                            uint8 r = urand(5, 11);
                            for (uint8 i = 0; i < r; ++i)
                                { if (auto ai = actor->AI()) ai->DoCast(241507); }
                        });

                        me->AddDelayedEvent(4100, [actor = me] () -> void
                        {
                            uint8 r = urand(8, 11);
                            for (uint8 i = 0; i < r; ++i)
                                { if (auto ai = actor->AI()) ai->DoCast(241507); }
                        });

                        events.RescheduleEvent(EVENT_3, 16000); // 241498 script (241507)
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_apocronAI(creature);
    }
};

// 117239
class boss_bs_brutallus : public CreatureScript
{
public:
    boss_bs_brutallus() : CreatureScript("boss_bs_brutallus") {}

    struct boss_bs_brutallusAI : public ScriptedAI
    {
        boss_bs_brutallusAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* unit) override
        {
            events.RescheduleEvent(EVENT_1, 19000); // 233484
            events.RescheduleEvent(EVENT_2, 30000); // 233566
            events.RescheduleEvent(EVENT_3, 20000); // 233515
        }


        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(233484);
                        events.RescheduleEvent(EVENT_1, 19000); // 233484
                        break;
                    case EVENT_2:
                        DoCast(233566);
                        events.RescheduleEvent(EVENT_2, 18000); // 233566
                        break;
                    case EVENT_3:
                        DoCast(233515);
                        events.RescheduleEvent(EVENT_3, 20+18000); // 233515
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_bs_brutallusAI(creature);
    }
};

//233484
class spell_meteor_slash : public SpellScript
{
    PrepareSpellScript(spell_meteor_slash);

    uint8 targetsCount = 0;

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targetsCount = targets.size();
    }

    void HandleDamage(SpellEffIndex /*effectIndex*/)
    {
        if (targetsCount)
            SetHitDamage(GetHitDamage() / targetsCount);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_meteor_slash::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
        OnEffectHitTarget += SpellEffectFn(spell_meteor_slash::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 117303
class boss_malificus : public CreatureScript
{
public:
    boss_malificus() : CreatureScript("boss_malificus") {}

    struct boss_malificusAI : public ScriptedAI
    {
        boss_malificusAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* unit) override
        {
            events.RescheduleEvent(EVENT_1, 17000); // 234452
            events.RescheduleEvent(EVENT_2, 15000); // 233614
            events.RescheduleEvent(EVENT_3, 16000); // 233569
        }


        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(234452);
                        events.RescheduleEvent(EVENT_1, 19000); // 234452
                        break;
                    case EVENT_2:
                        DoCast(233614);
                        events.RescheduleEvent(EVENT_2, 16000); // 233614
                        break;
                    case EVENT_3:
                        DoCast(233569);
                        events.RescheduleEvent(EVENT_3, 18000); // 233569
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_malificusAI(creature);
    }
};

void AddSC_broken_shore()
{
    new npc_mage_tower_tugar();
    new npc_mage_tower_fel_surge_totem();
    RegisterSpellScript(spell_mage_tower_fel_surge_missile);
    RegisterSpellScript(spell_mage_tower_fel_surge_summon);
    RegisterSpellScript(spell_mage_tower_fel_surge);
    new npc_mage_tower_agatha();
    new boss_sivash();
    new areatrigger_at_tidal_wave();
    new boss_apocron();
    new boss_bs_brutallus();
    RegisterSpellScript(spell_meteor_slash);
    new boss_malificus();
}
