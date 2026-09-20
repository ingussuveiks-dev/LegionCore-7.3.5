/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "the_underbog.h"

enum GhazanSpells
{
    SPELL_ACID_BREATH = 34268,
    SPELL_ACID_SPIT   = 34290,
    SPELL_TAIL_SWEEP  = 34267,
    SPELL_ENRAGE      = 15716
};

class boss_ghazan : public CreatureScript
{
public:
    boss_ghazan() : CreatureScript("boss_ghazan") { }

    struct boss_ghazanAI : public BossAI
    {
        boss_ghazanAI(Creature* creature) : BossAI(creature, DATA_GHAZAN) { }

        uint32 AcidBreathTimer;
        uint32 AcidSpitTimer;
        uint32 TailSweepTimer;
        bool Enraged;

        void Reset() override
        {
            _Reset();
            AcidBreathTimer = 3000;
            AcidSpitTimer = 1000;
            TailSweepTimer = IsHeroic() ? 5900 : 10000;
            Enraged = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (!Enraged && HealthBelowPct(20))
            {
                DoCast(me, SPELL_ENRAGE);
                Enraged = true;
            }

            if (AcidBreathTimer <= diff)
            {
                DoCastVictim(SPELL_ACID_BREATH);
                AcidBreathTimer = urand(7000, 9000);
            }
            else
                AcidBreathTimer -= diff;

            if (AcidSpitTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_ACID_SPIT);
                AcidSpitTimer = urand(7000, 9000);
            }
            else
                AcidSpitTimer -= diff;

            if (TailSweepTimer <= diff)
            {
                DoCastVictim(SPELL_TAIL_SWEEP);
                TailSweepTimer = urand(7000, 9000);
            }
            else
                TailSweepTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_ghazanAI(creature);
    }
};

void AddSC_boss_ghazan()
{
    new boss_ghazan();
}
