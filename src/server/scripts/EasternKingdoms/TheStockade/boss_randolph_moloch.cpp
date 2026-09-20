/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"
#include "MotionMaster.h"
#include "ScriptedCreature.h"
#include "the_stockade.h"

enum RandolphSpells
{
    SPELL_WILDLY_STABBING = 86726,
    SPELL_SWEEP           = 86729,
    SPELL_VANISH          = 55964,
    SPELL_SHADOWSTEP      = 55966
};

enum RandolphEvents
{
    EVENT_WILDLY_STABBING = 1,
    EVENT_SWEEP,
    EVENT_VANISH,
    EVENT_JUST_VANISHED,
    EVENT_MORTIMER_EMOTE,
    EVENT_MORTIMER_DEATH
};

enum RandolphTexts
{
    SAY_RANDOLPH_PULL   = 0,
    SAY_RANDOLPH_VANISH = 1,
    SAY_RANDOLPH_DEATH  = 2,
    SAY_MORTIMER_DEATH  = 0,
    SAY_MORTIMER_EMOTE  = 1
};

enum RandolphPoints
{
    POINT_MORTIMER_FINISH = 1
};

Position const MortimerMolochSpawnPosition = { 145.5811f, 0.7059f, -25.606f, 6.2f };

class boss_randolph_moloch : public CreatureScript
{
public:
    boss_randolph_moloch() : CreatureScript("boss_randolph_moloch") { }

    struct boss_randolph_molochAI : public BossAI
    {
        boss_randolph_molochAI(Creature* creature) : BossAI(creature, DATA_RANDOLPH_MOLOCH) { }

        void Reset() override
        {
            _Reset();
            _firstVanish = false;
            _secondVanish = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            Talk(SAY_RANDOLPH_PULL);
            events.RescheduleEvent(EVENT_WILDLY_STABBING, urand(4000, 5000));
            events.RescheduleEvent(EVENT_SWEEP, urand(2000, 3000));
        }

        void JustSummoned(Creature* summon) override
        {
            BossAI::JustSummoned(summon);
            if (summon->GetEntry() == NPC_MORTIMER_MOLOCH)
            {
                summon->SetWalk(true);
                summon->GetMotionMaster()->MovePoint(POINT_MORTIMER_FINISH, me->GetPosition());
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_RANDOLPH_DEATH);
            _JustDied();
            me->SummonCreature(NPC_MORTIMER_MOLOCH, MortimerMolochSpawnPosition);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (!_firstVanish && me->HealthBelowPctDamaged(71, damage) && me->HealthAbovePct(59))
            {
                _firstVanish = true;
                events.RescheduleEvent(EVENT_VANISH, 1000);
            }

            if (!_secondVanish && me->HealthBelowPctDamaged(41, damage) && me->HealthAbovePct(29))
            {
                _secondVanish = true;
                events.RescheduleEvent(EVENT_VANISH, 1000);
            }
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
                    case EVENT_WILDLY_STABBING:
                        DoCastVictim(SPELL_WILDLY_STABBING);
                        events.RescheduleEvent(EVENT_WILDLY_STABBING, urand(8000, 12000));
                        break;
                    case EVENT_SWEEP:
                        DoCastVictim(SPELL_SWEEP);
                        events.RescheduleEvent(EVENT_SWEEP, urand(6000, 7000));
                        break;
                    case EVENT_VANISH:
                        Talk(SAY_RANDOLPH_VANISH);
                        me->RemoveAllAuras();
                        DoCastSelf(SPELL_VANISH);
                        me->SetReactState(REACT_PASSIVE);
                        me->SetInCombatState();
                        events.RescheduleEvent(EVENT_JUST_VANISHED, 2000);
                        break;
                    case EVENT_JUST_VANISHED:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            DoCast(target, SPELL_SHADOWSTEP, true);
                        me->SetReactState(REACT_AGGRESSIVE);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        bool _firstVanish = false;
        bool _secondVanish = false;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetTheStockadeAI<boss_randolph_molochAI>(creature);
    }
};

class npc_mortimer_moloch : public CreatureScript
{
public:
    npc_mortimer_moloch() : CreatureScript("npc_mortimer_moloch") { }

    struct npc_mortimer_molochAI : public ScriptedAI
    {
        npc_mortimer_molochAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            me->SetReactState(REACT_PASSIVE);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == POINT_MORTIMER_FINISH)
                _events.RescheduleEvent(EVENT_MORTIMER_EMOTE, 4000);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_MORTIMER_EMOTE:
                        Talk(SAY_MORTIMER_EMOTE);
                        _events.RescheduleEvent(EVENT_MORTIMER_DEATH, 3000);
                        break;
                    case EVENT_MORTIMER_DEATH:
                        Talk(SAY_MORTIMER_DEATH);
                        me->Kill(me);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetTheStockadeAI<npc_mortimer_molochAI>(creature);
    }
};

void AddSC_boss_randolph_moloch()
{
    new boss_randolph_moloch();
    new npc_mortimer_moloch();
}
