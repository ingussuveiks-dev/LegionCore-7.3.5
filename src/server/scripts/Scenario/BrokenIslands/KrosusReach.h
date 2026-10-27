#ifndef BROKEN_SHORE_KROSUS_REACH_H
#define BROKEN_SHORE_KROSUS_REACH_H

#include <algorithm>
#include <cmath>

namespace BrokenShoreKrosus
{
    // The stationary actor is attacked from the shore. Its animated hands
    // extend beyond the old 30-yard sphere; use the matching model's finite
    // horizontal envelope, retaining ordinary 3D range and LOS checks.
    inline float CombatReach(float current, float const* box, float scale)
    {
        if (!box || !std::isfinite(scale) || scale <= 0.0f)
            return current;
        for (unsigned i = 0; i < 6; ++i)
            if (!std::isfinite(box[i]))
                return current;
        float x = std::max(std::abs(box[0]), std::abs(box[3]));
        float y = std::max(std::abs(box[1]), std::abs(box[4]));
        float reach = std::hypot(x, y) * scale;
        return std::isfinite(reach) ? std::max(current, reach) : current;
    }
}

#endif
