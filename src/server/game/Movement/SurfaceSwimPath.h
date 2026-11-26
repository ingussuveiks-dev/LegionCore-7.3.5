#ifndef LEGIONCORE_SURFACE_SWIM_PATH_H
#define LEGIONCORE_SURFACE_SWIM_PATH_H

#include "DetourNavMeshQuery.h"
#include "MapDefines.h"
#include <cmath>

namespace Movement
{
    // Detour can prefer a polygon on the bed directly under a surface swimmer.
    // Retry from water at the same XY, using the ordinary filter for the route ashore.
    inline int FindSurfaceSwimCorridor(dtNavMeshQuery const& query, dtQueryFilter const& filter,
        float const* start, float surface, dtPolyRef originalStart, dtPolyRef endPoly,
        float const* end, dtPolyRef* corridor, int capacity)
    {
        unsigned short endFlags = 0;
        if (!(filter.getIncludeFlags() & NAV_WATER) ||
            dtStatusFailed(query.getAttachedNavMesh()->getPolyFlags(endPoly, &endFlags)) || !(endFlags & NAV_GROUND))
            return 0;

        dtQueryFilter waterFilter = filter;
        waterFilter.setIncludeFlags(NAV_WATER);
        float const surfacePoint[] = {start[0], surface, start[2]};
        float const extents[] = {0.05f, 1.0f, 0.05f};
        float nearest[3];
        dtPolyRef waterPoly = 0;
        if (dtStatusFailed(query.findNearestPoly(surfacePoint, extents, &waterFilter, &waterPoly, nearest)) ||
            !waterPoly || waterPoly == originalStart || std::abs(nearest[0] - start[0]) > 0.01f ||
            std::abs(nearest[2] - start[2]) > 0.01f || std::abs(nearest[1] - surface) > extents[1])
            return 0;

        int count = 0;
        dtStatus status = query.findPath(waterPoly, endPoly, start, end, &filter, corridor, &count, capacity);
        return dtStatusSucceed(status) && count && corridor[count - 1] == endPoly ? count : 0;
    }
}
#endif
