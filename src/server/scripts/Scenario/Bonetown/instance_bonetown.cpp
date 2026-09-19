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

#include <array>
#include <map>
#include <set>

namespace
{
struct BonetownSpawn
{
    uint32 Entry;
    Position Pos;
};

BonetownSpawn const RequiredActors[] =
{
    { NPC_DUROTAN,              { 5857.80f, 3658.70f, 121.00f, 5.48f } },
    { NPC_THRALL,               { 5613.94f, 3552.10f, 112.58f, 4.74f } },
    { NPC_NARTHOK_SHADOWSIGHT,  { 5818.40f, 3641.80f, 120.90f, 0.10f } },
    { NPC_MORNETH,              { 5834.20f, 3635.10f, 120.90f, 2.80f } },
    { NPC_KELRATH,              { 5827.80f, 3651.20f, 120.90f, 4.70f } },
    { NPC_KILROGG_DEADEYE,      { 5597.40f, 3465.61f, 116.98f, 1.63f } }
};

Position const DefensePositions[] =
{
    { 5601.8f, 3575.0f, 112.9f, 4.70f },
    { 5612.7f, 3572.5f, 112.8f, 4.70f },
    { 5622.2f, 3568.2f, 112.7f, 4.70f },
    { 5595.8f, 3564.2f, 112.9f, 4.70f },
    { 5627.5f, 3558.1f, 112.6f, 4.70f }
};

uint32 const DefenseEntries[] =
{
    NPC_FRENZIED_SPIRIT,
    NPC_BALEFUL_SOULPRIEST,
    NPC_SHADOWMOON_VOID_SHAMAN,
    NPC_SHADOWMOON_DARKCASTER,
    NPC_FRENZIED_SPIRIT
};
}

