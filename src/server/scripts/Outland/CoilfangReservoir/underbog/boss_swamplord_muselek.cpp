/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "the_underbog.h"

enum MuselekSpells
{
    SPELL_SHOOT          = 22907,
    SPELL_MULTI_SHOT     = 34974,
    SPELL_KNOCK_AWAY     = 18813,
    SPELL_RAPTOR_STRIKE  = 31566,
    SPELL_DETERRENCE     = 31567,
    SPELL_HUNTERS_MARK   = 31615,
    SPELL_AIMED_SHOT     = 31623,
    SPELL_BEAR_COMMAND   = 34662,
    SPELL_ECHOING_ROAR   = 31429,
    SPELL_MAUL           = 34298,
    SPELL_FRENZY         = 34971,
    SPELL_FERAL_CHARGE   = 39435
};

enum MuselekEvents
{
    EVENT_SHOOT = 1,
    EVENT_MULTI_SHOT,
    EVENT_KNOCK_AWAY,
    EVENT_RAPTOR_STRIKE,
    EVENT_HUNTERS_MARK,
    EVENT_BEAR_COMMAND,
    EVENT_ECHOING_ROAR,
    EVENT_MAUL,
    EVENT_FERAL_CHARGE
};

class boss_swamplord_muselek : public CreatureScript
{
public:
    boss_swamplord_muselek() : CreatureScript("boss_swamplord_muselek") { }

    struct boss_swamplord_muselekAI : public BossAI
    {
        boss_swamplord_muselekAI(Creature* creature) : BossAI(creature, DATA_MUSELEK) { }

        void Reset() override
        {
            _Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_SHOOT, 1000);
            events.ScheduleEvent(EVENT_MULTI_SHOT, urand(20000, 30000));
            events.ScheduleEvent(EVENT_KNOCK_AWAY, urand(35000, 40000));
            events.ScheduleEvent(EVENT_RAPTOR_STRIKE, urand(10000, 20000));
            events.ScheduleEvent(EVENT_HUNTERS_MARK, 15000);
            events.ScheduleEvent(EVENT_BEAR_COMMAND, 10000);
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
                case EVENT_SHOOT:
                    DoCastVictim(SPELL_SHOOT);
                    events.ScheduleEvent(EVENT_SHOOT, urand(2000, 4000));
                    break;
                case EVENT_MULTI_SHOT:
                    DoCastVictim(SPELL_MULTI_SHOT);
                    events.ScheduleEvent(EVENT_MULTI_SHOT, urand(25000, 35000));
                    break;
                case EVENT_KNOCK_AWAY:
                    DoCastVictim(SPELL_KNOCK_AWAY);
                    events.ScheduleEvent(EVENT_KNOCK_AWAY, urand(30000, 40000));
                    break;
                case EVENT_RAPTOR_STRIKE:
                    DoCastVictim(SPELL_RAPTOR_STRIKE);
                    events.ScheduleEvent(EVENT_RAPTOR_STRIKE, urand(10000, 20000));
                    break;
                case EVENT_HUNTERS_MARK:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    {
                        DoCast(target, SPELL_HUNTERS_MARK);
                        DoCast(target, SPELL_AIMED_SHOT);
                    }
                    DoCast(me, SPELL_DETERRENCE);
                    events.ScheduleEvent(EVENT_HUNTERS_MARK, 30000);
                    break;
                case EVENT_BEAR_COMMAND:
                    if (Creature* claw = me->FindNearestCreature(NPC_CLAW, 100.0f, true))
                        me->CastSpell(claw, SPELL_BEAR_COMMAND, true);
                    events.ScheduleEvent(EVENT_BEAR_COMMAND, 25000);
                    break;
                default:
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_swamplord_muselekAI(creature);
    }
};

class npc_underbog_claw : public CreatureScript
{
public:
    npc_underbog_claw() : CreatureScript("npc_underbog_claw") { }

    struct npc_underbog_clawAI : public ScriptedAI
    {
        npc_underbog_clawAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap Events;
        bool Subdued;

        void Reset() override
        {
            Events.Reset();
            Subdued = me->GetEntry() == NPC_WINDCALLER_CLAW;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Events.ScheduleEvent(EVENT_ECHOING_ROAR, urand(20000, 25000));
            Events.ScheduleEvent(EVENT_MAUL, urand(5000, 10000));
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
        {
            if (!Subdued && spell->Id == SPELL_BEAR_COMMAND)
            {
                DoCast(me, SPELL_FRENZY);
                Events.ScheduleEvent(EVENT_FERAL_CHARGE, urand(2000, 5000));
            }
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (Subdued || !me->HealthBelowPctDamaged(20, damage))
                return;

            damage = 0;
            Subdued = true;
            me->UpdateEntry(NPC_WINDCALLER_CLAW);
            me->CombatStop(true);
            me->DeleteThreatList();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }

        void UpdateAI(uint32 diff) override
        {
            if (Subdued || !UpdateVictim())
                return;

            Events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (Events.ExecuteEvent())
            {
                case EVENT_ECHOING_ROAR:
                    DoCast(me, SPELL_ECHOING_ROAR);
                    Events.ScheduleEvent(EVENT_ECHOING_ROAR, urand(10000, 20000));
                    break;
                case EVENT_MAUL:
                    DoCastVictim(SPELL_MAUL);
                    Events.ScheduleEvent(EVENT_MAUL, urand(10000, 15000));
                    break;
                case EVENT_FERAL_CHARGE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(target, SPELL_FERAL_CHARGE);
                    break;
                default:
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_underbog_clawAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->GetEntry() == NPC_WINDCALLER_CLAW)
            player->KilledMonsterCredit(NPC_WINDCALLER_CLAW, creature->GetGUID());
        return true;
    }
};

void AddSC_boss_swamplord_muselek()
{
    new boss_swamplord_muselek();
    new npc_underbog_claw();
}
