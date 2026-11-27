#ifndef LEGION_MARDUM_INVASION_BANNER_H
#define LEGION_MARDUM_INVASION_BANNER_H

#include "GameObject.h"

namespace MardumInvasionBanner
{
constexpr uint32 Quest = 40077;

inline bool IsBanner(GameObject const& object)
{
    return object.GetMapId() == 1481 && object.GetEntry() == 244898 && object.GetDBTableGUIDLow() == 131210;
}
}

#endif
