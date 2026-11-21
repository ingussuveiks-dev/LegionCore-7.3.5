#ifndef HALLOWS_END_CRATE_H
#define HALLOWS_END_CRATE_H

#include "Define.h"

namespace HallowsEndCrate
{
struct Route
{
    uint32 Encounter;
    uint32 Return;
    uint32 Reward;
    uint32 StolenCrate;
    uint32 RewardCrate;
    uint64 SanathSpawn;
    uint64 RenSpawn;
};

constexpr Route Alliance{29403, 29411, 29413, 209072, 209076, 202611270, 202611274};
constexpr Route Horde{29427, 29428, 29429, 209094, 209095, 202611271, 202611275};

inline Route const* ForCrate(uint32 entry)
{
    if (entry == Alliance.StolenCrate || entry == Alliance.RewardCrate)
        return &Alliance;
    if (entry == Horde.StolenCrate || entry == Horde.RewardCrate)
        return &Horde;
    return nullptr;
}

inline Route const* ForSanath(uint64 spawn)
{
    if (spawn == Alliance.SanathSpawn)
        return &Alliance;
    if (spawn == Horde.SanathSpawn)
        return &Horde;
    return nullptr;
}

inline Route const* ForRen(uint64 spawn)
{
    if (spawn == Alliance.RenSpawn)
        return &Alliance;
    if (spawn == Horde.RenSpawn)
        return &Horde;
    return nullptr;
}

constexpr bool RefreshVisibility(uint32 quest)
{
    return quest == Alliance.Encounter || quest == Alliance.Return || quest == Alliance.Reward ||
        quest == Horde.Encounter || quest == Horde.Return || quest == Horde.Reward;
}
}

#endif
