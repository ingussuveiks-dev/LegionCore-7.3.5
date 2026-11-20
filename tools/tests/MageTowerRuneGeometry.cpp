#include "GridMap.h"
#include "MMapManager.h"
#include "MapDefines.h"
#include "VMapManager2.h"
#include <cmath>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    constexpr uint32 map = 1684;
    constexpr int tileX = 25, tileY = 29;
    constexpr float raestX = 3439.03f, raestY = 1535.98f, raestZ = 440.767f;
    std::string root = argv[1];
    GridMap grid;
    if (grid.loadData((root + "/maps/1684_25_29.map").c_str()) != GridMap::LoadResult::Ok)
        return 2;
    VMAP::VMapManager2 vmap;
    vmap.InitializeThreadUnsafe({ { map, {} } });
    if (vmap.loadMap((root + "/vmaps/").c_str(), map, tileX, tileY) != VMAP::VMAP_LOAD_RESULT_OK)
        return 3;
    MMAP::MMapManager mmap;
    mmap.InitializeThreadUnsafe({ { map, {} } });
    if (!mmap.loadMap(root + "/", map, tileX, tileY) || !mmap.loadMapInstance(root + "/", map, 0))
        return 4;
    auto* query = mmap.GetNavMeshQuery(map, 0);
    if (!query)
        return 5;

    float source[3] = { 1523.37f, 436.789f, 3456.88f }, nearest[3], extent[3] = { 1, 2, 1 };
    dtQueryFilter filter;
    filter.setIncludeFlags(NAV_GROUND);
    dtPolyRef sourceRef = 0;
    query->findNearestPoly(source, extent, &filter, &sourceRef, nearest);
    if (!sourceRef)
        return 6;

    unsigned accepted = 0;
    for (unsigned sample = 0; sample <= 40; ++sample)
    {
        bool valid = false;
        float angle = -1.6f + 0.05f * sample;
        for (unsigned attempt = 0; attempt < 4; ++attempt, angle += 0.6f)
        {
            float x = raestX + 8.0f * std::cos(angle), y = raestY + 8.0f * std::sin(angle);
            float terrain = grid.getHeight(x, y);
            float z = vmap.getHeight(map, x, y, raestZ + 3.0f, 10.0f);
            if (terrain <= raestZ + 3.0f && terrain > z)
                z = terrain;
            if (!std::isfinite(z) || z < 435.0f || z > 439.0f ||
                !vmap.isInLineOfSight(map, raestX, raestY, raestZ + 2.0f, x, y, z + 1.0f, VMAP::ModelIgnoreFlags::Nothing))
                continue;

            float endpoint[3] = { y, z, x }, projected[3];
            dtPolyRef ref = 0, path[128];
            int count = 0;
            query->findNearestPoly(endpoint, extent, &filter, &ref, projected);
            if (ref)
                query->findPath(sourceRef, ref, nearest, projected, &filter, path, &count, 128);
            valid = count && path[count - 1] == ref &&
                std::abs(projected[0] - y) < 0.5f && std::abs(projected[2] - x) < 0.5f &&
                std::abs(projected[1] - z) < 1.0f;
            if (!valid)
                return 7;
            ++accepted;
            break;
        }
        if (!valid)
            return 8;
    }
    std::cout << "PASS " << accepted << " arena angle samples land on reachable native ground.\n";
}
