#ifndef LEGION_TRANSPORT_GROUND_PATH_H
#define LEGION_TRANSPORT_GROUND_PATH_H

#include "Transport.h"
#include "GameObjectModel.h"
#include "MoveSplineInitArgs.h"
#include <algorithm>
#include <cmath>

namespace Movement
{
enum class TransportGroundResult { NoPath, TooLong, Complete };

// A charge is straight, but must still have continuous ground and an unobstructed
// body corridor. Terrain mmaps do not describe a moving transport's deck.
inline TransportGroundResult BuildTransportGroundPath(Transport const* transport,
    Position const& start, Position const& end, float collisionHeight,
    uint32 pointLimit, PointsArray& result, float collisionRadius = 0.4f)
{
    result.clear();
    if (!transport || !transport->m_model || !transport->m_model->isCollisionEnabled() ||
        !std::isfinite(collisionHeight) || collisionHeight <= 0.0f ||
        !std::isfinite(collisionRadius) || collisionRadius < 0.0f)
        return TransportGroundResult::NoPath;
    float distance = start.GetExactDist(&end);
    if (!std::isfinite(distance) || distance > 296.0f || pointLimit < 2)
        return TransportGroundResult::NoPath;
    uint32 spans = std::max<uint32>(1, uint32(std::ceil(distance / 4.0f)));
    if (spans + 1 > pointLimit)
        return TransportGroundResult::TooLong;
    uint32 samples = spans * 8; // Half-yard or finer collision checks.
    float dx = end.GetPositionX() - start.GetPositionX();
    float dy = end.GetPositionY() - start.GetPositionY();
    float horizontal = std::sqrt(dx * dx + dy * dy);
    G3D::Vector3 side = horizontal > 0.001f ? G3D::Vector3(-dy, dx, 0) * (collisionRadius / horizontal) : G3D::Vector3::zero();
    G3D::Vector3 previous;
    float height = start.GetPositionZ();
    PointsArray path;
    for (uint32 i = 0; i <= samples; ++i)
    {
        float t = float(i) / float(samples);
        float x = start.GetPositionX() + (end.GetPositionX() - start.GetPositionX()) * t;
        float y = start.GetPositionY() + (end.GetPositionY() - start.GetPositionY()) * t;
        float floor = transport->m_model->getHeight(x, y, height + 1.0f, 2.0f, transport->GetPhases(), false);
        if (!std::isfinite(floor) || std::fabs(floor - height) > 0.6f ||
            (i == samples && std::fabs(floor - end.GetPositionZ()) > 0.6f))
            return TransportGroundResult::NoPath;
        G3D::Vector3 current(x, y, floor);
        float body = std::max(0.5f, collisionHeight * 0.5f);
        for (float edge : {-1.0f, 0.0f, 1.0f})
        {
            G3D::Vector3 offset = side * edge;
            float edgeFloor = transport->m_model->getHeight(x + offset.x, y + offset.y,
                floor + 1.0f, 2.0f, transport->GetPhases(), false);
            if (!std::isfinite(edgeFloor) || std::fabs(edgeFloor - floor) > 0.6f ||
                (i && !transport->m_model->isInLineOfSight(previous + offset + G3D::Vector3(0, 0, body),
                    current + offset + G3D::Vector3(0, 0, body), transport->GetPhases(), false)))
                return TransportGroundResult::NoPath;
        }
        if (i % 8 == 0)
            path.push_back(current);
        previous = current;
        height = floor;
    }
    result.swap(path); // Publish only a completely checked path.
    return TransportGroundResult::Complete;
}
}
#endif
