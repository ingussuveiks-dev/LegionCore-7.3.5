#pragma once
#include <cstdint>

namespace StormheimFleet
{
enum class Kind : uint8_t { Gun, Flight, Air, Guard, Captain, Frost, Flare, Final, Guide, Parachute };
// Only completed actions are persisted. An interrupted ride/cast never consumes
// a stage. Spawn GUIDs are checked separately by the instance controller.
struct State
{
    bool alliance = false;
    uint32_t stage = 0, air = 0, captains = 0, flares = 0, frost = 0;
    bool final = false;
    bool BoardGun()
    {
        if (!alliance || stage != 0) return false;
        stage = 1; return true;
    }
    bool AirKill()
    {
        if (stage != 1 || air >= 30) return false;
        if (++air == 30) stage = 2;
        return true;
    }
    bool Captain(uint32_t index)
    {
        if (!alliance || stage != 2 || index >= 3 || (captains & (1u<<index))) return false;
        captains |= 1u<<index;
        if (captains == 7) stage = 3;
        return true;
    }
    bool WaveDone()
    {
        if (alliance || (stage != 2 && stage != 3)) return false;
        ++stage; return true;
    }
    bool Flare(uint32_t index)
    {
        if (stage != (alliance ? 3u : 4u) || index >= 7 || (flares & (1u<<index))) return false;
        flares |= 1u<<index;
        if (flares == 127) ++stage;
        return true;
    }
    bool Final()
    {
        if (stage != (alliance ? 4u : 5u) || final) return false;
        final = true; ++stage; return true;
    }
};
inline bool ShouldYield(uint64_t health, uint64_t maximum, uint32_t finalDamage)
{
    if (!maximum || !finalDamage) return false;
    uint64_t threshold = maximum / 5;
    return health <= threshold || finalDamage >= health - threshold;
}
inline uint32_t Bits(uint32_t mask) { uint32_t n=0;for(;mask;mask>>=1)n+=mask&1;return n; }
}
