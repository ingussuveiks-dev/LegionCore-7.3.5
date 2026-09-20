/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef DEF_THE_STOCKADE_H
#define DEF_THE_STOCKADE_H

#include "CreatureAIImpl.h"

#define TheStockadeScriptName "instance_the_stockade"
#define DataHeader "SS"

uint32 const EncounterCount = 3;

enum StockadeData
{
    DATA_RANDOLPH_MOLOCH = 0,
    DATA_LORD_OVERHEAT   = 1,
    DATA_HOGGER          = 2
};

enum StockadeCreatures
{
    NPC_RANDOLPH_MOLOCH  = 46383,
    NPC_LORD_OVERHEAT    = 46264,
    NPC_HOGGER           = 46254,
    NPC_WARDEN_THELWATER = 46409,
    NPC_MORTIMER_MOLOCH  = 46482
};

template<class AI>
CreatureAI* GetTheStockadeAI(Creature* creature)
{
    return GetInstanceAI<AI>(creature, TheStockadeScriptName);
}

#endif
