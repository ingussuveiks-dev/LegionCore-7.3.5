#include "VMapManager2.h"
#include "GridMap.h"
#include <filesystem>
#include <cmath>
#include <iostream>

// Static matching7.3.5 pool surfaces, not a guessed retail source position.
// The third XY is approximate guide UI39.2/48.6 converted using area1059.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    auto root=std::filesystem::path(argv[1]).lexically_normal();
    if(root.filename().empty())root=root.parent_path();
    root=root.parent_path().parent_path();
    VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1541,{}}});
    struct Anchor {char const* name;float x,y;};
    std::cout.precision(9);
    for(Anchor point: {Anchor{"native-whirlpool",-1017.29f,-2772.8f},Anchor{"native-boss",-1018.24f,-2771.94f},Anchor{"approx-guide",-1017.39456f,-2773.13536f}})
    {
        int tx=int(32-point.x/533.33333f),ty=int(32-point.y/533.33333f);
        GridMap terrain;
        auto terrainFile=root/"maps"/("1541_"+std::to_string(tx)+"_"+std::to_string(ty)+".map");
        auto ground=terrain.loadData(terrainFile.string().c_str());
        auto collision=vm.loadMap((root/"vmaps").string().append("/").c_str(),1541,tx,ty);
        std::cout<<"LOAD\t"<<point.name<<'\t'<<int(ground)<<'\t'<<collision<<'\t'<<tx<<'\t'<<ty<<'\n';
        if(ground!=GridMap::LoadResult::Ok||collision!=VMAP::VMAP_LOAD_RESULT_OK)return 2;
        for(float dx:{-0.5f,0.f,0.5f})for(float dy:{-0.5f,0.f,0.5f})
        {
            float x=point.x+dx,y=point.y+dy;
            std::cout<<"POOL_SURFACE\t"<<point.name<<'\t'<<x<<'\t'<<y<<'\t'<<terrain.getHeight(x,y)<<'\t'<<terrain.getLiquidLevel(x,y)<<'\t'<<vm.getHeight(1541,x,y,100.f,100.f)<<'\t'<<vm.getHeight(1541,x,y,40.f,10.f)<<'\n';
        }
    }
    std::cout<<"EOF\tsheilun-pool-surface\n";
}
