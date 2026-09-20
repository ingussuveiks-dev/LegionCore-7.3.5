/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "the_underbog.h"

ObjectData const creatureData[] =
{
    { NPC_HUNGARFEN,     DATA_HUNGARFEN },
    { NPC_GHAZAN,        DATA_GHAZAN },
    { NPC_MUSELEK,       DATA_MUSELEK },
    { NPC_BLACK_STALKER, DATA_BLACK_STALKER },
    { 0,                 0 }
};

class instance_the_underbog : public InstanceMapScript
{
public:
    instance_the_underbog() : InstanceMapScript(TheUnderbogScriptName, 546) { }

    struct instance_the_underbog_InstanceMapScript : public InstanceScript
    {
        instance_the_underbog_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);
            LoadObjectData(creatureData, nullptr);
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_the_underbog_InstanceMapScript(map);
    }
};

void AddSC_instance_the_underbog()
{
    new instance_the_underbog();
}
