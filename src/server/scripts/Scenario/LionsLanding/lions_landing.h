/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef DEF_LIONS_LANDING
#define DEF_LIONS_LANDING

enum LionsLandingData
{
    DATA_JOIN_DAGGIN = 1,
    DATA_JOIN_TAYLOR,
    DATA_LOCATE_TWINBRAID,
    DATA_AMBER_REPORT,
    DATA_MISHKA_REPORT,
    DATA_SULLY_REPORT,
    DATA_PLACE_ROCKETS,
    DATA_PLACE_BOMBS,
    DATA_PLACE_BOOMSTICKS
};

enum LionsLandingCreatures
{
    NPC_DAGGIN_WINDBEARD             = 68581,
    NPC_ADMIRAL_TAYLOR               = 68685,
    NPC_HIGH_MARSHAL_TWINBRAID       = 68851,
    NPC_MISHKA                       = 68870,
    NPC_AMBER_KEARNEN                = 68871,
    NPC_SULLY_MCLEARY                = 68883,
    NPC_PLACE_BOMBS                  = 68884,
    NPC_PLACE_BOOMSTICKS             = 68885,
    NPC_PLACE_ROCKETS                = 68886,

    NPC_DREADGUARD_CAVALRY           = 67309,
    NPC_WOLF_RIDER_GAJA              = 67413,
    NPC_UDUJI_BERSERKER              = 67430,
    NPC_GREAT_HEXER_UDUJI            = 67434,
    NPC_BAXEL_BRASSBOMBS             = 67504,
    NPC_SUNWALKER_CHAGON             = 67548,
    NPC_DARK_CLERIC_LARESA           = 67551,
    NPC_THAUMATURGE_SARESSE          = 67692,
    NPC_BLOODHILT_GRUNT              = 67814,
    NPC_UDUJI_HEXER                  = 67920,
    NPC_SHARK_SAPPER                 = 68294,
    NPC_BLOODHILT_EARTH_SINGER       = 68508,
    NPC_BLOODHILT_BATTLEMAGE         = 68639,
    NPC_BLOODHILT_LIGHTSWORN         = 68640,
    NPC_SHARK_MEDIC                  = 68668,
    NPC_SHARK_ROCKETEER              = 68669
};

enum LionsLandingCriteria
{
    SPELL_JOIN_DAGGIN                = 114894,
    SPELL_JOIN_TAYLOR                = 103605,
    SPELL_PLACE_ROCKETS              = 135654,
    SPELL_PLACE_BOMBS                = 135655,
    SPELL_PLACE_BOOMSTICKS           = 135656,

    CRITERIA_TWINBRAID_LOCATED       = 34785,
    CRITERIA_AMBER_REPORT            = 34789,
    CRITERIA_MISHKA_REPORT           = 34790,
    CRITERIA_SULLY_REPORT            = 34791,

    CRITERIA_SOUTH_ATTACKERS         = 46408,
    CRITERIA_EAST_ATTACKERS          = 46409,
    CRITERIA_WEST_ATTACKERS          = 46410,
    CRITERIA_ASSAULT_COMMANDER       = 46411,
    CRITERIA_WAVE_TWO                = 46419,
    CRITERIA_WAVE_THREE              = 46420,
    CRITERIA_WAVE_ONE                = 46421,
    CRITERIA_HORDE_COMMANDER         = 46422
};

constexpr uint32 MAP_LIONS_LANDING = 1103;
constexpr uint32 SCENARIO_LIONS_LANDING = 184;

#endif
