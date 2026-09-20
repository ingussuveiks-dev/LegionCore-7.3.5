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
#include "domination_point.h"

#include <map>
#include <set>

namespace
{
struct DominationPointSpawn
{
    uint32 Entry;
    Position Pos;
};

Position const KromtharSpawn = { -1768.01f, 2477.83f, 25.75f, 4.14f };

DominationPointSpawn const SouthAttackers[] =
{
    { NPC_LIONS_FOOTMAN,          { -1984.3f, 2416.4f, 4.63f, 6.19f } },
    { NPC_LIONS_FOOTMAN,          { -1980.5f, 2413.4f, 4.76f, 6.15f } },
    { NPC_GILNEAN_MAULER,         { -1983.6f, 2413.9f, 4.67f, 6.20f } },
    { NPC_LIONS_ARCANIST,         { -1981.2f, 2415.7f, 4.65f, 6.06f } }
};

DominationPointSpawn const EastAttackers[] =
{
    { NPC_ACE_SABOTEUR,           { -1939.2f, 2417.0f, 5.95f, 5.20f } },
    { NPC_ACE_SABOTEUR,           { -1935.0f, 2421.0f, 6.05f, 5.20f } },
    { NPC_ACE_BEAMGUNNER,         { -1938.0f, 2424.0f, 6.10f, 5.20f } },
    { NPC_ACE_MEDIC,              { -1932.0f, 2418.0f, 6.05f, 5.20f } }
};

DominationPointSpawn const WestAttackers[] =
{
    { NPC_STONEBEAK_BERSERKER,    { -2016.6f, 2415.1f, 4.41f, 6.25f } },
    { NPC_STONEBEAK_BERSERKER,    { -2016.3f, 2425.6f, 4.40f, 6.12f } },
    { NPC_IRONFORGE_CABALIST,     { -2012.0f, 2419.0f, 4.40f, 6.20f } },
    { NPC_LIONS_VINDICATOR,       { -2008.0f, 2422.0f, 4.40f, 6.20f } }
};

DominationPointSpawn const AssaultCommander[] =
{
    { NPC_CORDFELLOW_HACK,        { -1947.0f, 2390.0f, 6.65f, 1.50f } },
    { NPC_LIONS_FOOTMAN,          { -1943.0f, 2387.0f, 6.80f, 1.50f } },
    { NPC_LIONS_VINDICATOR,       { -1951.0f, 2387.0f, 6.80f, 1.50f } }
};

DominationPointSpawn const WaveOne[] =
{
    { NPC_LIONS_FOOTMAN,          { -2021.0f, 2414.0f, 4.42f, 0.00f } },
    { NPC_LIONS_FOOTMAN,          { -2021.0f, 2426.0f, 4.48f, 0.00f } },
    { NPC_STONEBEAK_BERSERKER,    { -2026.0f, 2417.0f, 4.45f, 0.00f } },
    { NPC_IRONFORGE_CABALIST,     { -2026.0f, 2423.0f, 4.45f, 0.00f } }
};

DominationPointSpawn const WaveTwo[] =
{
    { NPC_ACE_SABOTEUR,           { -2021.0f, 2414.0f, 4.42f, 0.00f } },
    { NPC_ACE_SABOTEUR,           { -2021.0f, 2426.0f, 4.48f, 0.00f } },
    { NPC_ACE_BEAMGUNNER,         { -2026.0f, 2417.0f, 4.45f, 0.00f } },
    { NPC_ACE_MEDIC,              { -2026.0f, 2423.0f, 4.45f, 0.00f } }
};

DominationPointSpawn const WaveThree[] =
{
    { NPC_KNIGHT_OF_THE_LION,     { -2021.0f, 2414.0f, 4.42f, 0.00f } },
    { NPC_KNIGHT_OF_THE_LION,     { -2021.0f, 2426.0f, 4.48f, 0.00f } },
    { NPC_LIONS_ARCANIST,         { -2026.0f, 2417.0f, 4.45f, 0.00f } },
    { NPC_LIONS_FAITHFUL,         { -2026.0f, 2423.0f, 4.45f, 0.00f } }
};

uint32 const AllianceCommanders[] =
{
    NPC_AROC_STONEBEAK,
    NPC_JOAN_LORRAINE,
    NPC_PRIESTESS_LARALLA,
    NPC_THAUMATURGE_MOONSPIRE,
    NPC_GNOMEREGAN_GRENADIER
};
}

