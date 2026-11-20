#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using uint32 = std::uint32_t;

struct AreaTrigger
{
    uint32 GetRealEntry() const { return entry; }
    int GetDuration() const { return duration; }
    float GetPositionZ() const { return 0.0f; }
    float GetExactDist2d(float targetX, float targetY) const { return std::hypot(targetX - x, targetY); }
    float GetRadius() const { return 6.0f; }
    uint32 entry = 9645;
    int duration = 10000;
    float x = 0.0f;
};

struct Unit
{
    bool IsAlive() const { return alive; }
    bool IsInMap(AreaTrigger const*) const { return sameMap; }
    bool InSamePhase(AreaTrigger const*) const { return samePhase; }
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; }
    float x = 0, y = 0, z = 0;
    bool alive = true, sameMap = true, samePhase = true;
};

#include "AgathaFelLava.inc"

int main()
{
    Unit player;
    AreaTrigger patch;
    assert(IsInsideAgathaFelLava(patch, player));
    player.x = 6.0f;
    assert(IsInsideAgathaFelLava(patch, player));
    player.x = 6.01f;
    assert(!IsInsideAgathaFelLava(patch, player));
    player.x = 0;
    player.z = 4.0f;
    assert(!IsInsideAgathaFelLava(patch, player));
    player.z = -4.0f;
    assert(!IsInsideAgathaFelLava(patch, player));
    player.z = 0;
    player.sameMap = false;
    assert(!IsInsideAgathaFelLava(patch, player));
    player.sameMap = true;
    player.samePhase = false;
    assert(!IsInsideAgathaFelLava(patch, player));
    player.samePhase = true;
    player.alive = false;
    assert(!IsInsideAgathaFelLava(patch, player));
    player.alive = true;
    patch.entry = 9743;
    assert(!IsInsideAgathaFelLava(patch, player));
    patch.entry = 9645;

    AreaTrigger other;
    other.x = 5.0f;
    std::array<AreaTrigger, 2> areas{patch, other};
    auto inside = [&player, &areas]
    {
        return std::any_of(areas.begin(), areas.end(), [&player](AreaTrigger const& area) { return IsInsideAgathaFelLava(area, player); });
    };
    assert(inside());
    areas[0].duration = 0;
    assert(inside());
    areas[1].duration = 0;
    assert(!inside());
    std::cout << "PASS Fel Lava requires the actual live patch, height, range and phase, including overlapping patches.\n";
}
