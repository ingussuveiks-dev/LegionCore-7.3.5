/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "the_stockade.h"

enum OverheatSpells
{
    SPELL_FIREBALL     = 12466,
    SPELL_OVERHEAT     = 86633,
    SPELL_RAIN_OF_FIRE = 86636
};

enum OverheatEvents
{
    EVENT_FIREBALL = 1,
    EVENT_OVERHEAT,
    EVENT_RAIN_OF_FIRE
};

enum OverheatTexts
{
    SAY_OVERHEAT_PULL  = 0,
    SAY_OVERHEAT_DEATH = 1
};

class boss_lord_overheat : public CreatureScript
{
public:
    boss_lord_overheat() : CreatureScript("boss_lord_overheat") { }

    struct boss_lord_overheatAI : public BossAI
    {
        boss_lord_overheatAI(Creature* creature) : BossAI(creature, DATA_LORD_OVERHEAT) { }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            Talk(SAY_OVERHEAT_PULL);
            events.RescheduleEvent(EVENT_FIREBALL, 2000);
            events.RescheduleEvent(EVENT_OVERHEAT, urand(9000, 11000));
            events.RescheduleEvent(EVENT_RAIN_OF_FIRE, urand(10000, 13000));
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_OVERHEAT_DEATH);
            _JustDied();
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
                    case EVENT_FIREBALL:
                        DoCastVictim(SPELL_FIREBALL);
                        events.RescheduleEvent(EVENT_FIREBALL, 2000);
                        break;
                    case EVENT_OVERHEAT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            DoCast(target, SPELL_OVERHEAT);
                        events.RescheduleEvent(EVENT_OVERHEAT, urand(9000, 10000));
                        break;
                    case EVENT_RAIN_OF_FIRE:
                        DoCastAOE(SPELL_RAIN_OF_FIRE);
                        events.RescheduleEvent(EVENT_RAIN_OF_FIRE, urand(15000, 20000));
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetTheStockadeAI<boss_lord_overheatAI>(creature);
    }
};

void AddSC_boss_lord_overheat()
{
    new boss_lord_overheat();
}
