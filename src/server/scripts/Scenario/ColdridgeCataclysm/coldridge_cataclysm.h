/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef DEF_COLDRIDGE_CATACLYSM
#define DEF_COLDRIDGE_CATACLYSM

enum ColdridgeData
{
    DATA_INTERACT_JOREN = 1,
    DATA_INTERACT_JONA,
    DATA_BOAR_TIRED,
    DATA_TAME_BOAR,
    DATA_USE_KEG
};

enum ColdridgeCreatures
{
    NPC_GRELIN_WHITEBEARD       = 786,
    NPC_CRAG_BOAR               = 1125,
    NPC_WENDIGO                 = 1135,
    NPC_APPRENTICE_SOREN        = 1354,
    NPC_JONA_IRONSTOCK          = 37087,
    NPC_FROSTMANE_BLADE         = 37507,
    NPC_JOREN_IRONSTOCK         = 119296,
    NPC_MONSTER_OF_A_TROLL      = 119358
};

enum ColdridgeGameObjects
{
    GO_THERAMORE_PALE_ALE       = 201609,
    GO_STORMHAMMER_STOUT        = 201610,
    GO_GNOMENBRAU               = 201611
};

enum ColdridgeCriteriaAssets
{
    CREDIT_LISTEN_TO_GRELIN     = 56859,
    CREDIT_FIND_THERAMORE       = 56862,
    CREDIT_JOREN_FLEES          = 56865,
    CREDIT_BOAR_TIRED           = 56866,
    CREDIT_TAME_BOAR            = 56867,
    CREDIT_RECOVER_THERAMORE    = 56868,
    CREDIT_RECOVER_GNOMENBRAU   = 56869,
    CREDIT_ENTER_HOVEL          = 56870,
    CREDIT_KILL_TROLL_WENDIGO   = 56872,
    CREDIT_FIND_GNOMENBRAU      = 56873,
    CREDIT_GEAR_UP              = 56879,
    CREDIT_RELAX                = 56882,
    CREDIT_FIND_STORMHAMMER     = 56883,

    CRITERIA_ASSET_EMBELLISH_TALE  = 237060,
    CRITERIA_ASSET_EXAGGERATE_TALE = 237061,
    CRITERIA_ASSET_BETTER_TALE     = 237062
};

constexpr uint32 MAP_COLDRIDGE_CATACLYSM = 1723;
constexpr uint32 SCENARIO_COLDRIDGE_CATACLYSM = 1292;

#endif
