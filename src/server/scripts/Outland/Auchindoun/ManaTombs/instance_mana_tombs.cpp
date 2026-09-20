/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "mana_tombs.h"

class instance_mana_tombs : public InstanceMapScript
{
public:
    instance_mana_tombs() : InstanceMapScript(ManaTombsScriptName, 557) { }

    struct instance_mana_tombs_InstanceMapScript : public InstanceScript
    {
        instance_mana_tombs_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_mana_tombs_InstanceMapScript(map);
    }
};

void AddSC_instance_mana_tombs()
{
    new instance_mana_tombs();
}
