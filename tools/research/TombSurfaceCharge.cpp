#include "../../src/server/game/Movement/SurfaceSwimPath.h"
#include "GridMap.h"
#include "MMapManager.h"
#include "ModelIgnoreFlags.h"
#include "VMapManager2.h"
#include <filesystem>
#include <iostream>

namespace
{
    constexpr uint32 MapId = 1539;
    struct PositionSample { float x, y, z; };

    bool CheckTarget(VMAP::VMapManager2& models, GridMap& grid, dtNavMeshQuery const& query,
        PositionSample target, int expectedStarts, int expectedOrdinary)
    {
        float liquid = 0, bed = 0;
        uint32 type = 0;
        if (models.GetLiquidLevel(MapId, target.x, target.y, target.z, 0, liquid, bed, type) && liquid > target.z)
            return false;
        if (std::abs(models.getHeight(MapId, target.x, target.y, target.z + 0.5f, 5.0f) - target.z) > 0.5f)
            return false;

        dtQueryFilter filter;
        filter.setIncludeFlags(NAV_GROUND | NAV_WATER | NAV_MAGMA_SLIME);
        float const extents[] = {3, 5, 3};
        float const end[] = {target.y, target.z, target.x};
        float nearest[3];
        dtPolyRef endPoly = 0;
        if (dtStatusFailed(query.findNearestPoly(end, extents, &filter, &endPoly, nearest)) || !endPoly)
            return false;

        int starts = 0, ordinary = 0, recovered = 0;
        for (int dx = -25; dx <= 25; ++dx)
            for (int dy = -25; dy <= 25; ++dy)
            {
                float distance = std::hypot(float(dx), float(dy));
                if (distance < 10 || distance > 25)
                    continue;
                float x = target.x + dx, y = target.y + dy;
                constexpr float waterHint = 68.17f;
                if (!models.GetLiquidLevel(MapId, x, y, waterHint, 0, liquid, bed, type) ||
                    std::abs(liquid - waterHint) > 2.0f)
                    continue;
                float floor = models.getHeight(MapId, x, y, waterHint + 0.5f, 100.0f);
                float terrain = grid.getHeight(x, y);
                if (terrain < waterHint + 0.5f)
                    floor = std::max(terrain, floor);
                if (floor < -10000 || liquid - floor < 2.5f)
                    continue;
                float const z = liquid - 0.6f;
                if (!models.isInLineOfSight(MapId, x, y, z+1.5f, target.x, target.y, target.z+1.5f, VMAP::ModelIgnoreFlags::Nothing) ||
                    models.isInLineOfSight(MapId, x, y, z+1.5f, x, y, z+70, VMAP::ModelIgnoreFlags::Nothing))
                    continue;
                ++starts;
                float const start[] = {y, z, x};
                dtPolyRef startPoly = 0, path[74];
                int count = 0;
                if (dtStatusFailed(query.findNearestPoly(start, extents, &filter, &startPoly, nearest)) || !startPoly)
                    return false;
                auto status = query.findPath(startPoly, endPoly, start, end, &filter, path, &count, 74);
                if (dtStatusFailed(status) || !count)
                    return false;
                if (path[count-1] == endPoly)
                    ++ordinary;
                else
                {
                    if (!dtStatusDetail(status, DT_PARTIAL_RESULT))
                        return false;
                    int connected = Movement::FindSurfaceSwimCorridor(query, filter, start, liquid, startPoly, endPoly, end, path, 74);
                    if (!connected || path[connected-1] != endPoly)
                        return false;
                    // A truncated corridor or water belonging to a different vertical layer must still fail.
                    if (Movement::FindSurfaceSwimCorridor(query, filter, start, liquid, startPoly, endPoly, end, path, 1) ||
                        Movement::FindSurfaceSwimCorridor(query, filter, start, liquid+100, startPoly, endPoly, end, path, 74))
                        return false;
                    ++recovered;
                }
            }
        std::cout << "Eligible starts " << starts << ", ordinary routes " << ordinary << ", recovered " << recovered << '\n';
        return starts == expectedStarts && ordinary == expectedOrdinary && recovered == starts - ordinary;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    auto dbc = std::filesystem::path(argv[1]).lexically_normal();
    if (dbc.filename().empty())
        dbc = dbc.parent_path();
    auto root = dbc.parent_path().parent_path();
    VMAP::VMapManager2 models;
    models.InitializeThreadUnsafe({{MapId,{}}});
    MMAP::MMapManager navigation;
    navigation.InitializeThreadUnsafe({{MapId,{}}});
    GridMap grid;
    if (models.loadMap(((root/"vmaps").string()+"/").c_str(),MapId,28,27) != VMAP::VMAP_LOAD_RESULT_OK ||
        grid.loadData((root/"maps/1539_28_27.map").string().c_str()) != GridMap::LoadResult::Ok ||
        !navigation.loadMap(root.string()+"/",MapId,28,27) || !navigation.loadMapInstance(root.string()+"/",MapId,0))
        return 2;
    auto query = navigation.GetNavMeshQuery(MapId,0);
    // Creature placements for Shadowlord Slaghammer and Flesh Spawn at the cave shore.
    return query && CheckTarget(models,grid,*query,{2044.88f,2334.23f,68.8537f},166,118) &&
        CheckTarget(models,grid,*query,{2041.5f,2348.44f,70.694f},183,115) ? 0 : 3;
}
