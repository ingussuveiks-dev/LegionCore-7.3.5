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
#include "lions_landing.h"

#include <map>
#include <set>

namespace
{
struct LionsLandingSpawn
{
    uint32 Entry;
    Position Pos;
};

Position const DagginSpawn = { -1175.74f, -1188.17f, 37.20f, 0.76f };

LionsLandingSpawn const SouthAttackers[] =
{
    { NPC_BLOODHILT_GRUNT,        { -1034.0f, -1088.0f, 11.45f, 3.15f } },
    { NPC_BLOODHILT_GRUNT,        { -1037.5f, -1092.0f, 11.45f, 3.15f } },
    { NPC_UDUJI_BERSERKER,        { -1031.0f, -1093.0f, 11.45f, 3.15f } },
    { NPC_BLOODHILT_EARTH_SINGER, { -1038.0f, -1085.0f, 11.45f, 3.15f } }
};

LionsLandingSpawn const EastAttackers[] =
{
    { NPC_SHARK_SAPPER,           { -968.0f, -1085.0f, 13.10f, 2.70f } },
    { NPC_SHARK_SAPPER,           { -965.0f, -1081.0f, 13.10f, 2.70f } },
    { NPC_SHARK_ROCKETEER,        { -971.0f, -1080.0f, 13.10f, 2.70f } },
    { NPC_SHARK_MEDIC,            { -963.0f, -1088.0f, 13.10f, 2.70f } }
};

LionsLandingSpawn const WestAttackers[] =
{
    { NPC_UDUJI_BERSERKER,        { -1003.0f, -1052.0f, 13.00f, 4.80f } },
    { NPC_UDUJI_BERSERKER,        { -998.0f,  -1050.0f, 13.00f, 4.80f } },
    { NPC_UDUJI_HEXER,            { -1000.0f, -1046.0f, 13.00f, 4.80f } },
    { NPC_BLOODHILT_BATTLEMAGE,   { -1006.0f, -1048.0f, 13.00f, 4.80f } }
};

LionsLandingSpawn const AssaultCommander[] =
{
    { NPC_GREAT_HEXER_UDUJI,      { -934.0f, -1068.0f, 12.70f, 3.10f } },
    { NPC_BLOODHILT_GRUNT,        { -938.0f, -1063.0f, 12.70f, 3.10f } },
    { NPC_BLOODHILT_LIGHTSWORN,   { -938.0f, -1073.0f, 12.70f, 3.10f } }
};

LionsLandingSpawn const WaveOne[] =
{
    { NPC_BLOODHILT_GRUNT,        { -944.0f, -1037.0f, 12.40f, 3.80f } },
    { NPC_BLOODHILT_GRUNT,        { -940.0f, -1041.0f, 12.40f, 3.80f } },
    { NPC_UDUJI_BERSERKER,        { -936.0f, -1037.0f, 12.40f, 3.80f } },
    { NPC_UDUJI_HEXER,            { -932.0f, -1041.0f, 12.40f, 3.80f } }
};

LionsLandingSpawn const WaveTwo[] =
{
    { NPC_SHARK_SAPPER,           { -944.0f, -1037.0f, 12.40f, 3.80f } },
    { NPC_SHARK_SAPPER,           { -940.0f, -1041.0f, 12.40f, 3.80f } },
    { NPC_SHARK_ROCKETEER,        { -936.0f, -1037.0f, 12.40f, 3.80f } },
    { NPC_SHARK_MEDIC,            { -932.0f, -1041.0f, 12.40f, 3.80f } }
};

LionsLandingSpawn const WaveThree[] =
{
    { NPC_DREADGUARD_CAVALRY,     { -944.0f, -1037.0f, 12.40f, 3.80f } },
    { NPC_DREADGUARD_CAVALRY,     { -940.0f, -1041.0f, 12.40f, 3.80f } },
    { NPC_BLOODHILT_BATTLEMAGE,   { -936.0f, -1037.0f, 12.40f, 3.80f } },
    { NPC_BLOODHILT_LIGHTSWORN,   { -932.0f, -1041.0f, 12.40f, 3.80f } }
};

uint32 const HordeCommanders[] =
{
    NPC_WOLF_RIDER_GAJA,
    NPC_BAXEL_BRASSBOMBS,
    NPC_SUNWALKER_CHAGON,
    NPC_DARK_CLERIC_LARESA,
    NPC_THAUMATURGE_SARESSE
};
}

