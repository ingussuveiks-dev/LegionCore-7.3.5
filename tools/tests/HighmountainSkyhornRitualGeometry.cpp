#include "MMapManager.h"
#include "MapDefines.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
// Native Lasan spawn is 1.72 m below the nearest ground polygon; allow 2 m.
// Actual Release navmesh libraries and installed 7.3.5 ground data.
int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    MMAP::MMapManager mm; mm.InitializeThreadUnsafe({{1220,{}}});
    std::string root = argv[1]; std::ifstream in(argv[2]);
    float points[2][3], nearest[2][3], ext[3]={2,3,2};
    dtPolyRef refs[2]={},path[2048]; int count=0;
    std::set<std::pair<int,int>> tiles;
    for (auto& p : points)
    {
        // Detour uses y,z,x order.
        if (!(in >> p[2] >> p[0] >> p[1])) return 3;
        int tx=int(32-p[2]/533.33333f),ty=int(32-p[0]/533.33333f);
        if (tiles.insert({tx,ty}).second) mm.loadMap(root+"/",1220,tx,ty);
    }
    mm.loadMapInstance(root+"/",1220,0);
    auto query=mm.GetNavMeshQuery(1220,0); if (!query) return 4;
    dtQueryFilter filter; filter.setIncludeFlags(NAV_GROUND);
    for (unsigned i=0;i<2;++i)
    {
        query->findNearestPoly(points[i],ext,&filter,&refs[i],nearest[i]);
        if (!refs[i] || std::abs(nearest[i][0]-points[i][0])>0.5f ||
            std::abs(nearest[i][2]-points[i][2])>0.5f || std::abs(nearest[i][1]-points[i][1])>2.0f) return 5;
    }
    query->findPath(refs[0],refs[1],nearest[0],nearest[1],&filter,path,&count,2048);
    if (!count || path[count-1]!=refs[1]) return 6;
    std::cout<<"PASS: Skyhorn ritual ground route reaches its destination through "<<count<<" navmesh polygons.\n";
}
