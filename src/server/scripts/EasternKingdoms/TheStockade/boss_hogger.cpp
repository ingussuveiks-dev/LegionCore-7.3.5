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

enum HoggerTexts
{
    SAY_HOGGER_PULL  = 0,
    SAY_HOGGER_ENRAGE = 1,
    SAY_HOGGER_DEATH = 2,
    SAY_WARDEN_1     = 0,
    SAY_WARDEN_2     = 1,
    SAY_WARDEN_3     = 2
};

enum HoggerSpells
{
    SPELL_VICIOUS_SLICE  = 86604,
    SPELL_MADDENING_CALL = 86620,
    SPELL_HOGGER_ENRAGE  = 86736
};

enum HoggerEvents
{
    EVENT_VICIOUS_SLICE = 1,
    EVENT_MADDENING_CALL,
    EVENT_WARDEN_1,
    EVENT_WARDEN_2,
    EVENT_WARDEN_3
};

enum HoggerPoints
{
    POINT_WARDEN_FINISH = 1
};

Position const WardenThelwaterSpawnPosition = { 138.369f, 78.2932f, -33.85627f, 1.082104f };
Position const WardenThelwaterFinishPosition = { 152.019f, 106.198f, -35.1896f, 1.082104f };

class boss_hogger : public CreatureScript
{
public:
    boss_hogger() : CreatureScript("boss_hogger") { }

    struct boss_hoggerAI : public BossAI
    {
        boss_hoggerAI(Creature* creature) : BossAI(creature, DATA_HOGGER) { }

        void Reset() override
        {
            _Reset();
            _hasEnraged = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            Talk(SAY_HOGGER_PULL);
            events.RescheduleEvent(EVENT_VICIOUS_SLICE, urand(3000, 4000));
            events.RescheduleEvent(EVENT_MADDENING_CALL, urand(1000, 2000));
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_HOGGER_DEATH);
            _JustDied();
            me->SummonCreature(NPC_WARDEN_THELWATER, WardenThelwaterSpawnPosition);
        }

        void JustSummoned(Creature* summon) override
        {
            BossAI::JustSummoned(summon);
            if (summon->GetEntry() == NPC_WARDEN_THELWATER)
                summon->GetMotionMaster()->MovePoint(POINT_WARDEN_FINISH, WardenThelwaterFinishPosition);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (!_hasEnraged && me->HealthBelowPctDamaged(30, damage))
            {
                _hasEnraged = true;
                Talk(SAY_HOGGER_ENRAGE);
                DoCastSelf(SPELL_HOGGER_ENRAGE);
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
                    case EVENT_VICIOUS_SLICE:
                        DoCastVictim(SPELL_VICIOUS_SLICE);
                        events.RescheduleEvent(EVENT_VICIOUS_SLICE, urand(10000, 14000));
                        break;
                    case EVENT_MADDENING_CALL:
                        DoCastAOE(SPELL_MADDENING_CALL);
                        events.RescheduleEvent(EVENT_MADDENING_CALL, urand(15000, 20000));
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        bool _hasEnraged = false;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetTheStockadeAI<boss_hoggerAI>(creature);
    }
};

class npc_warden_thelwater : public CreatureScript
{
public:
    npc_warden_thelwater() : CreatureScript("npc_warden_thelwater") { }

    struct npc_warden_thelwaterAI : public ScriptedAI
    {
        npc_warden_thelwaterAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == POINT_WARDEN_FINISH)
                _events.RescheduleEvent(EVENT_WARDEN_1, 1000);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_WARDEN_1:
                        Talk(SAY_WARDEN_1);
                        _events.RescheduleEvent(EVENT_WARDEN_2, 4000);
                        break;
                    case EVENT_WARDEN_2:
                        Talk(SAY_WARDEN_2);
                        _events.RescheduleEvent(EVENT_WARDEN_3, 3000);
                        break;
                    case EVENT_WARDEN_3:
                        Talk(SAY_WARDEN_3);
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
        return GetTheStockadeAI<npc_warden_thelwaterAI>(creature);
    }
};

void AddSC_boss_hogger()
{
    new boss_hogger();
    new npc_warden_thelwater();
}
