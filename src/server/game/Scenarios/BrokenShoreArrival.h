#ifndef BROKEN_SHORE_ARRIVAL_H
#define BROKEN_SHORE_ARRIVAL_H

#include "Position.h"
#include "SharedDefines.h"

namespace BrokenShoreArrival
{
    constexpr uint32 MapId = 1460;

    inline bool Matches(uint32 mapId, uint32 scenarioId, uint32 team)
    {
        return mapId == MapId && ((team == ALLIANCE && scenarioId == 786) || (team == HORDE && scenarioId == 1189));
    }

    inline Position Beach(uint32 team)
    {
        return team == ALLIANCE ? Position(486.929f, 2052.26f, 1.40809f, 0.0f) :
            Position(567.826f, 1886.94f, 0.737247f, 0.0f);
    }

    inline bool AtBeach(Position const& position, uint32 team)
    {
        return position.GetExactDist(Beach(team)) < 1.0f;
    }
}

#endif
