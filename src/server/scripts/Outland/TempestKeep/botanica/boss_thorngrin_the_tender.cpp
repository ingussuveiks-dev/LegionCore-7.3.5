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

enum ThorngrinTexts
{
    SAY_AGGRO          = 0,
    SAY_HELLFIRE       = 1,
    SAY_SACRIFICE      = 2,
    EMOTE_ENRAGE       = 3,
    SAY_50_PERCENT_HP  = 4,
    SAY_20_PERCENT_HP  = 5,
    SAY_KILL           = 6,
    SAY_DEATH          = 7
};

enum ThorngrinSpells
{
    SPELL_SACRIFICE       = 34661,
    SPELL_HELLFIRE_NORMAL = 34659,
    SPELL_HELLFIRE_HEROIC = 39131,
    SPELL_ENRAGE          = 34670
};

enum ThorngrinEvents
{
    EVENT_SACRIFICE = 1,
    EVENT_HELLFIRE,
    EVENT_ENRAGE
};

class boss_thorngrin_the_tender : public CreatureScript
{
public:
    boss_thorngrin_the_tender() : CreatureScript("boss_thorngrin_the_tender") { }

    struct boss_thorngrin_the_tenderAI : public BossAI
    {
        boss_thorngrin_the_tenderAI(Creature* creature) : BossAI(creature, DATA_THORNGRIN_THE_TENDER) { }

        bool SaidHalfHealth;
        bool SaidLowHealth;

        void Reset() override
        {
            _Reset();
            SaidHalfHealth = false;
            SaidLowHealth = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_SACRIFICE, urand(6000, 10000));
            events.ScheduleEvent(EVENT_HELLFIRE, urand(12000, 18000));
            events.ScheduleEvent(EVENT_ENRAGE, urand(21000, 25000));
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (!SaidHalfHealth && me->HealthBelowPctDamaged(50, damage))
            {
                SaidHalfHealth = true;
                Talk(SAY_50_PERCENT_HP);
            }

            if (!SaidLowHealth && me->HealthBelowPctDamaged(20, damage))
            {
                SaidLowHealth = true;
                Talk(SAY_20_PERCENT_HP);
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

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_SACRIFICE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                    {
                        Talk(SAY_SACRIFICE);
                        DoCast(target, SPELL_SACRIFICE);
                    }
                    events.ScheduleEvent(EVENT_SACRIFICE, urand(24000, 30000));
                    break;
                case EVENT_HELLFIRE:
                    Talk(SAY_HELLFIRE);
                    DoCast(me, DUNGEON_MODE(SPELL_HELLFIRE_NORMAL, SPELL_HELLFIRE_HEROIC));
                    events.ScheduleEvent(EVENT_HELLFIRE, urand(18000, 24000));
                    break;
                case EVENT_ENRAGE:
                    Talk(EMOTE_ENRAGE);
                    DoCast(me, SPELL_ENRAGE);
                    events.ScheduleEvent(EVENT_ENRAGE, urand(28000, 33000));
                    break;
                default:
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_thorngrin_the_tenderAI(creature);
    }
};

void AddSC_boss_thorngrin_the_tender()
{
    new boss_thorngrin_the_tender();
}
