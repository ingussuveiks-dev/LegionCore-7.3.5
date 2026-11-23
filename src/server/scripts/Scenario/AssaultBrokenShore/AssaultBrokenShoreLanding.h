#ifndef AssaultBrokenShoreLanding_h__
#define AssaultBrokenShoreLanding_h__

#include "Map.h"
#include "WaypointManager.h"
#include <cmath>

namespace AssaultBrokenShore
{
    constexpr uint32 RavenPath = 11322708;

    inline bool GetRavenLanding(Map const& map, Position& landing)
    {
        WaypointPath const* path = sWaypointMgr->GetPathScript(RavenPath);
        if (!path || path->empty())
            return false;

        WaypointData const& beach = path->back();
        float const floor = map.GetHeight(beach.x, beach.y, beach.z, false);
        if (!std::isfinite(floor) || std::abs(floor - beach.z) > 6.0f)
            return false;

        landing.Relocate(beach.x, beach.y, floor + 0.25f, beach.orientation);
        return true;
    }
}

#endif
