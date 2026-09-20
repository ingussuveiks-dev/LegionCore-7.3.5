/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "the_stockade.h"

class instance_the_stockade : public InstanceMapScript
{
public:
    instance_the_stockade() : InstanceMapScript(TheStockadeScriptName, 34) { }

    struct instance_the_stockade_InstanceMapScript : public InstanceScript
    {
        instance_the_stockade_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_the_stockade_InstanceMapScript(map);
    }
};

void AddSC_instance_the_stockade()
{
    new instance_the_stockade();
}
