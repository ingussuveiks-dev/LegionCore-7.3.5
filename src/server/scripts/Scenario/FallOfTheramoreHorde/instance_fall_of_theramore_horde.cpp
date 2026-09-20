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
#include "fall_of_theramore_horde.h"
#include "ScenarioMgr.h"
#include "Scenario.h"

class instance_fall_of_theramore_horde : public InstanceMapScript
{
    public:
        instance_fall_of_theramore_horde() : InstanceMapScript("instance_fall_of_theramore_horde", 999) { }

        struct instance_fall_of_theramore_horde_InstanceMapScript : public InstanceScript
        {
            instance_fall_of_theramore_horde_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

            std::map<uint32, ObjectGuid> encounters;
            std::list<ObjectGuid> guardGuids;
            uint32 chapterOne, chapterTwo, chapterThree, chapterFour, chapterFive;
            uint32 barrelsExploded, gryphonsDied, baldrucDied, tanksDestroyed;
            uint32 bigBessaDied, hedricDied, captainsSlain;

            void Initialize() override
            {
                SetBossNumber(CHAPTERS);
                chapterOne = NOT_STARTED;
                chapterTwo = NOT_STARTED;
                chapterThree = NOT_STARTED;
                chapterFour = NOT_STARTED;
                chapterFive = NOT_STARTED;
                barrelsExploded = 0;
                gryphonsDied = 0;
                baldrucDied = 0;
                tanksDestroyed = 0;
                bigBessaDied = 0;
                hedricDied = 0;
                captainsSlain = 0;
                encounters.clear();
                guardGuids.clear();
            }

            void OnPlayerEnter(Player* player) override
            {
                if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                {
                    uint8 step = DATA_TO_THE_WATERLINE;
                    if (chapterOne == DONE)
                        step = DATA_THE_BLASTMASTER;
                    if (chapterTwo == DONE)
                        step = DATA_GRYPHON_DOWN;
                    if (chapterThree == DONE)
                        step = DATA_TANKS_FOR_NOTHING;
                    if (chapterFour == DONE)
                        step = DATA_FLY_SPY;
                    scenario->SetCurrentStep(step);
                }
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_BLASTMASTER_SPARKFUSE:
                    case NPC_CAPTAIN_DROK:
                        encounters[creature->GetEntry()] = creature->GetGUID();
                        break;
                    case NPC_SABOTAGED_TANK:
                        creature->CastSpell(creature, SPELL_TANK_EXPLOSION, true);
                        creature->DealDamage(creature, creature->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
                        break;
                    case NPC_THERAMORE_ARCANIST:
                    case NPC_THERAMORE_FOOTMAN:
                    case NPC_THERAMORE_GRYPHON:
                    case NPC_THERAMORE_MARKSMAN:
                    case NPC_THERAMORE_OFFICER:
                    case NPC_KNIGHT_OF_THERAMORE:
                    case NPC_BIG_BESSA:
                    case NPC_UNMANNED_TANK:
                    case NPC_BALDRUC:
                    case NPC_HEDRIC_EVENCANE:
                    case NPC_THERAMORE_FAITHFUL:
                        if (creature->GetPositionX() > -3869.0f)
                        {
                            guardGuids.push_back(creature->GetGUID());
                            creature->SetVisible(false);
                        }
                        else
                            creature->setFaction(1077);
                        break;
                    case NPC_THALEN_SONGWEAVER:
                        encounters[creature->GetEntry()] = creature->GetGUID();
                        guardGuids.push_back(creature->GetGUID());
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
                    case NPC_BALDRUC:
                        baldrucDied = 1;
                        SetData(DATA_GRYPHON_DOWN, SPECIAL);
                        break;
                    case NPC_THERAMORE_GRYPHON:
                        ++gryphonsDied;
                        SetData(DATA_GRYPHON_DOWN, SPECIAL);
                        break;
                    case NPC_BIG_BESSA:
                        bigBessaDied = 1;
                        SetData(DATA_TANKS_FOR_NOTHING, SPECIAL);
                        break;
                    case NPC_HEDRIC_EVENCANE:
                        hedricDied = 1;
                        SetData(DATA_FLY_SPY, SPECIAL);
                        break;
                    case NPC_CAPTAIN_TELLERN:
                    case NPC_SPELLSHARPER_LANARA:
                    case NPC_CAPTAIN_DASHING:
                        ++captainsSlain;
                        SetData(DATA_TO_THE_WATERLINE, SPECIAL);
                        break;
                }
            }

