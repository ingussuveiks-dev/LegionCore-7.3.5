#ifndef LEGION_TUURE_SCENARIO_H
#define LEGION_TUURE_SCENARIO_H

#include "Define.h"

class Player;
class Unit;
class Creature;
class Map;
class Scenario;
class ObjectGuid;

namespace TuureFirstFight
{
    bool Eligible(Player* player, Unit* anchor, uint32 firstStep, uint32 lastStep);
    // The instance validates the real361428 death and accepted step2 counter.
    Player* Recipient(Creature* defeated, Unit* killer);
    Player* Find(Map* map, ObjectGuid const& guid);
    bool HasVictory(Scenario* scenario);
}

#endif
