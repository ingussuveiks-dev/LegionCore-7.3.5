/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "domination_point.h"
#include "ScriptedCreature.h"

namespace
{
constexpr uint32 ACTION_DOMINATION_POINT_INTERACT = GOSSIP_ACTION_INFO_DEF + 1;

bool GetInteraction(uint32 entry, uint32 step, char const*& text, uint32& data)
{
    switch (entry)
    {
        case NPC_KROMTHAR:
            if (step == 0)
            {
                text = "Take us to General Nazgrim.";
                data = DATA_JOIN_KROMTHAR;
                return true;
            }
            break;
        case NPC_GENERAL_NAZGRIM:
            if (step == 1)
            {
                text = "The refinery is secure. What's next?";
                data = DATA_JOIN_NAZGRIM;
                return true;
            }
            break;
        case NPC_WARLORD_BLOODHILT:
            if (step == 3)
            {
                text = "Warlord, we're here to reinforce the keep.";
                data = DATA_LOCATE_BLOODHILT;
                return true;
            }
            break;
        case NPC_SHOKIA:
            if (step == 4)
            {
                text = "Shokia, report.";
                data = DATA_SHOKIA_REPORT;
                return true;
            }
            break;
        case NPC_SHADEMASTER_KIRYN:
            if (step == 4)
            {
                text = "Kiryn, report.";
                data = DATA_KIRYN_REPORT;
                return true;
            }
            break;
        case NPC_RIVETT_CLUTCHPOP:
            if (step == 4)
            {
                text = "Rivett, report.";
                data = DATA_RIVETT_REPORT;
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}
}

class npc_domination_point_interaction : public CreatureScript
{
public:
    npc_domination_point_interaction() : CreatureScript("npc_domination_point_interaction") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance)
            return true;

        char const* text = nullptr;
        uint32 data = 0;
        if (!GetInteraction(creature->GetEntry(), instance->getScenarionStep(), text, data))
            return true;

        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, text, GOSSIP_SENDER_MAIN, ACTION_DOMINATION_POINT_INTERACT);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->CLOSE_GOSSIP_MENU();
        if (action != ACTION_DOMINATION_POINT_INTERACT)
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new ScriptedAI(creature);
    }
};

void AddSC_domination_point()
{
    new npc_domination_point_interaction();
}