class instance_bonetown : public InstanceMapScript
{
public:
    instance_bonetown() : InstanceMapScript("instance_bonetown", MAP_BONETOWN) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_bonetown_InstanceMapScript(map);
    }

    struct instance_bonetown_InstanceMapScript : public InstanceScript
    {
        instance_bonetown_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

        std::map<uint32, ObjectGuid> ImportantCreatures;
        std::set<uint32> DefeatedSoulShamans;
        std::set<ObjectGuid> DefenseCreatures;
        uint32 ProximityTimer = 1000;
        uint8 DefenseWave = 0;
        bool IntroPlayed = false;
        bool DefenseWaiting = false;
        bool DefenseActive = false;
        bool DefenseComplete = false;
        bool KilroggActivated = false;

        void Initialize() override
        {
            ImportantCreatures.clear();
            DefeatedSoulShamans.clear();
            DefenseCreatures.clear();
            ProximityTimer = 1000;
            DefenseWave = 0;
            IntroPlayed = false;
            DefenseWaiting = false;
            DefenseActive = false;
            DefenseComplete = false;
            KilroggActivated = false;
        }

        void OnPlayerEnter(Player* /*player*/) override
        {
            EnsureActors();

            if (!IntroPlayed && getScenarionStep() == 0)
            {
                IntroPlayed = true;
                Talk(NPC_DUROTAN, 0);
                AddDelayedEvent(3500, [this]() { Talk(NPC_DUROTAN, 1); });
                AddDelayedEvent(7500, [this]() { Talk(NPC_DUROTAN, 2); });
            }

            if (getScenarionStep() >= 2)
                ActivateKilrogg();
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_DUROTAN:
                case NPC_THRALL:
                    ImportantCreatures[creature->GetEntry()] = creature->GetGUID();
                    creature->setFaction(35);
                    creature->SetReactState(REACT_DEFENSIVE);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 |
                        UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_PACIFIED | UNIT_FLAG_STUNNED);
                    break;
                case NPC_NARTHOK_SHADOWSIGHT:
                case NPC_MORNETH:
                case NPC_KELRATH:
                    ImportantCreatures[creature->GetEntry()] = creature->GetGUID();
                    PrepareEnemy(creature);
                    break;
                case NPC_KILROGG_DEADEYE:
                    ImportantCreatures[creature->GetEntry()] = creature->GetGUID();
                    if (getScenarionStep() < 2)
                    {
                        creature->SetReactState(REACT_PASSIVE);
                        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE |
                            UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                    }
                    else
                        PrepareEnemy(creature);
                    break;
                default:
                    break;
            }
        }

        void OnUnitDeath(Unit* unit) override
        {
            uint32 entry = unit->GetEntry();

            if (entry == NPC_NARTHOK_SHADOWSIGHT || entry == NPC_MORNETH || entry == NPC_KELRATH)
            {
                if (DefeatedSoulShamans.insert(entry).second)
                    DoUpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE, entry, 1, 0, unit);

                if (DefeatedSoulShamans.size() == 3)
                    ArmDefenseStage();
                return;
            }

            if (entry == NPC_KILROGG_DEADEYE)
            {
                DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_DEFEAT_KILROGG, 1, 0, unit);
                return;
            }

            auto itr = DefenseCreatures.find(unit->GetGUID());
            if (itr == DefenseCreatures.end())
                return;

            DefenseCreatures.erase(itr);
            if (DefenseCreatures.empty() && DefenseActive)
                AddDelayedEvent(2500, [this]() { SpawnNextDefenseWave(); });
        }

        void onScenarionNextStep(uint32 newStep) override
        {
            switch (newStep)
            {
                case 1:
                    ArmDefenseStage();
                    break;
                case 2:
                    ActivateKilrogg();
                    break;
                default:
                    break;
            }
        }

        void Update(uint32 diff) override
        {
            if (!DefenseWaiting || DefenseActive)
                return;

            if (ProximityTimer > diff)
            {
                ProximityTimer -= diff;
                return;
            }

            ProximityTimer = 1000;
            Creature* thrall = GetImportantCreature(NPC_THRALL);
            if (!thrall)
                return;

            for (auto const& reference : instance->GetPlayers())
            {
                if (Player* player = reference.getSource())
                    if (player->IsAlive() && player->GetDistance(thrall) <= 65.0f)
                    {
                        StartDefense();
                        break;
                    }
            }
        }

    private:
        void EnsureActors()
        {
            for (BonetownSpawn const& actor : RequiredActors)
                if (!GetImportantCreature(actor.Entry))
                    instance->SummonCreature(actor.Entry, actor.Pos);
        }

        Creature* GetImportantCreature(uint32 entry) const
        {
            auto itr = ImportantCreatures.find(entry);
            if (itr == ImportantCreatures.end())
                return nullptr;

            return instance->GetCreature(itr->second);
        }

        void Talk(uint32 entry, uint8 group)
        {
            if (Creature* creature = GetImportantCreature(entry))
                if (creature->IsInWorld())
                    creature->AI()->Talk(group);
        }

        void PrepareEnemy(Creature* creature)
        {
            creature->setFaction(14);
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 |
                UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE |
                UNIT_FLAG_PACIFIED | UNIT_FLAG_STUNNED);
            creature->SetReactState(REACT_AGGRESSIVE);
        }

        void ArmDefenseStage()
        {
            if (DefenseWaiting || DefenseActive || DefenseComplete)
                return;

            DefenseWaiting = true;
            Talk(NPC_DUROTAN, 3); // Rescue the surviving Frostwolves.
            AddDelayedEvent(3500, [this]() { Talk(NPC_DUROTAN, 4); });
        }

        void StartDefense()
        {
            if (!DefenseWaiting || DefenseActive || DefenseComplete)
                return;

            DefenseWaiting = false;
            DefenseActive = true;
            DefenseWave = 0;

            Talk(NPC_THRALL, 0);
            AddDelayedEvent(3000, [this]() { Talk(NPC_THRALL, 1); });
            AddDelayedEvent(6000, [this]() { Talk(NPC_THRALL, 2); });

            AddDelayedEvent(8000, [this]() { SpawnNextDefenseWave(); });
        }

        void SpawnNextDefenseWave()
        {
            if (!DefenseActive || !DefenseCreatures.empty())
                return;

            if (DefenseWave >= 3)
            {
                CompleteDefense();
                return;
            }

            ++DefenseWave;
            for (uint32 i = 0; i < 5; ++i)
            {
                uint32 entry = DefenseEntries[(i + DefenseWave - 1) % (sizeof(DefenseEntries) / sizeof(uint32))];
                Position position = DefensePositions[i];
                position.m_positionX += float(DefenseWave - 1) * 2.0f;

                if (Creature* summon = instance->SummonCreature(entry, position))
                {
                    summon->setFaction(14);
                    summon->SetLevel(100);
                    summon->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 |
                        UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE |
                        UNIT_FLAG_PACIFIED | UNIT_FLAG_STUNNED);
                    summon->SetReactState(REACT_AGGRESSIVE);
                    DefenseCreatures.insert(summon->GetGUID());
                    summon->AI()->DoZoneInCombat(summon, 80.0f);
                }
            }

            if (DefenseCreatures.empty())
                AddDelayedEvent(1000, [this]() { SpawnNextDefenseWave(); });
        }

        void CompleteDefense()
        {
            if (DefenseComplete)
                return;

            DefenseActive = false;
            DefenseComplete = true;

            Talk(NPC_THRALL, 3);
            AddDelayedEvent(3000, [this]() { Talk(NPC_THRALL, 4); });
            AddDelayedEvent(6000, [this]() { Talk(NPC_THRALL, 5); });
            AddDelayedEvent(9500, [this]() { Talk(NPC_THRALL, 6); });

            DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_MEET_DUROTAN, 1);
            DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CRITERIA_DEFEND_THRALL, 1);
            AddDelayedEvent(4500, [this]() { ActivateKilrogg(); });
        }

        void ActivateKilrogg()
        {
            if (KilroggActivated)
                return;

            Creature* kilrogg = GetImportantCreature(NPC_KILROGG_DEADEYE);
            if (!kilrogg || !kilrogg->IsAlive())
                return;

            KilroggActivated = true;
            PrepareEnemy(kilrogg);
            Talk(NPC_KILROGG_DEADEYE, 0);
            Talk(NPC_DUROTAN, 5);
        }
    };
};

void AddSC_instance_bonetown()
{
    new instance_bonetown();
}
