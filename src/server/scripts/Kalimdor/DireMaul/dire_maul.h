/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef DIRE_MAUL_H
#define DIRE_MAUL_H

#define DireMaulScriptName "instance_dire_maul"
#define DataHeader "DM"

enum DireMaulData
{
    DATA_PUSILLIN = 0,
    DATA_LETHTENDRIS,
    DATA_HYDROSPAWN,
    DATA_ZEVRIM_THORNHOOF,
    DATA_ALZZIN_THE_WILDSHAPER,
    DATA_TENDRIS_WARPWOOD,
    DATA_MAGISTER_KALENDRIS,
    DATA_TSUZEE,
    DATA_ILLYANNA_RAVENOAK,
    DATA_IMMOLTHAR,
    DATA_PRINCE_TORTHELDRIN,
    DATA_GUARD_MOLDAR,
    DATA_STOMPER_KREEG,
    DATA_GUARD_FENGUS,
    DATA_GUARD_SLIPKIK,
    DATA_CAPTAIN_KROMCRUSH,
    DATA_KING_GORDOK,
    MAX_ENCOUNTER
};

enum DireMaulCreatures
{
    NPC_PRINCE_TORTHELDRIN = 11486,
    NPC_MAGISTER_KALENDRIS = 11487,
    NPC_ILLYANNA_RAVENOAK  = 11488,
    NPC_TENDRIS_WARPWOOD   = 11489,
    NPC_ZEVRIM_THORNHOOF   = 11490,
    NPC_ALZZIN_WILDSHAPER  = 11492,
    NPC_IMMOLTHAR          = 11496,
    NPC_KING_GORDOK        = 11501,
    NPC_HYDROSPAWN         = 13280,
    NPC_GUARD_FENGUS       = 14321,
    NPC_STOMPER_KREEG      = 14322,
    NPC_GUARD_SLIPKIK      = 14323,
    NPC_CAPTAIN_KROMCRUSH  = 14325,
    NPC_GUARD_MOLDAR       = 14326,
    NPC_LETHTENDRIS        = 14327,
    NPC_PUSILLIN           = 14354,
    NPC_TSUZEE             = 11467
};

enum DireMaulGameObjects
{
    GO_PRINCE_CHEST = 179545
};

#endif
