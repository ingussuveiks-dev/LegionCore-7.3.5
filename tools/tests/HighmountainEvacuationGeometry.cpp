#include "MMapManager.h"
#include "MapDefines.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <array>
int main(int argc,char** argv)
{
    if (argc!=3) return 2;
    MMAP::MMapManager mm;mm.InitializeThreadUnsafe({{1532,{}}});
    std::string root=argv[1];
    if (!mm.loadMap(root+"/",1532,24,24) || !mm.loadMapInstance(root+"/",1532,0)) return 3;
    auto query=mm.GetNavMeshQuery(1532,0);if (!query) return 4;
    dtQueryFilter filter;filter.setIncludeFlags(NAV_GROUND);
    std::ifstream in(argv[2]);unsigned checked=0;
    for(unsigned route=0;route<2;++route)
    {
        unsigned count;if(!(in>>count)||count<3)return 5;
        std::vector<std::array<float,3>> points(count);
        for(auto& p:points)if(!(in>>p[2]>>p[0]>>p[1]))return 6;
        for(unsigned i=1;i<count;++i)
        {
            auto& a=points[i-1];auto& b=points[i];
            float distance=std::sqrt((a[0]-b[0])*(a[0]-b[0])+(a[2]-b[2])*(a[2]-b[2]));
            unsigned samples=unsigned(std::ceil(distance/0.5f))+1;
            for(unsigned j=0;j<=samples;++j)
            {
                float p[3],nearest[3],ext[3]={0.2f,2.0f,0.2f};dtPolyRef ref=0;
                for(unsigned k=0;k<3;++k)p[k]=a[k]+(b[k]-a[k])*float(j)/samples;
                query->findNearestPoly(p,ext,&filter,&ref,nearest);
                if(!ref||std::abs(p[0]-nearest[0])>0.08f||std::abs(p[2]-nearest[2])>0.08f||std::abs(p[1]-nearest[1])>0.65f)
                {std::cerr<<"Off ground: route "<<route<<" segment "<<i<<" sample "<<j<<" at "<<p[2]<<","<<p[0]<<","<<p[1]<<" nearest "<<nearest[2]<<","<<nearest[0]<<","<<nearest[1]<<"\n";return 7;}
                ++checked;
            }
        }
    }
    std::cout<<"PASS: both linear evacuation routes stay on terrain 1532 ground at "<<checked<<" samples (0.5 m spacing).\n";
}
