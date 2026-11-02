#pragma once

#include "CreatureTextMgr.h"

namespace MonkChenRoutes
{
constexpr uint32 EscortData = 94348, HaitData = 94349;

inline bool Source(Creature* actor)
{
    return actor && actor->GetMapId() == 1014 && actor->GetEntry() == 97778 && actor->GetDBTableGUIDLow() == 362271;
}

inline bool Data()
{
    auto path = sWaypointMgr->GetPathScript(9859006);
    return path && path->size() == 9;
}

inline bool Issued(Creature* actor)
{
    auto path = MonkChenArrival::Path(actor);
    return Data() && path && path->GetPathId() == 9859006 && path->GetCurrentNode() == 0 &&
        actor->GetCurrentWaypointID() == uint32(-1);
}

struct Dispatch
{
    explicit Dispatch(bool& value) : busy(value) { busy = true; }
    ~Dispatch() { busy = false; }
    bool& busy;
};
}
