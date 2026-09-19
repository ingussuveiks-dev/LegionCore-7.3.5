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

namespace
{
enum ChromieGossipActions
{
    ACTION_SELECT_TALENTS = GOSSIP_ACTION_INFO_DEF + 1,
    ACTION_RETURN_PRESENT,
    ACTION_RETURN_WYRMREST,
    ACTION_AZURE_SHRINE,
    ACTION_OBSIDIAN_SHRINE,
    ACTION_RUBY_SHRINE,
    ACTION_EMERALD_SHRINE,
    ACTION_ANDORHAL,
    ACTION_HYJAL,
    ACTION_STRATHOLME,
    ACTION_WELL_OF_ETERNITY
};

struct ChromieDestination
{
    uint32 Action;
    char const* Label;
    Position Pos;
};

Position const WyrmrestPosition = { 13670.22f, 13062.12f, 342.80f, 0.91f };

ChromieDestination const TimelineDestinations[] =
{
    { ACTION_AZURE_SHRINE,       "Travel to the Azure Dragonshrine.",       { 13240.16f, 13258.41f,   21.80f, 0.00f } },
    { ACTION_OBSIDIAN_SHRINE,    "Travel to the Obsidian Dragonshrine.",    { 14534.07f, 14295.95f,  126.50f, 0.00f } },
    { ACTION_RUBY_SHRINE,        "Travel to the Ruby Dragonshrine.",        { 13864.68f, 13751.94f,   54.00f, 0.00f } },
    { ACTION_EMERALD_SHRINE,     "Travel into the Emerald Nightmare.",      { 12997.09f, 12849.00f, -155.80f, 0.00f } },
    { ACTION_ANDORHAL,           "Enter the War for Andorhal.",             { 14231.68f,  4804.27f,   35.00f, 0.00f } },
    { ACTION_HYJAL,              "Enter the Burning of Mount Hyjal.",       {  8816.79f,  4054.42f,  968.00f, 0.00f } },
    { ACTION_STRATHOLME,         "Enter the Culling of Stratholme.",        {  8702.87f, 14812.73f,  159.00f, 0.00f } },
    { ACTION_WELL_OF_ETERNITY,   "Enter the Well of Eternity.",             {  2733.35f, 10026.72f,   34.00f, 0.00f } }
};

ChromieDestination const* FindDestination(uint32 action)
{
    for (ChromieDestination const& destination : TimelineDestinations)
        if (destination.Action == action)
            return &destination;
    return nullptr;
}

Position const* GetPortalDestination(uint32 entry)
{
    switch (entry)
    {
        case GO_PORTAL_ANDORHAL:         return &TimelineDestinations[4].Pos;
        case GO_PORTAL_HYJAL:            return &TimelineDestinations[5].Pos;
        case GO_PORTAL_STRATHOLME:       return &TimelineDestinations[6].Pos;
        case GO_PORTAL_WELL_OF_ETERNITY: return &TimelineDestinations[7].Pos;
        default:                         return nullptr;
    }
}

enum ChromieCombatEvents
{
    EVENT_PRIMARY = 1,
    EVENT_SECONDARY,
    EVENT_TERTIARY
};
}

class npc_deaths_of_chromie : public CreatureScript
{
public:
    npc_deaths_of_chromie() : CreatureScript("npc_deaths_of_chromie") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance)
            return true;

        if (creature->GetEntry() != NPC_CHROMIE_WYRMREST)
        {
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "Return to Wyrmrest Temple.", GOSSIP_SENDER_MAIN, ACTION_RETURN_WYRMREST);
        }
        else
        {
            switch (instance->getScenarionStep())
            {
                case 0:
                    player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "Select Chromie's talents and begin the attempt.", GOSSIP_SENDER_MAIN, ACTION_SELECT_TALENTS);
                    break;
                case 1:
                    for (ChromieDestination const& destination : TimelineDestinations)
                        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, destination.Label, GOSSIP_SENDER_MAIN, destination.Action);
                    break;
                case 2:
                    player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "Return to present-day Wyrmrest.", GOSSIP_SENDER_MAIN, ACTION_RETURN_PRESENT);
                    break;
                default:
                    break;
            }
        }

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->CLOSE_GOSSIP_MENU();

        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance)
            return true;

        if (action == ACTION_SELECT_TALENTS)
        {
            instance->SetData(DATA_SELECT_CHROMIE_TALENTS, 1);
            return true;
        }

        if (action == ACTION_RETURN_PRESENT)
        {
            instance->SetData(DATA_RETURN_TO_PRESENT, 1);
            return true;
        }

        if (action == ACTION_RETURN_WYRMREST)
        {
            player->NearTeleportTo(WyrmrestPosition.GetPositionX(), WyrmrestPosition.GetPositionY(),
                WyrmrestPosition.GetPositionZ(), WyrmrestPosition.GetOrientation());
            return true;
        }

        if (instance->getScenarionStep() == 1)
            if (ChromieDestination const* destination = FindDestination(action))
                player->NearTeleportTo(destination->Pos.GetPositionX(), destination->Pos.GetPositionY(),
                    destination->Pos.GetPositionZ(), destination->Pos.GetOrientation());

        return true;
    }
};

