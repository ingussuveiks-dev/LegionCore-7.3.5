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

enum StockadeTrashEntries
{
    NPC_RIVERPAW_BASHER         = 46248,
    NPC_RIVERPAW_SLAYER         = 46249,
    NPC_RIVERPAW_POACHER        = 46250,
    NPC_RIVERPAW_LOOTER         = 46251,
    NPC_RIVERPAW_SHAMAN         = 46252,
    NPC_SEARING_DESTROYER       = 46260,
    NPC_ENRAGED_FIRE_ELEMENTAL  = 46261,
    NPC_RUMBLING_EARTH          = 46262,
    NPC_SLAG_FURY               = 46263,
    NPC_ROWDY_TROUBLEMAKER      = 46375,
    NPC_VICIOUS_THUG            = 46379,
    NPC_SHIFTY_THIEF            = 46381
};

enum StockadeTrashSpells
{
    SPELL_BASH_HEAD       = 86814,
    SPELL_STRIKE          = 14516,
    SPELL_EXECUTE         = 77720,
    SPELL_SHOOT           = 15547,
    SPELL_NET             = 31290,
    SPELL_SHANK           = 15248,
    SPELL_MAD_DASH        = 86820,
    SPELL_LIGHTNING_BOLT  = 12167,
    SPELL_HEALING_TOUCH   = 23381,
    SPELL_BLAST_WAVE      = 15744,
    SPELL_SCORCH          = 38391,
    SPELL_FIRE_BLAST      = 13341,
    SPELL_ENRAGE          = 56646,
    SPELL_ROCK_SHARDS     = 86771,
    SPELL_MELT_ARMOR      = 86765,
    SPELL_DIRTY_BLOW      = 86740,
    SPELL_HAMSTRING       = 9080,
    SPELL_DEEP_BRUISE     = 86738,
    SPELL_DEADLY_POISON   = 3583,
    SPELL_SINISTER_STRIKE = 14873
};

enum StockadeTrashEvents
{
    EVENT_BASH_HEAD = 1,
    EVENT_STRIKE,
    EVENT_EXECUTE,
    EVENT_SHOOT,
    EVENT_NET,
    EVENT_SHANK,
    EVENT_MAD_DASH,
    EVENT_LIGHTNING_BOLT,
    EVENT_HEALING_TOUCH,
    EVENT_BLAST_WAVE,
    EVENT_SCORCH,
    EVENT_FIRE_BLAST,
    EVENT_ROCK_SHARDS,
    EVENT_MELT_ARMOR,
    EVENT_DIRTY_BLOW,
    EVENT_HAMSTRING,
    EVENT_DEEP_BRUISE,
    EVENT_DEADLY_POISON,
    EVENT_SINISTER_STRIKE
};

class npc_the_stockade_trash : public CreatureScript
{
public:
    npc_the_stockade_trash() : CreatureScript("npc_the_stockade_trash") { }

