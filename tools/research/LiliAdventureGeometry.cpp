#include "VMapManager2.h"
#include "GridMap.h"
#include "MMapManager.h"
#include <cmath>
#include <filesystem>
#include <iostream>
struct Point {float x,y,z;};
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    auto root=std::filesystem::path(argv[1]).lexically_normal();
    if(root.filename().empty())root=root.parent_path();
    root=root.parent_path().parent_path();
    VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1514,{}}});
    std::string path=(root/"vmaps").string()+"/";
    for(int x=29;x<=31;++x)for(int y=24;y<=26;++y)vm.loadMap(path.c_str(),1514,x,y);
    GridMap grid;
    if(grid.loadData((root/"maps/1514_30_25.map").string().c_str())!=GridMap::LoadResult::Ok)return 2;
    Point const points[]={{741.679f,3667.43f,143.347f},{745.349f,3664.7365f,143.2642f},
        {734.5573f,3631.977f,142.6309f},{733.8f,3630.5f,142.6309f}};
    for(auto p:points)
    {
        float terrain=grid.getHeight(p.x,p.y),mesh=vm.getHeight(1514,p.x,p.y,p.z+2,20);
        float floor=(p.z+2>terrain)?terrain:-100000.f;
        if(mesh>-10000&&(floor<-10000||mesh>floor||std::fabs(mesh-p.z)<std::fabs(floor-p.z)))floor=mesh;
        std::cout<<"FLOOR\t"<<p.x<<'\t'<<p.y<<'\t'<<p.z<<'\t'<<floor<<'\n';
        if(!std::isfinite(floor)||std::fabs(floor-p.z)>0.75f)return 3;
    }
    // Straight segments are only a collision check. Runtime grounded movement
    // still uses the native path generator and requires its arrival callback.
    for(unsigned i=0;i<2;++i)
    {
        auto a=points[i],b=points[i+1];
        bool clear=vm.isInLineOfSight(1514,a.x,a.y,a.z+1,b.x,b.y,b.z+1,VMAP::ModelIgnoreFlags::Nothing);
        std::cout<<"WALK_BODY_LOS\t"<<i<<'\t'<<clear<<'\n';if(!clear)return 4;
    }
    bool landing=vm.isInLineOfSight(1514,730.0816f,3616.991f,147,733.8f,3630.5f,144.13f,VMAP::ModelIgnoreFlags::Nothing);
    bool flight=vm.isInLineOfSight(1514,733.8f,3630.5f,144.13f,730,3618,170,VMAP::ModelIgnoreFlags::Nothing);
    std::cout<<"CRANE_CLEAR\t"<<landing<<'\t'<<flight<<'\n';
    if(!landing||!flight)return 5;
    MMAP::MMapManager mm;mm.InitializeThreadUnsafe({{1514,{}}});
    std::string data=root.string()+"/";
    if(!mm.loadMap(data,1514,30,25)||!mm.loadMapInstance(data,1514,0))return 6;
    auto query=mm.GetNavMeshQuery(1514,0);if(!query)return 7;
    dtQueryFilter filter;filter.setIncludeFlags(1);filter.setExcludeFlags(0);
    float extents[]={3,5,3};
    for(unsigned i=0;i<2;++i)
    {
        auto a=points[i],b=points[i+1];float from[]={a.y,a.z,a.x},to[]={b.y,b.z,b.x},nearFrom[3],nearTo[3];
        dtPolyRef start=0,end=0,pathPolys[128];int count=0;
        if(dtStatusFailed(query->findNearestPoly(from,extents,&filter,&start,nearFrom))||!start||
            dtStatusFailed(query->findNearestPoly(to,extents,&filter,&end,nearTo))||!end)return 8;
        if(dtStatusFailed(query->findPath(start,end,nearFrom,nearTo,&filter,pathPolys,&count,128))||!count||pathPolys[count-1]!=end)return 9;
        std::cout<<"CONNECTED_NATIVE_NAVMESH\t"<<i<<'\t'<<count<<'\n';
    }
    return 0;
}
