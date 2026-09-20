/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef THE_UNDERBOG_H
#define THE_UNDERBOG_H

#define TheUnderbogScriptName "instance_the_underbog"
#define DataHeader "UB"

enum TheUnderbogData
{
    DATA_HUNGARFEN     = 0,
    DATA_GHAZAN        = 1,
    DATA_MUSELEK       = 2,
    DATA_BLACK_STALKER = 3,
    MAX_ENCOUNTER      = 4
};

enum TheUnderbogCreatures
{
    NPC_HUNGARFEN      = 17770,
    NPC_MUSELEK        = 17826,
    NPC_CLAW           = 17827,
    NPC_BLACK_STALKER  = 17882,
    NPC_GHAZAN         = 18105,
    NPC_WINDCALLER_CLAW = 17894
};

#endif
