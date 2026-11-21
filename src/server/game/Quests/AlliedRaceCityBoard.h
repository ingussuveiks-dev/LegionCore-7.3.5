#ifndef ALLIED_RACE_CITY_BOARD_H
#define ALLIED_RACE_CITY_BOARD_H

#include "Define.h"

class Player;

namespace AlliedRaceCityBoard
{
constexpr uint32 CommandBoard = 206109;
constexpr uint32 CallBoard = 206294;

struct Route
{
    uint32 Quest;
    uint32 Board;
};

constexpr Route Routes[] = {{50303, CommandBoard}, {50305, CallBoard}, {50313, CallBoard}, {50319, CommandBoard}};

constexpr uint32 BoardForQuest(uint32 quest)
{
    for (Route route : Routes)
        if (route.Quest == quest)
            return route.Board;
    return 0;
}

constexpr bool IsCapitalBoard(uint32 entry, uint32 map, uint64 spawn)
{
    return (entry == CommandBoard && map == 1 && spawn == 107487) ||
           (entry == CallBoard && map == 0 && spawn == 108162);
}

void RestoreIncompleteQuests(Player& player);
}

#endif
