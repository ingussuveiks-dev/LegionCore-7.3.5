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
#include "blood_in_the_snow.h"
#include "ScenarioMgr.h"
#include "Scenario.h"
#include "AchievementMgr.h"

class instance_blood_in_the_snow : public InstanceMapScript
{
    public:
        instance_blood_in_the_snow() : InstanceMapScript("instance_blood_in_the_snow", 1130) { }

        struct instance_blood_in_the_snow_InstanceMapScript : public InstanceScript
        {
            instance_blood_in_the_snow_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

            EventMap events;
            uint32 m_auiEncounter[4];
            uint32 chapterOne, chapterTwo, chapterThree, chapterFour, chapterFive, chapterSix;
            ObjectGuid grimboltGUID;
            ObjectGuid stonebeardGUID;
            ObjectGuid forgeFellowGUID;
            ObjectGuid boldBrewGUID;
            ObjectGuid varianIntroGUID;
            ObjectGuid varianOutroGUID;
            ObjectGuid moiraIntroGUID;
            ObjectGuid moiraOutroGUID;
            ObjectGuid roastingSpitGUID;
            ObjectGuid hekimaGUID;
            ObjectGuid farastuGUID;
            uint32 shimmerweedCount;
            bool scenarioCompleted;
            std::vector<ObjectGuid> victoryorDeathSceneGUIDs;

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
                SetBossNumber(6);
                memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
                chapterOne = 0;
                chapterTwo = 0;
                chapterThree = 0;
                chapterFour = 0;
                chapterFive = 0;
                chapterSix = 0;
                grimboltGUID = ObjectGuid::Empty;
                stonebeardGUID = ObjectGuid::Empty;
                forgeFellowGUID = ObjectGuid::Empty;
                boldBrewGUID = ObjectGuid::Empty;
                roastingSpitGUID = ObjectGuid::Empty;
                hekimaGUID = ObjectGuid::Empty;
                farastuGUID = ObjectGuid::Empty;
                varianIntroGUID = ObjectGuid::Empty;
                varianOutroGUID = ObjectGuid::Empty;
                moiraIntroGUID = ObjectGuid::Empty;
                moiraOutroGUID = ObjectGuid::Empty;
                shimmerweedCount = 0;
                scenarioCompleted = false;
                events.ScheduleEvent(1, 3500);

                victoryorDeathSceneGUIDs.clear();
                DoUpdateWorldState(static_cast<WorldStates>(WORLDSTATE_HEKIMAS_HEAL_HALTER), 1);
            }

            void OnPlayerEnter(Player* player) override
            {
                player->CastSpell(player, SPELL_BLIZZARD_WEATHER, true);

                uint8 step = DATA_MOUNTAINEERS;
                if (chapterFive >= DONE)
                    step = DATA_VICTORY_OF_DEATH;
                else if (chapterFour >= DONE)
                    step = DATA_COLD_CAVE;
                else if (chapterThree >= DONE)
                    step = DATA_SAVE_SCOUT_FORGEFELLOW;
                else if (chapterTwo >= DONE)
                    step = DATA_SAVE_SCOUT_BOLD_BREW;
                else if (chapterOne >= DONE)
                    step = DATA_SAVE_SCOUT_STONEBEARD;
                SetScenarioStep(step);
            }

