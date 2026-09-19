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
#include "coldridge_cataclysm.h"

#include <array>
#include <map>
#include <set>

namespace
{
struct ColdridgeSpawn
{
    uint32 Entry;
    Position Pos;
};

ColdridgeSpawn const StoryActors[] =
{
    { NPC_JOREN_IRONSTOCK,   { -6228.31f, 331.55f, 383.27f, 2.60f } },
    { NPC_GRELIN_WHITEBEARD, { -6219.40f, 340.10f, 383.30f, 3.10f } },
    { NPC_JONA_IRONSTOCK,    { -6087.60f, 386.03f, 395.54f, 1.81f } },
    { NPC_APPRENTICE_SOREN,  { -6216.20f, 343.40f, 383.20f, 3.40f } }
};

Position const GnomenbrauStoryPosition = { -6334.0f, 354.0f, 379.0f, 3.20f };
Position const TheramoreStoryPosition  = { -6360.0f, 505.0f, 386.0f, 3.80f };
Position const HovelEntrancePosition   = { -6438.0f, 381.0f, 384.0f, 3.80f };
Position const BoarPosition            = { -6351.0f, 493.0f, 385.0f, 3.80f };
Position const BossStoryPosition       = { -6482.0f, 340.0f, 374.0f, 3.80f };
Position const BossPosition            = { -6502.4f, 304.7f, 370.3f, 1.19f };

Position const HovelEnemyPositions[] =
{
    { -6471.4f, 360.1f, 378.1f, 0.7f },
    { -6478.8f, 346.1f, 377.4f, 3.3f },
    { -6488.5f, 345.7f, 375.6f, 4.4f },
    { -6494.7f, 334.3f, 368.6f, 3.8f },
    { -6505.7f, 351.7f, 373.6f, 3.2f },
    { -6519.1f, 347.4f, 372.2f, 5.1f },
    { -6478.2f, 388.0f, 383.7f, 2.8f },
    { -6492.9f, 391.9f, 384.4f, 5.4f },
    { -6507.7f, 385.2f, 385.2f, 3.2f },
    { -6525.7f, 381.9f, 382.6f, 3.5f },
    { -6537.2f, 387.8f, 381.3f, 3.0f },
    { -6511.2f, 419.1f, 386.6f, 1.1f }
};
}

