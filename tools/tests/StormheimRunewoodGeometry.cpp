#include "MMapManager.h"
#include "MapDefines.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>
#include <algorithm>
// Check actual interaction approaches; a static model center is not necessarily
// a walkable navmesh point. Use native tree interaction size and the script rune radius.
// Actual Release navmesh libraries and installed 7.3.5 ground data.
int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    MMAP::MMapManager mm; mm.InitializeThreadUnsafe({{1220,{}}});
    std::string root = argv[1]; std::ifstream in(argv[2]);
    float points[2][3], nearest[2][3], ext[3]={6,3,6};
    dtPolyRef refs[2]={},path[2048]; int count=0;
    std::set<std::pair<int,int>> tiles;
    for (auto& p : points)
    {
        // Detour uses y,z,x order.
        if (!(in >> p[2] >> p[0] >> p[1])) return 3;
        int tx=int(32-p[2]/533.33333f),ty=int(32-p[0]/533.33333f);
        for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)
            if(tiles.insert({tx+dx,ty+dy}).second)mm.loadMap(root+"/",1220,tx+dx,ty+dy);
    }
    mm.loadMapInstance(root+"/",1220,0);
    auto query=mm.GetNavMeshQuery(1220,0); if (!query) return 4;
    dtQueryFilter filter;filter.setIncludeFlags(NAV_GROUND);
    struct Approach {dtPolyRef ref;float p[3];};std::vector<Approach> choices[2];
    for(unsigned i=0;i<2;++i) {
        std::set<dtPolyRef> seen;
        for(int radius=0;radius<=6;++radius)for(int angle=0;angle<16;++angle) {
            float a=angle*6.283185307f/16,probe[3]={points[i][0]+radius*std::sin(a),points[i][1],points[i][2]+radius*std::cos(a)};
            float search[3]={1.5f,4,1.5f},near[3];dtPolyRef ref=0;
            query->findNearestPoly(probe,search,&filter,&ref,near);
            float distance=std::sqrt((near[0]-points[i][0])*(near[0]-points[i][0])+(near[1]-points[i][1])*(near[1]-points[i][1])+(near[2]-points[i][2])*(near[2]-points[i][2]));
            // Native tree radius2.52414 plus normal NPC interaction5. Rune
            // clicks use the production script's stricter exact six-yard bound.
            if(ref&&distance<=(i==0?7.5f:6.0f)&&seen.insert(ref).second) {
                Approach choice;choice.ref=ref;std::copy(near,near+3,choice.p);choices[i].push_back(choice);
            }
        }
    }
    for(auto const& start:choices[0])for(auto const& end:choices[1]) {
        count=0;query->findPath(start.ref,end.ref,start.p,end.p,&filter,path,&count,2048);
        if(!count||path[count-1]!=end.ref)continue;
        std::cout<<"PASS: Vydhar approach "<<start.p[2]<<","<<start.p[0]<<","<<start.p[1]<<" -> rune approach "<<end.p[2]<<","<<end.p[0]<<","<<end.p[1]<<" through "<<count<<" native navmesh polygons.\n";return 0;
    }
    std::cout<<"FAIL: no connected interaction approaches ("<<choices[0].size()<<","<<choices[1].size()<<").\n";return 6;
}
