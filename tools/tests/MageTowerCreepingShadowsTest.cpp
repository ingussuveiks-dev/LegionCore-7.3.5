#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using uint32 = std::uint32_t;

struct AreaTrigger
{
    float GetPositionZ() const { return z; }
    float GetExactDist2d(float targetX, float targetY) const { return std::hypot(targetX - x, targetY - y); }
    float x = 70.0f, y = 100.0f, z = 266.0f;
};

struct Unit
{
    bool IsAlive() const { return alive; }
    bool IsInMap(AreaTrigger const*) const { return sameMap; }
    bool InSamePhase(AreaTrigger const*) const { return samePhase; }
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; }
    float x = 75.0f, y = 100.0f, z = 266.0f;
    bool alive = true, sameMap = true, samePhase = true;
};

#include "CreepingShadows.inc"

int main()
{
    assert(CreepingShadowsRadius(0) == 5.0f);
    assert(CreepingShadowsRadius(120000) > 50.0f);
    assert(CreepingShadowsRadius(120000) < CreepingShadowsRadius(180000));
    assert(CreepingShadowsRadius(300000) == 100.0f);

    AreaTrigger ground;
    Unit target;
    assert(IsInCreepingShadows(ground, target, CreepingShadowsRadius(0)));
    target.x = 75.1f;
    assert(!IsInCreepingShadows(ground, target, CreepingShadowsRadius(0)));
    assert(IsInCreepingShadows(ground, target, CreepingShadowsRadius(500)));
    target.z += 4.1f;
    assert(!IsInCreepingShadows(ground, target, CreepingShadowsRadius(500)));
    target.z -= 8.2f;
    assert(!IsInCreepingShadows(ground, target, CreepingShadowsRadius(500)));
    target.z = ground.z;
    target.sameMap = false;
    assert(!IsInCreepingShadows(ground, target, CreepingShadowsRadius(300000)));
    target.sameMap = true;
    target.samePhase = false;
    assert(!IsInCreepingShadows(ground, target, CreepingShadowsRadius(300000)));
    target.samePhase = true;
    target.alive = false;
    assert(!IsInCreepingShadows(ground, target, CreepingShadowsRadius(300000)));
    std::cout << "PASS Creeping Shadows growth and exact ground, map, phase and life boundaries.\n";
}
