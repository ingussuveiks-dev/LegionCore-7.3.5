#ifndef FUZAN_RETURN_ROUTE_H
#define FUZAN_RETURN_ROUTE_H

#include <G3D/Vector3.h>
#include <vector>
#include <cmath>

// Functional reconstruction approved by the user, 2026-11-14. These waypoints
// are not a recovered retail spline. The actor, seat, spells and destination
// are matching 7.3.5 data; interpolation is checked against extracted geometry.
namespace FuZanReturnRoute
{
inline bool Valid(G3D::Vector3 const& p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
        std::abs(p.x) < 17000.f && std::abs(p.y) < 17000.f && std::abs(p.z) < 17000.f;
}
inline std::vector<G3D::Vector3> Departure(G3D::Vector3 start)
{
    if (!Valid(start) || (start-G3D::Vector3(881.767f,-2742.08f,192.236f)).length() > 10.f) return {};
    return {start, {884.f,-2736.f,199.f}, {891.f,-2718.f,205.f}, {901.f,-2695.f,205.f}};
}
}
#endif
