/*
* This file is part of the Pandaria 5.4.8 Project. See THANKS file for Copyright information
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

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "VMapFactory.h"
#include "dagger_in_the_dark.h"
#include "ScenarioMgr.h"
#include "Scenario.h"
#include "AchievementMgr.h"

class instance_dagger_in_the_dark : public InstanceMapScript
{
    public:
        instance_dagger_in_the_dark() : InstanceMapScript("instance_dagger_in_the_dark", 1095) { }

        struct instance_dagger_in_the_dark_InstanceMapScript : public InstanceScript
        {
            instance_dagger_in_the_dark_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

            EventMap events;
            uint32 m_auiEncounter[4];
            uint32 chapterOne, chapterTwo, chapterThree, chapterFour, chapterFive, chapterSix, chapterSeven, chapterEight;
            uint32 driflerGuardsCount;
            uint32 tabletsCount;
            uint32 factionData;
            uint32 waterJetsSlainCount;
            ObjectGuid voljinGUID;
            ObjectGuid rakgorGUID;
            ObjectGuid grizzleGUID;
            ObjectGuid noshiGUID;
            ObjectGuid lizardLordGUID;
            ObjectGuid saurokSlayerBoatGUID;
            ObjectGuid bloodrazorCaveGUID;
            bool hasInit;

            Scenario* GetScenario() const
            {
                return sScenarioMgr->GetScenario(instance->GetInstanceId());
            }

            void SetScenarioStep(uint8 step)
            {
                if (Scenario* scenario = GetScenario())
                    scenario->SetCurrentStep(step);
            }

            void SendScenarioCriteria(uint32 treeId, uint64 counter = 1)
            {
                Scenario* scenario = GetScenario();
                CriteriaTree const* tree = sAchievementMgr->GetCriteriaTree(treeId);
                if (!scenario || !tree || !tree->Entry)
                    return;

                CriteriaProgress progress;
                progress.Counter = counter;
                progress.date = time(nullptr);
                progress.criteriaTree = tree->Entry;
                scenario->SendCriteriaUpdate(&progress);
            }

            void Initialize() override
            {
                SetBossNumber(8);
                memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));

                chapterOne     = 0;
                chapterTwo     = 0;
                chapterThree   = 0;
                chapterFour    = 0;
                chapterFive    = 0;
                chapterSix     = 0;
                chapterSeven   = 0;
                chapterEight   = 0;
                voljinGUID = ObjectGuid::Empty;
                rakgorGUID = ObjectGuid::Empty;
                grizzleGUID = ObjectGuid::Empty;
                noshiGUID = ObjectGuid::Empty;
                lizardLordGUID = ObjectGuid::Empty;
                driflerGuardsCount   = 0;
                saurokSlayerBoatGUID = ObjectGuid::Empty;
                bloodrazorCaveGUID = ObjectGuid::Empty;
                waterJetsSlainCount  = 0;
                tabletsCount   = 0;
                factionData    = 0;
                hasInit = false;

                DoUpdateWorldState(static_cast<WorldStates>(WORLDSTATE_U_MEAN_THATS_NOT_VOID_ZONE), 1);
                DoUpdateWorldState(static_cast<WorldStates>(WORLDSTATE_WATERY_GRAVE), 0);
                DoUpdateWorldState(static_cast<WorldStates>(WORLDSTATE_NO_EGG_LEFT_BEHIND), 1);
            }

            void OnPlayerEnter(Player* player) override
            {
                if (!hasInit)
                {
                    hasInit = true;
                    factionData = player->GetTeam();
                }

                uint8 step = DATA_THE_MISSION;
                if (chapterOne >= DONE)
                    step = DATA_WE_RIDE;
                if (chapterTwo >= DONE)
                    step = DATA_GUARDS;
                if (chapterThree >= DONE)
                    step = DATA_INTO_THE_CAVE;
                if (chapterFour >= DONE)
                    step = DATA_THE_SOURCE;
                if (chapterFive >= DONE)
                    step = DATA_THE_BROODMASTER;
                if (chapterSix >= DONE)
                    step = DATA_INVESTIGATION;
                if (chapterSeven >= DONE)
                    step = DATA_DEFEAT_RAKGOR;
                SetScenarioStep(step);
            }

            void OnUnitDeath(Unit* unit) override
            {
                switch (unit->GetEntry())
                {
                    case NPC_DARKHATCHED_SHAMAN:
                    case NPC_DARKHATCHED_SKINFLAYER:
                    case NPC_DARKHATCHED_SKULKER:
                        if (++driflerGuardsCount == 3) // Lizard come to us
                        {
                            SendScenarioCriteria(CRITERIA_TREE_DEFEAT_SAUROK_PATROL);

                            if (Creature* lizardLord = instance->GetCreature(GetGuidData(NPC_DARKHATCHED_LIZARD_LORD)))
                                lizardLord->AI()->DoAction(ACTION_START_INTRO);

                            if (Creature* voljin = instance->GetCreature(GetGuidData(NPC_VOLJIN)))
                                voljin->AI()->DoAction(ACTION_MEET_LIZARD_LORD);
                        }
                        break;
                }
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_VOLJIN:
                        voljinGUID = creature->GetGUID();
                        break;
                    case NPC_RAKGOR_BLOODRAZOR:
                        if (creature->GetPositionZ() < 410.0f)
                            rakgorGUID = creature->GetGUID();
                        else
                        {
                            creature->SetVisible(false);
                            bloodrazorCaveGUID = creature->GetGUID();
                        }
                        break;
                    case NPC_GRIZZLE_GEARSLIP:
                        grizzleGUID = creature->GetGUID();
                        break;
                    case NPC_BROODMASTER_NOSHI:
                        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC);
                        noshiGUID = creature->GetGUID();
                        break;
                    case NPC_DARKHATCHED_LIZARD_LORD:
                        creature->SetVisible(false);
                        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC);
                        lizardLordGUID = creature->GetGUID();
                        break;
                    case NPC_SPRING_DRIFTER_TARGET:
                        creature->SetVisible(false);
                        break;
                    case NPC_SPRING_SAUROK_SLAYER:
                        creature->SetVisible(false);
                        saurokSlayerBoatGUID = creature->GetGUID();
                        break;
                    case NPC_DARKHATCHED_SORCERER:
                        creature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                        break;
                    case NPC_BROKEN_MOGU_TABLET:
                    case NPC_BROKEN_MOGU_TABLET_2:
                        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        break;
                }
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_THE_MISSION:
                        chapterOne = data;
                        SetBossState(DATA_THE_MISSION, EncounterState(data));
                        if (data == DONE)
                            SetScenarioStep(DATA_WE_RIDE);
                        break;
                    case DATA_WE_RIDE:
                        chapterTwo = data;
                        SetBossState(DATA_WE_RIDE, EncounterState(data));
                        if (data == DONE)
                            SetScenarioStep(DATA_GUARDS);
                        break;
                    case DATA_GUARDS:
                        chapterThree = data;
                        SetBossState(DATA_GUARDS, EncounterState(data));
                        if (data == DONE)
                            SetScenarioStep(DATA_INTO_THE_CAVE);

                        if (Creature* voljin = instance->GetCreature(GetGuidData(NPC_VOLJIN)))
                            voljin->AI()->DoAction(ACTION_LIZARD_DIED);
                        break;
                    case DATA_INTO_THE_CAVE:
                        chapterFour = data;
                        SetBossState(DATA_INTO_THE_CAVE, EncounterState(data));
                        if (data == DONE)
                            SetScenarioStep(DATA_THE_SOURCE);

                        if (Creature* voljin = instance->GetCreature(GetGuidData(NPC_VOLJIN)))
                            voljin->AI()->DoAction(ACTION_VOLJIN_LEAVE_BOAT);
                        break;
                    case DATA_THE_SOURCE:
                        chapterFive = data;
                        SetBossState(DATA_THE_SOURCE, EncounterState(data));
                        if (data == DONE)
                            SetScenarioStep(DATA_THE_BROODMASTER);

                        if (Creature* voljin = instance->GetCreature(GetGuidData(NPC_VOLJIN)))
                            voljin->AI()->DoAction(ACTION_REACH_BROODMASTER);

                        if (Creature* broodmaster = instance->GetCreature(GetGuidData(NPC_BROODMASTER_NOSHI)))
                            broodmaster->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED | UNIT_FLAG_IMMUNE_TO_PC);
                        break;
                    case DATA_THE_BROODMASTER:
                        chapterSix = data;
                        SetBossState(DATA_THE_BROODMASTER, EncounterState(data));

                        if (Creature* voljin = instance->GetCreature(GetGuidData(NPC_VOLJIN)))
                            voljin->AI()->DoAction(ACTION_DEFEAT_BROODMASTER);

                        if (Creature* bloodrazor = instance->GetCreature(GetGuidData(NPC_RAKGOR_BLOODRAZOR + 1)))
                            bloodrazor->AI()->DoAction(ACTION_DEFEAT_BROODMASTER);

                        // Only credit this until talking event is done.
                        SendScenarioCriteria(CRITERIA_TREE_DEFEAT_BROODMASTER);
                        if (data == DONE)
                            SetScenarioStep(DATA_INVESTIGATION);
                        break;
                    case DATA_INVESTIGATION:
                        chapterSeven = data;
                        SetBossState(DATA_INVESTIGATION, EncounterState(data));
                        if (data == DONE)
                            SetScenarioStep(DATA_DEFEAT_RAKGOR);

                        if (Creature* voljin = instance->GetCreature(GetGuidData(NPC_VOLJIN)))
                            voljin->AI()->DoAction(ACTION_INVESTIGATE_DONE);
                        break;
                    case DATA_DEFEAT_RAKGOR:
                        chapterEight = data;
                        SetBossState(DATA_DEFEAT_RAKGOR, EncounterState(data));
                        SendScenarioCriteria(CRITERIA_TREE_DEFEAT_BLOODRAZOR);

                        if (Creature* voljin = instance->GetCreature(GetGuidData(NPC_VOLJIN)))
                            voljin->AI()->DoAction(ACTION_BLOODRAZOR_SLAIN);

                        if (data == DONE)
                            if (Scenario* scenario = GetScenario())
                                scenario->Reward(false, scenario->GetCurrentStep());
                        break;
                    case DATA_TABLETS_COUNT:
                        if (++tabletsCount > 1)
                            SetData(DATA_INVESTIGATION, DONE);
                        break;
                    case WATER_JETS_SLAIN_DATA:
                        if (++waterJetsSlainCount > 2)
                            DoUpdateWorldState(static_cast<WorldStates>(WORLDSTATE_WATERY_GRAVE), 1);
                        break;
                }

                if (data == DONE)
                    SaveToDB();
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_THE_MISSION:
                        return chapterOne;
                    case DATA_WE_RIDE:
                        return chapterTwo;
                    case DATA_GUARDS:
                        return chapterThree;
                    case DATA_INTO_THE_CAVE:
                        return chapterFour;
                    case DATA_THE_SOURCE:
                        return chapterFive;
                    case DATA_THE_BROODMASTER:
                        return chapterSix;
                    case DATA_INVESTIGATION:
                        return chapterSeven;
                    case DATA_DEFEAT_RAKGOR:
                        return chapterEight;
                    case FACTION_DATA:
                        return factionData == HORDE ? 1 : 0;
                }

                return 0;
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case NPC_VOLJIN:
                        return voljinGUID;
                    case NPC_RAKGOR_BLOODRAZOR:
                        return rakgorGUID;
                    case NPC_GRIZZLE_GEARSLIP:
                        return grizzleGUID;
                    case NPC_BROODMASTER_NOSHI:
                        return noshiGUID;
                    case NPC_DARKHATCHED_LIZARD_LORD:
                        return lizardLordGUID;
                    case NPC_SPRING_SAUROK_SLAYER:
                        return saurokSlayerBoatGUID;
                    case NPC_RAKGOR_BLOODRAZOR + 1:
                        return bloodrazorCaveGUID;
                }

                return ObjectGuid::Empty;
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                return true;
            }

            std::string GetSaveData() override
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << "D I D " << chapterOne << ' ' << chapterTwo << ' ' << chapterThree << ' ' << chapterFour << ' ' << chapterFive << ' ' << chapterSix;
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

                std::istringstream loadStream(in);
                loadStream >> dataHead1 >> dataHead2 >> dataHead3;

                if (dataHead1 == 'D' && dataHead2 == 'I'&& dataHead3 == 'D')
                {
                    uint32 temp = 0;
                    loadStream >> temp; // chapterOne complete
                    chapterOne = temp;
                    SetData(DATA_THE_MISSION, chapterOne);
                    loadStream >> temp; // chapterTwo complete
                    chapterTwo = temp;
                    SetData(DATA_WE_RIDE, chapterTwo);
                    loadStream >> temp; // chapterThree complete
                    chapterThree = temp;
                    SetData(DATA_GUARDS, chapterThree);
                    loadStream >> temp; // chapterFour complete
                    chapterFour = temp;
                    SetData(DATA_INTO_THE_CAVE, chapterFour);
                    loadStream >> temp; // chapterFive complete
                    chapterFive = temp;
                    SetData(DATA_THE_SOURCE, chapterFive);
                    loadStream >> temp; // chapterSix complete
                    chapterSix = temp;
                    SetData(DATA_THE_BROODMASTER, chapterSix);
                    loadStream >> temp; // chapterSeven complete
                    chapterSeven = temp;
                    SetData(DATA_INVESTIGATION, chapterSeven);
                    loadStream >> temp; // chapterEight complete
                    chapterEight = temp;
                    SetData(DATA_DEFEAT_RAKGOR, chapterEight);
                }
                else OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_dagger_in_the_dark_InstanceMapScript(map);
        }
};

void AddSC_instance_dagger_in_the_dark()
{
    new instance_dagger_in_the_dark();
}
