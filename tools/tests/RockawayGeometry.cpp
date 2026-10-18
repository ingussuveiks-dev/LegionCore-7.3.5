#include "VMapManager2.h"
#include "GridMap.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>

int main(int argc, char** argv)
{
    if (argc != 3) return 1;
    std::string root = argv[1];
    std::ifstream points(argv[2]);
    VMAP::VMapManager2 vm;
    vm.InitializeThreadUnsafe({{1220,{}}});
    std::map<std::pair<int,int>,std::unique_ptr<GridMap>> tiles;
    float x,y,z; unsigned count=0;
    while (points >> x >> y >> z)
    {
        int tx=int(32-x/533.33333f),ty=int(32-y/533.33333f);
        auto& grid=tiles[{tx,ty}];
        if (!grid)
        {
            grid.reset(new GridMap);
            auto path=root+"/maps/1220_"+std::to_string(tx)+"_"+std::to_string(ty)+".map";
            if (grid->loadData(path.c_str())!=GridMap::LoadResult::Ok ||
                vm.loadMap((root+"/vmaps/").c_str(),1220,tx,ty)!=VMAP::VMAP_LOAD_RESULT_OK) return 2;
        }
        auto floor=std::max(grid->getHeight(x,y),vm.getHeight(1220,x,y,z+2,10));
        std::cout << x << ' ' << y << " stored=" << z << " actual=" << floor << '\n';
        if (grid->getArea(x,y)!=7833 || !std::isfinite(floor) || std::abs(z-floor)>.02f) return 3;
        ++count;
    }
    if (count!=4) return 4;
    std::cout << "PASS: all four reconstructed ritualist positions lie on actual terrain/WMO collision in Rockaway Coast. Exact retail placement and client pathing remain pending.\n";
}
