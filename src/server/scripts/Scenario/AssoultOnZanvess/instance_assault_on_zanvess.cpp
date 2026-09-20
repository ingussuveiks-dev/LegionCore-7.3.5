/*
* This file is part of the Pandaria 5.4.8 Project. See THANKS file for Copyright information
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "assault_on_zanvess.h"
#include "ScenarioMgr.h"
#include "Scenario.h"
#include "AchievementMgr.h"

class instance_assault_on_zanvess : public InstanceMapScript
{
    public:
        instance_assault_on_zanvess() : InstanceMapScript("instance_assault_on_zanvess", 1050) { }

        struct instance_assault_on_zanvess_InstanceMapScript : public InstanceScript
        {
            instance_assault_on_zanvess_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

            std::map<uint32, ObjectGuid> assaultEncounters;
            std::vector<ObjectGuid> gyroGUIDs;
            std::vector<ObjectGuid> nalleyGUIDs;
            uint32 chapterOne;
            uint32 chapterTwo;
            uint32 chapterThree;
            uint32 chapterFour;
            uint32 sonicTowerCount;
            uint32 alterTowerCount;
            uint32 teamValue;
            uint32 telvrakLinked;
            bool teamLeaderDefeated;
            bool heartReached;

            void Initialize() override
            {
                SetBossNumber(CHAPTERS);

                assaultEncounters.clear();
                gyroGUIDs.clear();
                nalleyGUIDs.clear();
                chapterOne = NOT_STARTED;
                chapterTwo = 0;
                chapterThree = 0;
                chapterFour = NOT_STARTED;
                sonicTowerCount = 0;
                alterTowerCount = 0;
                teamValue = 0;
                telvrakLinked = 0;
                teamLeaderDefeated = false;
                heartReached = false;
            }

            Scenario* GetScenario() const
            {
                return sScenarioMgr->GetScenario(instance->GetInstanceId());
            }

            void SendScenarioCriteria(uint32 hordeTreeId, uint32 allianceTreeId, uint64 counter = 1)
            {
                Scenario* scenario = GetScenario();
                uint32 treeId = teamValue == HORDE ? hordeTreeId : allianceTreeId;
                CriteriaTree const* tree = sAchievementMgr->GetCriteriaTree(treeId);
                if (!scenario || !tree || !tree->Entry)
                    return;

                CriteriaProgress progress;
                progress.Counter = counter;
                progress.date = time(nullptr);
                progress.criteriaTree = tree->Entry;
                scenario->SendCriteriaUpdate(&progress);
            }

            void SetScenarioStep(uint8 step)
            {
                if (Scenario* scenario = GetScenario())
                    scenario->SetCurrentStep(step);
            }

            void CompleteHeartChapter()
            {
                chapterThree = uint32(teamLeaderDefeated) + uint32(heartReached);
                if (chapterThree < 2 || GetBossState(DATA_HEART_OF_ZANVESS) == DONE)
                    return;

                SetBossState(DATA_HEART_OF_ZANVESS, DONE);
                SetScenarioStep(DATA_WEAPON_OF_ZANVESS);

                if (Creature* telvrak = instance->GetCreature(GetGuidData(NPC_COMMANDER_TELVRAK)))
                    telvrak->AI()->DoAction(ACTION_TELVRAK_ASSAULT);
            }

            void OnPlayerEnter(Player* player) override
            {
                if (!teamValue)
                    teamValue = player->GetTeam();

                uint8 step = DATA_ASSAULT_ON_ZANVESS;
                if (chapterOne == DONE)
                    step = DATA_DEFENSES_OF_ZANVESS;
                if (chapterTwo >= 12)
                    step = DATA_HEART_OF_ZANVESS;
                if (chapterThree >= 2)
                    step = DATA_WEAPON_OF_ZANVESS;
                SetScenarioStep(step);

                if (chapterOne == DONE)
                    SendScenarioCriteria(CRITERIA_TREE_ASSAULT_HORDE, CRITERIA_TREE_ASSAULT_ALLIANCE);
                if (sonicTowerCount >= 3)
                    SendScenarioCriteria(CRITERIA_TREE_KYPARITE_HORDE, CRITERIA_TREE_KYPARITE_ALLIANCE);
                if (alterTowerCount >= 3)
                    SendScenarioCriteria(CRITERIA_TREE_WHISPERING_STONES_HORDE, CRITERIA_TREE_WHISPERING_STONES_ALLIANCE);
                if (chapterTwo >= 12)
                    SendScenarioCriteria(CRITERIA_TREE_VENOMSTING_PITS_HORDE, CRITERIA_TREE_VENOMSTING_PITS_ALLIANCE);
                if (teamLeaderDefeated)
                    SendScenarioCriteria(CRITERIA_TREE_TEAM_LEADER_HORDE, CRITERIA_TREE_TEAM_LEADER_ALLIANCE);
                if (heartReached)
                    SendScenarioCriteria(CRITERIA_TREE_REACH_HEART_HORDE, CRITERIA_TREE_REACH_HEART_ALLIANCE);

                if (teamValue == HORDE)
                {
                    if (Creature* admiral = instance->GetCreature(GetGuidData(NPC_REAR_ADMIRAL_ZIGG)))
                        admiral->AI()->Talk(TALK_INTRO, player->GetGUID());
                }
                else if (Creature* rodgers = instance->GetCreature(GetGuidData(NPC_SKY_GENERAL_RODGER)))
                    rodgers->AI()->Talk(TALK_INTRO, player->GetGUID());
            }

            void OnCreatureCreate(Creature* creature) override
            {
                if (!teamValue)
                {
                    Map::PlayerList const& players = instance->GetPlayers();
                    if (!players.isEmpty())
                        if (Player* player = players.begin()->getSource())
                            teamValue = player->GetTeam();
                }

                switch (creature->GetEntry())
                {
                    case NPC_SLG_GENERIC_MOP:
                    case NPC_SHIELD_CHANNEL_POINT:
                        creature->SetDisplayId(11686);
                        break;
                    case NPC_KORKRON_GUNSHIP:
                    case NPC_SKYFIRE_GYRO_VEHICLE:
                        if (!creature->GetDBTableGUIDLow())
                            gyroGUIDs.push_back(creature->GetGUID());
                        break;
                    case NPC_SCENARIO_CONTROLLER:
                    case NPC_COMMANDER_TELVRAK:
                    case NPC_REAR_ADMIRAL_ZIGG:
                    case NPC_SKY_GENERAL_RODGER:
                        assaultEncounters[creature->GetEntry()] = creature->GetGUID();
                        break;
                    case NPC_NALLEY:
                        if (!creature->GetDBTableGUIDLow())
                            nalleyGUIDs.push_back(creature->GetGUID());
                        break;
                    case NPC_7TH_LEGION_INFILTRATOR:
                        if (teamValue == HORDE)
                            creature->UpdateEntry(NPC_KORKRON_INFILTRATOR, HORDE);
                        break;
                    case NPC_7TH_LIGION_PRIEST:
                        if (teamValue == HORDE)
                            creature->UpdateEntry(NPC_KORKRON_PRIEST, HORDE);
                        break;
                    case NPC_TEAM_LEADER_SCOOTER:
                        // The chapter boss is the opposing faction's leader:
                        // Horde fights Scooter, Alliance fights Bosh.
                        if (teamValue == ALLIANCE)
                            creature->UpdateEntry(NPC_SQUAD_LEADER_BOSH, ALLIANCE);
                        break;
                }
            }

            void OnUnitDeath(Unit* unit) override
            {
                switch (unit->GetEntry())
                {
                    case NPC_SONIC_CONTROL_TOWER:
                        if (++sonicTowerCount == 3)
                        {
                            for (ObjectGuid const& guid : gyroGUIDs)
                                if (Creature* gunship = instance->GetCreature(guid))
                                    gunship->AI()->DoAction(ACTION_FIRST_TOWER_OFFENSIVE);

                            SendScenarioCriteria(CRITERIA_TREE_KYPARITE_HORDE, CRITERIA_TREE_KYPARITE_ALLIANCE);
                        }
                        break;
                    case NPC_SONIC_CONTROL_TOWER_2:
                        if (++alterTowerCount == 3)
                        {
                            for (ObjectGuid const& guid : gyroGUIDs)
                                if (Creature* gunship = instance->GetCreature(guid))
                                    gunship->AI()->DoAction(ACTION_SECOND_TOWER_OFFENSIVE);

                            SendScenarioCriteria(CRITERIA_TREE_WHISPERING_STONES_HORDE, CRITERIA_TREE_WHISPERING_STONES_ALLIANCE);
                        }
                        break;
                }
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_ASSAULT_ON_ZANVESS:
                        if (chapterOne == DONE || data != DONE)
                            break;

                        chapterOne = DONE;
                        SetBossState(DATA_ASSAULT_ON_ZANVESS, DONE);
                        SendScenarioCriteria(CRITERIA_TREE_ASSAULT_HORDE, CRITERIA_TREE_ASSAULT_ALLIANCE);
                        SetScenarioStep(DATA_DEFENSES_OF_ZANVESS);

                        for (ObjectGuid const& guid : nalleyGUIDs)
                            if (Creature* nalley = instance->GetCreature(guid))
                                nalley->AI()->DoAction(ACTION_NALLEY_DEFENDERS);
                        SaveToDB();
                        break;
                    case DATA_DEFENSES_OF_ZANVESS:
                        if (chapterTwo >= 12)
                            break;

                        ++chapterTwo;
                        if (chapterTwo == 12)
                        {
                            SendScenarioCriteria(CRITERIA_TREE_VENOMSTING_PITS_HORDE, CRITERIA_TREE_VENOMSTING_PITS_ALLIANCE);
                            SetBossState(DATA_DEFENSES_OF_ZANVESS, DONE);
                            SetScenarioStep(DATA_HEART_OF_ZANVESS);

                            for (ObjectGuid const& guid : gyroGUIDs)
                                if (Creature* gunship = instance->GetCreature(guid))
                                    gunship->AI()->DoAction(ACTION_MOVE_TO_ISLAND);

                            if (Creature* controller = instance->GetCreature(GetGuidData(NPC_SCENARIO_CONTROLLER)))
                                controller->RemoveAurasDueToSpell(SPELL_ISLAND_SHIELD);

                            DoCastSpellOnPlayers(SPELL_STRAFING_RAN);

                            for (ObjectGuid const& guid : nalleyGUIDs)
                                if (Creature* nalley = instance->GetCreature(guid))
                                    nalley->AI()->DoAction(ACTION_BEACH);
                        }
                        SaveToDB();
                        break;
                    case DATA_TEAM_LEADER_DEFEATED:
                        if (!teamLeaderDefeated)
                        {
                            teamLeaderDefeated = true;
                            SendScenarioCriteria(CRITERIA_TREE_TEAM_LEADER_HORDE, CRITERIA_TREE_TEAM_LEADER_ALLIANCE);
                            CompleteHeartChapter();
                            SaveToDB();
                        }
                        break;
                    case DATA_REACHED_HEART:
                        if (!heartReached)
                        {
                            heartReached = true;
                            telvrakLinked = 1;
                            SendScenarioCriteria(CRITERIA_TREE_REACH_HEART_HORDE, CRITERIA_TREE_REACH_HEART_ALLIANCE);
                            CompleteHeartChapter();
                            SaveToDB();
                        }
                        break;
                    case DATA_TELVRAK_LINKED:
                        telvrakLinked = data;
                        break;
                    case DATA_WEAPON_OF_ZANVESS:
                        if (chapterFour == DONE || data != DONE)
                            break;

                        chapterFour = DONE;
                        SetBossState(DATA_WEAPON_OF_ZANVESS, DONE);
                        SendScenarioCriteria(CRITERIA_TREE_TELVRAK_HORDE, CRITERIA_TREE_TELVRAK_ALLIANCE);
                        if (Scenario* scenario = GetScenario())
                            scenario->Reward(false, scenario->GetCurrentStep());
                        SaveToDB();
                        break;
                }
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_ASSAULT_ON_ZANVESS:
                        return chapterOne;
                    case DATA_DEFENSES_OF_ZANVESS:
                        return chapterTwo;
                    case DATA_HEART_OF_ZANVESS:
                        return chapterThree;
                    case DATA_WEAPON_OF_ZANVESS:
                        return chapterFour;
                    case DATA_FACTION:
                        return teamValue == HORDE ? 1 : 0;
                    case DATA_TELVRAK_LINKED:
                        return telvrakLinked;
                    case DATA_TEAM_LEADER_DEFEATED:
                        return teamLeaderDefeated;
                    case DATA_REACHED_HEART:
                        return heartReached;
                }

                return 0;
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                auto itr = assaultEncounters.find(type);
                return itr != assaultEncounters.end() ? itr->second : ObjectGuid::Empty;
            }

            bool IsWipe() const override
            {
                for (Map::PlayerList::const_iterator itr = instance->GetPlayers().begin(); itr != instance->GetPlayers().end(); ++itr)
                {
                    Player* player = itr->getSource();
                    if (player && player->IsAlive() && !player->isGameMaster())
                        return false;
                }

                return true;
            }

            std::string GetSaveData() override
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << "A O Z " << chapterOne << ' ' << chapterTwo << ' ' << chapterThree << ' ' << chapterFour << ' '
                    << sonicTowerCount << ' ' << alterTowerCount << ' ' << teamValue << ' ' << telvrakLinked << ' '
                    << uint32(teamLeaderDefeated) << ' ' << uint32(heartReached);

                OUT_SAVE_INST_DATA_COMPLETE;
                return saveStream.str();
            }

            void Load(char const* in) override
            {
                if (!in)
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                OUT_LOAD_INST_DATA(in);

                char dataHead1, dataHead2, dataHead3;
                uint32 leader = 0;
                uint32 reached = 0;
                std::istringstream loadStream(in);
                loadStream >> dataHead1 >> dataHead2 >> dataHead3;

                if (dataHead1 != 'A' || dataHead2 != 'O' || dataHead3 != 'Z')
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                loadStream >> chapterOne >> chapterTwo >> chapterThree >> chapterFour;
                loadStream >> sonicTowerCount >> alterTowerCount >> teamValue >> telvrakLinked >> leader >> reached;

                chapterOne = chapterOne == DONE ? DONE : NOT_STARTED;
                chapterTwo = std::min<uint32>(chapterTwo, 12);
                chapterFour = chapterFour == DONE ? DONE : NOT_STARTED;
                sonicTowerCount = std::min<uint32>(sonicTowerCount, 3);
                alterTowerCount = std::min<uint32>(alterTowerCount, 3);
                teamLeaderDefeated = leader != 0;
                heartReached = reached != 0;
                chapterThree = uint32(teamLeaderDefeated) + uint32(heartReached);

                if (chapterOne == DONE)
                    SetBossState(DATA_ASSAULT_ON_ZANVESS, DONE);
                if (chapterTwo >= 12)
                    SetBossState(DATA_DEFENSES_OF_ZANVESS, DONE);
                if (chapterThree >= 2)
                    SetBossState(DATA_HEART_OF_ZANVESS, DONE);
                if (chapterFour == DONE)
                    SetBossState(DATA_WEAPON_OF_ZANVESS, DONE);

                OUT_LOAD_INST_DATA_COMPLETE;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_assault_on_zanvess_InstanceMapScript(map);
        }
};

void AddSC_instance_assault_on_zanvess()
{
    new instance_assault_on_zanvess();
}