class instance_lions_landing : public InstanceMapScript
{
public:
    instance_lions_landing() : InstanceMapScript("instance_lions_landing", MAP_LIONS_LANDING) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_lions_landing_InstanceMapScript(map);
    }

    struct instance_lions_landing_InstanceMapScript : public InstanceScript
    {
        instance_lions_landing_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

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
            if (getScenarionStep() != 0 || ImportantCreatures.count(NPC_DAGGIN_WINDBEARD))
                return;

            if (Creature* daggin = instance->SummonCreature(NPC_DAGGIN_WINDBEARD, DagginSpawn))
                ImportantCreatures[NPC_DAGGIN_WINDBEARD] = daggin->GetGUID();
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_DAGGIN_WINDBEARD:
                case NPC_ADMIRAL_TAYLOR:
                case NPC_HIGH_MARSHAL_TWINBRAID:
                case NPC_MISHKA:
                case NPC_AMBER_KEARNEN:
                case NPC_SULLY_MCLEARY:
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
                        SpawnObjective(CRITERIA_WAVE_TWO, WaveTwo, sizeof(WaveTwo) / sizeof(LionsLandingSpawn));
                    });
                    break;
                case CRITERIA_WAVE_TWO:
                    AddDelayedEvent(2500, [this]()
                    {
                        SpawnObjective(CRITERIA_WAVE_THREE, WaveThree, sizeof(WaveThree) / sizeof(LionsLandingSpawn));
                    });
                    break;
                case CRITERIA_WAVE_THREE:
                    AddDelayedEvent(2500, [this]() { SpawnHordeCommander(); });
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
                    SpawnObjective(CRITERIA_SOUTH_ATTACKERS, SouthAttackers, sizeof(SouthAttackers) / sizeof(LionsLandingSpawn));
                    SpawnObjective(CRITERIA_EAST_ATTACKERS, EastAttackers, sizeof(EastAttackers) / sizeof(LionsLandingSpawn));
                    SpawnObjective(CRITERIA_WEST_ATTACKERS, WestAttackers, sizeof(WestAttackers) / sizeof(LionsLandingSpawn));
                    break;
                case 6:
                    SpawnObjective(CRITERIA_WAVE_ONE, WaveOne, sizeof(WaveOne) / sizeof(LionsLandingSpawn));
                    break;
                default:
                    break;
            }
        }

        void SetData(uint32 type, uint32 /*data*/) override
        {
            switch (type)
            {
                case DATA_JOIN_DAGGIN:
                    if (getScenarionStep() == 0)
                        CreditCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_JOIN_DAGGIN);
                    break;
                case DATA_JOIN_TAYLOR:
                    if (getScenarionStep() == 1)
                        CreditCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_JOIN_TAYLOR);
                    break;
                case DATA_LOCATE_TWINBRAID:
                    if (getScenarionStep() == 3)
                    {
                        CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_TWINBRAID_LOCATED);
                        SpawnObjective(CRITERIA_ASSAULT_COMMANDER, AssaultCommander, sizeof(AssaultCommander) / sizeof(LionsLandingSpawn));
                    }
                    break;
                case DATA_AMBER_REPORT:
                    if (getScenarionStep() == 4)
                        CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_AMBER_REPORT);
                    break;
                case DATA_MISHKA_REPORT:
                    if (getScenarionStep() == 4)
                        CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_MISHKA_REPORT);
                    break;
                case DATA_SULLY_REPORT:
                    if (getScenarionStep() == 4)
                        CreditCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_SULLY_REPORT);
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

        void SpawnObjective(uint32 objective, LionsLandingSpawn const* spawns, uint32 count)
        {
            if (!SpawnedObjectives.insert(objective).second || CompletedObjectives.count(objective))
                return;

            for (uint32 i = 0; i < count; ++i)
            {
                Creature* summon = instance->SummonCreature(spawns[i].Entry, spawns[i].Pos);
                if (!summon)
                    continue;

                summon->setFaction(14);
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

        void SpawnHordeCommander()
        {
            if (!SpawnedObjectives.insert(CRITERIA_HORDE_COMMANDER).second || CompletedObjectives.count(CRITERIA_HORDE_COMMANDER))
                return;

            LionsLandingSpawn commander =
            {
                HordeCommanders[urand(0, sizeof(HordeCommanders) / sizeof(uint32) - 1)],
                { -936.0f, -1039.0f, 12.40f, 3.80f }
            };

            // Use the same tracked group path so killing the randomly selected commander
            // completes the final client criterion and lets Scenario::Reward finish LFG.
            SpawnedObjectives.erase(CRITERIA_HORDE_COMMANDER);
            SpawnObjective(CRITERIA_HORDE_COMMANDER, &commander, 1);
        }
    };
};

void AddSC_instance_lions_landing()
{
    new instance_lions_landing();
}
