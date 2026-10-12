#include <set>
#include "GameObjectModel.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

// Use the production collision reader against the extracted 7.3.5 city WMO.
struct DalaranOwner : GameObjectModelOwnerBase
{
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
    std::unique_ptr<GameObjectModel> model(GameObjectModel::Create(std::make_unique<DalaranOwner>(), argv[1]));
    if (!model)
        return 2;
    model->enableCollision(true);
    float x = std::strtof(argv[2], nullptr), y = std::strtof(argv[3], nullptr), z = std::strtof(argv[4], nullptr);
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
    std::cout << "PASS: nine landing samples within 1 unit of the city floor; old landing gap=" << 792 - oldFloor << '\n';
}
