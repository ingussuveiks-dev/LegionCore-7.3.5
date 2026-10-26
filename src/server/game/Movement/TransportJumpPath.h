#ifndef LEGION_TRANSPORT_JUMP_PATH_H
#define LEGION_TRANSPORT_JUMP_PATH_H

#include "Transport.h"
#include "GameObjectModel.h"
#include "MovementTypedefs.h"
#include <algorithm>
#include <cmath>

namespace Movement
{
enum class TransportJumpResult { NotOnDeck, NoPath, Complete };

// Check the same parabola used by MotionMaster::MoveJump/MoveSpline, against
// the moving model. A leap needs a landing surface, not a walkable ground path.
inline TransportJumpResult CheckTransportJumpPath(Transport const* transport,
    Position const& start, Position const& end, float collisionHeight,
    float collisionRadius, float speedZ)
{
    if (!transport || !transport->m_model || !transport->m_model->isCollisionEnabled() ||
        !std::isfinite(collisionHeight) || collisionHeight <= 0.0f ||
        !std::isfinite(collisionRadius) || collisionRadius < 0.0f ||
        !std::isfinite(speedZ) || speedZ <= 0.0f)
        return TransportJumpResult::NoPath;
    float distance = start.GetExactDist(&end);
    if (!std::isfinite(distance) || distance > 296.0f)
        return TransportJumpResult::NoPath;
    auto model = transport->m_model;
    auto const& phases = transport->GetPhases();
    float floor = model->getHeight(end.GetPositionX(), end.GetPositionY(), end.GetPositionZ() + 0.6f, 1.2f, phases, false);
    if (!std::isfinite(floor) || std::fabs(floor - end.GetPositionZ()) > 0.6f)
        return TransportJumpResult::NotOnDeck; // Preserve ordinary ship-to-world admission.
    for (G3D::Vector3 const& edge : {G3D::Vector3(collisionRadius, 0, 0), G3D::Vector3(-collisionRadius, 0, 0),
        G3D::Vector3(0, collisionRadius, 0), G3D::Vector3(0, -collisionRadius, 0)})
    {
        float support = model->getHeight(end.GetPositionX() + edge.x, end.GetPositionY() + edge.y,
            floor + 0.6f, 1.2f, phases, false);
        if (!std::isfinite(support) || std::fabs(support - floor) > 0.6f)
            return TransportJumpResult::NoPath;
    }
    // Match the existing moving-deck corridor contact tolerance; tiny deck
    // curvature must not count as a wall through the actor body.
    float foot = std::min(0.5f, collisionHeight * 0.25f);
    float apex = -computeFallElevation(speedZ / gravity, false, -speedZ);
    if (!std::isfinite(apex) || apex < 0.0f || apex > 296.0f)
        return TransportJumpResult::NoPath;
    uint32 samples = std::max<uint32>(1, uint32(std::ceil((distance + 2.0f * apex) * 4.0f)));
    G3D::Vector3 previous(start.GetPositionX(), start.GetPositionY(), start.GetPositionZ());
    for (uint32 i = 0; i <= samples; ++i)
    {
        float t = float(i) / float(samples);
        G3D::Vector3 current(start.GetPositionX() + (end.GetPositionX() - start.GetPositionX()) * t,
            start.GetPositionY() + (end.GetPositionY() - start.GetPositionY()) * t,
            start.GetPositionZ() + (end.GetPositionZ() - start.GetPositionZ()) * t + 4.0f * apex * t * (1.0f - t));
        for (G3D::Vector3 const& edge : {G3D::Vector3::zero(), G3D::Vector3(collisionRadius, 0, 0),
            G3D::Vector3(-collisionRadius, 0, 0), G3D::Vector3(0, collisionRadius, 0), G3D::Vector3(0, -collisionRadius, 0)})
        {
            // Vertical clearance at the endpoints too (e.g. a landing under a low roof).
            if (!model->isInLineOfSight(current + edge + G3D::Vector3(0, 0, foot),
                current + edge + G3D::Vector3(0, 0, collisionHeight), phases, false))
                return TransportJumpResult::NoPath;
            for (float body : {foot, collisionHeight * 0.5f, collisionHeight})
                if (i && !model->isInLineOfSight(previous + edge + G3D::Vector3(0, 0, body),
                    current + edge + G3D::Vector3(0, 0, body), phases, false))
                    return TransportJumpResult::NoPath;
        }
        previous = current;
    }
    return TransportJumpResult::Complete;
}
}
#endif
