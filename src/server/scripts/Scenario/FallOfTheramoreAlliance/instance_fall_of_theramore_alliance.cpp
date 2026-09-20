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
#include "fall_of_theramore_alliance.h"
#include "ScenarioMgr.h"
#include "Scenario.h"

class instance_fall_of_theramore_alliance : public InstanceMapScript
{
    public:
        instance_fall_of_theramore_alliance() : InstanceMapScript("instance_fall_of_theramore_alliance", 1000) { }

        struct instance_fall_of_theramore_alliance_InstanceMapScript : public InstanceScript
        {
            instance_fall_of_theramore_alliance_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

            std::list<ObjectGuid> beachPhaseGuids;
            std::list<ObjectGuid> ruinsPhaseGuids;
            ObjectGuid jainaGuid;
            uint32 captainsSlain;
            uint32 barrelsExploded;
            uint32 harborCaptainsSlain;
            uint32 harborTrashSlain;
            uint32 wyvernsSlain;
            uint32 gashnulSlain;
            uint32 stolenStandards;
            uint32 gatecrusherSlain;
            uint32 chapterOne;
            uint32 chapterTwo;
            uint32 chapterThree;
            uint32 chapterFour;
            uint32 chapterFive;
            uint32 chapterSix;

            void Initialize() override
            {
                SetBossNumber(CHAPTERS);
                beachPhaseGuids.clear();
                ruinsPhaseGuids.clear();
                jainaGuid.Clear();
                captainsSlain = 0;
                barrelsExploded = 0;
                harborCaptainsSlain = 0;
                harborTrashSlain = 0;
                wyvernsSlain = 0;
                gashnulSlain = 0;
                stolenStandards = 0;
                gatecrusherSlain = 0;
                chapterOne = NOT_STARTED;
                chapterTwo = NOT_STARTED;
                chapterThree = NOT_STARTED;
                chapterFour = NOT_STARTED;
                chapterFive = NOT_STARTED;
                chapterSix = NOT_STARTED;
            }

            void OnPlayerEnter(Player* player) override
            {
                if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                {
                    uint8 step = DATA_WELCOME_TO_THERAMORE;
                    if (chapterOne == DONE)
                        step = DATA_SWEEP_THE_HARBOR;
                    if (chapterTwo == DONE)
                        step = DATA_INTO_THE_RUINS;
                    if (chapterThree == DONE)
                        step = DATA_LADY_PROUDMOORE;
                    if (chapterFour == DONE)
                        step = DATA_DESTROY_THE_DESTROYER;
                    if (chapterFive == DONE)
                        step = DATA_LAST_STAND;
                    scenario->SetCurrentStep(step);
                }

                if (chapterOne == NOT_STARTED)
                    player->CastSpell(player, SPELL_THERAMORE_EXPLOSION_CINEMATIC, true);
            }

            bool IsInitialWaveGuard(Creature const* creature) const
            {
                if (creature->GetEntry() == NPC_SERGEANT_GRUD || creature->GetEntry() == NPC_WAVE_CALLER_ZULGA)
                    return true;

                return creature->GetEntry() == NPC_ROKNAH_GRUNT &&
                    creature->GetDistance2d(-3964.0f, -4686.0f) < 20.0f;
            }

            void ActivateCreature(Creature* creature)
            {
                creature->SetVisible(true);
                creature->AI()->DoAction(ACTION_ACTIVATE_BY_REGION);
            }

            void OnCreatureCreate(Creature* creature) override
            {
                if (IsInitialWaveGuard(creature))
                {
                    creature->setFaction(14);
                    return;
                }

                switch (creature->GetEntry())
                {
                    case NPC_LEAKED_OIL_DRUM:
                    case NPC_ROKNAH_GRUNT:
                    case NPC_ROKNAH_SKRIMISHER:
                    case NPC_ROKNAH_FELCASTER:
                    case NPC_ROKNAH_HEADHUNTER:
                    case NPC_ROKNAH_LOA_SINGER:
                    case NPC_ROKNAH_HAG:
                    case NPC_ROKNAH_RIDER:
                    case NPC_AIRSHIP_MARINE:
                    case NPC_AIRSHIP_CREWMAN:
                    case NPC_VICIOUS_WYVERN:
                    case NPC_SKY_CAPTAIN_DAZRIP:
                    case NPC_CAPTAIN_MOUSSON:
                    case NPC_CAPTAIN_KORTHOK:
                    case NPC_GASHNUL:
                    case NPC_ROKNAH_WAVE_CALLER:
                        if (creature->GetAreaId() == 6500 || creature->GetPositionX() < -3861.92f)
                        {
                            beachPhaseGuids.push_back(creature->GetGUID());
                            if (chapterOne == DONE)
                                ActivateCreature(creature);
                            else
                                creature->SetVisible(false);
                        }
                        else if (creature->GetAreaId() == 6502 && creature->GetPositionX() > -3861.92f)
                        {
                            ruinsPhaseGuids.push_back(creature->GetGUID());
                            if (chapterTwo == DONE)
                                ActivateCreature(creature);
                            else
                                creature->SetVisible(false);
                        }
                        else
                            creature->setFaction(14);
                        break;
                    case NPC_JAINA_PROUDMOORE:
                        jainaGuid = creature->GetGUID();
                        ruinsPhaseGuids.push_back(creature->GetGUID());
                        if (chapterThree == DONE)
                            ActivateCreature(creature);
                        else
                            creature->SetVisible(false);
                        break;
                }
            }

