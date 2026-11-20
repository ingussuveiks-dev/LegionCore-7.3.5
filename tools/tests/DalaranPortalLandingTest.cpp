#include <set>
#include "GameObjectModel.h"
#include "DynamicTree.h"
#include "GridDefines.h"
#include "GridMap.h"
#include "VMapManager2.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// Use the production collision reader against the extracted 7.3.5 city WMO.
struct DalaranOwner : GameObjectModelOwnerBase
{
    // Identity only: the collision reader stores this pointer in its callback
    // and never dereferences it. No fake GameObject class/layout is linked.
    GameObject const* identity = nullptr;
    GameObject const* GetOwner() const override { return identity; }
    bool IsSpawned() const override { return true; }
    uint32 GetDisplayId() const override { return 35175; }
    uint8 GetNameSetId() const override { return 0; }
    bool InSamePhaseId(std::set<uint32> const& phases, bool) const override
    { return phases.count(7714) || phases.count(7552); }
    G3D::Vector3 GetPosition() const override { return {-11088.3f, -1993.4f, 741.034f}; }
    float GetOrientation() const override { return 5.60251f; }
    float GetScale() const override { return 1; }
    void DebugVisualizeCorner(G3D::Vector3 const&) const override { }
};

int main(int argc, char** argv)
{
    if (argc != 5)
        return 1;
    LoadGameObjectModelList(argv[1]);
    int identityToken = 0;
    auto owner = std::make_unique<DalaranOwner>();
    owner->identity = reinterpret_cast<GameObject const*>(&identityToken);
    std::unique_ptr<GameObjectModel> model(GameObjectModel::Create(std::move(owner), argv[1]));
    if (!model)
        return 2;
    model->enableCollision(true);
    float x = std::strtof(argv[2], nullptr), y = std::strtof(argv[3], nullptr), z = std::strtof(argv[4], nullptr);

    std::string dataDir = argv[1];
    int gx = static_cast<int>(CENTER_GRID_ID - x / SIZE_OF_GRIDS);
    int gy = static_cast<int>(CENTER_GRID_ID - y / SIZE_OF_GRIDS);
    char tile[64];
    std::snprintf(tile, sizeof(tile), "maps/0000_%02d_%02d.map", gx, gy);
    GridMap terrain;
    if (terrain.loadData((dataDir + tile).c_str()) != GridMap::LoadResult::Ok)
        return 8;
    VMAP::VMapManager2 staticModels;
    if (staticModels.loadMap((dataDir + "vmaps").c_str(), 0, gx, gy) != VMAP::VMAP_LOAD_RESULT_OK)
        return 9;
    float terrainZ = terrain.getHeight(x, y);
    float staticWmoZ = staticModels.getHeight(0, x, y, z + 2.f, 5.f);
    auto supportsLanding = [z](float floor) { return floor <= z && z - floor <= 0.25f; };
    if (supportsLanding(terrainZ) || supportsLanding(staticWmoZ))
        return 10;

    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
        {
            float floor = model->getHeight(x + dx, y + dy, z, 10, {7714, 7552}, true);
            if (!std::isfinite(floor) || floor < z - 1.0f || floor > z)
            {
                std::cerr << "Unsafe landing " << x + dx << ',' << y + dy << ',' << z << " floor=" << floor << '\n';
                return 3;
            }
        }
    float oldFloor = model->getHeight(-11099.3f, -2211.6f, 792, 100, {7714, 7552}, true);
    if (oldFloor < 700 || 792 - oldFloor < 30)
        return 4;
    // Use the production dynamic collision tree with the new arrival query's
    // exact search window, not only a mocked 'floor exists' result.
    DynamicMapTree tree;
    tree.insert(*model);
    tree.balance();
    DynamicTreeCallback support;
    float arrivalFloor = tree.getHeight(x, y, z + 2, 5, {7714,7552}, true, &support);
    if (!std::isfinite(arrivalFloor) || std::fabs(arrivalFloor-z) > 2
        || support.go != reinterpret_cast<GameObject*>(&identityToken))
        return 5;
    DynamicTreeCallback hidden;
    if (std::isfinite(tree.getHeight(x,y,z+2,5,{999},true,&hidden)) || hidden.go)
        return 6;
    model->enableCollision(false);
    DynamicTreeCallback disabled;
    if (std::isfinite(tree.getHeight(x,y,z+2,5,{7714,7552},true,&disabled)) || disabled.go)
        return 7;
    tree.remove(*model);
    std::cout << "PASS: nine landing samples within 1 unit of the city floor; old landing gap=" << 792 - oldFloor << '\n';
    std::cout << "PASS: actual Dalaran WMO/dynamic-tree arrival support callback, 5-yard ray, phase rejection and disabled collision\n";
    std::cout << "PASS: Dalaran landing " << z << " requires the dynamic city floor, terrain=" << terrainZ
        << ", baked WMO=" << staticWmoZ << '\n';
}
