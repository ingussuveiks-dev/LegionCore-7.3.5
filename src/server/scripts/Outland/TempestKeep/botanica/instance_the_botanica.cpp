/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "the_botanica.h"

class instance_the_botanica : public InstanceMapScript
{
public:
    instance_the_botanica() : InstanceMapScript(BotanicaScriptName, 553) { }

    struct instance_the_botanica_InstanceMapScript : public InstanceScript
    {
        instance_the_botanica_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_the_botanica_InstanceMapScript(map);
    }
};

void AddSC_instance_the_botanica()
{
    new instance_the_botanica();
}
