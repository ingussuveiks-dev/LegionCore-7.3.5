#include "VMapManager2.h"
#include "GridMap.h"
#include <filesystem>
#include <iostream>
#include <cmath>

// Read-only geometry evidence, not a renderer or a scenario traversal.
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    auto root = std::filesystem::path(argv[1]).lexically_normal();
    if (root.filename().empty()) root = root.parent_path();
    root = root.parent_path().parent_path(); // native reader convention: dbc/enUS/
    VMAP::VMapManager2 vm;
    vm.InitializeThreadUnsafe({{1539,{}}});
    auto vmap = (root / "vmaps").string() + "/";
    constexpr float x = 1882.18f, y = 2163.35f, z = 35.9247f;
    int tx = int(32 - x / 533.33333f), ty = int(32 - y / 533.33333f);
    auto loaded = vm.loadMap(vmap.c_str(), 1539, tx, ty);
    std::cout << "VMAP_LOAD\t" << loaded << '\n';
    GridMap grid;
    auto mapFile = root / "maps" / ("1539_" + std::to_string(tx) + "_" + std::to_string(ty) + ".map");
    auto terrain = grid.loadData(mapFile.string().c_str());
    if (terrain != GridMap::LoadResult::Ok) return 2;
    std::cout.precision(9);
    bool center = false;
    for (float dx : {-14.f,-7.f,0.f,7.f,14.f})
        for (float dy : {-14.f,-7.f,0.f,7.f,14.f})
        {
            float floor = vm.getHeight(1539, x+dx, y+dy, z+5, 15);
            float liquid = 0, liquidFloor = 0; uint32 type = 0;
            bool hasLiquid = vm.GetLiquidLevel(1539, x+dx, y+dy, z+5, 0xFF, liquid, liquidFloor, type);
            std::cout << "SURFACE\t" << dx << '\t' << dy << '\t' << floor << '\t'
                << grid.getHeight(x+dx,y+dy) << '\t' << grid.getLiquidLevel(x+dx,y+dy)
                << '\t' << hasLiquid << '\t' << liquid << '\t' << liquidFloor << '\t' << type << '\n';
            if (!dx && !dy) center = std::isfinite(floor) && floor > -1000;
        }
    return center ? 0 : 3;
}
