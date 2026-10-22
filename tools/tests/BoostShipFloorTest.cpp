#include <set>
#include "GameObjectModel.h"
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Containers.h"
#include "MovementPackets.h"
#include "Transport.h"
#include <cmath>
#include <iostream>

#include "PreparePassengerPosition.inc"
#include "Offset.inc"

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
        Transport transport;
        transport.Relocate(10,20,100,0.37f);
        transport.SetPhaseId({1},false);
        auto owner = std::make_unique<ShipOwner>();
        owner->display = pair.second;
        owner->identity = &transport;
        auto model=GameObjectModel::Create(std::move(owner),root);
        if (!model) return 4;
        transport.m_model=model; // Native transport owns its collision model.
        model->enableCollision(true);
        float x=10+std::cos(0.37f)*location->Loc.X-std::sin(0.37f)*location->Loc.Y;
        float y=20+std::sin(0.37f)*location->Loc.X+std::cos(0.37f)*location->Loc.Y;
        float z=100+location->Loc.Z;
        float height=model->getHeight(x,y,z+2,5,{1},true);
        std::cout << "SHIP " << pair.second << " safe " << pair.first << " local "
                  << location->Loc.X << ',' << location->Loc.Y << ',' << location->Loc.Z
                  << " floor " << height << " delta " << height-z << '\n';
        if (!std::isfinite(height) || std::fabs(height-z)>2) return 5;
        Position base(location->Loc.X,location->Loc.Y,location->Loc.Z,pair.first==5219?2.937f:6.2787f);
        std::vector<std::pair<float,float>> offsets{{5.f,0.f},{8.f,-6.f},{8.f,-2.f},{8.f,2.f},{8.f,6.f},{5.f,-8.f},{4.f,-7.f},{4.f,7.f},{12.f,0.f},{3.f,4.f}};
        for(int index=0;index<10;++index)offsets.emplace_back(2+float(index%5)*2,-8+float(index/5)*16);
        for(auto const& offset:offsets)
        {
            Position pose=Offset(base,offset.first,offset.second,0,3.14f);
            Position original=pose;
            if(!PreparePassengerPosition(&transport,pose,true))return 8;
            float wx,wy,wz,wo;pose.GetPosition(wx,wy,wz,wo);
            transport.CalculatePassengerPosition(wx,wy,wz,&wo);
            float floor=model->getHeight(wx,wy,wz+2,5,{1},false);
            if(std::fabs(floor-wz)>0.001f || std::fabs(pose.GetPositionX()-original.GetPositionX())>0.001f
                || std::fabs(pose.GetPositionY()-original.GetPositionY())>0.001f)return 9;
            if(!PreparePassengerPosition(&transport,pose,true)||std::fabs(pose.GetPositionZ()-(floor-100))>0.001f)return 10;
        }
        Position air=Offset(base,0,8,3,0);float airZ=air.GetPositionZ();
        if(!PreparePassengerPosition(&transport,air,false)||air.GetPositionZ()!=airZ)return 11;
        Position absent=base;absent.m_positionX+=500;
        if(PreparePassengerPosition(&transport,absent,true))return 12;
        if (std::isfinite(model->getHeight(x,y,z+2,5,{2},true))) return 6;
        model->enableCollision(false);
        if (std::isfinite(model->getHeight(x,y,z+2,5,{1},true))) return 7;
        Position disabled=base;
        if(PreparePassengerPosition(&transport,disabled,true))return 13;
        if(!PreparePassengerPosition(&transport,air,false)||air.GetPositionZ()!=airZ)return 14;
    }
    std::cout << "PASS: native ship/DB2, actual Transport transforms and production placement helper put all trainer/entourage/dummy/sparring/Legion/exit poses on their own deck surface; flight, missing floor and disabled collision preserved\n";
}
