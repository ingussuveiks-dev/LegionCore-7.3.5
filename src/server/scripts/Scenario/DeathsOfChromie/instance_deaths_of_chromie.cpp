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
#include "deaths_of_chromie.h"

#include <set>

namespace
{
Position const WyrmrestReturnPosition = { 13670.22f, 13062.12f, 342.80f, 0.91f };

bool IsChromieAlly(uint32 entry)
{
    switch (entry)
    {
        case NPC_CHROMIE_WYRMREST:
        case NPC_CHROMIE_HYJAL:
        case NPC_CHROMIE_WELL:
        case NPC_CHROMIE_STRATHOLME:
        case NPC_CHROMIE_ANDORHAL:
            return true;
        default:
            return false;
    }
}

bool IsTimelineBoss(uint32 entry)
{
    switch (entry)
    {
        case NPC_VOID_GARGANTUAN:
        case NPC_ZORATHIDES:
        case NPC_TALAR_ICECHILL:
        case NPC_THALAS_VYLETHORN:
        case NPC_FIERY_BEHEMOTH:
        case NPC_GROLETHAX:
        case NPC_NEZAR_AZRET:
        case NPC_HORDE_SIEGE_CANNON:
            return true;
        default:
            return false;
    }
}
}

class instance_deaths_of_chromie : public InstanceMapScript
{
public:
    instance_deaths_of_chromie() : InstanceMapScript("instance_deaths_of_chromie", MAP_DEATHS_OF_CHROMIE) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_deaths_of_chromie_InstanceMapScript(map);
    }

    struct instance_deaths_of_chromie_InstanceMapScript : public InstanceScript
    {
        instance_deaths_of_chromie_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

        std::set<uint32> DefeatedAttacks;
        uint32 AttemptElapsed = 0;
        bool AttemptStarted = false;

        void Initialize() override
        {
            DefeatedAttacks.clear();
            AttemptElapsed = 0;
            AttemptStarted = false;
        }

        void OnPlayerEnter(Player* /*player*/) override
        {
            if (getScenarionStep() == 1)
                AttemptStarted = true;
        }

        void OnCreatureCreate(Creature* creature) override
        {
            if (IsChromieAlly(creature->GetEntry()))
            {
                creature->setFaction(35);
                creature->SetReactState(REACT_PASSIVE);
                creature->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                return;
            }

            if (!IsTimelineBoss(creature->GetEntry()))
                return;

            creature->setFaction(14);
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 |
                UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE |
                UNIT_FLAG_PACIFIED | UNIT_FLAG_STUNNED);
            creature->SetReactState(REACT_AGGRESSIVE);
        }

        void OnUnitDeath(Unit* unit) override
        {
            uint32 entry = unit->GetEntry();
            if (getScenarionStep() != 1 || !IsTimelineBoss(entry) || !DefeatedAttacks.insert(entry).second)
                return;

            DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_STOP_ATTACK, 1, 0, unit);

            // Retail returns the player from the detached chronoportal maps through
            // a time exit. The scenario map stores all five floors in one MapID, so
            // return to the Wyrmrest hub after the kill without completing another
            // criterion on the player's behalf.
            AddDelayedEvent(1800, [this]()
            {
                for (auto const& reference : instance->GetPlayers())
                    if (Player* player = reference.getSource())
                        if (player->IsAlive())
                            player->NearTeleportTo(WyrmrestReturnPosition.GetPositionX(), WyrmrestReturnPosition.GetPositionY(),
                                WyrmrestReturnPosition.GetPositionZ(), WyrmrestReturnPosition.GetOrientation());
            });
        }

        void SetData(uint32 type, uint32 /*data*/) override
        {
            switch (type)
            {
                case DATA_SELECT_CHROMIE_TALENTS:
                    if (getScenarionStep() == 0)
                        DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_SELECT_TALENTS, 1);
                    break;
                case DATA_RETURN_TO_PRESENT:
                    if (getScenarionStep() != 2)
                        break;

                    DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_RETURN_TO_PRESENT, 1);
                    if (AttemptStarted && AttemptElapsed <= 15 * MINUTE * IN_MILLISECONDS)
                    {
                        // Let Scenario reward the three ordinary stages first, then
                        // complete the separately flagged fifteen-minute objective.
                        AddDelayedEvent(500, [this]()
                        {
                            DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_SAVE_ALL_CHROMIES, 1);
                        });
                    }
                    AttemptStarted = false;
                    break;
                default:
                    break;
            }
        }

        void onScenarionNextStep(uint32 newStep) override
        {
            if (newStep == 1)
            {
                AttemptElapsed = 0;
                AttemptStarted = true;
            }
        }

        void Update(uint32 diff) override
        {
            if (AttemptStarted && AttemptElapsed < 60 * MINUTE * IN_MILLISECONDS)
                AttemptElapsed += diff;
        }
    };
};

void AddSC_instance_deaths_of_chromie()
{
    new instance_deaths_of_chromie();
}
