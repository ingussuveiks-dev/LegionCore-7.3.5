/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "noodle_time.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"

namespace
{
constexpr uint32 ACTION_START_NOODLE_TUTORIAL = GOSSIP_ACTION_INFO_DEF + 1;
}

class npc_noodle_time_interaction : public CreatureScript
{
public:
    npc_noodle_time_interaction() : CreatureScript("npc_noodle_time_interaction") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->GetEntry() != NPC_SUNGSHIN_IRONPAW)
            return true;

        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance || instance->getScenarionStep() != 0)
            return true;

        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "I'm ready to discover the secret ingredient.",
            GOSSIP_SENDER_MAIN, ACTION_START_NOODLE_TUTORIAL);
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->CLOSE_GOSSIP_MENU();
        if (action == ACTION_START_NOODLE_TUTORIAL)
            if (InstanceScript* instance = creature->GetInstanceScript())
                instance->SetData(DATA_START_TUTORIAL, 1);
        return true;
    }

    struct npc_noodle_time_interactionAI : ScriptedAI
    {
        npc_noodle_time_interactionAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
        }

        void OnSpellClick(Unit* clicker) override
        {
            if (!clicker->IsPlayer())
                return;

            InstanceScript* instance = me->GetInstanceScript();
            if (!instance)
                return;

            switch (me->GetEntry())
            {
                case NPC_SEAT:
                    instance->SetGuidData(DATA_SELECT_SEAT, me->GetGUID());
                    break;
                case NPC_NOODLE_SOUP:
                case NPC_DRAGONS_NEST_NOODLE_SOUP:
                case NPC_PANDAREN_TREASURES_NOODLE_SOUP:
                    instance->SetGuidData(DATA_SELECT_FOOD, me->GetGUID());
                    break;
                default:
                    if (me->GetEntry() >= NPC_MERCHANT_MALE && me->GetEntry() <= NPC_HUNGRY_MERCHANT_FEMALE)
                        instance->SetGuidData(DATA_SELECT_CUSTOMER, me->GetGUID());
                    else if (me->GetEntry() >= NPC_HUNGRY_SCHOLAR_MALE && me->GetEntry() <= NPC_HUNGRY_FARMHAND_FEMALE)
                        instance->SetGuidData(DATA_SELECT_CUSTOMER, me->GetGUID());
                    break;
            }

            if (me->GetEntry() >= NPC_MERCHANT_MALE && me->GetEntry() <= NPC_HUNGRY_MERCHANT_FEMALE)
                instance->SetGuidData(DATA_SERVE_CUSTOMER, me->GetGUID());
            else if (me->GetEntry() >= NPC_HUNGRY_SCHOLAR_MALE && me->GetEntry() <= NPC_HUNGRY_FARMHAND_FEMALE)
                instance->SetGuidData(DATA_SERVE_CUSTOMER, me->GetGUID());
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_noodle_time_interactionAI(creature);
    }
};

class spell_noodle_time_cook : public SpellScriptLoader
{
public:
    spell_noodle_time_cook() : SpellScriptLoader("spell_noodle_time_cook") { }

    class spell_noodle_time_cook_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_noodle_time_cook_SpellScript);

        void HandleDummy(SpellEffIndex /*effectIndex*/)
        {
            Unit* caster = GetCaster();
            InstanceScript* instance = caster ? caster->GetInstanceScript() : nullptr;
            if (!instance)
                return;

            switch (GetSpellInfo()->Id)
            {
                case SPELL_MAKE_NOODLE_SOUP:
                    instance->SetData(DATA_COOK_NOODLE_SOUP, 1);
                    break;
                case SPELL_MAKE_DRAGONS_NEST_SOUP:
                    instance->SetData(DATA_COOK_DRAGONS_NEST_SOUP, 1);
                    break;
                case SPELL_MAKE_PANDAREN_TREASURES_SOUP:
                    instance->SetData(DATA_COOK_PANDAREN_TREASURES_SOUP, 1);
                    break;
                default:
                    break;
            }
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_noodle_time_cook_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_noodle_time_cook_SpellScript();
    }
};

class spell_noodle_time_clear : public SpellScriptLoader
{
public:
    spell_noodle_time_clear() : SpellScriptLoader("spell_noodle_time_clear") { }

    class spell_noodle_time_clear_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_noodle_time_clear_SpellScript);

        void HandleScript(SpellEffIndex /*effectIndex*/)
        {
            Unit* caster = GetCaster();
            InstanceScript* instance = caster ? caster->GetInstanceScript() : nullptr;
            if (!instance)
                return;

            if (GetSpellInfo()->Id == SPELL_CLEAR_CUSTOMERS)
                instance->SetData(DATA_CLEAR_CUSTOMERS, 1);
            else if (GetSpellInfo()->Id == SPELL_CLEAR_FOOD)
                instance->SetData(DATA_CLEAR_FOOD, 1);
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_noodle_time_clear_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_noodle_time_clear_SpellScript();
    }
};

void AddSC_noodle_time()
{
    new npc_noodle_time_interaction();
    new spell_noodle_time_cook();
    new spell_noodle_time_clear();
}
