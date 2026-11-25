#include "GridMap.h"
#include "MMapManager.h"
#include "MapDefines.h"
#include "VMapManager2.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Point { float x, y, z; };

int main(int argc, char** argv)
{
    if (argc != 3)
        return 2;

    std::string const data = argv[1];
    std::ifstream input(argv[2]);
    std::vector<Point> nodes;
    for (Point p; input >> p.x >> p.y >> p.z;)
        nodes.push_back(p);
    if (!input.eof() || nodes.size() < 2)
        return 3;

    constexpr uint32 mapId = 1220;
    constexpr int gridX = 33, gridY = 23;
    GridMap terrain;
    VMAP::VMapManager2 models;
    MMAP::MMapManager navigation;
    models.InitializeThreadUnsafe({{mapId, {}}});
    navigation.InitializeThreadUnsafe({{mapId, {}}});
    if (terrain.loadData((data + "/maps/1220_33_23.map").c_str()) != GridMap::LoadResult::Ok ||
        models.loadMap((data + "/vmaps/").c_str(), mapId, gridX, gridY) != VMAP::VMAP_LOAD_RESULT_OK ||
        !navigation.loadMap(data + '/', mapId, gridX, gridY) || !navigation.loadMapInstance(data + '/', mapId, 0))
        return 4;

    auto query = navigation.GetNavMeshQuery(mapId, 0);
    dtQueryFilter filter;
    filter.setIncludeFlags(NAV_GROUND);
    float const extents[] = { 0.9f, 2.0f, 0.9f };
    bool valid = true;
    size_t samples = 0;
    float maxBelow = 0.0f;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        Point const a = nodes[i], b = nodes[(i + 1) % nodes.size()];
        float const position[] = { a.y, a.z, a.x };
        std::array<float, 3> near{};
        dtPolyRef polygon = 0;
        if (int(32 - a.x / 533.33333f) != gridX || int(32 - a.y / 533.33333f) != gridY ||
            !dtStatusSucceed(query->findNearestPoly(position, extents, &filter, &polygon, near.data())) || !polygon)
            return 5;

        // Ordinary waypoint movement uses a straight spline between each pair.
        float below = 0.0f, above = 0.0f;
        int const intervals = std::max(1, int(std::ceil(std::hypot(b.x - a.x, b.y - a.y) * 4.0f)));
        for (int step = 0; step <= intervals; ++step)
        {
            float const t = float(step) / intervals;
            float const x = a.x + (b.x - a.x) * t;
            float const y = a.y + (b.y - a.y) * t;
            float const z = a.z + (b.z - a.z) * t;
            float const floor = std::max(terrain.getHeight(x, y), models.getHeight(mapId, x, y, z + 5.0f, 10.0f));
            below = std::max(below, floor - z);
            above = std::max(above, z - floor);
            ++samples;
        }

        bool const clear = models.isInLineOfSight(mapId, a.x, a.y, a.z + 1.1f, b.x, b.y, b.z + 1.1f, VMAP::ModelIgnoreFlags::Nothing);
        if (!clear || below > 0.075f || above > 0.50f)
        {
            std::cerr << "Unsafe link " << i << " below=" << below << " above=" << above << " clear=" << clear << '\n';
            valid = false;
        }
        maxBelow = std::max(maxBelow, below);
    }

    std::cout << nodes.size() << " cyclic links, " << samples << " native floor samples, max floor intersection=" << maxBelow << '\n';
    return valid ? 0 : 1;
}
