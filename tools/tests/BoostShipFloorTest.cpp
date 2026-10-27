#include <set>
#include "GameObjectModel.h"
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Containers.h"
#include "MovementPackets.h"
#include "Transport.h"
#include "TransportGroundPath.h"
#include "TransportJumpPath.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace PriorLeap { using Movement::computeFallElevation; using Movement::gravity; }
#include "PriorTransportJumpPath.inc"
namespace PriorCharge { using Movement::PointsArray; }
#include "PriorTransportGroundPath.inc"

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
        std::vector<std::pair<float,float>> offsets{{5.f,0.f},{8.f,-6.f},{8.f,-2.f},{8.f,2.f},{8.f,6.f},{5.f,-8.f},{4.f,-7.f},{4.f,7.f},{12.f,0.f},{3.f,4.f},{6.f,-7.f},{5.f,-10.f}};
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
        // Use the real moving-ship model, transforms and production corridor.
        auto world=[&](Position local){float x,y,z,o;local.GetPosition(x,y,z,o);transport.CalculatePassengerPosition(x,y,z,&o);return Position{x,y,z,o};};
        Position start=base,finish=Offset(base,12,0,0,0);
        if(!PreparePassengerPosition(&transport,start,true)||!PreparePassengerPosition(&transport,finish,true))return 15;
        // Pair the reported uneven-deck rejection with the pinned old helper,
        // using actual 26972 models rather than a flat mock.
        Position regression=Offset(base,pair.first==5219?-4.0f:0.0f,pair.first==5219?0.0f:-8.0f,0,0);
        if(!PreparePassengerPosition(&transport,regression,true))return 28;
        if(PriorLeap::CheckTransportJumpPath(&transport,world(base),world(regression),2,.4f,2.5f)==PriorLeap::TransportJumpResult::Complete)return 29;
        if(Movement::CheckTransportJumpPath(&transport,world(base),world(regression),2,.4f,2.5f)!=Movement::TransportJumpResult::Complete)return 30;
        Position selected=world(finish); selected.m_positionZ+=0.3f;
        if(PriorLeap::CheckTransportJumpPath(&transport,world(base),selected,2,.4f,2.5f)!=PriorLeap::TransportJumpResult::NotOnDeck)return 31;
        if(Movement::CheckTransportJumpPath(&transport,world(base),selected,2,.4f,2.5f)!=Movement::TransportJumpResult::Complete)return 32;
        unsigned supported=0,accepted=0;
        for(float forward=-8;forward<=24;forward+=2)
            for(float side=-12;side<=12;side+=1)
            {
                Position landing=Offset(base,forward,side,0,0);
                if(!PreparePassengerPosition(&transport,landing,true))continue;
                ++supported;
                auto result=Movement::CheckTransportJumpPath(&transport,world(base),world(landing),2.0f,0.4f,2.5f);
                if(result==Movement::TransportJumpResult::Complete)++accepted;
            }
        std::cout<<"SURVEY "<<pair.second<<" supported "<<supported<<" accepted "<<accepted<<'\n';
        // Find genuine model edges: center supported, part of the actor radius
        // over empty space, and the full body corridor still unobstructed.
        unsigned edges=0;
        for(float forward=0;forward<=24;forward+=2)
            for(float side=-28;side<=28;side+=0.2f)
            {
                Position landing=Offset(base,forward,side,0,0);
                if(!PreparePassengerPosition(&transport,landing,true))continue;
                auto p=world(landing);
                bool missing=false;
                for(auto delta:{G3D::Vector3(.4f,0,0),G3D::Vector3(-.4f,0,0),G3D::Vector3(0,.4f,0),G3D::Vector3(0,-.4f,0)})
                    if(!std::isfinite(model->getHeight(p.GetPositionX()+delta.x,p.GetPositionY()+delta.y,p.GetPositionZ()+.1f,1.2f,{1},false)))missing=true;
                if(missing&&Movement::CheckTransportJumpPath(&transport,world(base),p,2,.4f,2.5f)==Movement::TransportJumpResult::Complete)++edges;
            }
        std::cout<<"SUPPORTED EDGES "<<pair.second<<' '<<edges<<'\n';
        // Horde railings can obstruct the edge corridor; do not bypass them.
        if(pair.first==5219&&!edges)return 33;
        if(!PreparePassengerPosition(&transport,start,true)||!PreparePassengerPosition(&transport,finish,true))return 15;
        Movement::PointsArray charge;
        if(pair.second==34366)for(auto offset:{std::make_pair(-8.0f,-8.0f),std::make_pair(-6.0f,0.0f)})
        {
            Position ledge=Offset(base,offset.first,offset.second,0,0);
            if(!PreparePassengerPosition(&transport,ledge,true))return 36;
            auto a=world(ledge),b=world(finish);
            float previous=a.GetPositionZ(),up=0,down=0;
            Position previousPoint=a,crest=a;
            for(unsigned i=1;i<=100;++i){float t=float(i)/100;
                float x=a.GetPositionX()+(b.GetPositionX()-a.GetPositionX())*t,y=a.GetPositionY()+(b.GetPositionY()-a.GetPositionY())*t;
                float z=model->getHeight(x,y,previous+1.7f,5,{1},false);
                if(!std::isfinite(z))return 37;
                up=std::max(up,z-previous);
                if(previous-z>down){down=previous-z;crest=previousPoint;}
                previous=z;previousPoint=Position{x,y,z,0};
            }
            std::cout<<"CURB "<<pair.second<<' '<<offset.first<<','<<offset.second<<" up "<<up<<" down "<<down<<" crest distance "<<crest.GetExactDist(&b)<<'\n';
            if(down<=.6f||down>1.0f||up>.6f)return 38;
            if(PriorCharge::BuildTransportGroundPath(&transport,a,b,2,74,charge)!=PriorCharge::TransportGroundResult::NoPath)return 39;
            if(Movement::BuildTransportGroundPath(&transport,a,b,2,74,charge)!=Movement::TransportGroundResult::Complete)return 40;
            // Native 25-yard Charge allocates ten points after its range margin.
            if(Movement::BuildTransportGroundPath(&transport,a,b,2,10,charge)!=Movement::TransportGroundResult::Complete)return 42;
            // Standing on the actual raised trim, not only approaching it.
            if(Movement::BuildTransportGroundPath(&transport,crest,b,2,74,charge)!=Movement::TransportGroundResult::Complete)return 41;
        }
        Position unsnapped=world(start);unsnapped.m_positionZ+=.55f;
        auto priorGround=PriorCharge::BuildTransportGroundPath(&transport,unsnapped,world(finish),2,74,charge);
        std::cout<<"CHARGE HEIGHT BASELINE "<<pair.second<<' '<<int(priorGround)<<'\n';
        if(priorGround!=PriorCharge::TransportGroundResult::NoPath)return 34;
        if(Movement::BuildTransportGroundPath(&transport,unsnapped,world(finish),2,74,charge)!=Movement::TransportGroundResult::Complete)return 35;
        if(Movement::BuildTransportGroundPath(&transport,world(start),world(finish),2.0f,74,charge)!=Movement::TransportGroundResult::Complete||charge.size()<2)return 16;
        if(Movement::CheckTransportJumpPath(&transport,world(start),world(finish),2.0f,0.4f,2.5f)!=Movement::TransportJumpResult::Complete)return 23;
        if(Movement::BuildTransportGroundPath(&transport,world(start),world(finish),2.0f,2,charge)!=Movement::TransportGroundResult::TooLong||!charge.empty())return 17;
        Position voidEnd=Offset(base,0,100,0,0);
        if(Movement::BuildTransportGroundPath(&transport,world(start),world(voidEnd),2.0f,74,charge)!=Movement::TransportGroundResult::NoPath||!charge.empty())return 18;
        Position airborne=world(finish);airborne.m_positionZ+=5;
        if(Movement::CheckTransportJumpPath(&transport,world(start),airborne,2.0f,0.4f,2.5f)!=Movement::TransportJumpResult::NotOnDeck)return 24;
        if(Movement::CheckTransportJumpPath(&transport,world(start),world(finish),2.0f,std::numeric_limits<float>::quiet_NaN(),2.5f)!=Movement::TransportJumpResult::NoPath)return 25;
        if(Movement::BuildTransportGroundPath(&transport,world(start),airborne,2.0f,74,charge)!=Movement::TransportGroundResult::NoPath)return 19;
        if(Movement::BuildTransportGroundPath(&transport,world(start),world(finish),2.0f,74,charge,100.0f)!=Movement::TransportGroundResult::NoPath)return 22;
        transport.SetPhaseId({2},false);
        if(Movement::CheckTransportJumpPath(&transport,world(start),world(finish),2.0f,0.4f,2.5f)!=Movement::TransportJumpResult::NotOnDeck)return 26;
        if(Movement::BuildTransportGroundPath(&transport,world(start),world(finish),2.0f,74,charge)!=Movement::TransportGroundResult::NoPath)return 20;
        transport.SetPhaseId({1},false);
        if (std::isfinite(model->getHeight(x,y,z+2,5,{2},true))) return 6;
        model->enableCollision(false);
        if(Movement::CheckTransportJumpPath(&transport,world(start),world(finish),2.0f,0.4f,2.5f)!=Movement::TransportJumpResult::NoPath)return 27;
        if (std::isfinite(model->getHeight(x,y,z+2,5,{1},true))) return 7;
        Position disabled=base;
        if(PreparePassengerPosition(&transport,disabled,true))return 13;
        if(!PreparePassengerPosition(&transport,air,false)||air.GetPositionZ()!=airZ)return 14;
        if(Movement::BuildTransportGroundPath(&transport,world(start),world(finish),2.0f,74,charge)!=Movement::TransportGroundResult::NoPath)return 21;
    }
    std::cout << "PASS: native ship/DB2, actual Transport transforms and production placement helper put all trainer/entourage/dummy/sparring/Legion/exit poses on their own deck surface; flight, missing floor and disabled collision preserved\n";
}