            void OnPlayerLeave(Player* player) override
            {
                player->RemoveAurasDueToSpell(SPELL_BLIZZARD_WEATHER);
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_MOUNTAINER_GRIMBOLT:
                        grimboltGUID = creature->GetGUID();
                        break;
                    case NPC_SCOUT_STONEBEARD:
                        stonebeardGUID = creature->GetGUID();
                        break;
                    case NPC_SCOUT_FORGEFELLOW:
                        if (creature->GetPositionY() < -120.0f)
                        {
                            forgeFellowGUID = creature->GetGUID();
                            creature->SetVisible(false);
                        }
                        break;
                    case NPC_SCOUT_BOLDBREW:
                        creature->SetVisible(false);
                        boldBrewGUID = creature->GetGUID();
                        break;
                    case NPC_KING_VARIAN_WRYNN:
                        if (creature->GetPositionZ() < 400.0f)
                            varianIntroGUID = creature->GetGUID();
                        else
                        {
                            creature->SetVisible(false);
                            varianOutroGUID = creature->GetGUID();
                        }
                        break;
                    case NPC_MOIRA_THAURISSAN:
                        if (creature->GetPositionZ() < 400.0f)
                            moiraIntroGUID = creature->GetGUID();
                        else
                        {
                            creature->SetVisible(false);
                            moiraOutroGUID = creature->GetGUID();
                        }
                        break;
                    case NPC_ROASTING_SPIT:
                        roastingSpitGUID = creature->GetGUID();
                        break;
                    case NPC_STORMWIND_ROYAL_GUARD:
                        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
                        break;
                    case NPC_HEKIMA_THE_WISE:
                        creature->SetVisible(false);
                        hekimaGUID = creature->GetGUID();
                        break;
                    case NPC_FARASTU:
                        creature->SetVisible(false);
                        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_PACIFIED);
                        farastuGUID = creature->GetGUID();
                        break;
                    case NPC_DARK_IRON_FORGEGUARD:
                    case NPC_PYROMANCE_FLAMEHEARTH:
                        if (creature->GetPositionZ() > 400.0f)
                        {
                            creature->SetVisible(false);
                            creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
                            victoryorDeathSceneGUIDs.push_back(creature->GetGUID());
                        }
                        break;
                }
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_MOUNTAINEERS:
                        chapterOne = data;
                        if (data != DONE)
                            break;

                        SendScenarioCriteria(32624);
                        SetScenarioStep(DATA_SAVE_SCOUT_STONEBEARD);

                        // Allow spellclick to spit
                        if (Creature* spit = instance->GetCreature(GetGuidData(NPC_ROASTING_SPIT)))
                            spit->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                        break;
                    case DATA_SAVE_SCOUT_STONEBEARD:
                        chapterTwo = data;
                        if (data != DONE)
                            break;

                        SendScenarioCriteria(32647);
                        SetScenarioStep(DATA_SAVE_SCOUT_BOLD_BREW);

                        if (Creature* boldbrew = instance->GetCreature(GetGuidData(NPC_SCOUT_BOLDBREW)))
                            boldbrew->SetVisible(true);
                        break;
                    case DATA_SAVE_SCOUT_BOLD_BREW:
                        chapterThree = data;
                        if (data != DONE)
                            break;

                        SendScenarioCriteria(32649);
                        SetScenarioStep(DATA_SAVE_SCOUT_FORGEFELLOW);

                        if (Creature* forgefellow = instance->GetCreature(GetGuidData(NPC_SCOUT_FORGEFELLOW)))
                            forgefellow->SetVisible(true);
                        break;
                    case DATA_SAVE_SCOUT_FORGEFELLOW:
                        chapterFour = data;
                        if (data != DONE)
                            break;

                        SendScenarioCriteria(32651);
                        SetScenarioStep(DATA_COLD_CAVE);

                        if (Creature* farastu = instance->GetCreature(GetGuidData(NPC_FARASTU)))
                        {
                            farastu->SetVisible(true);
                            farastu->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_PACIFIED);
                        }
                        break;
                    case DATA_COLD_CAVE:
                        chapterFive = data;
                        if (data != DONE)
                            break;

                        SendScenarioCriteria(32653);
                        SetScenarioStep(DATA_VICTORY_OF_DEATH);

                        // Remove Blizzard Weather from players
                        DoRemoveAurasDueToSpellOnPlayers(SPELL_BLIZZARD_WEATHER);

                        if (Creature* moira = instance->GetCreature(GetGuidData(NPC_MOIRA_THAURISSAN + 1)))
                        {
                            moira->SetVisible(true);
                            moira->AI()->Talk(TALK_SPECIAL_7);
                        }

                        // Activate ironforge guardians for cosmetic battle and remove old trolls from this area
                        for (auto&& itr : victoryorDeathSceneGUIDs)
                        {
                            if (Creature* ironforgeGuard = instance->GetCreature(itr))
                            {
                                ironforgeGuard->SetVisible(true);
                                ironforgeGuard->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED);
                            }
                        }

                        if (Creature* hekima = instance->GetCreature(GetGuidData(NPC_HEKIMA_THE_WISE)))
                        {
                            hekima->AI()->DoAction(ACTION_LAST_STAND);

                            std::list<Creature*> trolls;
                            GetCreatureListWithEntryInGrid(trolls, hekima, NPC_FROSTMANE_HEADHUNTER, 42.0f);
                            GetCreatureListWithEntryInGrid(trolls, hekima, NPC_FROSTMANE_WITCH_DOCTOR, 42.0f);

                            for (auto&& itr : trolls)
                            {
                                itr->CastSpell(itr, SPELL_PERM_FEIGN_DEATH, true);
                                itr->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_PACIFIED | UNIT_FLAG_NOT_SELECTABLE);
                            }

                            events.ScheduleEvent(2, 1000);
                        }
                        break;
                    case DATA_VICTORY_OF_DEATH:
                        if (data == DONE && !(chapterSix & 1))
                        {
                            chapterSix |= 1;
                            SendScenarioCriteria(32655);
                        }
                        else if (data == IN_PROGRESS && !(chapterSix & 2))
                        {
                            chapterSix |= 2;
                            SendScenarioCriteria(32656);
                        }

                        if (chapterSix == DONE && !scenarioCompleted)
                        {
                            scenarioCompleted = true;
                            // Last Outscene with Varian
                            if (Creature* varian = instance->GetCreature(GetGuidData(NPC_KING_VARIAN_WRYNN + 1)))
                                varian->AI()->DoAction(ACTION_LAST_STAND);

                            if (Scenario* scenario = GetScenario())
                                scenario->Reward(false, scenario->GetCurrentStep());

                            SaveToDB();
                        }
                        break;
                    case DATA_SHIMMERWEED:
                        if (++shimmerweedCount > 7)
                            DoCastSpellOnPlayers(SPELL_SHIMMERWEED_ACHIEV);
                        break;
                }

                if (data == DONE && type != DATA_VICTORY_OF_DEATH)
                    SaveToDB();
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_MOUNTAINEERS:
                        return chapterOne;
                    case DATA_SAVE_SCOUT_STONEBEARD:
                        return chapterTwo;
                    case DATA_SAVE_SCOUT_BOLD_BREW:
                        return chapterThree;
                    case DATA_SAVE_SCOUT_FORGEFELLOW:
                        return chapterFour;
                    case DATA_COLD_CAVE:
                        return chapterFive;
                    case DATA_VICTORY_OF_DEATH:
                        return chapterSix;
                    case DATA_SHIMMERWEED:
                        return shimmerweedCount;
                }

                return 0;
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case NPC_MOUNTAINER_GRIMBOLT:
                        return grimboltGUID;
                    case NPC_SCOUT_STONEBEARD:
                        return stonebeardGUID;
                    case NPC_SCOUT_FORGEFELLOW:
                        return forgeFellowGUID;
                    case NPC_SCOUT_BOLDBREW:
                        return boldBrewGUID;
                    case NPC_KING_VARIAN_WRYNN:
                        return varianIntroGUID;
                    case NPC_KING_VARIAN_WRYNN + 1:
                        return varianOutroGUID;
                    case NPC_MOIRA_THAURISSAN:
                        return moiraIntroGUID;
                    case NPC_MOIRA_THAURISSAN + 1:
                        return moiraOutroGUID;
                    case NPC_ROASTING_SPIT:
                        return roastingSpitGUID;
                    case NPC_HEKIMA_THE_WISE:
                        return hekimaGUID;
                    case NPC_FARASTU:
                        return farastuGUID;
                }

                return ObjectGuid::Empty;
            }

            void Update(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case 1:
                            if (Creature* varian = instance->GetCreature(GetGuidData(NPC_KING_VARIAN_WRYNN)))
                                varian->AI()->DoAction(ACTION_START_INTRO);
                            break;
                        case 2:
                            if (Creature* moira = instance->GetCreature(GetGuidData(NPC_MOIRA_THAURISSAN + 1)))
                            {
                                // Check Trolls every 2s
                                std::list<Creature*> trolls;
                                GetCreatureListWithEntryInGrid(trolls, moira, NPC_FROSTMANE_SNOWSTALKER, 35.0f);
                                GetCreatureListWithEntryInGrid(trolls, moira, NPC_FROSTMANE_BERSERKER, 35.0f);
                                GetCreatureListWithEntryInGrid(trolls, moira, NPC_FROSTMANE_BONECHILLER, 35.0f);

                                trolls.remove_if([=](Creature* target) { return target && !target->IsAlive(); });

                                if (trolls.empty())
                                {
                                    SetData(DATA_VICTORY_OF_DEATH, IN_PROGRESS);
                                }
                                else
                                    events.ScheduleEvent(2, 2000);

                            }
                            break;
                    }
                }
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
                saveStream << "B I S " << chapterOne << ' ' << chapterTwo << ' ' << chapterThree << ' ' << chapterFour << ' ' << chapterFive << ' ' << chapterSix;
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

                if (dataHead1 == 'B' && dataHead2 == 'I'&& dataHead3 == 'S')
                {
                    uint32 temp = 0;
                    loadStream >> temp; // chapterOne complete
                    chapterOne = temp;
                    SetData(DATA_MOUNTAINEERS, chapterOne);
                    loadStream >> temp; // chapterTwo complete
                    chapterTwo = temp;
                    SetData(DATA_SAVE_SCOUT_STONEBEARD, chapterTwo);
                    loadStream >> temp; // chapterThree complete
                    chapterThree = temp;
                    SetData(DATA_SAVE_SCOUT_BOLD_BREW, chapterThree);
                    loadStream >> temp; // chapterFour complete
                    chapterFour = temp;
                    SetData(DATA_SAVE_SCOUT_FORGEFELLOW, chapterFour);
                    loadStream >> temp; // chapterFive complete
                    chapterFive = temp;
                    SetData(DATA_COLD_CAVE, chapterFive);
                    loadStream >> temp; // chapterSix complete
                    chapterSix = temp;
                    scenarioCompleted = chapterSix == DONE;
                }
                else OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_blood_in_the_snow_InstanceMapScript(map);
        }
};

void AddSC_instance_blood_in_the_snow()
{
    new instance_blood_in_the_snow();
}
