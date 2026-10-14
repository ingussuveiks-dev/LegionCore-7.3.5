#include "VMapManager2.h"
#include "GridMap.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <algorithm>

struct Point { float x,y,z; };
// Actual terrain heights and VMAP model collision; sample every <= 1 metre.
int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    std::string root = argv[1]; std::ifstream input(argv[2]);
    std::vector<Point> points; Point p;
    while (input >> p.x >> p.y >> p.z) points.push_back(p);
    if (points.size() != 18) return 3;
    VMAP::VMapManager2 vm; vm.InitializeThreadUnsafe({{1220,{}}});
    std::map<std::pair<int,int>,std::unique_ptr<GridMap>> maps;
    auto terrain = [&](float x,float y) -> GridMap* {
        int tx=int(32-x/533.33333f),ty=int(32-y/533.33333f);
        auto key=std::make_pair(tx,ty);
        auto& grid=maps[key];
        if (!grid) {
            grid.reset(new GridMap);
            std::string file=root+"/maps/1220_"+std::to_string(tx)+"_"+std::to_string(ty)+".map";
            if (grid->loadData(file.c_str()) != GridMap::LoadResult::Ok ||
                vm.loadMap((root+"/vmaps/").c_str(),1220,tx,ty) != VMAP::VMAP_LOAD_RESULT_OK)
            { std::cerr<<"Missing geometry "<<file<<"\n"; return nullptr; }
        }
        return grid.get();
    };
    // Preload every crossed tile, including the 2 m corridor around the bird.
    for (size_t i=1;i<points.size();++i) {
        auto a=points[i-1],b=points[i];
        int samples=int(std::ceil(std::hypot(std::hypot(b.x-a.x,b.y-a.y),b.z-a.z)))+1;
        for (int s=0;s<=samples;++s) for (int dx=-2;dx<=2;dx+=2) for (int dy=-2;dy<=2;dy+=2)
            if (!terrain(a.x+(b.x-a.x)*s/samples+dx,a.y+(b.y-a.y)*s/samples+dy)) return 4;
    }
    unsigned failures=0,total=0;
    for(Point spot:std::vector<Point>{{3248.89f,3170.68f,526.299f},{3246.89f,3170.68f,526.299f},{3250.89f,3170.68f,526.299f},{3243.49f,3143.09f,526.777f}}){
        auto grid=terrain(spot.x,spot.y);if(!grid)return 4;
        float floor=grid->getHeight(spot.x,spot.y);
        float model=vm.getHeight(1220,spot.x,spot.y,spot.z+3,8);
        if(std::isfinite(model)&&model>-10000)floor=std::max(floor,model);
        bool valid=std::abs(spot.z-floor)<=1.0f;
        if(!valid)++failures;
        std::cout<<"ground "<<spot.x<<" "<<spot.y<<" feet="<<spot.z<<" floor="<<floor<<" "<<(valid?"PASS":"FAIL")<<"\n";
    }
    for (size_t i=1;i<points.size();++i) {
        auto a=points[i-1],b=points[i];float minGround=10000;bool clear=true;
        int samples=int(std::ceil(std::hypot(std::hypot(b.x-a.x,b.y-a.y),b.z-a.z)))+1;
        // The boarding point clears the source rock by 3 m. Every segment
        // requires >= 1 m terrain clearance throughout a 2 m corridor.
        for (int s=0;s<=samples;++s) for (int dx=-2;dx<=2;dx+=2) for (int dy=-2;dy<=2;dy+=2) {
            float t=float(s)/samples,x=a.x+(b.x-a.x)*t+dx,y=a.y+(b.y-a.y)*t+dy,z=a.z+(b.z-a.z)*t;
            float height=terrain(x,y)->getHeight(x,y);
            minGround=std::min(minGround,z-height);++total;
            if (!std::isfinite(height) || height < -10000 || z-height < 1.0f) clear=false;
        }
        // Rays cover the bird's center and a corridor, including rider height.
        for (int dx=-2;dx<=2;dx+=2) for (int dy=-2;dy<=2;dy+=2) for (int dz=0;dz<=4;dz+=2)
            if (!vm.isInLineOfSight(1220,a.x+dx,a.y+dy,a.z+dz,b.x+dx,b.y+dy,b.z+dz,VMAP::ModelIgnoreFlags::Nothing)) clear=false;
        std::cout<<"segment "<<i<<" terrain clearance="<<minGround<<" "<<(clear?"PASS":"FAIL")<<"\n";
        if (!clear) ++failures;
    }
    std::cout<<total<<" terrain samples; "<<failures<<" failed segments\n";
    return failures ? 1 : 0;
}
