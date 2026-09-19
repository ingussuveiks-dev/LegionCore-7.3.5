/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef DEF_NIGHTBORNE_UNLOCK
#define DEF_NIGHTBORNE_UNLOCK

enum NightborneUnlockData
{
    DATA_SPEAK_WITH_ARLUIN = 1
};

enum NightborneUnlockCreatures
{
    NPC_ARLUIN = 107253
};

enum NightborneUnlockCriteriaAssets
{
    CREDIT_SPEAK_WITH_ARLUIN = 60314
};

constexpr uint32 MAP_NIGHTBORNE_UNLOCK = 1812;
constexpr uint32 SCENARIO_NIGHTBORNE_UNLOCK = 1423;

#endif
