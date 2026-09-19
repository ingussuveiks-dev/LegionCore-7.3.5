/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef DEF_BONETOWN
#define DEF_BONETOWN

enum BonetownCreatures
{
    NPC_FRENZIED_SPIRIT          = 60033,
    NPC_NARTHOK_SHADOWSIGHT     = 76312,
    NPC_MORNETH                 = 76313,
    NPC_KELRATH                 = 76316,
    NPC_KILROGG_DEADEYE         = 76317,
    NPC_BALEFUL_SOULPRIEST      = 76318,
    NPC_SHADOWMOON_VOID_SHAMAN  = 76322,
    NPC_SHADOWMOON_DARKCASTER   = 76323,
    NPC_IRON_SHIELDBEARER       = 76324,
    NPC_IRON_GRUNT              = 76325,
    NPC_FALLEN_SOULPRIEST       = 76327,
    NPC_THRALL                  = 76330,
    NPC_DUROTAN                 = 76353
};

enum BonetownSpells
{
    SPELL_SOULLESS              = 154947,
    SPELL_SOULNADO              = 154967,
    SPELL_REND_SOUL             = 155008,
    SPELL_SOUL_BOMB             = 155036,
    SPELL_GRIP_OF_DEATH         = 155100,
    SPELL_SOUL_STORM            = 155105,
    SPELL_NETHER_SHIELD         = 155110,
    SPELL_SCYTHE_SWIPE          = 155112,
    SPELL_CORRUPTED_SOUL        = 155229,
    SPELL_SOUL_TEMPEST          = 155234
};

enum BonetownCriteria
{
    CRITERIA_MEET_DUROTAN       = 36305,
    CRITERIA_DEFEND_THRALL      = 38789,
    CRITERIA_DEFEAT_KILROGG     = 38790
};

constexpr uint32 MAP_BONETOWN = 1200;
constexpr uint32 SCENARIO_BONETOWN = 323;

#endif
