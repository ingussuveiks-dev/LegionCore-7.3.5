#ifndef TRINITY_CREATURE_SCALING_H
#define TRINITY_CREATURE_SCALING_H

#include <cstdint>

namespace CreatureScaling
{
constexpr bool HasValidLevelRange(std::uint16_t minimum, std::uint16_t maximum)
{
    return minimum > 0 && maximum >= minimum && maximum <= 255;
}
}

#endif
