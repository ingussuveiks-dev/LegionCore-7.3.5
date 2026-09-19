/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef DEF_DEATHS_OF_CHROMIE
#define DEF_DEATHS_OF_CHROMIE

enum DeathsOfChromieData
{
    DATA_SELECT_CHROMIE_TALENTS = 1,
    DATA_RETURN_TO_PRESENT
};

enum DeathsOfChromieCreatures
{
    NPC_CHROMIE_WYRMREST        = 122510,

    NPC_VOID_GARGANTUAN         = 122553,
    NPC_ZORATHIDES              = 122871,
    NPC_TALAR_ICECHILL          = 122874,
    NPC_THALAS_VYLETHORN        = 122875,

    NPC_FIERY_BEHEMOTH          = 123021,
    NPC_CHROMIE_HYJAL           = 123628,
    NPC_GROLETHAX               = 123721,
    NPC_CHROMIE_WELL            = 123722,
    NPC_NEZAR_AZRET             = 123836,
    NPC_CHROMIE_STRATHOLME      = 123837,
    NPC_HORDE_SIEGE_CANNON      = 124052,
    NPC_CHROMIE_ANDORHAL        = 124154
};

enum DeathsOfChromieGameObjects
{
    GO_PORTAL_ANDORHAL          = 272379,
    GO_PORTAL_HYJAL             = 272381,
    GO_PORTAL_STRATHOLME        = 272382,
    GO_PORTAL_WELL_OF_ETERNITY  = 272383
};

enum DeathsOfChromieCriteriaAssets
{
    CREDIT_SELECT_TALENTS       = 58147,
    CREDIT_SAVE_ALL_CHROMIES    = 58538,
    CREDIT_RETURN_TO_PRESENT    = 59069,
    CREDIT_STOP_ATTACK          = 59073
};

constexpr uint32 MAP_DEATHS_OF_CHROMIE = 1756;
constexpr uint32 SCENARIO_DEATHS_OF_CHROMIE = 1351;

#endif
