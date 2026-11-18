// Build model-local navigation from the actual extracted Maw ship collision.
// The public VMAP mesh is read only; output goes to an explicit staging folder.
#define HARBARON_NATIVE_SUPPORT_ONLY
#include "../tests/HarbaronFragmentNative.cpp"
#include "WorldModel.h"
#include "Recast.h"
#include "DetourNavMeshBuilder.h"
#include "MapDefines.h"
#include "DB2Store.h"
#include "DB2LoadInfo.h"
#include <fstream>

struct ShipModelTag {using Type=VMAP::WorldModel* GameObjectModel::*;friend Type Access(ShipModelTag);};
template struct FixtureAccess<ShipModelTag,&GameObjectModel::iModel>;
struct ShipRotationTag {using Type=G3D::Matrix3 GameObjectModel::*;friend Type Access(ShipRotationTag);};
template struct FixtureAccess<ShipRotationTag,&GameObjectModel::iInvRot>;
struct ShipScaleTag {using Type=float GameObjectModel::*;friend Type Access(ShipScaleTag);};
template struct FixtureAccess<ShipScaleTag,&GameObjectModel::iInvScale>;

static void BuildModel(GameObject& ship,std::filesystem::path const& output,DungeonMapEntry const* floor)
{
    Require(ship.m_model&&ship.m_model->isCollisionEnabled(),"Native ship collision missing");
    auto model=ship.m_model->*Access(ShipModelTag{});Require(model,"Native ship VMAP model absent");
    std::vector<VMAP::GroupModel> groups;model->getGroupModels(groups);
    std::vector<float> vertices;std::vector<int> triangles;
    for(auto& group:groups)
    {
        std::vector<G3D::Vector3> points;std::vector<VMAP::MeshTriangle> faces;VMAP::WmoLiquid* liquid=nullptr;
        group.getMeshData(points,faces,liquid);int offset=int(vertices.size()/3);
        for(auto const& p:points){vertices.push_back(p.y);vertices.push_back(p.z);vertices.push_back(p.x);}
        // Match the native TerrainBuilder's WMO/M2 winding convention.
        for(auto const& f:faces)
            if(ship.m_model->isMapObject()){triangles.push_back(offset+f.idx0);triangles.push_back(offset+f.idx1);triangles.push_back(offset+f.idx2);}
            else{triangles.push_back(offset+f.idx2);triangles.push_back(offset+f.idx1);triangles.push_back(offset+f.idx0);}
    }
    Require(!vertices.empty()&&!triangles.empty(),"Native model has no collision triangles");
    int nv=int(vertices.size()/3),nt=int(triangles.size()/3);rcContext context;
    std::cout<<"INPUT\t"<<ship.GetDisplayId()<<'\t'<<ship.m_model->name<<'\t'<<groups.size()<<'\t'<<nv<<'\t'<<nt<<'\t'<<ship.m_model->isMapObject()<<std::endl;
    rcConfig cfg{};cfg.cs=cfg.ch=0.2666666f;cfg.walkableSlopeAngle=70.f;cfg.walkableHeight=6;cfg.walkableRadius=2;cfg.walkableClimb=2;
    cfg.maxEdgeLen=81;cfg.maxSimplificationError=1.3f;cfg.minRegionArea=rcSqr(8);cfg.mergeRegionArea=rcSqr(20);cfg.maxVertsPerPoly=DT_VERTS_PER_POLYGON;
    cfg.detailSampleDist=cfg.cs*6;cfg.detailSampleMaxError=cfg.ch;
    rcCalcBounds(vertices.data(),nv,cfg.bmin,cfg.bmax);cfg.bmin[0]-=cfg.cs*3;cfg.bmin[2]-=cfg.cs*3;cfg.bmax[0]+=cfg.cs*3;cfg.bmax[2]+=cfg.cs*3;
    std::cout<<"BOUNDS\t"<<cfg.bmin[0]<<'\t'<<cfg.bmin[1]<<'\t'<<cfg.bmin[2]<<'\t'<<cfg.bmax[0]<<'\t'<<cfg.bmax[1]<<'\t'<<cfg.bmax[2]<<std::endl;
    // This display also contains distant scenery. Rasterize only the two native
    // ship floors, retaining every collision triangle intersecting that volume.
    Require(floor&&floor->ID==967&&floor->MapID==1492,"Native ship floor metadata missing");
    G3D::AABox crop=G3D::AABox::empty();
    auto invRot=ship.m_model->*Access(ShipRotationTag{});auto invScale=ship.m_model->*Access(ShipScaleTag{});
    for(float wx:{floor->Min[1]-2.f,floor->Max[1]+2.f})for(float wy:{floor->Min[0]-2.f,floor->Max[0]+2.f})for(float wz:{500.f,600.f})
    {auto p=invRot*(G3D::Vector3(wx,wy,wz)-ship.m_model->getPosition())*invScale;crop.merge(p);}
    auto low=crop.low(),high=crop.high();cfg.bmin[0]=low.y;cfg.bmin[1]=low.z;cfg.bmin[2]=low.x;cfg.bmax[0]=high.y;cfg.bmax[1]=high.z;cfg.bmax[2]=high.x;
    rcCalcGridSize(cfg.bmin,cfg.bmax,cfg.cs,&cfg.width,&cfg.height);
    auto solid=std::unique_ptr<rcHeightfield,decltype(&rcFreeHeightField)>(rcAllocHeightfield(),rcFreeHeightField);
    Require(solid&&rcCreateHeightfield(&context,*solid,cfg.width,cfg.height,cfg.bmin,cfg.bmax,cfg.cs,cfg.ch),"Model heightfield creation failed");
    std::vector<unsigned char> areas(nt,NAV_AREA_GROUND);rcClearUnwalkableTriangles(&context,cfg.walkableSlopeAngle,vertices.data(),nv,triangles.data(),nt,areas.data());
    std::cout<<"RASTER\t"<<cfg.width<<'\t'<<cfg.height<<'\t'<<std::count(areas.begin(),areas.end(),NAV_AREA_GROUND)<<std::endl;
    Require(rcRasterizeTriangles(&context,vertices.data(),nv,triangles.data(),areas.data(),nt,*solid,cfg.walkableClimb),"Model triangle rasterization failed");
    rcFilterLowHangingWalkableObstacles(&context,cfg.walkableClimb,*solid);rcFilterLedgeSpans(&context,cfg.walkableHeight,cfg.walkableClimb,*solid);rcFilterWalkableLowHeightSpans(&context,cfg.walkableHeight,*solid);
    auto compact=std::unique_ptr<rcCompactHeightfield,decltype(&rcFreeCompactHeightfield)>(rcAllocCompactHeightfield(),rcFreeCompactHeightfield);
    Require(compact&&rcBuildCompactHeightfield(&context,cfg.walkableHeight,cfg.walkableClimb,*solid,*compact),"Model compact field failed");solid.reset();
    std::cout<<"COMPACT\t"<<compact->spanCount<<'\t'<<std::count(compact->areas,compact->areas+compact->spanCount,NAV_AREA_GROUND)<<std::endl;
    Require(rcErodeWalkableArea(&context,cfg.walkableRadius,*compact)&&rcBuildDistanceField(&context,*compact)&&rcBuildRegions(&context,*compact,0,cfg.minRegionArea,cfg.mergeRegionArea),"Model walkable-region build failed");
    auto contours=std::unique_ptr<rcContourSet,decltype(&rcFreeContourSet)>(rcAllocContourSet(),rcFreeContourSet);
    Require(contours&&rcBuildContours(&context,*compact,cfg.maxSimplificationError,cfg.maxEdgeLen,*contours),"Model contour build failed");
    auto mesh=std::unique_ptr<rcPolyMesh,decltype(&rcFreePolyMesh)>(rcAllocPolyMesh(),rcFreePolyMesh);
    Require(mesh&&rcBuildPolyMesh(&context,*contours,cfg.maxVertsPerPoly,*mesh),"Model polygon build failed");
    auto detail=std::unique_ptr<rcPolyMeshDetail,decltype(&rcFreePolyMeshDetail)>(rcAllocPolyMeshDetail(),rcFreePolyMeshDetail);
    Require(detail&&rcBuildPolyMeshDetail(&context,*mesh,*compact,cfg.detailSampleDist,cfg.detailSampleMaxError,*detail),"Model detail build failed");
    std::cout<<"POLYGONS\t"<<mesh->nverts<<'\t'<<mesh->npolys<<'\t'<<mesh->nvp<<std::endl;
    for(int i=0;i<mesh->npolys;++i)mesh->flags[i]=mesh->areas[i]?NAV_GROUND:0;
    dtNavMeshCreateParams params{};params.verts=mesh->verts;params.vertCount=mesh->nverts;params.polys=mesh->polys;params.polyAreas=mesh->areas;params.polyFlags=mesh->flags;params.polyCount=mesh->npolys;params.nvp=mesh->nvp;
    params.detailMeshes=detail->meshes;params.detailVerts=detail->verts;params.detailVertsCount=detail->nverts;params.detailTris=detail->tris;params.detailTriCount=detail->ntris;
    params.walkableHeight=cfg.ch*cfg.walkableHeight;params.walkableRadius=cfg.cs*cfg.walkableRadius;params.walkableClimb=cfg.ch*cfg.walkableClimb;params.cs=cfg.cs;params.ch=cfg.ch;params.buildBvTree=true;
    rcVcopy(params.bmin,mesh->bmin);rcVcopy(params.bmax,mesh->bmax);
    unsigned char* bytes=nullptr;int size=0;Require(dtCreateNavMeshData(&params,&bytes,&size)&&size>0,"Model Detour tile build failed");
    auto data=std::unique_ptr<unsigned char,decltype(&dtFree)>(bytes,dtFree);MmapTileHeader header;header.size=size;header.usesLiquids=false;
    char name[64];snprintf(name,sizeof(name),"go%05u.mmap",ship.GetDisplayId());auto path=output/name;
    std::ofstream file(path,std::ios::binary);Require(bool(file),"Staged model file unavailable");file.write(reinterpret_cast<char const*>(&header),sizeof(header));file.write(reinterpret_cast<char const*>(bytes),size);file.close();Require(bool(file),"Staged model file write failed");
    std::cout<<"MODEL\t"<<ship.GetDisplayId()<<'\t'<<ship.m_model->name<<'\t'<<groups.size()<<'\t'<<nv<<'\t'<<nt<<'\t'<<mesh->npolys<<'\t'<<size<<'\t'<<cfg.width<<'\t'<<cfg.height<<'\n';
}

