#include <array>
#include <map>
#include <set>
#include "VMapManager2.h"
#include "VMapFactory.h"
#include "GridMap.h"
#include "Position.h"
#include "GridDefines.h"
#include "MovementTypedefs.h"
#include "UnitDefines.h"
#include "ObjectDefines.h"
#include "GameTime.h"
#include <filesystem>
#include <iostream>
#include <cmath>

// Map.h's matching native constants; the full Map owns a live world/grid.
constexpr float MAX_HEIGHT=100000.f, INVALID_HEIGHT=-100000.f;

// Execute the actual native first-collision operation against extracted WMO
// surfaces. Dynamic live objects, water/flying and phase trees are endpoints;
// the actual runtime operation still checks them normally.
namespace SplitGeometry
{
struct GeometryMap
{
    GridMap grid;
    bool getObjectHitPos(std::set<uint32> const&, bool, float,float,float,float,float,float,float&,float&,float&,float) { return false; }
    float GetGridMapHeigh(float x,float y) { return grid.getHeight(x,y); }
};
struct Unit { bool CanFly() const { return false; } float GetHoverOffset() const { return 0; } };
struct WorldObject : Position
{
    GeometryMap map;
    struct { struct {uint32 lastTimeUpdate=0;} fall; bool HasMovementFlag(uint32) const { return false; } } m_movementInfo;
    Unit unit;
    uint32 GetMapId() const { return 1539; }
    bool IsInWater() const { return false; } bool IsPlayer() const { return false; } bool IsUnitOwnedByPlayer() const { return false; }
    std::set<uint32> const& GetPhases() const { static std::set<uint32> phases; return phases; }
    GeometryMap* GetMap() { return &map; } Unit const* ToUnit() const { return &unit; }
    float GetHeight(float x,float y,float z,bool) { return VMAP::VMapFactory::createOrGetVMapManager()->getHeight(1539,x,y,z,2000.f); }
    void UpdateAllowedPositionZ(float x,float y,float& z,float* ground) { *ground=GetHeight(x,y,z+2,true);if(*ground>INVALID_HEIGHT)z=*ground; }
    void MovePositionToFirstCollision(Position&,float,float);
};
#include "../../build-extractors/runtime-test/stromkar-split-geometry/FirstCollision.inc"
}
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    auto root=std::filesystem::path(argv[1]).lexically_normal();if(root.filename().empty())root=root.parent_path();root=root.parent_path().parent_path();
    auto vm=static_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager());vm->InitializeThreadUnsafe({{1539,{}}});
    std::string vmap=(root/"vmaps").string()+"/";for(int x=27;x<=29;++x)for(int y=26;y<=28;++y)vm->loadMap(vmap.c_str(),1539,x,y);
    SplitGeometry::WorldObject actor;actor.Relocate(1815.32f,2291.55f,45.4565f,0.697497f);
    auto tile=root/"maps"/"1539_28_27.map";if(actor.map.grid.loadData(tile.string().c_str())!=GridMap::LoadResult::Ok)return 2;
    std::cout.precision(9);uint32 valid=0;
    for(uint32 rotation=0;rotation<12;++rotation)
    {
        actor.SetOrientation(0.697497f+float(rotation)*0.5235988f);std::array<Position,3> poses;bool supported=true;
        for(uint32 i=0;i<3;++i){poses[i]=actor;actor.MovePositionToFirstCollision(poses[i],6.f,float(i)*2.0943951f);supported=supported&&poses[i].GetExactDist2d(&actor)>=2&&std::fabs(poses[i].GetPositionZ()-actor.GetPositionZ())<=4;for(uint32 j=0;j<i;++j)supported=supported&&poses[i].GetExactDist2d(&poses[j])>=2;std::cout<<"POSE\t"<<rotation<<'\t'<<i<<'\t'<<poses[i].GetPositionX()<<'\t'<<poses[i].GetPositionY()<<'\t'<<poses[i].GetPositionZ()<<'\n';}
        std::cout<<"SUPPORTED\t"<<rotation<<'\t'<<supported<<'\n';valid+=supported;
    }
    std::cout<<"VALID_NATIVE_WMO_ORIENTATIONS\t"<<valid<<"/12; dynamic live obstacles remain native runtime checks\n";
    return valid==12?0:3;
}
