#ifndef LEGION_MAW_SHIP_GROUND_PATH_H
#define LEGION_MAW_SHIP_GROUND_PATH_H

#include "Creature.h"
#include "GameObject.h"
#include "GameObjectModel.h"
#include "Map.h"
#include "MMapFactory.h"
#include "MMapManager.h"
#include "MapDefines.h"
#include "MoveSplineInitArgs.h"
#include <cmath>

namespace Movement
{
enum class MawShipPathResult { NotApplicable, NoPath, TooLong, Complete };

// The shared destructible Naglfar contributes live collision but no terrain
// mmap. Its model-local corridor is only a proposal: validate the complete
// route against current map collision, phases and the actual requesting actor.
inline MawShipPathResult BuildMawShipGroundPath(WorldObject const* source,
    G3D::Vector3 const& start, G3D::Vector3 const& end, uint32 limit, PointsArray& result)
{
    auto actor = source ? source->ToCreature() : nullptr;
    if (!actor || actor->GetMapId() != 1492 || !actor->CanWalk() || actor->IsFlying() || actor->IsFalling() ||
        actor->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_HOVER) || actor->GetTransport() || actor->GetVehicle())
        return MawShipPathResult::NotApplicable;
    auto map = actor->GetMap();
    auto ship = actor->FindNearestGameObject(246919, 300.f);
    if (!ship || ship->HasQuestObjectScope() || !ship->CanCollideWith(actor) ||
        !ship->m_model || !ship->m_model->isCollisionEnabled())
        return MawShipPathResult::NotApplicable;
    auto support = [&](G3D::Vector3 const& point, float& height) -> GameObject*
    {
        DynamicTreeCallback receipt;
        height = map->GetHeight(actor->GetPhases(), point.x, point.y, point.z,
            true, 3.f, &receipt, actor);
        float modelHeight = ship->m_model->getHeight(point.x, point.y, point.z, 1.6f,
            actor->GetPhases(), actor->IsUnitOwnedByPlayer());
        // Static and dynamic extraction can contain the same deck. Map clears
        // its GO receipt when the static copy wins by float rounding; retain
        // direct support only when both real surfaces agree within one cm.
        return std::isfinite(height) && std::isfinite(modelHeight) &&
            std::fabs(height - point.z) <= 0.6f && std::fabs(modelHeight - height) <= 0.01f &&
            (!receipt.go || receipt.go == ship) ? ship : nullptr;
    };
    float startHeight, endHeight;
    if (support(start, startHeight) != ship || support(end, endHeight) != ship)
        return MawShipPathResult::NotApplicable;
    auto query = MMAP::MMapFactory::createOrGetMMapManager()->GetModelNavMeshQuery(ship->GetDisplayId());
    if (!query)
        return MawShipPathResult::NotApplicable; // Startup reports missing model data.
    result.clear();
    // Match the native Recast movement envelope. NPC model bounds/combat reach
    // describe appearance and combat; ordinary terrain paths use this same
    // fixed navigation agent rather than a different mesh for every display.
    auto tile = query->getAttachedNavMesh()->getTile(0);
    if (!tile || !tile->header)
        return MawShipPathResult::NoPath;
    float body = tile->header->walkableHeight, radius = tile->header->walkableRadius;
    if (limit < 2 || !std::isfinite(body) || body <= 0.f || !std::isfinite(radius) || radius < 0.f)
        return MawShipPathResult::NoPath;
    auto localStart = ship->m_model->WorldToModel(start), localEnd = ship->m_model->WorldToModel(end);
    float from[3] = {localStart.y, localStart.z, localStart.x};
    float to[3] = {localEnd.y, localEnd.z, localEnd.x};
    float extents[3] = {2.f, 1.f, 2.f}, nearStart[3], nearEnd[3];
    dtQueryFilter filter; filter.setIncludeFlags(NAV_GROUND); filter.setExcludeFlags(0);
    dtPolyRef startPoly = 0, endPoly = 0;
    if (dtStatusFailed(query->findNearestPoly(from, extents, &filter, &startPoly, nearStart)) || !startPoly ||
        dtStatusFailed(query->findNearestPoly(to, extents, &filter, &endPoly, nearEnd)) || !endPoly ||
        std::fabs(nearStart[1] - from[1]) > 0.8f || std::fabs(nearEnd[1] - to[1]) > 0.8f)
        return MawShipPathResult::NoPath;
    dtPolyRef corridor[512]; int count = 0;
    dtStatus status = query->findPath(startPoly, endPoly, nearStart, nearEnd, &filter, corridor, &count, 512);
    if (dtStatusFailed(status) || dtStatusDetail(status, DT_PARTIAL_RESULT | DT_OUT_OF_NODES | DT_BUFFER_TOO_SMALL) ||
        !count || corridor[count - 1] != endPoly)
        return MawShipPathResult::NoPath;
    float corners[512 * 3]; int cornerCount = 0;
    status = query->findStraightPath(nearStart, nearEnd, corridor, count, corners, nullptr, nullptr, &cornerCount, 512);
    if (dtStatusFailed(status) || dtStatusDetail(status, DT_BUFFER_TOO_SMALL) || cornerCount < 1)
        return MawShipPathResult::NoPath;
    PointsArray proposed; proposed.push_back(start);
    for (int i = 0; i < cornerCount; ++i)
        proposed.push_back(ship->m_model->ModelToWorld(G3D::Vector3(corners[i * 3 + 2], corners[i * 3], corners[i * 3 + 1])));
    proposed.push_back(end);

