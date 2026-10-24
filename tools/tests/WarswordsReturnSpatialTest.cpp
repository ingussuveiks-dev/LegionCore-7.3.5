#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using uint16 = uint16_t;
using uint32 = uint32_t;
struct WorldMapAreaEntry
{
    uint32 ID = 0;
    uint16 AreaID = 0;
    float LocLeft = 0, LocRight = 0, LocTop = 0, LocBottom = 0;
    uint32 MapID = 0, parent = 0;
};
std::map<uint16, WorldMapAreaEntry const*> _worldMapArea;
namespace Trinity::Containers
{
    template<class Map> auto MapGetValuePtr(Map const& values, uint16 key) -> typename Map::mapped_type
    {
        auto found = values.find(key);
        return found == values.end() ? nullptr : found->second;
    }
}
struct DB2Manager
{
    WorldMapAreaEntry const* GetWorldMapArea(uint16);
    void Zone2MapCoordinates(float&, float&, uint32);
    void Map2ZoneCoordinates(float&, float&, uint32);
};
#include "Lookup.inc"
#include "ToWorld.inc"
#include "ToZone.inc"
int main(int argc, char** argv)
{
    assert(argc == 3);
    std::ifstream catalogue(argv[1]), world(argv[2]);
    assert(catalogue && world);
    std::map<uint32, WorldMapAreaEntry> areas;
    std::string line, tag;
    while (std::getline(catalogue, line))
    {
        std::istringstream fields(line);
        fields >> tag;
        if (tag != "WORLD_AREA") continue;
        WorldMapAreaEntry area;
        fields >> area.ID >> area.AreaID >> area.LocLeft >> area.LocRight >> area.LocTop >> area.LocBottom >> area.MapID >> area.parent;
        assert(!fields.fail()); areas.emplace(area.ID, area);
    }
    assert(areas.size() == 2 && areas.at(1017).AreaID == 7541 && areas.at(1017).MapID == 1220);
    assert(areas.at(1034).AreaID == 7830 && areas.at(1034).MapID == 1511);
    for (auto const& [identity, area] : areas) _worldMapArea[area.AreaID] = &area;
    DB2Manager manager;
    uint32 guid, map, zone, area;
    float weaponX, weaponY, weaponZ;
    world >> tag >> guid >> map >> zone >> area >> weaponX >> weaponY >> weaponZ;
    assert(!world.fail() && tag == "weapon" && guid == 134150 && map == 1511 && zone == 7541 && area == 7830);
    assert(manager.GetWorldMapArea(static_cast<uint16>(zone))->ID == 1017 && manager.GetWorldMapArea(static_cast<uint16>(area))->ID == 1034);
    uint32 cases = 0;
    for (uint32 selection : {zone, area}) for (uint32 horizontal = 0; horizontal <= 100; horizontal += 10)
        for (uint32 vertical = 0; vertical <= 100; vertical += 10)
    {
        float worldX = float(horizontal), worldY = float(vertical);
        manager.Zone2MapCoordinates(worldX, worldY, selection);
        manager.Map2ZoneCoordinates(worldX, worldY, selection);
        assert(std::abs(worldX - horizontal) < 0.001f && std::abs(worldY - vertical) < 0.001f);
        ++cases;
    }
    float mappedX = weaponX, mappedY = weaponY;
    manager.Map2ZoneCoordinates(mappedX, mappedY, zone);
    assert(std::abs(mappedX - 55.72f) < 0.02f && std::abs(mappedY - 42.96f) < 0.02f);
    float observedX = 55.73f, observedY = 43.02f;
    manager.Zone2MapCoordinates(observedX, observedY, zone);
    assert(std::hypot(observedX - weaponX, observedY - weaponY) < 5.f);
    float beamX = 55.40f, beamY = 42.94f;
    manager.Zone2MapCoordinates(beamX, beamY, zone);
    float distance = std::hypot(beamX - weaponX, beamY - weaponY);
    assert(distance > 15.f && distance < 25.f);
    float wrongX = 55.40f, wrongY = 42.94f;
    manager.Zone2MapCoordinates(wrongX, wrongY, area);
    assert(std::hypot(wrongX - beamX, wrongY - beamY) > 200.f);
    float missingX = 55.40f, missingY = 42.94f;
    manager.Zone2MapCoordinates(missingX, missingY, 65535);
    assert(missingX == 55.40f && missingY == 42.94f);
    manager.Map2ZoneCoordinates(missingX, missingY, 65535);
    assert(missingX == 55.40f && missingY == 42.94f);
    std::cout << "SPATIAL: weapon=" << mappedX << ',' << mappedY << " on zone7541; sampled beam=" << beamX << ',' << beamY
        << " distance=" << distance << "yd, wrong area7830 result=" << wrongX << ',' << wrongY << '\n';
    std::cout << "PASS: " << cases << " actual native zone-map round trips and missing-zone behavior; typed rows distinguish Stormheim zone7541 from instance area7830. Sampled video mapping is inferred XY only, not retail spawn/actor identity, ground Z, actual rendered placement or client travel.\n";
}
