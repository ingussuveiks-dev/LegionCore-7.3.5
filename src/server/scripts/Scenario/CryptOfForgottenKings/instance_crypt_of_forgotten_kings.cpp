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
#include "crypt_of_forgotten_kings.h"
#include "ScenarioMgr.h"
#include "Scenario.h"

static DoorData const doorData[] =
{
    { GO_GOLDEN_DOORS_2, DATA_POOL_OF_LIFE, DOOR_TYPE_PASSAGE, BOUNDARY_E },
    { 0,                 0,                 DOOR_TYPE_ROOM,    BOUNDARY_NONE }
};

class instance_crypt_of_forgotten_kings : public InstanceMapScript
{
    public:
        instance_crypt_of_forgotten_kings() : InstanceMapScript("instance_crypt_of_forgotten_kings", 1030) { }

        struct instance_crypt_of_forgotten_kings_InstanceMapScript : public InstanceScript
        {
            instance_crypt_of_forgotten_kings_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

            uint32 chapterOne, chapterTwo, chapterThree, essence;
            ObjectGuid jinGUID, abominationGUID, lifePoolGUID, cryptGuardianHallGUID;
            std::list<ObjectGuid> cryptGuardians;
            std::unordered_map<uint32, ObjectGuid> gameObjectGuids;

            void Initialize() override
            {
                SetBossNumber(CHAPTERS);
                LoadDoorData(doorData);
                chapterOne = NOT_STARTED;
                chapterTwo = NOT_STARTED;
                chapterThree = NOT_STARTED;
                essence = 0;
                jinGUID.Clear();
                abominationGUID.Clear();
                lifePoolGUID.Clear();
                cryptGuardianHallGUID.Clear();
                cryptGuardians.clear();
                gameObjectGuids.clear();
            }

            void OnPlayerEnter(Player* player) override
            {
                // ScenarioMgr initializes the chapter state from scenario_data and DB2.
                if (chapterTwo != IN_PROGRESS)
                    return;

                player->RemoveAurasDueToSpell(SPELL_POOL_OF_LIFE_PROGRESS_BAR);
                player->CastSpell(player, SPELL_POOL_OF_LIFE_PROGRESS_BAR, false);
                player->SetPower(POWER_ALTERNATE, essence);
            }

            GameObject* GetStoredGameObject(uint32 entry)
            {
                auto itr = gameObjectGuids.find(entry);
                return itr != gameObjectGuids.end() ? instance->GetGameObject(itr->second) : nullptr;
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_JIN_IRONFIST:
                        jinGUID = creature->GetGUID();
                        break;
                    case NPC_ABOMINATION_OF_ANGER:
                        abominationGUID = creature->GetGUID();
                        creature->SetVisible(chapterTwo == DONE);
                        break;
                    case NPC_CRYPT_GUARDIAN:
                        creature->SetVisible(chapterTwo == DONE);
                        cryptGuardians.push_back(creature->GetGUID());
                        break;
                    case NPC_ESSENCE_OF_HATE:
                        if (chapterOne != DONE)
                        {
                            creature->SetVisible(false);
                            creature->SetReactState(REACT_PASSIVE);
                        }
                        break;
                    case NPC_POOL_OF_LIFE:
                        lifePoolGUID = creature->GetGUID();
                        break;
                    case NPC_CRYPT_GUARDIAN_2:
                        cryptGuardianHallGUID = creature->GetGUID();
                        break;
                    case NPC_SHADOWS_OF_ANGER:
                    case NPC_CLOUD_OF_ANGER:
                        creature->SetDisplayId(42332);
                        break;
                    case NPC_WOUNDED_SHADO_PAN:
                        creature->setRegeneratingHealth(false);
                        creature->SetHealth(uint32(creature->GetMaxHealth() * 0.1f));
                        creature->SetStandState(UNIT_STAND_STATE_DEAD);
                        break;
                }
            }