int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv);auto output=std::getenv("LEGION_MAW_MODEL_OUTPUT");Require(output,"Explicit model staging folder required");std::filesystem::path folder(output);Require(std::filesystem::is_directory(folder),"Model staging folder absent");
        DB2Storage<DungeonMapEntry> floors("DungeonMap.db2",DungeonMapLoadInfo::Instance());Require(floors.Load(argv[1],0),"Native floor DB2 load failed");floors.LoadFromDB();
        MapInstanced parent(1492,300);InstanceMap map(1492,300,9948100,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);
        {
            DungeonShip ship(map);auto current=ship.GetDisplayId();Require(current==31852,"Maw ship model changed;review matching data first");
            std::set<uint32> displays={current};
            if(auto state=sDestructibleModelDataStore.LookupEntry(ship.GetGOInfo()->destructibleBuilding.DestructibleModelRec))
            {std::cout<<"STATES\t"<<state->ID<<'\t'<<state->State0Wmo<<'\t'<<state->State1Wmo<<'\t'<<state->State2Wmo<<'\t'<<state->State3Wmo<<'\n';if(state->State1Wmo)displays.insert(state->State1Wmo);if(state->State3Wmo)displays.insert(state->State3Wmo);}
            for(auto display:displays){ship.SetDisplayId(display);ship.EnableCollision(true);BuildModel(ship,folder,floors.LookupEntry(967));}
        }
        std::cout<<"PASS staged model-local meshes built from actual extracted collision;no canonical asset changed\n";
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
}