class instance_domination_point : public InstanceMapScript
{
public:
    instance_domination_point() : InstanceMapScript("instance_domination_point", MAP_DOMINATION_POINT) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_domination_point_InstanceMapScript(map);
    }

    struct instance_domination_point_InstanceMapScript : public InstanceScript
    {
        instance_domination_point_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

        std::map<uint32, ObjectGuid> ImportantCreatures;
        std::map<ObjectGuid, uint32> ObjectiveBySummon;
        std::map<uint32, uint32> RemainingByObjective;
        std::set<uint32> SpawnedObjectives;
        std::set<uint32> CompletedObjectives;

        void Initialize() override
        {
            ImportantCreatures.clear();
            ObjectiveBySummon.clear();
            RemainingByObjective.clear();
            SpawnedObjectives.clear();
            CompletedObjectives.clear();
        }

        void OnPlayerEnter(Player* /*player*/) override
        {
            if (getScenarionStep() != 0 || ImportantCreatures.count(NPC_KROMTHAR))
                return;

            if (Creature* kromthar = instance->SummonCreature(NPC_KROMTHAR, KromtharSpawn))
                ImportantCreatures[NPC_KROMTHAR] = kromthar->GetGUID();
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_KROMTHAR:
                case NPC_GENERAL_NAZGRIM:
                case NPC_WARLORD_BLOODHILT:
                case NPC_SHOKIA:
                case NPC_SHADEMASTER_KIRYN:
                case NPC_RIVETT_CLUTCHPOP:
                    ImportantCreatures[creature->GetEntry()] = creature->GetGUID();
                    creature->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    break;
                case NPC_PLACE_BOMBS:
                case NPC_PLACE_BOOMSTICKS:
                case NPC_PLACE_ROCKETS:
                    ImportantCreatures[creature->GetEntry()] = creature->GetGUID();
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    break;
                default:
                    break;
            }
        }

        void OnUnitDeath(Unit* unit) override
        {
            auto itr = ObjectiveBySummon.find(unit->GetGUID());
            if (itr == ObjectiveBySummon.end())
                return;

            uint32 objective = itr->second;
            ObjectiveBySummon.erase(itr);

            auto remaining = RemainingByObjective.find(objective);
            if (remaining == RemainingByObjective.end() || !remaining->second)
                return;

            if (--remaining->second)
                return;

            CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_3, objective);

            switch (objective)
            {
                case CRITERIA_WAVE_ONE:
                    AddDelayedEvent(2500, [this]()
                    {
                        SpawnObjective(CRITERIA_WAVE_TWO, WaveTwo, sizeof(WaveTwo) / sizeof(DominationPointSpawn));
                    });
                    break;
                case CRITERIA_WAVE_TWO:
                    AddDelayedEvent(2500, [this]()
                    {
                        SpawnObjective(CRITERIA_WAVE_THREE, WaveThree, sizeof(WaveThree) / sizeof(DominationPointSpawn));
                    });
                    break;
                case CRITERIA_WAVE_THREE:
                    AddDelayedEvent(2500, [this]() { SpawnAllianceCommander(); });
                    break;
                default:
                    break;
            }
        }

        void onScenarionNextStep(uint32 newStep) override
        {
            switch (newStep)
            {
                case 2:
                    SpawnObjective(CRITERIA_SOUTH_ATTACKERS, SouthAttackers, sizeof(SouthAttackers) / sizeof(DominationPointSpawn));
                    SpawnObjective(CRITERIA_EAST_ATTACKERS, EastAttackers, sizeof(EastAttackers) / sizeof(DominationPointSpawn));
                    SpawnObjective(CRITERIA_WEST_ATTACKERS, WestAttackers, sizeof(WestAttackers) / sizeof(DominationPointSpawn));
                    break;
                case 6:
                    SpawnObjective(CRITERIA_WAVE_ONE, WaveOne, sizeof(WaveOne) / sizeof(DominationPointSpawn));
                    break;
                default:
                    break;
            }
        }

        void SetData(uint32 type, uint32 /*data*/) override
        {
            switch (type)
            {
                case DATA_JOIN_KROMTHAR:
                    if (getScenarionStep() == 0)
                        CreditCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_JOIN_KROMTHAR);
                    break;
                case DATA_JOIN_NAZGRIM:
                    if (getScenarionStep() == 1)
                        CreditCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_JOIN_NAZGRIM);
                    break;
                case DATA_LOCATE_BLOODHILT:
                    if (getScenarionStep() == 3)
                    {
                        CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_BLOODHILT_LOCATED);
                        SpawnObjective(CRITERIA_ASSAULT_COMMANDER, AssaultCommander, sizeof(AssaultCommander) / sizeof(DominationPointSpawn));
                    }
                    break;
                case DATA_SHOKIA_REPORT:
                    if (getScenarionStep() == 4)
                        CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_SHOKIA_REPORT);
                    break;
                case DATA_KIRYN_REPORT:
                    if (getScenarionStep() == 4)
                        CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_KIRYN_REPORT);
                    break;
                case DATA_RIVETT_REPORT:
                    if (getScenarionStep() == 4)
                        CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_RIVETT_REPORT);
                    break;
                case DATA_PLACE_ROCKETS:
                    if (getScenarionStep() == 5)
                        CreditCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_PLACE_ROCKETS);
                    break;
                case DATA_PLACE_BOMBS:
                    if (getScenarionStep() == 5)
                        CreditCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_PLACE_BOMBS);
                    break;
                case DATA_PLACE_BOOMSTICKS:
                    if (getScenarionStep() == 5)
                        CreditCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_PLACE_BOOMSTICKS);
                    break;
                default:
                    break;
            }
        }

    private:
        void CreditCriteria(CriteriaTypes type, uint32 objective)
        {
            if (!CompletedObjectives.insert(objective).second)
                return;

            DoUpdateAchievementCriteria(type, objective, 1);
        }

        void SpawnObjective(uint32 objective, DominationPointSpawn const* spawns, uint32 count)
        {
            if (!SpawnedObjectives.insert(objective).second || CompletedObjectives.count(objective))
                return;

            for (uint32 i = 0; i < count; ++i)
            {
                Creature* summon = instance->SummonCreature(spawns[i].Entry, spawns[i].Pos);
                if (!summon)
                    continue;

                summon->setFaction(84);
                summon->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 |
                    UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE |
                    UNIT_FLAG_PACIFIED | UNIT_FLAG_STUNNED);
                summon->SetReactState(REACT_AGGRESSIVE);

                ObjectiveBySummon[summon->GetGUID()] = objective;
                ++RemainingByObjective[objective];
                summon->AI()->DoZoneInCombat(summon, 80.0f);
            }

            if (!RemainingByObjective[objective])
                CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_3, objective);
        }

        void SpawnAllianceCommander()
        {
            if (!SpawnedObjectives.insert(CRITERIA_ALLIANCE_COMMANDER).second || CompletedObjectives.count(CRITERIA_ALLIANCE_COMMANDER))
                return;

            DominationPointSpawn commander =
            {
                AllianceCommanders[urand(0, sizeof(AllianceCommanders) / sizeof(uint32) - 1)],
                { -2026.0f, 2420.0f, 4.45f, 0.00f }
            };

            SpawnedObjectives.erase(CRITERIA_ALLIANCE_COMMANDER);
            SpawnObjective(CRITERIA_ALLIANCE_COMMANDER, &commander, 1);
        }
    };
};

void AddSC_instance_domination_point()
{
    new instance_domination_point();
}
