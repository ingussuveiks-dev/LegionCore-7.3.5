#include "VMapManager2.h"
#include "GridMap.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
struct Point {float x,y,z;};
int main(int argc,char** argv)
{
    if(argc!=3)return 2;
    std::string root=argv[1],kind;std::ifstream input(argv[2]);uint64 guid;Point p;
    VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1220,{}}});
    std::map<std::pair<int,int>,std::unique_ptr<GridMap>> maps;
    unsigned points=0,failures=0;
    while(input>>kind>>guid>>p.x>>p.y>>p.z){
        int tx=int(32-p.x/533.33333f),ty=int(32-p.y/533.33333f);auto& grid=maps[{tx,ty}];
        if(!grid){grid=std::make_unique<GridMap>();std::string path=root+"/maps/1220_"+std::to_string(tx)+"_"+std::to_string(ty)+".map";
            if(grid->loadData(path.c_str())!=GridMap::LoadResult::Ok||vm.loadMap((root+"/vmaps/").c_str(),1220,tx,ty)!=VMAP::VMAP_LOAD_RESULT_OK)return 3;}
        float floor=grid->getHeight(p.x,p.y),mesh=vm.getHeight(1220,p.x,p.y,p.z+3,10);
        if(std::isfinite(mesh)&&mesh>-10000)floor=std::max(floor,mesh);
        float tolerance=kind=="npc"?1.0f:2.0f;
        bool valid=std::isfinite(floor)&&floor>-10000&&std::abs(p.z-floor)<=tolerance;
        if(!valid)++failures;

        std::cout<<kind<<" "<<guid<<" Z="<<p.z<<" floor="<<floor<<" terrain_area="<<grid->getArea(p.x,p.y)<<" "<<(valid?"PASS":"FAIL")<<"\n";++points;
    }
    if(points!=8)++failures;
    std::cout<<points<<" Will archive/defense anchors; "<<failures<<" failures. Client collision, opponent landing and visuals remain unverified.\n";
    return failures?1:0;
}
