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
            auto terrainStatus=grid->loadData(path.c_str());auto vmapStatus=vm.loadMap((root+"/vmaps/").c_str(),1220,tx,ty);if(terrainStatus!=GridMap::LoadResult::Ok||vmapStatus!=VMAP::VMAP_LOAD_RESULT_OK){std::cout<<"LOAD FAILED terrain="<<int(terrainStatus)<<" vmap="<<vmapStatus<<" tile="<<tx<<","<<ty<<"\n";return 3;}}
        // Match Map::GetHeight: terrain above a cave's supplied Z is not
        // a candidate floor. Prefer the same nearby surface as the server.
        float terrain=grid->getHeight(p.x,p.y);
        float floor=p.z+2.0f>terrain?terrain:-100000.0f;
        float mesh=vm.getHeight(1220,p.x,p.y,p.z+2.0f,10);
        if(std::isfinite(mesh)&&mesh>-10000 &&
            (floor<=-10000 || mesh>floor || std::abs(floor-p.z)>std::abs(mesh-p.z)))floor=mesh;
        // Quest objects use a conservative
        // center-to-ground approach within native INTERACTION_DISTANCE5, without
        // relying on its additional model bounding-box interaction reach.
        float tolerance=kind=="object"?5.0f:1.0f;
        bool valid=std::isfinite(floor)&&floor>-10000&&std::abs(p.z-floor)<=tolerance;
        if(!valid)++failures;

        std::cout<<kind<<" "<<guid<<" Z="<<p.z<<" floor="<<floor<<" terrain_area="<<grid->getArea(p.x,p.y)<<" "<<(valid?("PASS"):"FAIL")<<"\n";++points;
    }
    if(points!=4)++failures;
    std::cout<<points<<" Weather actor/chest anchors; "<<failures<<" failures. Live client movement and interaction remain unverified.\n";
    return failures?1:0;
}
