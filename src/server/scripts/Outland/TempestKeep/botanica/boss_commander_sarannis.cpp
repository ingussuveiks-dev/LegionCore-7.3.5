/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "the_botanica.h"

enum SarannisTexts
{
    SAY_AGGRO              = 0,
    SAY_ARCANE_RESONANCE   = 1,
    SAY_ARCANE_DEVASTATION = 2,
    SAY_SUMMON             = 3,
    SAY_KILL               = 4,
    SAY_DEATH              = 5
};

enum SarannisSpells
{
    SPELL_ARCANE_RESONANCE  = 34794,
    SPELL_ARCANE_DEVASTATION = 34799
};

enum SarannisEvents
{
    EVENT_ARCANE_RESONANCE = 1,
    EVENT_ARCANE_DEVASTATION,
    EVENT_SUMMON_REINFORCEMENTS
};

enum SarannisCreatures
{
    NPC_SUMMONED_BLOODWARDER_RESERVIST = 20078,
    NPC_SUMMONED_BLOODWARDER_MENDER    = 20083
};

Position const ReinforcementPositions[4] =
{
    { 160.4483f, 287.6435f, -3.887904f, 2.3841f },
    { 153.4406f, 289.9929f, -4.736916f, 2.3841f },
    { 154.4137f, 292.8956f, -4.683603f, 2.3841f },
    { 157.1544f, 294.2599f, -4.726504f, 2.3841f }
};

class boss_commander_sarannis : public CreatureScript
{
public:
    boss_commander_sarannis() : CreatureScript("boss_commander_sarannis") { }

    struct boss_commander_sarannisAI : public BossAI
    {
        boss_commander_sarannisAI(Creature* creature) : BossAI(creature, DATA_COMMANDER_SARANNIS) { }

        bool ReinforcementsSummoned;

        void Reset() override
        {
            _Reset();
            ReinforcementsSummoned = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_ARCANE_RESONANCE, urand(11000, 16000));
            events.ScheduleEvent(EVENT_ARCANE_DEVASTATION, urand(11000, 19000));
            if (IsHeroic())
                events.ScheduleEvent(EVENT_SUMMON_REINFORCEMENTS, 60000);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (!IsHeroic() && !ReinforcementsSummoned && me->HealthBelowPctDamaged(50, damage))
            {
                ReinforcementsSummoned = true;
                events.ScheduleEvent(EVENT_SUMMON_REINFORCEMENTS, 1);
            }
        }

        void KilledUnit(Unit* /*victim*/) override
        {
            Talk(SAY_KILL);
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            Talk(SAY_DEATH);
        }

        void SummonReinforcements()
        {
            ReinforcementsSummoned = true;
            Talk(SAY_SUMMON);
            me->SummonCreature(NPC_SUMMONED_BLOODWARDER_MENDER, ReinforcementPositions[0], TEMPSUMMON_CORPSE_DESPAWN);
            me->SummonCreature(NPC_SUMMONED_BLOODWARDER_RESERVIST, ReinforcementPositions[1], TEMPSUMMON_CORPSE_DESPAWN);
            me->SummonCreature(NPC_SUMMONED_BLOODWARDER_RESERVIST, ReinforcementPositions[2], TEMPSUMMON_CORPSE_DESPAWN);
            if (IsHeroic())
                me->SummonCreature(NPC_SUMMONED_BLOODWARDER_RESERVIST, ReinforcementPositions[3], TEMPSUMMON_CORPSE_DESPAWN);
        }

        void JustSummoned(Creature* summon) override
        {
            BossAI::JustSummoned(summon);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_ARCANE_RESONANCE:
                    Talk(SAY_ARCANE_RESONANCE);
                    DoCastVictim(SPELL_ARCANE_RESONANCE);
                    events.ScheduleEvent(EVENT_ARCANE_RESONANCE, urand(30000, 43000));
                    break;
                case EVENT_ARCANE_DEVASTATION:
                    Talk(SAY_ARCANE_DEVASTATION);
                    DoCastVictim(SPELL_ARCANE_DEVASTATION);
                    events.ScheduleEvent(EVENT_ARCANE_DEVASTATION, urand(11000, 19000));
                    break;
                case EVENT_SUMMON_REINFORCEMENTS:
                    SummonReinforcements();
                    if (IsHeroic())
                        events.ScheduleEvent(EVENT_SUMMON_REINFORCEMENTS, 60000);
                    break;
                default:
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_commander_sarannisAI(creature);
    }
};

void AddSC_boss_commander_sarannis()
{
    new boss_commander_sarannis();
}