            void OnUnitDeath(Unit* unit) override
            {
                switch (unit->GetEntry())
                {
                    case NPC_ESSENCE_OF_HATE:
                    {
                        if (essence >= 40)
                            break;

                        ++essence;
                        SetData(DATA_ESSENCE, essence);
                        std::list<Player*> players;
                        GetPlayerListInGrid(players, unit, 200.0f);
                        for (Player* player : players)
                            if (player->HasAura(SPELL_POOL_OF_LIFE_PROGRESS_BAR))
                                player->SetPower(POWER_ALTERNATE, essence);

                        if (essence == 40)
                        {
                            DoRemoveAurasDueToSpellOnPlayers(SPELL_POOL_OF_LIFE_PROGRESS_BAR);
                            if (Creature* lifePool = instance->GetCreature(lifePoolGUID))
                                lifePool->AI()->DoAction(ACTION_CHAPTER_TWO_RESET);
                        }
                        break;
                    }
                    case NPC_CRYPT_GUARDIAN_3:
                        if (GameObject* vault = GetStoredGameObject(GO_GOLDEN_DOORS_3))
                            vault->UseDoorOrButton();
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* gameObject) override
            {
                switch (gameObject->GetEntry())
                {
                    case GO_GOLDEN_DOORS:
                    case GO_GOLDEN_DOORS_2:
                    case GO_GOLDEN_DOORS_3:
                        gameObjectGuids[gameObject->GetEntry()] = gameObject->GetGUID();
                        gameObject->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_INTERACT_COND);
                        break;
                    case GO_SHA_CORRUPTION:
                        gameObjectGuids[gameObject->GetEntry()] = gameObject->GetGUID();
                        break;
                }
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_JIN_IRONFIST:
                        chapterOne = data;
                        if (data == DONE)
                            if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                                scenario->SetCurrentStep(DATA_POOL_OF_LIFE);
                        break;
                    case DATA_POOL_OF_LIFE:
                        chapterTwo = data;
                        if (data == DONE)
                        {
                            if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                                scenario->SetCurrentStep(DATA_ABOMINATION_OF_ANGER);
                            if (GameObject* door = GetStoredGameObject(GO_GOLDEN_DOORS_2))
                                door->UseDoorOrButton();
                            if (GameObject* corruption = GetStoredGameObject(GO_SHA_CORRUPTION))
                                corruption->Delete();
                            if (Creature* guardian = instance->GetCreature(cryptGuardianHallGUID))
                                guardian->AI()->DoAction(ACTION_GUARDIAN_INIT);
                            for (ObjectGuid guid : cryptGuardians)
                                if (Creature* guardian = instance->GetCreature(guid))
                                {
                                    guardian->SetVisible(true);
                                    if (guardian->IsAlive() && guardian->AI())
                                        guardian->AI()->DoAction(ACTION_GUARDIAN_INIT);
                                }
                        }
                        break;
                    case DATA_ABOMINATION_OF_ANGER:
                        chapterThree = data;
                        if (data == DONE)
                            if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                                scenario->Reward(false, scenario->GetCurrentStep());
                        break;
                    case DATA_ESSENCE:
                        essence = data;
                        SaveToDB();
                        return;
                }

                if (type < CHAPTERS)
                    SetBossState(type, EncounterState(data));
                if (data == DONE || data == SPECIAL)
                    SaveToDB();
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_JIN_IRONFIST:         return chapterOne;
                    case DATA_POOL_OF_LIFE:         return chapterTwo;
                    case DATA_ABOMINATION_OF_ANGER: return chapterThree;
                    case DATA_ESSENCE:              return essence;
                    default:                        return 0;
                }
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case NPC_JIN_IRONFIST:         return jinGUID;
                    case NPC_ABOMINATION_OF_ANGER: return abominationGUID;
                    case NPC_POOL_OF_LIFE:         return lifePoolGUID;
                    default:                       return ObjectGuid::Empty;
                }
            }

            std::string GetSaveData() override
            {
                OUT_SAVE_INST_DATA;
                std::ostringstream saveStream;
                saveStream << "C F K " << chapterOne << ' ' << chapterTwo << ' ' << chapterThree << ' ' << essence;
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
                char head1, head2, head3;
                uint32 one, two, three, savedEssence;
                std::istringstream loadStream(in);
                loadStream >> head1 >> head2 >> head3 >> one >> two >> three >> savedEssence;
                if (head1 != 'C' || head2 != 'F' || head3 != 'K')
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                chapterOne = one == IN_PROGRESS ? NOT_STARTED : one;
                chapterTwo = two == IN_PROGRESS ? NOT_STARTED : two;
                chapterThree = three == IN_PROGRESS ? NOT_STARTED : three;
                essence = std::min(savedEssence, 40u);
                SetBossState(DATA_JIN_IRONFIST, EncounterState(chapterOne));
                SetBossState(DATA_POOL_OF_LIFE, EncounterState(chapterTwo));
                SetBossState(DATA_ABOMINATION_OF_ANGER, EncounterState(chapterThree));
                OUT_LOAD_INST_DATA_COMPLETE;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_crypt_of_forgotten_kings_InstanceMapScript(map);
        }
};

void AddSC_instance_crypt_of_forgotten_kings()
{
    new instance_crypt_of_forgotten_kings();
}
