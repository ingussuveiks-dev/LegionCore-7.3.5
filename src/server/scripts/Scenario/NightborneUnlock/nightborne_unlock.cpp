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
#include "nightborne_unlock.h"

namespace
{
enum NightborneUnlockGossipActions
{
    ACTION_SPEAK_WITH_ARLUIN = GOSSIP_ACTION_INFO_DEF + 1
};

enum NightborneUnlockTexts
{
    SAY_ARLUIN_WARNING = 8
};
}

class instance_nightborne_unlock : public InstanceMapScript
{
public:
    instance_nightborne_unlock() : InstanceMapScript("instance_nightborne_unlock", MAP_NIGHTBORNE_UNLOCK) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_nightborne_unlock_InstanceMapScript(map);
    }

    struct instance_nightborne_unlock_InstanceMapScript : public InstanceScript
    {
        instance_nightborne_unlock_InstanceMapScript(InstanceMap* map) : InstanceScript(map) { }

        bool SpokeWithArluin = false;

        void Initialize() override
        {
            SpokeWithArluin = false;
        }

        void OnCreatureCreate(Creature* creature) override
        {
            if (creature->GetEntry() != NPC_ARLUIN)
                return;

            creature->setFaction(35);
            creature->SetReactState(REACT_PASSIVE);
            creature->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void SetData(uint32 type, uint32 /*data*/) override
        {
            if (type != DATA_SPEAK_WITH_ARLUIN || SpokeWithArluin || getScenarionStep() != 0)
                return;

            SpokeWithArluin = true;
            DoUpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, CREDIT_SPEAK_WITH_ARLUIN, 1);
        }
    };
};

class npc_nightborne_unlock_arluin : public CreatureScript
{
public:
    npc_nightborne_unlock_arluin() : CreatureScript("npc_nightborne_unlock_arluin") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->GetMapId() != MAP_NIGHTBORNE_UNLOCK)
            return false;

        if (InstanceScript* instance = creature->GetInstanceScript())
            if (instance->getScenarionStep() == 0)
                player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "I am here to speak with you, Arluin.",
                    GOSSIP_SENDER_MAIN, ACTION_SPEAK_WITH_ARLUIN);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->CLOSE_GOSSIP_MENU();

        if (creature->GetMapId() != MAP_NIGHTBORNE_UNLOCK || action != ACTION_SPEAK_WITH_ARLUIN)
            return true;

        creature->AI()->Talk(SAY_ARLUIN_WARNING, player->GetGUID());
        if (InstanceScript* instance = creature->GetInstanceScript())
            instance->SetData(DATA_SPEAK_WITH_ARLUIN, 1);
        return true;
    }
};

void AddSC_nightborne_unlock()
{
    new instance_nightborne_unlock();
    new npc_nightborne_unlock_arluin();
}
