#ifndef SHEILUN_RETURN_ROUTE_H
#define SHEILUN_RETURN_ROUTE_H

#include <G3D/Vector3.h>
#include "GridDefines.h"
#include <vector>

namespace SheilunReturnRoute
{
constexpr unsigned ReturnMap = 1514;
constexpr unsigned Landing = 5710;
constexpr unsigned CandidatePath = 5619;
constexpr unsigned NativePointCount = 8;

inline bool Valid(G3D::Vector3 const& point)
{
    return Trinity::IsValidMapCoord(point.x,point.y,point.z);
}

// Functional takeoff, validated against matching native smooth interpolation
// and static collision. These are derived points, not a recovered retail path.
inline std::vector<G3D::Vector3> First(G3D::Vector3 const& start)
{
    if (!Valid(start)) return {};
    return {start, {start.x,start.y,start.z+10.f},
        {-1017.29f,-2745.f,68.57111f}, {-1032.f,-2705.f,95.f}, {-1060.f,-2660.f,135.f}};
}

// Native5619 is a route candidate with the correct return landing. Its original
// retail caller is unknown. Keep the actual node data and refuse an unexpected
// event, pause, map or shape instead of silently discarding native semantics.
template<class Nodes>
bool Second(G3D::Vector3 const& start, Nodes const& nodes, std::vector<G3D::Vector3>& result)
{
    result.clear();
    if (!Valid(start) || nodes.size() != NativePointCount) return false;
    std::vector<G3D::Vector3> path{start};
    unsigned index = 0;
    for (auto const* node : nodes)
    {
        if (!node || node->NodeIndex != index++ || node->ContinentID != ReturnMap ||
            node->Flags || node->Delay || node->ArrivalEventID || node->DepartureEventID)
            return false;
        G3D::Vector3 point(node->Loc.X,node->Loc.Y,node->Loc.Z);
        if (!Valid(point)) return false;
        path.push_back(point);
    }
    result.swap(path);
    return true;
}
}
#endif