    struct npc_the_stockade_trashAI : public ScriptedAI
    {
        npc_the_stockade_trashAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _hasEnraged = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            switch (me->GetEntry())
            {
                case NPC_RIVERPAW_BASHER:
                    _events.RescheduleEvent(EVENT_BASH_HEAD, urand(4000, 7000));
                    break;
                case NPC_RIVERPAW_SLAYER:
                    _events.RescheduleEvent(EVENT_STRIKE, urand(3000, 5000));
                    _events.RescheduleEvent(EVENT_EXECUTE, 1000);
                    break;
                case NPC_RIVERPAW_POACHER:
                    _events.RescheduleEvent(EVENT_SHOOT, 1000);
                    _events.RescheduleEvent(EVENT_NET, urand(5000, 8000));
                    break;
                case NPC_RIVERPAW_LOOTER:
                    _events.RescheduleEvent(EVENT_SHANK, urand(3000, 5000));
                    _events.RescheduleEvent(EVENT_MAD_DASH, urand(1000, 2000));
                    break;
                case NPC_RIVERPAW_SHAMAN:
                    _events.RescheduleEvent(EVENT_LIGHTNING_BOLT, 1000);
                    _events.RescheduleEvent(EVENT_HEALING_TOUCH, urand(6000, 9000));
                    break;
                case NPC_SEARING_DESTROYER:
                    _events.RescheduleEvent(EVENT_SCORCH, 1000);
                    _events.RescheduleEvent(EVENT_BLAST_WAVE, urand(5000, 8000));
                    break;
                case NPC_ENRAGED_FIRE_ELEMENTAL:
                    _events.RescheduleEvent(EVENT_FIRE_BLAST, urand(2000, 4000));
                    break;
                case NPC_RUMBLING_EARTH:
                    _events.RescheduleEvent(EVENT_ROCK_SHARDS, urand(4000, 7000));
                    break;
                case NPC_SLAG_FURY:
                    _events.RescheduleEvent(EVENT_MELT_ARMOR, urand(3000, 5000));
                    break;
                case NPC_ROWDY_TROUBLEMAKER:
                    _events.RescheduleEvent(EVENT_DIRTY_BLOW, urand(5000, 8000));
                    break;
                case NPC_VICIOUS_THUG:
                    _events.RescheduleEvent(EVENT_HAMSTRING, urand(3000, 5000));
                    _events.RescheduleEvent(EVENT_DEEP_BRUISE, urand(6000, 9000));
                    break;
                case NPC_SHIFTY_THIEF:
                    _events.RescheduleEvent(EVENT_DEADLY_POISON, urand(2000, 4000));
                    _events.RescheduleEvent(EVENT_SINISTER_STRIKE, urand(4000, 6000));
                    break;
                default:
                    break;
            }
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (me->GetEntry() == NPC_ENRAGED_FIRE_ELEMENTAL && !_hasEnraged && me->HealthBelowPctDamaged(30, damage))
            {
                _hasEnraged = true;
                DoCastSelf(SPELL_ENRAGE);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BASH_HEAD:
                        DoCastVictim(SPELL_BASH_HEAD);
                        _events.RescheduleEvent(EVENT_BASH_HEAD, urand(12000, 18000));
                        break;
                    case EVENT_STRIKE:
                        DoCastVictim(SPELL_STRIKE);
                        _events.RescheduleEvent(EVENT_STRIKE, urand(6000, 9000));
                        break;
                    case EVENT_EXECUTE:
                        if (me->getVictim() && me->getVictim()->HealthBelowPct(21))
                        {
                            DoCastVictim(SPELL_EXECUTE);
                            _events.RescheduleEvent(EVENT_EXECUTE, urand(8000, 12000));
                        }
                        else
                            _events.RescheduleEvent(EVENT_EXECUTE, 1000);
                        break;
                    case EVENT_SHOOT:
                        DoCastVictim(SPELL_SHOOT);
                        _events.RescheduleEvent(EVENT_SHOOT, urand(2500, 3500));
                        break;
                    case EVENT_NET:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                            DoCast(target, SPELL_NET);
                        _events.RescheduleEvent(EVENT_NET, urand(12000, 18000));
                        break;
                    case EVENT_SHANK:
                        DoCastVictim(SPELL_SHANK);
                        _events.RescheduleEvent(EVENT_SHANK, urand(6000, 9000));
                        break;
                    case EVENT_MAD_DASH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                            DoCast(target, SPELL_MAD_DASH);
                        _events.RescheduleEvent(EVENT_MAD_DASH, urand(12000, 18000));
                        break;
                    case EVENT_LIGHTNING_BOLT:
                        DoCastVictim(SPELL_LIGHTNING_BOLT);
                        _events.RescheduleEvent(EVENT_LIGHTNING_BOLT, urand(3000, 4500));
                        break;
                    case EVENT_HEALING_TOUCH:
                        if (Unit* target = DoSelectLowestHpFriendly(30.0f, 1))
                            DoCast(target, SPELL_HEALING_TOUCH);
                        _events.RescheduleEvent(EVENT_HEALING_TOUCH, urand(8000, 12000));
                        break;
                    case EVENT_BLAST_WAVE:
                        DoCastAOE(SPELL_BLAST_WAVE);
                        _events.RescheduleEvent(EVENT_BLAST_WAVE, urand(10000, 15000));
                        break;
                    case EVENT_SCORCH:
                        DoCastVictim(SPELL_SCORCH);
                        _events.RescheduleEvent(EVENT_SCORCH, urand(3000, 4500));
                        break;
                    case EVENT_FIRE_BLAST:
                        DoCastVictim(SPELL_FIRE_BLAST);
                        _events.RescheduleEvent(EVENT_FIRE_BLAST, urand(6000, 9000));
                        break;
                    case EVENT_ROCK_SHARDS:
                        DoCastAOE(SPELL_ROCK_SHARDS);
                        _events.RescheduleEvent(EVENT_ROCK_SHARDS, urand(8000, 12000));
                        break;
                    case EVENT_MELT_ARMOR:
                        DoCastVictim(SPELL_MELT_ARMOR);
                        _events.RescheduleEvent(EVENT_MELT_ARMOR, urand(8000, 12000));
                        break;
                    case EVENT_DIRTY_BLOW:
                        DoCastVictim(SPELL_DIRTY_BLOW);
                        _events.RescheduleEvent(EVENT_DIRTY_BLOW, urand(10000, 15000));
                        break;
                    case EVENT_HAMSTRING:
                        DoCastVictim(SPELL_HAMSTRING);
                        _events.RescheduleEvent(EVENT_HAMSTRING, urand(7000, 10000));
                        break;
                    case EVENT_DEEP_BRUISE:
                        DoCastVictim(SPELL_DEEP_BRUISE);
                        _events.RescheduleEvent(EVENT_DEEP_BRUISE, urand(10000, 14000));
                        break;
                    case EVENT_DEADLY_POISON:
                        DoCastVictim(SPELL_DEADLY_POISON);
                        _events.RescheduleEvent(EVENT_DEADLY_POISON, urand(10000, 14000));
                        break;
                    case EVENT_SINISTER_STRIKE:
                        DoCastVictim(SPELL_SINISTER_STRIKE);
                        _events.RescheduleEvent(EVENT_SINISTER_STRIKE, urand(6000, 9000));
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool _hasEnraged = false;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetTheStockadeAI<npc_the_stockade_trashAI>(creature);
    }
};

void AddSC_the_stockade()
{
    new npc_the_stockade_trash();
}
