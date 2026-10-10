#include "VMapManager2.h"
#include "MMapManager.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
// Uses the actual Release collision/navmesh libraries and installed 7.3.5 data.
int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    VMAP::VMapManager2 vm; MMAP::MMapManager mm;
    vm.InitializeThreadUnsafe({{1220,{}}}); mm.InitializeThreadUnsafe({{1220,{}}});
    std::string root = argv[1]; std::ifstream in(argv[2]);
    float x, y, z; unsigned count = 0, failed = 0;
    std::set<std::pair<int,int>> tiles;
    while (in >> x >> y >> z)
    {
        int tx = int(32-x/533.33333f), ty = int(32-y/533.33333f);
        if (tiles.insert({tx,ty}).second)
        {
            vm.loadMap((root+"/vmaps/").c_str(),1220,tx,ty);
            mm.loadMap(root+"/",1220,tx,ty); mm.loadMapInstance(root+"/",1220,0);
        }
        float ground = vm.getHeight(1220,x,y,z+3,8);
        auto query = mm.GetNavMeshQuery(1220,0);
        dtPolyRef ref=0, startRef=0, path[2048]; int pathCount=0;
        float near[3]={}, p[3]={y,z,x}, ext[3]={2,3,2};
        // First node inside the cave; do not claim navigation from Jale's isolated entrance polygon.
        float start[3]={4983.76f,660.717f,4100.06f}, startNear[3]={};
        dtQueryFilter filter;
        if (query)
        {
            query->findNearestPoly(p,ext,&filter,&ref,near);
            query->findNearestPoly(start,ext,&filter,&startRef,startNear);
            if (ref && startRef) query->findPath(startRef,ref,startNear,near,&filter,path,&pathCount,2048);
        }
        bool ok = std::isfinite(ground) && z-ground>0 && z-ground<0.15f && ref && startRef &&
            std::abs(near[0]-y)<0.5f && std::abs(near[2]-x)<0.5f && std::abs(near[1]-z)<1.5f &&
            pathCount && path[pathCount-1]==ref;
        std::cout << ++count << " " << x << " " << y << " " << z << " floor=" << ground
                  << " pathPolygons=" << pathCount << " " << (ok ? "PASS" : "FAIL") << "\n";
        if (!ok) ++failed;
    }
    return count==12 && !failed ? 0 : 1;
}
