/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "ScriptMgr.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "dire_maul.h"

class instance_dire_maul : public InstanceMapScript
{
public:
    instance_dire_maul() : InstanceMapScript(DireMaulScriptName, 429) { }

    struct instance_dire_maul_InstanceMapScript : public InstanceScript
    {
        instance_dire_maul_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);
        }

        ObjectGuid PrinceGuid;
        ObjectGuid PrinceChestGuid;

        void OnCreatureCreate(Creature* creature) override
        {
            if (creature->GetEntry() == NPC_PRINCE_TORTHELDRIN)
            {
                PrinceGuid = creature->GetGUID();
                if (GetBossState(DATA_IMMOLTHAR) == DONE)
                    creature->setFaction(14);
            }
        }

        void OnGameObjectCreate(GameObject* gameObject) override
        {
            InstanceScript::OnGameObjectCreate(gameObject);

            switch (gameObject->GetEntry())
            {
                case GO_PRINCE_CHEST:
                    PrinceChestGuid = gameObject->GetGUID();
                    if (GetBossState(DATA_PRINCE_TORTHELDRIN) == DONE)
                        gameObject->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                default:
                    break;
            }
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (!unit->ToCreature())
                return;

            uint32 encounter = MAX_ENCOUNTER;
            switch (unit->GetEntry())
            {
                case NPC_PUSILLIN:           encounter = DATA_PUSILLIN; break;
                case NPC_LETHTENDRIS:         encounter = DATA_LETHTENDRIS; break;
                case NPC_HYDROSPAWN:          encounter = DATA_HYDROSPAWN; break;
                case NPC_ZEVRIM_THORNHOOF:    encounter = DATA_ZEVRIM_THORNHOOF; break;
                case NPC_ALZZIN_WILDSHAPER:   encounter = DATA_ALZZIN_THE_WILDSHAPER; break;
                case NPC_TENDRIS_WARPWOOD:    encounter = DATA_TENDRIS_WARPWOOD; break;
                case NPC_MAGISTER_KALENDRIS:  encounter = DATA_MAGISTER_KALENDRIS; break;
                case NPC_TSUZEE:              encounter = DATA_TSUZEE; break;
                case NPC_ILLYANNA_RAVENOAK:   encounter = DATA_ILLYANNA_RAVENOAK; break;
                case NPC_IMMOLTHAR:           encounter = DATA_IMMOLTHAR; break;
                case NPC_PRINCE_TORTHELDRIN:  encounter = DATA_PRINCE_TORTHELDRIN; break;
                case NPC_GUARD_MOLDAR:        encounter = DATA_GUARD_MOLDAR; break;
                case NPC_STOMPER_KREEG:       encounter = DATA_STOMPER_KREEG; break;
                case NPC_GUARD_FENGUS:        encounter = DATA_GUARD_FENGUS; break;
                case NPC_GUARD_SLIPKIK:       encounter = DATA_GUARD_SLIPKIK; break;
                case NPC_CAPTAIN_KROMCRUSH:   encounter = DATA_CAPTAIN_KROMCRUSH; break;
                case NPC_KING_GORDOK:         encounter = DATA_KING_GORDOK; break;
                default:                      return;
            }

            SetBossState(encounter, DONE);

            if (encounter == DATA_IMMOLTHAR)
            {
                if (Creature* prince = instance->GetCreature(PrinceGuid))
                    prince->setFaction(14);
            }
            else if (encounter == DATA_PRINCE_TORTHELDRIN)
            {
                if (GameObject* chest = instance->GetGameObject(PrinceChestGuid))
                    chest->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
            }
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_dire_maul_InstanceMapScript(map);
    }
};

void AddSC_instance_dire_maul()
{
    new instance_dire_maul();
}
