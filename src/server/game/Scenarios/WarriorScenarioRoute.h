#ifndef WARRIOR_SCENARIO_ROUTE_H
#define WARRIOR_SCENARIO_ROUTE_H

#include <cstdint>

namespace WarriorScenarioRoute
{
struct Entry
{
    std::uint32_t map, scenario, quest, spell;
};

inline Entry const* Find(std::uint32_t map)
{
    static Entry const entries[] = {
        {1495, 909, 39191, 187803},
        {1511, 944, 40043, 193335},
        {1539, 1037, 41105, 203920}
    };
    for (Entry const& entry : entries)
        if (entry.map == map)
            return &entry;
    return nullptr;
}
}
#endif