            void ActivateSpawnRegion(std::initializer_list<uint32> entries)
            {
                for (ObjectGuid guid : guardGuids)
                    if (Creature* guard = instance->GetCreature(guid))
                        if (std::find(entries.begin(), entries.end(), guard->GetEntry()) != entries.end())
                            guard->AI()->DoAction(ACTION_ACTIVATE_BY_REGION);
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_TO_THE_WATERLINE:
                        if (data <= 6)
                            barrelsExploded = data;

                        if (barrelsExploded >= 6 && captainsSlain >= 3 && chapterOne != DONE)
                        {
                            chapterOne = DONE;
                            SetBossState(type, DONE);
                            if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                                scenario->SetCurrentStep(DATA_THE_BLASTMASTER);
                        }
                        break;
                    case DATA_THE_BLASTMASTER:
                        if (data == DONE && chapterTwo != DONE)
                        {
                            chapterTwo = DONE;
                            SetBossState(type, DONE);
                            if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                                scenario->SetCurrentStep(DATA_GRYPHON_DOWN);
                        }
                        break;
                    case DATA_GRYPHON_DOWN:
                        if (baldrucDied && gryphonsDied >= 2 && chapterThree != DONE)
                        {
                            chapterThree = DONE;
                            SetBossState(type, DONE);
                            if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                                scenario->SetCurrentStep(DATA_TANKS_FOR_NOTHING);
                        }
                        break;
                    case DATA_TANKS_FOR_NOTHING:
                        if (data == IN_PROGRESS && tanksDestroyed < 3)
                            ++tanksDestroyed;

                        if (bigBessaDied && tanksDestroyed >= 3 && chapterFour != DONE)
                        {
                            chapterFour = DONE;
                            SetBossState(type, DONE);
                            if (Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId()))
                                scenario->SetCurrentStep(DATA_FLY_SPY);
                        }
                        break;
                    case DATA_FLY_SPY:
                        if (data == DONE && hedricDied && chapterFive != DONE)
                        {
                            chapterFive = DONE;
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
                    case DATA_TO_THE_WATERLINE: return barrelsExploded < 6 ? barrelsExploded : chapterOne;
                    case DATA_THE_BLASTMASTER:  return chapterTwo;
                    case DATA_GRYPHON_DOWN:     return chapterThree;
                    case DATA_TANKS_FOR_NOTHING:return chapterFour;
                    case DATA_FLY_SPY:          return chapterFive;
                    default:                    return 0;
                }
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                auto itr = encounters.find(type);
                return itr != encounters.end() ? itr->second : ObjectGuid::Empty;
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                if (state != DONE)
                    return true;

                switch (type)
                {
                    case DATA_TO_THE_WATERLINE:
                        if (Creature* sparkfuse = instance->GetCreature(GetGuidData(NPC_BLASTMASTER_SPARKFUSE)))
                            sparkfuse->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        break;
                    case DATA_THE_BLASTMASTER:
                        ActivateSpawnRegion({ NPC_THERAMORE_FOOTMAN, NPC_THERAMORE_ARCANIST, NPC_THERAMORE_MARKSMAN,
                            NPC_THERAMORE_OFFICER, NPC_THERAMORE_GRYPHON, NPC_KNIGHT_OF_THERAMORE,
                            NPC_BALDRUC, NPC_THERAMORE_FAITHFUL });
                        break;
                    case DATA_GRYPHON_DOWN:
                        ActivateSpawnRegion({ NPC_BIG_BESSA, NPC_UNMANNED_TANK });
                        break;
                    case DATA_TANKS_FOR_NOTHING:
                        ActivateSpawnRegion({ NPC_THALEN_SONGWEAVER, NPC_HEDRIC_EVENCANE });
                        break;
                }

                return true;
            }

            std::string GetSaveData() override
            {
                OUT_SAVE_INST_DATA;
                std::ostringstream stream;
                stream << "F O T H " << barrelsExploded << ' ' << captainsSlain << ' '
                    << gryphonsDied << ' ' << baldrucDied << ' ' << tanksDestroyed << ' '
                    << bigBessaDied << ' ' << hedricDied << ' ' << chapterOne << ' '
                    << chapterTwo << ' ' << chapterThree << ' ' << chapterFour << ' ' << chapterFive;
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
                if (h1 != 'F' || h2 != 'O' || h3 != 'T' || h4 != 'H')
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                stream >> barrelsExploded >> captainsSlain >> gryphonsDied >> baldrucDied
                    >> tanksDestroyed >> bigBessaDied >> hedricDied >> chapterOne
                    >> chapterTwo >> chapterThree >> chapterFour >> chapterFive;

                chapterOne = chapterOne == IN_PROGRESS ? NOT_STARTED : chapterOne;
                chapterTwo = chapterTwo == IN_PROGRESS ? NOT_STARTED : chapterTwo;
                chapterThree = chapterThree == IN_PROGRESS ? NOT_STARTED : chapterThree;
                chapterFour = chapterFour == IN_PROGRESS ? NOT_STARTED : chapterFour;
                chapterFive = chapterFive == IN_PROGRESS ? NOT_STARTED : chapterFive;

                SetBossState(DATA_TO_THE_WATERLINE, EncounterState(chapterOne));
                SetBossState(DATA_THE_BLASTMASTER, EncounterState(chapterTwo));
                SetBossState(DATA_GRYPHON_DOWN, EncounterState(chapterThree));
                SetBossState(DATA_TANKS_FOR_NOTHING, EncounterState(chapterFour));
                SetBossState(DATA_FLY_SPY, EncounterState(chapterFive));
                OUT_LOAD_INST_DATA_COMPLETE;
            }
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_fall_of_theramore_horde_InstanceMapScript(map);
        }
};

void AddSC_instance_fall_of_theramore_horde()
{
    new instance_fall_of_theramore_horde();
}
