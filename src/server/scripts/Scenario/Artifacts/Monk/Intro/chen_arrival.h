#pragma once

#include "WaypointMovementGenerator.h"
#include "MoveSpline.h"
#include "Timer.h"

namespace MonkChenArrival
{
constexpr uint32 StatusData = 94343, ClaimData = 94344, RequestData = 94345, PlayerData = 94346, RouteData = 94347;

inline WaypointPath const* Data()
{
    auto path = sWaypointMgr->GetPathScript(9859005);
    return path && path->size() == 6 ? path : nullptr;
}

inline WaypointMovementGenerator<Creature>* Path(Creature* actor)
{
    MovementGenerator* movement = actor ? actor->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE) : nullptr;
    return movement && movement->GetMovementGeneratorType() == WAYPOINT_MOTION_TYPE ?
        static_cast<WaypointMovementGenerator<Creature>*>(movement) : nullptr;
}

inline bool Pose(Creature* actor)
{
    auto path = Data();
    if (!actor || !path || !actor->movespline || !actor->movespline->Finalized()) return false;
    auto const& last = path->back();
    return actor->IsWithinDist3d(last.x, last.y, last.z, 0.5f, false);
}

inline bool Issued(Creature* actor)
{
    auto path = Path(actor);
    return Data() && path && path->GetPathId() == 9859005 && path->GetCurrentNode() == 0 &&
        actor->GetCurrentWaypointID() == uint32(-1);
}

inline bool Callback(Creature* actor)
{
    auto path = Path(actor);
    return path && path->GetPathId() == 9859005 && path->GetCurrentNode() == 5 &&
        !actor->GetMotionMaster()->empty() && actor->GetMotionMaster()->top() == path && Pose(actor);
}

inline bool Recorded(Creature* actor)
{
    if (!actor || actor->GetCurrentWaypointID() != 5 || !Pose(actor)) return false;
    auto movement = actor->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE);
    auto path = Path(actor);
    // Native OnArrived writes node5 even without an AI; the nonrepeating
    // six-node generator can already be retired before InstanceMap::Update.
    return !movement || movement->GetMovementGeneratorType() == IDLE_MOTION_TYPE ||
        (path && path->GetPathId() == 9859005);
}
}
