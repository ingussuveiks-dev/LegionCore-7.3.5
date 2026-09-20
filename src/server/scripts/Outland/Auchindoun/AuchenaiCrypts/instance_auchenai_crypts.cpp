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
#include "auchenai_crypts.h"

class instance_auchenai_crypts : public InstanceMapScript
{
public:
    instance_auchenai_crypts() : InstanceMapScript(AuchenaiCryptsScriptName, 558) { }

    struct instance_auchenai_crypts_InstanceMapScript : public InstanceScript
    {
        instance_auchenai_crypts_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_auchenai_crypts_InstanceMapScript(map);
    }
};

void AddSC_instance_auchenai_crypts()
{
    new instance_auchenai_crypts();
}
