#ifndef ACQUISITION_SCENARIO_ROUTE_H
#define ACQUISITION_SCENARIO_ROUTE_H

#include "WarriorScenarioRoute.h"

namespace AcquisitionScenarioRoute
{
// Retain the existing four-field native route representation and Warrior data.
using Entry = WarriorScenarioRoute::Entry;
inline Entry const* Find(std::uint32_t map, std::uint32_t playerClass)
{
    if (playerClass == 1)
        return WarriorScenarioRoute::Find(map);
    static Entry const marksmanship = {1489, 972, 40419, 201674};
    if (playerClass == 3 && map == marksmanship.map)
        return &marksmanship;
    static Entry const survival = {1511, 973, 40385, 198950};
    return playerClass == 3 && map == survival.map ? &survival : nullptr;
}
}
#endif