class instance_coldridge_cataclysm : public InstanceMapScript
{
public:
    instance_coldridge_cataclysm() : InstanceMapScript("instance_coldridge_cataclysm", MAP_COLDRIDGE_CATACLYSM) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_coldridge_cataclysm_InstanceMapScript(map);
    }

    struct instance_coldridge_cataclysm_InstanceMapScript : public InstanceScript
    {
        instance_coldridge_cataclysm_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

        std::map<uint32, ObjectGuid> ImportantCreatures;
        std::set<ObjectGuid> ActiveEnemies;
        uint32 ProximityTimer = 1000;
        bool HovelEntered = false;

        void Initialize() override
        {
            ImportantCreatures.clear();
            ActiveEnemies.clear();
            ProximityTimer = 1000;
            HovelEntered = false;
        }

        void OnPlayerEnter(Player* /*player*/) override
        {
            EnsureStoryActors();
            RestoreCurrentStage();
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_JOREN_IRONSTOCK:
                case NPC_GRELIN_WHITEBEARD:
                case NPC_JONA_IRONSTOCK:
                case NPC_APPRENTICE_SOREN:
                    ImportantCreatures[creature->GetEntry()] = creature->GetGUID();
                    creature->setFaction(35);
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                    break;
                case NPC_CRAG_BOAR:
                    ImportantCreatures[creature->GetEntry()] = creature->GetGUID();
                    PrepareEnemy(creature);
                    break;
                case NPC_MONSTER_OF_A_TROLL:
                    ImportantCreatures[creature->GetEntry()] = creature->GetGUID();
                    PrepareEnemy(creature);
                    break;
                default:
                    break;
            }
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (unit->GetEntry() == NPC_MONSTER_OF_A_TROLL)
            {
                DoUpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE, NPC_MONSTER_OF_A_TROLL, 1, 0, unit);
                return;
            }

            auto itr = ActiveEnemies.find(unit->GetGUID());
            if (itr == ActiveEnemies.end())
                return;

            ActiveEnemies.erase(itr);
            DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_KILL_TROLL_WENDIGO, 1, 0, unit);
        }

        void SetData(uint32 type, uint32 data) override
        {
            uint32 step = getScenarionStep();
            switch (type)
            {
                case DATA_INTERACT_JOREN:
                    switch (step)
                    {
                        case 0:
                            DoUpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE, NPC_JOREN_IRONSTOCK, 1);
                            break;
                        case 4:
                        case 13:
                            DoUpdateAchievementCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_EMBELLISH_TALE, 1);
                            break;
                        case 7:
                        case 16:
                            DoUpdateAchievementCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_EXAGGERATE_TALE, 1);
                            break;
                        case 18:
                            DoUpdateAchievementCriteria(CRITERIA_TYPE_CAST_SPELL, SPELL_BETTER_TALE, 1);
                            break;
                        default:
                            break;
                    }
                    break;
                case DATA_INTERACT_JONA:
                    if (step == 2)
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_GEAR_UP, 1);
                    break;
                case DATA_BOAR_TIRED:
                    if (step == 9)
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_BOAR_TIRED, 1);
                    break;
                case DATA_TAME_BOAR:
                    if (step == 10)
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_TAME_BOAR, 1);
                    break;
                case DATA_USE_KEG:
                    HandleKeg(data, step);
                    break;
                default:
                    break;
            }
        }

        void onScenarionNextStep(uint32 newStep) override
        {
            switch (newStep)
            {
                case 1:
                    StartGrelinPanic();
                    break;
                case 3:
                case 4:
                case 5:
                    MoveJoren(GnomenbrauStoryPosition);
                    break;
                case 6:
                case 7:
                    MoveJoren(TheramoreStoryPosition);
                    break;
                case 8:
                    RunJorenFromBoar();
                    break;
                case 9:
                    SpawnBoar();
                    break;
                case 12:
                case 13:
                    MoveJoren(HovelEntrancePosition);
                    break;
                case 14:
                case 17:
                    SpawnHovelEnemies();
                    break;
                case 15:
                case 16:
                case 18:
                    MoveJoren(BossStoryPosition);
                    break;
                case 19:
                    SpawnFinalBoss();
                    break;
                case 20:
                    AddDelayedEvent(3500, [this]()
                    {
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_RELAX, 1);
                    });
                    break;
                default:
                    break;
            }
        }

        void Update(uint32 diff) override
        {
            if (getScenarionStep() != 12 || HovelEntered)
                return;

            if (ProximityTimer > diff)
            {
                ProximityTimer -= diff;
                return;
            }

            ProximityTimer = 1000;
            for (auto const& reference : instance->GetPlayers())
                if (Player* player = reference.getSource())
                    if (player->IsAlive() && player->GetDistance(HovelEntrancePosition) <= 25.0f)
                    {
                        HovelEntered = true;
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_ENTER_HOVEL, 1);
                        break;
                    }
        }

    private:
        void EnsureStoryActors()
        {
            for (ColdridgeSpawn const& actor : StoryActors)
                if (!GetImportantCreature(actor.Entry))
                    instance->SummonCreature(actor.Entry, actor.Pos);
        }

        Creature* GetImportantCreature(uint32 entry) const
        {
            auto itr = ImportantCreatures.find(entry);
            return itr != ImportantCreatures.end() ? instance->GetCreature(itr->second) : nullptr;
        }

        void PrepareEnemy(Creature* creature)
        {
            creature->setFaction(14);
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 |
                UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE |
                UNIT_FLAG_PACIFIED | UNIT_FLAG_STUNNED);
            creature->SetReactState(REACT_AGGRESSIVE);
        }

        void MoveJoren(Position const& position)
        {
            if (Creature* joren = GetImportantCreature(NPC_JOREN_IRONSTOCK))
                joren->NearTeleportTo(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), position.GetOrientation());
        }

        void StartGrelinPanic()
        {
            if (Creature* grelin = GetImportantCreature(NPC_GRELIN_WHITEBEARD))
                grelin->AI()->Talk(0);

            AddDelayedEvent(2500, [this]() { TalkGrelin(1); });
            AddDelayedEvent(5000, [this]() { TalkGrelin(2); });
            AddDelayedEvent(7500, [this]() { TalkGrelin(3); });
            AddDelayedEvent(9500, [this]()
            {
                DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_LISTEN_TO_GRELIN, 1);
            });
        }

        void TalkGrelin(uint8 group)
        {
            if (Creature* grelin = GetImportantCreature(NPC_GRELIN_WHITEBEARD))
                if (grelin->IsInWorld())
                    grelin->AI()->Talk(group);
        }

        void RunJorenFromBoar()
        {
            if (Creature* joren = GetImportantCreature(NPC_JOREN_IRONSTOCK))
                joren->GetMotionMaster()->MovePoint(1, -6322.0f, 468.0f, 384.0f);

            AddDelayedEvent(5500, [this]()
            {
                DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_JOREN_FLEES, 1);
            });
        }

        void SpawnBoar()
        {
            if (Creature* oldBoar = GetImportantCreature(NPC_CRAG_BOAR))
                if (oldBoar->IsAlive())
                    return;

            if (Creature* boar = instance->SummonCreature(NPC_CRAG_BOAR, BoarPosition))
            {
                boar->SetLevel(110);
                PrepareEnemy(boar);
                boar->AI()->DoZoneInCombat(boar, 60.0f);
            }
        }

        void SpawnHovelEnemies()
        {
            ActiveEnemies.clear();
            for (uint32 i = 0; i < sizeof(HovelEnemyPositions) / sizeof(Position); ++i)
            {
                uint32 entry = i % 3 == 0 ? NPC_WENDIGO : NPC_FROSTMANE_BLADE;
                if (Creature* enemy = instance->SummonCreature(entry, HovelEnemyPositions[i]))
                {
                    enemy->SetLevel(110);
                    PrepareEnemy(enemy);
                    ActiveEnemies.insert(enemy->GetGUID());
                    enemy->AI()->DoZoneInCombat(enemy, 80.0f);
                }
            }
        }

        void SpawnFinalBoss()
        {
            if (Creature* boss = GetImportantCreature(NPC_MONSTER_OF_A_TROLL))
                if (boss->IsAlive())
                    return;

            if (Creature* boss = instance->SummonCreature(NPC_MONSTER_OF_A_TROLL, BossPosition))
            {
                boss->SetLevel(110);
                PrepareEnemy(boss);
                boss->AI()->DoZoneInCombat(boss, 100.0f);
            }
        }

        void HandleKeg(uint32 entry, uint32 step)
        {
            switch (entry)
            {
                case GO_GNOMENBRAU:
                    if (step == 3)
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_FIND_GNOMENBRAU, 1);
                    else if (step == 5)
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_RECOVER_GNOMENBRAU, 1);
                    break;
                case GO_THERAMORE_PALE_ALE:
                    if (step == 6)
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_FIND_THERAMORE, 1);
                    else if (step == 11)
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_RECOVER_THERAMORE, 1);
                    break;
                case GO_STORMHAMMER_STOUT:
                    if (step == 15)
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_FIND_STORMHAMMER, 1);
                    break;
                default:
                    break;
            }
        }

        void RestoreCurrentStage()
        {
            uint32 step = getScenarionStep();
            if (step >= 3 && step <= 5)
                MoveJoren(GnomenbrauStoryPosition);
            else if (step >= 6 && step <= 8)
                MoveJoren(TheramoreStoryPosition);
            else if (step >= 12 && step <= 13)
                MoveJoren(HovelEntrancePosition);
            else if (step >= 15 && step <= 18)
                MoveJoren(BossStoryPosition);

            if (step == 9 || step == 10)
                SpawnBoar();
            else if ((step == 14 || step == 17) && ActiveEnemies.empty())
                SpawnHovelEnemies();
            else if (step == 19)
                SpawnFinalBoss();
        }
    };
};

void AddSC_instance_coldridge_cataclysm()
{
    new instance_coldridge_cataclysm();
}
