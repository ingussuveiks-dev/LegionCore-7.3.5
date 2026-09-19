/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "bonetown.h"

namespace
{
enum BonetownCombatEvents
{
    EVENT_PRIMARY_SPELL = 1,
    EVENT_SECONDARY_SPELL
};
}

class npc_bonetown_combat : public CreatureScript
{
public:
    npc_bonetown_combat() : CreatureScript("npc_bonetown_combat") { }

    struct npc_bonetown_combatAI : public ScriptedAI
    {
        npc_bonetown_combatAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap Events;

        void Reset() override
        {
            Events.Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Events.ScheduleEvent(EVENT_PRIMARY_SPELL, urand(3500, 5500));
            Events.ScheduleEvent(EVENT_SECONDARY_SPELL, urand(8500, 11000));
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            Events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (Events.ExecuteEvent())
            {
                case EVENT_PRIMARY_SPELL:
                    CastPrimarySpell();
                    Events.ScheduleEvent(EVENT_PRIMARY_SPELL, urand(7000, 10000));
                    break;
                case EVENT_SECONDARY_SPELL:
                    CastSecondarySpell();
                    Events.ScheduleEvent(EVENT_SECONDARY_SPELL, urand(12000, 16000));
                    break;
                default:
                    break;
            }

            DoMeleeAttackIfReady();
        }

    private:
        void CastPrimarySpell()
        {
            switch (me->GetEntry())
            {
                case NPC_NARTHOK_SHADOWSIGHT:
                    DoCastVictim(SPELL_SOUL_BOMB);
                    break;
                case NPC_MORNETH:
                    DoCast(me, SPELL_SOUL_STORM);
                    break;
                case NPC_KELRATH:
                    DoCastVictim(SPELL_SCYTHE_SWIPE);
                    break;
                case NPC_KILROGG_DEADEYE:
                    DoCastVictim(SPELL_REND_SOUL);
                    break;
                case NPC_BALEFUL_SOULPRIEST:
                case NPC_FALLEN_SOULPRIEST:
                    DoCastVictim(SPELL_SOULLESS);
                    break;
                case NPC_SHADOWMOON_VOID_SHAMAN:
                case NPC_SHADOWMOON_DARKCASTER:
                    DoCastVictim(SPELL_CORRUPTED_SOUL);
                    break;
                default:
                    break;
            }
        }

        void CastSecondarySpell()
        {
            switch (me->GetEntry())
            {
                case NPC_NARTHOK_SHADOWSIGHT:
                    DoCastVictim(SPELL_GRIP_OF_DEATH);
                    break;
                case NPC_MORNETH:
                    DoCastVictim(SPELL_SOUL_BOMB);
                    break;
                case NPC_KELRATH:
                    DoCast(me, SPELL_NETHER_SHIELD);
                    break;
                case NPC_KILROGG_DEADEYE:
                    DoCast(me, SPELL_SOUL_TEMPEST);
                    break;
                case NPC_SHADOWMOON_VOID_SHAMAN:
                case NPC_SHADOWMOON_DARKCASTER:
                    DoCastVictim(SPELL_SOULNADO);
                    break;
                default:
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bonetown_combatAI(creature);
    }
};

void AddSC_bonetown()
{
    new npc_bonetown_combat();
}
