#include "MMapManager.h"
#include "MapDefines.h"
#include "GridDefines.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <tuple>
int main(int argc,char** argv)
{
    if(argc!=3)return 2;
    std::ifstream input(argv[2]);std::string root=argv[1];
    MMAP::MMapManager mm;mm.InitializeThreadUnsafe({{0,{}},{1,{}}});
    uint32 map,path,node,lastMap=999,lastPath=0;float x,y,z;unsigned points=0,segments=0;
    float last[3]={};dtQueryFilter filter;filter.setIncludeFlags(NAV_GROUND);unsigned missingDeck=0;
    std::set<std::tuple<uint32,uint32,uint32>> tiles;std::set<uint32> instances;
    while(input>>map>>path>>node>>x>>y>>z)
    {
        uint32 tileX=uint32(std::floor(32-x/SIZE_OF_GRIDS)),tileY=uint32(std::floor(32-y/SIZE_OF_GRIDS));
        if(tiles.emplace(map,tileX,tileY).second && !mm.loadMap(root+"/",map,tileX,tileY))return 3;
        if(instances.insert(map).second && !mm.loadMapInstance(root+"/",map,0))return 3;
        auto query=mm.GetNavMeshQuery(map,0);if(!query)return 4;
        float point[3]={y,z,x},nearest[3]={},extent[3]={2,3,2};dtPolyRef ref=0;
        query->findNearestPoly(point,extent,&filter,&ref,nearest);
        if(!ref||std::abs(nearest[1]-z)>2.0f)++missingDeck;
        if(map==lastMap&&path==lastPath)
        {
            float dx=point[0]-last[0],dy=point[1]-last[1],dz=point[2]-last[2];
            if(std::sqrt(dx*dx+dy*dy+dz*dz)>12.0f||std::abs(dy)>5.0f)return 6;
            ++segments;
        }
        lastMap=map;lastPath=path;for(int n=0;n<3;++n)last[n]=point[n];++points;
    }
    if(points!=59||segments!=53)return 7;
    std::cout<<"PASS: 59 retained waypoints and 53 bounded short deck segments; "<<missingDeck<<" points lack deck polygons in Release navmesh. Direct waypoint movement is required; this does not verify client deck collision.\n";
}
