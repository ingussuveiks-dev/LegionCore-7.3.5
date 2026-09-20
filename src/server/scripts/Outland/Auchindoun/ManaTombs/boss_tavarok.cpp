/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "mana_tombs.h"

enum TavarokSpells
{
    SPELL_EARTHQUAKE     = 33919,
    SPELL_CRYSTAL_PRISON = 32361,
    SPELL_ARCING_SMASH   = 8374
};

enum TavarokEvents
{
    EVENT_EARTHQUAKE = 1,
    EVENT_CRYSTAL_PRISON,
    EVENT_ARCING_SMASH
};

class boss_tavarok : public CreatureScript
{
public:
    boss_tavarok() : CreatureScript("boss_tavarok") { }

    struct boss_tavarokAI : public BossAI
    {
        boss_tavarokAI(Creature* creature) : BossAI(creature, DATA_TAVAROK) { }

        void Reset() override
        {
            _Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_EARTHQUAKE, urand(10000, 14200));
            events.ScheduleEvent(EVENT_CRYSTAL_PRISON, urand(12000, 22000));
            events.ScheduleEvent(EVENT_ARCING_SMASH, 5900);
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
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
                case EVENT_EARTHQUAKE:
                    DoCast(me, SPELL_EARTHQUAKE);
                    events.ScheduleEvent(EVENT_EARTHQUAKE, urand(20000, 31000));
                    break;
                case EVENT_CRYSTAL_PRISON:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(target, SPELL_CRYSTAL_PRISON);
                    events.ScheduleEvent(EVENT_CRYSTAL_PRISON, urand(15000, 22000));
                    break;
                case EVENT_ARCING_SMASH:
                    DoCastVictim(SPELL_ARCING_SMASH);
                    events.ScheduleEvent(EVENT_ARCING_SMASH, urand(8000, 12000));
                    break;
                default:
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_tavarokAI(creature);
    }
};

void AddSC_boss_tavarok()
{
    new boss_tavarok();
}
