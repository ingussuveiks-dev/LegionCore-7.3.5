#include "MMapManager.h"
#include "MapDefines.h"
#include <cmath>
#include <fstream>
#include <iostream>
int main(int argc,char**argv)
{
    if(argc!=3)return 2;
    std::ifstream in(argv[2]);float points[2][3];
    for(auto& p:points)if(!(in>>p[2]>>p[0]>>p[1]))return 3;
    for(unsigned map:{1532u,1527u})
    {
        MMAP::MMapManager mm;mm.InitializeThreadUnsafe({{map,{}}});std::string root=argv[1];
        if(!mm.loadMap(root+"/",map,24,24)||!mm.loadMapInstance(root+"/",map,0))return 4;
        auto query=mm.GetNavMeshQuery(map,0);if(!query)return 5;
        dtQueryFilter filter;filter.setIncludeFlags(NAV_GROUND);
        for(unsigned i=(map==1532?0:1);i<2;++i)
        {
            float nearest[3],extent[3]={1,2,1};dtPolyRef ref=0;
            query->findNearestPoly(points[i],extent,&filter,&ref,nearest);
            if(!ref||std::abs(nearest[0]-points[i][0])>0.1f||std::abs(nearest[2]-points[i][2])>0.1f||std::abs(nearest[1]-points[i][1])>0.5f)return 6;
        }
    }
    std::cout<<"PASS: native confrontation POI on intact 1532 ground; arrival remains safe on both 1532 and destroyed 1527 terrain.\n";
}
