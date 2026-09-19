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
#include "coldridge_cataclysm.h"

namespace
{
constexpr uint32 ACTION_COLDRIDGE_INTERACT = GOSSIP_ACTION_INFO_DEF + 1;

enum ColdridgeCombatEvents
{
    EVENT_SMASH = 1,
    EVENT_FROSTBOLT
};

bool GetColdridgeInteraction(uint32 entry, uint32 step, char const*& text, uint32& data)
{
    switch (entry)
    {
        case NPC_JOREN_IRONSTOCK:
            data = DATA_INTERACT_JOREN;
            switch (step)
            {
                case 0:  text = "Dance with Joren."; return true;
                case 4:  text = "Embellish the tale."; return true;
                case 7:  text = "Exaggerate what happened."; return true;
                case 13: text = "Enhance the tale."; return true;
                case 16: text = "Tell an obvious lie."; return true;
                case 18: text = "Make the story better."; return true;
                default: break;
            }
            break;
        case NPC_JONA_IRONSTOCK:
            if (step == 2)
            {
                text = "Gear up for the search.";
                data = DATA_INTERACT_JONA;
                return true;
            }
            break;
        case NPC_CRAG_BOAR:
            if (step == 10)
            {
                text = "Calm and tame the exhausted boar.";
                data = DATA_TAME_BOAR;
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}
}

class npc_coldridge_story : public CreatureScript
{
public:
    npc_coldridge_story() : CreatureScript("npc_coldridge_story") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance)
            return true;

        char const* text = nullptr;
        uint32 data = 0;
        if (!GetColdridgeInteraction(creature->GetEntry(), instance->getScenarionStep(), text, data))
            return true;

        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, text, GOSSIP_SENDER_MAIN, ACTION_COLDRIDGE_INTERACT);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->CLOSE_GOSSIP_MENU();
        if (action != ACTION_COLDRIDGE_INTERACT)
            return true;

        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance)
            return true;

        char const* text = nullptr;
        uint32 data = 0;
        if (!GetColdridgeInteraction(creature->GetEntry(), instance->getScenarionStep(), text, data))
            return true;

        if (creature->GetEntry() == NPC_JOREN_IRONSTOCK && instance->getScenarionStep() == 0)
        {
            player->HandleEmoteCommand(EMOTE_ONESHOT_DANCE);
            creature->HandleEmoteCommand(EMOTE_ONESHOT_DANCE);
        }

        instance->SetData(data, 1);
        return true;
    }

    struct npc_coldridge_storyAI : public ScriptedAI
    {
        npc_coldridge_storyAI(Creature* creature) : ScriptedAI(creature) { }

        EventMap Events;
        bool Tired = false;

        void Reset() override
        {
            Events.Reset();
            Tired = false;
        }

        void EnterCombat(Unit* /*who*/) override
        {
            if (me->GetEntry() == NPC_MONSTER_OF_A_TROLL)
            {
                Events.ScheduleEvent(EVENT_SMASH, 5000);
                Events.ScheduleEvent(EVENT_FROSTBOLT, 9000);
            }
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*dmgType*/) override
        {
            if (me->GetEntry() != NPC_CRAG_BOAR || Tired)
                return;

            InstanceScript* instance = me->GetInstanceScript();
            if (!instance || instance->getScenarionStep() != 9)
                return;

            if (damage < me->GetHealth() - me->CountPctFromMaxHealth(15))
                return;

            damage = 0;
            Tired = true;
            me->CombatStop(true);
            me->SetReactState(REACT_PASSIVE);
            me->setFaction(35);
            me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            instance->SetData(DATA_BOAR_TIRED, 1);
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
                case EVENT_SMASH:
                    DoCastVictim(11976); // Strike
                    Events.ScheduleEvent(EVENT_SMASH, urand(7000, 10000));
                    break;
                case EVENT_FROSTBOLT:
                    DoCastVictim(9672); // Frostbolt
                    Events.ScheduleEvent(EVENT_FROSTBOLT, urand(9000, 13000));
                    break;
                default:
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_coldridge_storyAI(creature);
    }
};

class go_coldridge_story_keg : public GameObjectScript
{
public:
    go_coldridge_story_keg() : GameObjectScript("go_coldridge_story_keg") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go) override
    {
        if (go->GetMapId() != MAP_COLDRIDGE_CATACLYSM)
            return false;

        if (InstanceScript* instance = go->GetInstanceScript())
        {
            instance->SetData(DATA_USE_KEG, go->GetEntry());
            return true;
        }

        return false;
    }
};

void AddSC_coldridge_cataclysm()
{
    new npc_coldridge_story();
    new go_coldridge_story_keg();
}
