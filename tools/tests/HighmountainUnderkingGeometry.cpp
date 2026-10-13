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
    MMAP::MMapManager mm;mm.InitializeThreadUnsafe({{1527,{}}});std::string root=argv[1];
    if(!mm.loadMap(root+"/",1527,25,24)||!mm.loadMapInstance(root+"/",1527,0))return 4;
    auto query=mm.GetNavMeshQuery(1527,0);if(!query)return 5;
    dtQueryFilter filter;filter.setIncludeFlags(NAV_GROUND);
    dtPolyRef refs[2]={};float nearest[2][3],extent[3]={1,2,1};
    for(unsigned i=0;i<2;++i)
    {
        query->findNearestPoly(points[i],extent,&filter,&refs[i],nearest[i]);
        if(!refs[i]||std::abs(nearest[i][0]-points[i][0])>0.1f||std::abs(nearest[i][2]-points[i][2])>0.1f||std::abs(nearest[i][1]-points[i][1])>0.75f)return 6;
    }
    dtPolyRef path[512];int count=0;
    auto status=query->findPath(refs[0],refs[1],nearest[0],nearest[1],&filter,path,&count,512);
    if(dtStatusFailed(status)||dtStatusDetail(status,DT_PARTIAL_RESULT)||!count||path[count-1]!=refs[1])return 7;
    std::cout<<"PASS: Mayla meeting and retained boss are on connected cave ground in destroyed terrain 1527, below the Snowmane roof.\n";
}