            void OnUnitDeath(Unit* unit) override
            {
                if (!unit->ToCreature())
                    return;

                switch (unit->GetEntry())
                {
                    case NPC_SERGEANT_GRUD:
                    case NPC_WAVE_CALLER_ZULGA:
                        SetData(DATA_WELCOME_TO_THERAMORE, SPECIAL);
                        break;
                    case NPC_CAPTAIN_KORTHOK:
                    case NPC_CAPTAIN_MOUSSON:
                    case NPC_SKY_CAPTAIN_DAZRIP:
                        SetData(DATA_SWEEP_THE_HARBOR, SPECIAL + 1);
                        break;
                    case NPC_ROKNAH_GRUNT:
                    case NPC_ROKNAH_SKRIMISHER:
                    case NPC_ROKNAH_FELCASTER:
                    case NPC_ROKNAH_HEADHUNTER:
                    case NPC_ROKNAH_LOA_SINGER:
                    case NPC_ROKNAH_HAG:
                    case NPC_ROKNAH_RIDER:
                    case NPC_AIRSHIP_MARINE:
                    case NPC_AIRSHIP_CREWMAN:
                        if (chapterTwo != DONE)
                            SetData(DATA_SWEEP_THE_HARBOR, SPECIAL + 2);
                        break;
                    case NPC_VICIOUS_WYVERN:
                        ++wyvernsSlain;
                        SetData(DATA_INTO_THE_RUINS, SPECIAL);
                        break;
                    case NPC_GASHNUL:
                        gashnulSlain = 1;
                        SetData(DATA_INTO_THE_RUINS, SPECIAL);
                        break;
                    case NPC_GATECRUSHER:
                        gatecrusherSlain = 1;
                        SetData(DATA_DESTROY_THE_DESTROYER, SPECIAL);
                        break;
                }
            }

            void ActivateSpawnRegion(std::initializer_list<uint32> entries, std::list<ObjectGuid> const& guids)
            {
                for (ObjectGuid guid : guids)
                    if (Creature* guard = instance->GetCreature(guid))
                        if (std::find(entries.begin(), entries.end(), guard->GetEntry()) != entries.end())
                            ActivateCreature(guard);
            }

            void AdvanceScenario(uint8 step)
            {
                if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                    scenario->SetCurrentStep(step);
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_WELCOME_TO_THERAMORE:
                        if (data == SPECIAL)
                            ++captainsSlain;

                        if (captainsSlain >= 2 && chapterOne != DONE)
                        {
                            chapterOne = DONE;
                            harborTrashSlain = 0;
                            SetBossState(type, DONE);
                            AdvanceScenario(DATA_SWEEP_THE_HARBOR);
                        }
                        break;
                    case DATA_SWEEP_THE_HARBOR:
                        if (data == SPECIAL)
                            ++barrelsExploded;
                        else if (data == SPECIAL + 1)
                            ++harborCaptainsSlain;
                        else if (data == SPECIAL + 2)
                            ++harborTrashSlain;

                        if (barrelsExploded >= 3 && harborCaptainsSlain >= 3 && harborTrashSlain >= 10 && chapterTwo != DONE)
                        {
                            chapterTwo = DONE;
                            SetBossState(type, DONE);
                            AdvanceScenario(DATA_INTO_THE_RUINS);
                        }
                        break;
                    case DATA_INTO_THE_RUINS:
                        if (wyvernsSlain >= 2 && gashnulSlain && chapterThree != DONE)
                        {
                            chapterThree = DONE;
                            SetBossState(type, DONE);
                            AdvanceScenario(DATA_LADY_PROUDMOORE);
                        }
                        break;
                    case DATA_LADY_PROUDMOORE:
                        if (data == DONE && chapterFour != DONE)
                        {
                            chapterFour = DONE;
                            SetBossState(type, DONE);
                            AdvanceScenario(DATA_DESTROY_THE_DESTROYER);
                        }
                        break;
                    case DATA_DESTROY_THE_DESTROYER:
                        if (data == SPECIAL + 1)
                            ++stolenStandards;

                        if (stolenStandards >= 3 && gatecrusherSlain && chapterFive != DONE)
                        {
                            chapterFive = DONE;
                            SetBossState(type, DONE);
                            AdvanceScenario(DATA_LAST_STAND);
                        }
                        break;
                    case DATA_LAST_STAND:
                        if (data == DONE && chapterSix != DONE)
                        {
                            chapterSix = DONE;
                            SetBossState(type, DONE);
                            if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                                scenario->Reward(false, scenario->GetCurrentStep());
                        }
                        break;
                }

