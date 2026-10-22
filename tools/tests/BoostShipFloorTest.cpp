#include <set>
#include "GameObjectModel.h"
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <cmath>
#include <iostream>

struct ShipOwner : GameObjectModelOwnerBase
{
    uint32 display;
    GameObject const* identity;
    bool IsSpawned() const override { return true; }
    GameObject const* GetOwner() const override { return identity; }
    uint32 GetDisplayId() const override { return display; }
    uint8 GetNameSetId() const override { return 0; }
    bool InSamePhaseId(std::set<uint32> const& phases, bool) const override { return phases.count(1); }
    G3D::Vector3 GetPosition() const override { return {10,20,100}; }
    float GetOrientation() const override { return 0.37f; }
    float GetScale() const override { return 1; }
    void DebugVisualizeCorner(G3D::Vector3 const&) const override { }
};

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string root = argv[1];
    LoadGameObjectModelList(root);
    DB2Storage<WorldSafeLocsEntry> locations("WorldSafeLocs.db2", WorldSafeLocsLoadInfo::Instance());
    if (!locations.Load((root + "dbc/enUS/").c_str(),0)) return 2;
    for (auto pair : {std::make_pair(5219u,34338u), std::make_pair(5752u,34366u)})
    {
        auto location = locations.LookupEntry(pair.first);
        if (!location) return 3;
        auto owner = std::make_unique<ShipOwner>();
        owner->display = pair.second;
        int token = 0;
        owner->identity = reinterpret_cast<GameObject const*>(&token);
        std::unique_ptr<GameObjectModel> model(GameObjectModel::Create(std::move(owner),root));
        if (!model) return 4;
        model->enableCollision(true);
        float x=10+std::cos(0.37f)*location->Loc.X-std::sin(0.37f)*location->Loc.Y;
        float y=20+std::sin(0.37f)*location->Loc.X+std::cos(0.37f)*location->Loc.Y;
        float z=100+location->Loc.Z;
        float height=model->getHeight(x,y,z+2,5,{1},true);
        std::cout << "SHIP " << pair.second << " safe " << pair.first << " local "
                  << location->Loc.X << ',' << location->Loc.Y << ',' << location->Loc.Z
                  << " floor " << height << " delta " << height-z << '\n';
        if (!std::isfinite(height) || std::fabs(height-z)>2) return 5;
        if (std::isfinite(model->getHeight(x,y,z+2,5,{2},true))) return 6;
        model->enableCollision(false);
        if (std::isfinite(model->getHeight(x,y,z+2,5,{1},true))) return 7;
    }
    std::cout << "PASS: both actual 7.3.5 ship models support their native WorldSafeLoc deck poses; wrong phase/disabled collision rejected\n";
}
