/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ragefire_chasm.h"

enum SlagmawSpells
{
    SPELL_LAVA_SPIT       = 119434,
    SPELL_MAGNAW_SUBMERGE = 120384
};

enum SlagmawEvents
{
    EVENT_LAVA_SPIT       = 1,
    EVENT_TELEPORT,
    EVENT_EMERGE,
    EVENT_BOUNDARY_CHECK,
};

Position const SlagmawTeleportPositions[4] =
{
    { -222.940f, 165.703f, -19.721f, 3.79782f },
    { -226.477f, 135.704f, -19.721f, 2.33029f },
    { -263.212f, 136.244f, -19.721f, 0.755677f },
    { -256.389f, 172.884f, -19.721f, 5.57793f }
};

// 61463 - Slagmaw
struct boss_slagmaw : public BossAI
{
    boss_slagmaw(Creature* creature) : BossAI(creature, BOSS_SLAGMAW), _lavaSpitCounter(0), _lastTeleportIndex(3) { }

    void Reset() override
    {
        _Reset();
        _lavaSpitCounter = 0;
        _lastTeleportIndex = 3;
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
    }

    void EnterEvadeMode() override
    {
        BossAI::EnterEvadeMode();
        _DespawnAtEvade();
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
    }

    void EnterCombat(Unit* who) override
    {
        BossAI::EnterCombat(who);

        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);

        events.ScheduleEvent(EVENT_LAVA_SPIT, 1s);
        events.ScheduleEvent(EVENT_BOUNDARY_CHECK, 2500ms);
    }

    void HandleSubmergePhase()
    {
        DoCastSelf(SPELL_MAGNAW_SUBMERGE);
        _lavaSpitCounter = 0;

        events.ScheduleEvent(EVENT_TELEPORT, 3s);
    }

    Position const& GetNextTeleportPosition()
    {
        uint8 teleportIndex;
        do
            teleportIndex = urand(0, 3);
        while (teleportIndex == _lastTeleportIndex);

        _lastTeleportIndex = teleportIndex;
        return SlagmawTeleportPositions[_lastTeleportIndex];
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
            case EVENT_LAVA_SPIT:
            {
                if (_lavaSpitCounter < 5)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    {
                        DoCast(target, SPELL_LAVA_SPIT);
                        _lavaSpitCounter++;
                    }
                    events.Repeat(1s);
                    break;
                }
                else if (_lavaSpitCounter == 5)
                {
                    HandleSubmergePhase();
                    break;
                }
                break;
            }
            case EVENT_TELEPORT:
            {
                me->NearTeleportTo(GetNextTeleportPosition());
                events.ScheduleEvent(EVENT_EMERGE, 1s);
                break;
            }
            case EVENT_EMERGE:
            {
                me->RemoveAurasDueToSpell(SPELL_MAGNAW_SUBMERGE);
                events.ScheduleEvent(EVENT_LAVA_SPIT, 1s);
                break;
            }
            case EVENT_BOUNDARY_CHECK:
            {
                if (me->getVictim()->GetDistance(me) > 50.0f)
                    EnterEvadeMode();
                events.ScheduleEvent(EVENT_BOUNDARY_CHECK, 2500ms);
                break;
            }
            default:
                break;
        }

        DoMeleeAttackIfReady();
    }

private:
    uint8 _lavaSpitCounter;
    uint8 _lastTeleportIndex;
};

void AddSC_boss_slagmaw()
{
    RegisterRagefireChasmCreatureAI(boss_slagmaw);
}