    PointsArray checked; G3D::Vector3 previous = start; float distance = 0.f;
    for (size_t segment = 1; segment < proposed.size(); ++segment)
    {
        auto a = proposed[segment - 1], b = proposed[segment];
        float length = (b - a).length();
        distance += length;
        if (!std::isfinite(distance) || distance > 296.f)
            return MawShipPathResult::TooLong;
        uint32 samples = std::max<uint32>(1, uint32(std::ceil(length / 0.5f)));
        auto direction = b - a; float horizontal = std::hypot(direction.x, direction.y);
        auto side = horizontal > 0.001f ? G3D::Vector3(-direction.y, direction.x, 0.f) * (radius / horizontal) : G3D::Vector3(radius, 0.f, 0.f);
        for (uint32 i = segment == 1 ? 0 : 1; i <= samples; ++i)
        {
            auto point = a + direction * (float(i) / samples);
            float floor;
            if (support(point, floor) != ship ||
                (!checked.empty() && (floor - previous.z > 0.6f || previous.z - floor > 1.f)))
                return MawShipPathResult::NoPath;
            point.z = floor;
            for (float edge : {-1.f, 0.f, 1.f})
            {
                auto offset = side * edge;
                float edgeFloor;
                if (support(point + offset, edgeFloor) != ship ||
                    !map->isInLineOfSight(point.x + offset.x, point.y + offset.y, std::max(floor, edgeFloor) + 0.15f,
                        point.x + offset.x, point.y + offset.y, floor + body, actor->GetPhases(), VMAP::ModelIgnoreFlags::Nothing, nullptr, actor))
                    return MawShipPathResult::NoPath;
                if (!checked.empty())
                    for (float height : {std::max(0.5f, body * 0.5f), body})
                        if (!map->isInLineOfSight(previous.x + offset.x, previous.y + offset.y, previous.z + height,
                            point.x + offset.x, point.y + offset.y, point.z + height, actor->GetPhases(), VMAP::ModelIgnoreFlags::Nothing, nullptr, actor))
                            return MawShipPathResult::NoPath;
            }
            // Retain corners and curb transitions; check the intervening body
            // corridor at half-yard intervals without exceeding native limits.
            // Projected Detour endpoints can share the same real floor point.
            // Native splines reject consecutive vertices closer than 0.1 yard.
            if (checked.empty() || ((point - checked.back()).length() >= 0.1f &&
                (i == samples || (point - checked.back()).length() >= 4.f || std::fabs(point.z - previous.z) > 0.3f)))
                checked.push_back(point);
            if (checked.size() > limit)
                return MawShipPathResult::TooLong;
            previous = point;
        }
    }
    if (checked.size() < 2 || (checked.back() - end).length() > 0.6f)
        return MawShipPathResult::NoPath;
    result.swap(checked);
    return MawShipPathResult::Complete;
}
}
#endif