                SaveToDB();
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_WELCOME_TO_THERAMORE:  return chapterOne;
                    case DATA_SWEEP_THE_HARBOR:      return chapterTwo;
                    case DATA_INTO_THE_RUINS:        return chapterThree;
                    case DATA_LADY_PROUDMOORE:       return chapterFour;
                    case DATA_DESTROY_THE_DESTROYER: return chapterFive;
                    case DATA_LAST_STAND:            return chapterSix;
                    default:                         return 0;
                }
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                return type == NPC_JAINA_PROUDMOORE ? jainaGuid : ObjectGuid::Empty;
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                if (state != DONE)
                    return true;

                switch (type)
                {
                    case DATA_WELCOME_TO_THERAMORE:
                        ActivateSpawnRegion({ NPC_AIRSHIP_CREWMAN, NPC_AIRSHIP_MARINE, NPC_ROKNAH_HAG,
                            NPC_ROKNAH_GRUNT, NPC_ROKNAH_FELCASTER, NPC_ROKNAH_HEADHUNTER,
                            NPC_ROKNAH_LOA_SINGER, NPC_ROKNAH_RIDER, NPC_ROKNAH_SKRIMISHER,
                            NPC_LEAKED_OIL_DRUM, NPC_SKY_CAPTAIN_DAZRIP, NPC_CAPTAIN_KORTHOK,
                            NPC_CAPTAIN_MOUSSON }, beachPhaseGuids);
                        break;
                    case DATA_SWEEP_THE_HARBOR:
                        ActivateSpawnRegion({ NPC_ROKNAH_HAG, NPC_ROKNAH_GRUNT, NPC_ROKNAH_FELCASTER,
                            NPC_ROKNAH_HEADHUNTER, NPC_ROKNAH_LOA_SINGER, NPC_ROKNAH_RIDER,
                            NPC_ROKNAH_SKRIMISHER, NPC_VICIOUS_WYVERN, NPC_GASHNUL,
                            NPC_ROKNAH_WAVE_CALLER }, ruinsPhaseGuids);
                        break;
                    case DATA_INTO_THE_RUINS:
                        ActivateSpawnRegion({ NPC_JAINA_PROUDMOORE }, ruinsPhaseGuids);
                        break;
                    case DATA_LADY_PROUDMOORE:
                        instance->SummonCreature(NPC_GATECRUSHER, SiegeMachineSpawnPos);
                        break;
                }

                return true;
            }

            std::string GetSaveData() override
            {
                OUT_SAVE_INST_DATA;
                std::ostringstream stream;
                stream << "F O T A " << captainsSlain << ' ' << barrelsExploded << ' '
                    << harborCaptainsSlain << ' ' << harborTrashSlain << ' ' << wyvernsSlain << ' '
                    << gashnulSlain << ' ' << stolenStandards << ' ' << gatecrusherSlain << ' '
                    << chapterOne << ' ' << chapterTwo << ' ' << chapterThree << ' '
                    << chapterFour << ' ' << chapterFive << ' ' << chapterSix;
                OUT_SAVE_INST_DATA_COMPLETE;
                return stream.str();
            }

            void Load(char const* in) override
            {
                if (!in)
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                OUT_LOAD_INST_DATA(in);
                char h1, h2, h3, h4;
                std::istringstream stream(in);
                stream >> h1 >> h2 >> h3 >> h4;
                if (h1 != 'F' || h2 != 'O' || h3 != 'T' || h4 != 'A')
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                stream >> captainsSlain >> barrelsExploded >> harborCaptainsSlain >> harborTrashSlain
                    >> wyvernsSlain >> gashnulSlain >> stolenStandards >> gatecrusherSlain
                    >> chapterOne >> chapterTwo >> chapterThree >> chapterFour >> chapterFive >> chapterSix;

                chapterOne = chapterOne == IN_PROGRESS ? NOT_STARTED : chapterOne;
                chapterTwo = chapterTwo == IN_PROGRESS ? NOT_STARTED : chapterTwo;
                chapterThree = chapterThree == IN_PROGRESS ? NOT_STARTED : chapterThree;
                chapterFour = chapterFour == IN_PROGRESS ? NOT_STARTED : chapterFour;
                chapterFive = chapterFive == IN_PROGRESS ? NOT_STARTED : chapterFive;
                chapterSix = chapterSix == IN_PROGRESS ? NOT_STARTED : chapterSix;

                SetBossState(DATA_WELCOME_TO_THERAMORE, EncounterState(chapterOne));
                SetBossState(DATA_SWEEP_THE_HARBOR, EncounterState(chapterTwo));
                SetBossState(DATA_INTO_THE_RUINS, EncounterState(chapterThree));
                SetBossState(DATA_LADY_PROUDMOORE, EncounterState(chapterFour));
                SetBossState(DATA_DESTROY_THE_DESTROYER, EncounterState(chapterFive));
                SetBossState(DATA_LAST_STAND, EncounterState(chapterSix));
                OUT_LOAD_INST_DATA_COMPLETE;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_fall_of_theramore_alliance_InstanceMapScript(map);
        }
};

void AddSC_instance_fall_of_theramore_alliance()
{
    new instance_fall_of_theramore_alliance();
}
