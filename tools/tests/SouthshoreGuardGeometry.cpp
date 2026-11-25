#include "GridMap.h"
#include "MMapManager.h"
#include "MapDefines.h"
#include "VMapManager2.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

struct Point
{
    std::string name;
    float x, y, z;
    std::array<float, 3> ground;
    dtPolyRef polygon = 0;
};

int main(int argc, char** argv)
{
    if (argc != 3 && argc != 4)
        return 2;

    bool survey = argc == 4 && std::string(argv[3]) == "--survey";
    constexpr uint32 mapId = 1280;
    std::string root = argv[1];
    std::ifstream input(argv[2]);
    std::map<std::pair<int, int>, GridMap> grids;
    std::set<std::pair<int, int>> loadedTiles;
    std::vector<Point> patrol;
    VMAP::VMapManager2 vmaps;
    MMAP::MMapManager mmaps;
    vmaps.InitializeThreadUnsafe({{mapId, {}}});
    mmaps.InitializeThreadUnsafe({{mapId, {}}});
    dtQueryFilter filter;
    filter.setIncludeFlags(NAV_GROUND);
    int spawns = 0;
    bool valid = true;
    for (Point p; input >> p.name >> p.x >> p.y >> p.z;)
    {
        int tileX = int(32 - p.x / 533.33333f);
        int tileY = int(32 - p.y / 533.33333f);
        for (int x = tileX - 1; x <= tileX + 1; ++x)
            for (int y = tileY - 1; y <= tileY + 1; ++y)
                if (loadedTiles.emplace(x, y).second)
                {
                    vmaps.loadMap((root + "/vmaps/").c_str(), mapId, x, y);
                    mmaps.loadMap(root + "/", mapId, x, y);
                }

        auto [grid, fresh] = grids.try_emplace(std::pair{tileX, tileY});
        if (fresh && grid->second.loadData((root + "/maps/1280_" + std::to_string(tileX) + "_" + std::to_string(tileY) + ".map").c_str()) != GridMap::LoadResult::Ok)
            return 3;
        if (!mmaps.GetNavMeshQuery(mapId, 0) && !mmaps.loadMapInstance(root + "/", mapId, 0))
            return 4;

        float terrain = grid->second.getHeight(p.x, p.y);
        float collision = vmaps.getHeight(mapId, p.x, p.y, p.z + 2.0f, 8.0f);
        float floor = std::max(terrain, collision);
        float position[3] = {p.y, floor, p.x};
        float extent[3] = {1.0f, 2.0f, 1.0f};
        auto query = mmaps.GetNavMeshQuery(mapId, 0);
        bool supported = std::isfinite(floor) && floor > -10000 &&
            dtStatusSucceed(query->findNearestPoly(position, extent, &filter, &p.polygon, p.ground.data())) &&
            p.polygon && std::abs(p.x - p.ground[2]) < 0.15f && std::abs(p.y - p.ground[0]) < 0.15f &&
            std::abs(floor - p.ground[1]) < 1.0f;
        std::cout << p.name << " terrain=" << terrain << " collision=" << collision << " floor=" << floor
            << " navHeight=" << p.ground[1] << " supported=" << supported << '\n';
        valid &= supported && std::abs(floor - p.z) < 0.15f;
        if (p.name == "PatrolSpawn" || p.name == "PatrolTurn")
            patrol.push_back(p);
        if (p.name != "PatrolTurn")
            ++spawns;
    }

    if (survey)
        return 0;
    if (!input.eof() || !valid || spawns != 4 || patrol.size() != 2)
        return 5;
    auto const& a = patrol[0];
    auto const& b = patrol[1];
    auto query = mmaps.GetNavMeshQuery(mapId, 0);
    for (auto pair : {std::pair{&a, &b}, std::pair{&b, &a}})
    {
        auto const& start = *pair.first;
        auto const& end = *pair.second;
        dtPolyRef corridor[32];
        int count = 0;
        auto status = query->findPath(start.polygon, end.polygon, start.ground.data(), end.ground.data(), &filter, corridor, &count, 32);
        if (status != DT_SUCCESS || count == 0 || corridor[count - 1] != end.polygon ||
            !vmaps.isInLineOfSight(mapId, start.x, start.y, start.z + 1.2f, end.x, end.y, end.z + 1.2f, VMAP::ModelIgnoreFlags::Nothing))
            return 6;
    }
    std::cout << "PASS: all four positions have native ground support and the patrol corridor works in both directions.\n";
}
