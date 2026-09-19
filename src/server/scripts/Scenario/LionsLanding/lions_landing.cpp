/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "lions_landing.h"
#include "ScriptedCreature.h"

namespace
{
constexpr uint32 ACTION_LIONS_LANDING_INTERACT = GOSSIP_ACTION_INFO_DEF + 1;

bool GetInteraction(uint32 entry, uint32 step, char const*& text, uint32& data)
{
    switch (entry)
    {
        case NPC_DAGGIN_WINDBEARD:
            if (step == 0)
            {
                text = "Let's get to Admiral Taylor.";
                data = DATA_JOIN_DAGGIN;
                return true;
            }
            break;
        case NPC_ADMIRAL_TAYLOR:
            if (step == 1)
            {
                text = "The docks are secure. What's next?";
                data = DATA_JOIN_TAYLOR;
                return true;
            }
            break;
        case NPC_HIGH_MARSHAL_TWINBRAID:
            if (step == 3)
            {
                text = "High Marshal, we're here to reinforce the keep.";
                data = DATA_LOCATE_TWINBRAID;
                return true;
            }
            break;
        case NPC_AMBER_KEARNEN:
            if (step == 4)
            {
                text = "Amber, report.";
                data = DATA_AMBER_REPORT;
                return true;
            }
            break;
        case NPC_MISHKA:
            if (step == 4)
            {
                text = "Mishka, report.";
                data = DATA_MISHKA_REPORT;
                return true;
            }
            break;
        case NPC_SULLY_MCLEARY:
            if (step == 4)
            {
                text = "Sully, report.";
                data = DATA_SULLY_REPORT;
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}
}

class npc_lions_landing_interaction : public CreatureScript
{
public:
    npc_lions_landing_interaction() : CreatureScript("npc_lions_landing_interaction") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance)
            return true;

        char const* text = nullptr;
        uint32 data = 0;
        if (!GetInteraction(creature->GetEntry(), instance->getScenarionStep(), text, data))
            return true;

        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, text, GOSSIP_SENDER_MAIN, ACTION_LIONS_LANDING_INTERACT);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->CLOSE_GOSSIP_MENU();
        if (action != ACTION_LIONS_LANDING_INTERACT)
            return true;

        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance)
            return true;

        char const* text = nullptr;
        uint32 data = 0;
        if (GetInteraction(creature->GetEntry(), instance->getScenarionStep(), text, data))
            instance->SetData(data, 1);

        return true;
    }

    struct npc_lions_landing_interactionAI : ScriptedAI
    {
        npc_lions_landing_interactionAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
        }

        void OnSpellClick(Unit* clicker) override
        {
            if (!clicker->IsPlayer())
                return;

            InstanceScript* instance = me->GetInstanceScript();
            if (!instance || instance->getScenarionStep() != 5)
                return;

            switch (me->GetEntry())
            {
                case NPC_PLACE_ROCKETS:
                    instance->SetData(DATA_PLACE_ROCKETS, 1);
                    break;
                case NPC_PLACE_BOMBS:
                    instance->SetData(DATA_PLACE_BOMBS, 1);
                    break;
                case NPC_PLACE_BOOMSTICKS:
                    instance->SetData(DATA_PLACE_BOOMSTICKS, 1);
                    break;
                default:
                    return;
            }

            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lions_landing_interactionAI(creature);
    }
};

void AddSC_lions_landing()
{
    new npc_lions_landing_interaction();
}