class npc_deaths_of_chromie_combat : public CreatureScript
{
public:
    npc_deaths_of_chromie_combat() : CreatureScript("npc_deaths_of_chromie_combat") { }

    struct npc_deaths_of_chromie_combatAI : public ScriptedAI
    {
        npc_deaths_of_chromie_combatAI(Creature* creature) : ScriptedAI(creature)
        {
            if (me->GetEntry() == NPC_HORDE_SIEGE_CANNON)
                SetCombatMovement(false);
        }

        EventMap Events;

        void Reset() override
        {
            Events.Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Events.ScheduleEvent(EVENT_PRIMARY, 3000);
            Events.ScheduleEvent(EVENT_SECONDARY, 9000);
            Events.ScheduleEvent(EVENT_TERTIARY, 15000);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            Events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (Events.ExecuteEvent())
            {
                case EVENT_PRIMARY:
                    CastPrimaryAbility();
                    Events.ScheduleEvent(EVENT_PRIMARY, urand(4500, 7000));
                    break;
                case EVENT_SECONDARY:
                    CastSecondaryAbility();
                    Events.ScheduleEvent(EVENT_SECONDARY, urand(10000, 14000));
                    break;
                case EVENT_TERTIARY:
                    CastTertiaryAbility();
                    Events.ScheduleEvent(EVENT_TERTIARY, urand(16000, 22000));
                    break;
                default:
                    break;
            }

            if (me->GetEntry() != NPC_HORDE_SIEGE_CANNON)
                DoMeleeAttackIfReady();
        }

    private:
        void CastPrimaryAbility()
        {
            switch (me->GetEntry())
            {
                case NPC_VOID_GARGANTUAN:    DoCastVictim(246170); break; // Void Blast
                case NPC_ZORATHIDES:         DoCastVictim(246942); break; // Carrion Swarm
                case NPC_TALAR_ICECHILL:     DoCastVictim(248487); break; // Frostbolt
                case NPC_THALAS_VYLETHORN:   DoCastVictim(247009); break; // Nightmare Bolt
                case NPC_FIERY_BEHEMOTH:     DoCastAOE(245191);    break; // Fire Nova
                case NPC_GROLETHAX:          DoCastVictim(248486); break; // Fel Glare
                case NPC_NEZAR_AZRET:        DoCast(248551);       break; // Summon Crypt Deceiver
                case NPC_HORDE_SIEGE_CANNON: DoCastVictim(247098); break; // Cannonfire
                default: break;
            }
        }

        void CastSecondaryAbility()
        {
            switch (me->GetEntry())
            {
                case NPC_VOID_GARGANTUAN:  DoCastAOE(246940);    break; // Looming Shadows
                case NPC_ZORATHIDES:       DoCastVictim(248660); break; // Cripple
                case NPC_TALAR_ICECHILL:   DoCastVictim(247683); break; // Deep Freeze
                case NPC_THALAS_VYLETHORN: DoCastVictim(247008); break; // Vile Thorns
                case NPC_FIERY_BEHEMOTH:   DoCastAOE(248598);    break; // Volcano
                case NPC_GROLETHAX:        DoCastAOE(248442);    break; // Images of Doom
                default: break;
            }
        }

        void CastTertiaryAbility()
        {
            switch (me->GetEntry())
            {
                case NPC_ZORATHIDES:       DoCastVictim(248664); break; // Infernal Strike
                case NPC_THALAS_VYLETHORN: DoCastVictim(248516); break; // Sleep
                case NPC_FIERY_BEHEMOTH:   DoCastAOE(248599);    break; // Lava
                default: break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_deaths_of_chromie_combatAI(creature);
    }
};

class go_deaths_of_chromie_portal : public GameObjectScript
{
public:
    go_deaths_of_chromie_portal() : GameObjectScript("go_deaths_of_chromie_portal") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (go->GetMapId() != MAP_DEATHS_OF_CHROMIE)
            return false;

        InstanceScript* instance = go->GetInstanceScript();
        Position const* destination = GetPortalDestination(go->GetEntry());
        if (!instance || instance->getScenarionStep() != 1 || !destination)
            return true;

        player->NearTeleportTo(destination->GetPositionX(), destination->GetPositionY(),
            destination->GetPositionZ(), destination->GetOrientation());
        return true;
    }
};

void AddSC_deaths_of_chromie()
{
    new npc_deaths_of_chromie();
    new npc_deaths_of_chromie_combat();
    new go_deaths_of_chromie_portal();
}
