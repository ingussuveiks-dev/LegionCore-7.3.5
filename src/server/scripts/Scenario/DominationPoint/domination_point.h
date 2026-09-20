/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef DEF_DOMINATION_POINT
#define DEF_DOMINATION_POINT

enum DominationPointData
{
    DATA_JOIN_KROMTHAR = 1,
    DATA_JOIN_NAZGRIM,
    DATA_LOCATE_BLOODHILT,
    DATA_SHOKIA_REPORT,
    DATA_KIRYN_REPORT,
    DATA_RIVETT_REPORT,
    DATA_PLACE_ROCKETS,
    DATA_PLACE_BOMBS,
    DATA_PLACE_BOOMSTICKS
};

enum DominationPointCreatures
{
    NPC_GENERAL_NAZGRIM             = 68997,
    NPC_KROMTHAR                    = 68998,
    NPC_RIVETT_CLUTCHPOP            = 68999,
    NPC_SHADEMASTER_KIRYN           = 69000,
    NPC_SHOKIA                      = 69001,
    NPC_WARLORD_BLOODHILT           = 69002,
    NPC_PLACE_BOMBS                 = 68884,
    NPC_PLACE_BOOMSTICKS            = 68885,
    NPC_PLACE_ROCKETS               = 68886,

    NPC_KNIGHT_OF_THE_LION          = 67305,
    NPC_GNOMEREGAN_GRENADIER        = 67419,
    NPC_CORDFELLOW_HACK             = 67420,
    NPC_STONEBEAK_BERSERKER         = 67428,
    NPC_AROC_STONEBEAK              = 67431,
    NPC_JOAN_LORRAINE               = 67530,
    NPC_PRIESTESS_LARALLA           = 67550,
    NPC_THAUMATURGE_MOONSPIRE       = 67690,
    NPC_IRONFORGE_CABALIST          = 67762,
    NPC_GILNEAN_MAULER              = 67794,
    NPC_LIONS_FOOTMAN               = 67811,
    NPC_GNOMEREGAN_FIELD_MEDIC      = 68983,
    NPC_LIONS_FAITHFUL              = 69004,
    NPC_LIONS_ARCANIST              = 69005,
    NPC_LIONS_VINDICATOR            = 69006,
    NPC_ACE_MEDIC                   = 69007,
    NPC_ACE_BEAMGUNNER              = 69008,
    NPC_ACE_SABOTEUR                = 69009
};

enum DominationPointCriteria
{
    SPELL_JOIN_KROMTHAR             = 135968,
    SPELL_JOIN_NAZGRIM              = 135969,
    SPELL_PLACE_ROCKETS             = 135654,
    SPELL_PLACE_BOMBS               = 135655,
    SPELL_PLACE_BOOMSTICKS          = 135656,

    CRITERIA_RIVETT_REPORT          = 34877,
    CRITERIA_KIRYN_REPORT           = 34878,
    CRITERIA_SHOKIA_REPORT          = 34879,
    CRITERIA_BLOODHILT_LOCATED      = 34880,

    CRITERIA_SOUTH_ATTACKERS        = 46567,
    CRITERIA_EAST_ATTACKERS         = 46568,
    CRITERIA_WEST_ATTACKERS         = 46569,
    CRITERIA_ASSAULT_COMMANDER       = 46572,
    CRITERIA_ALLIANCE_COMMANDER      = 46575,
    CRITERIA_WAVE_ONE               = 46576,
    CRITERIA_WAVE_TWO               = 46577,
    CRITERIA_WAVE_THREE             = 46578
};

constexpr uint32 MAP_DOMINATION_POINT = 1102;
constexpr uint32 SCENARIO_DOMINATION_POINT = 201;

#endif
