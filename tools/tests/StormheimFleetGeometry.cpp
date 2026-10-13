#include "VMapManager2.h"
#include "WorldModel.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=3)return 2;
    std::string root=argv[1],kind;std::ifstream input(argv[2]);
    VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1475,{}}});
    VMAP::WorldModel* ship=vm.acquireModelInstance(root+"/vmaps/","File001164A0.xxx");if(!ship)return 3;
    std::set<std::pair<int,int>> tiles;float x,y,z;unsigned points=0,failures=0;
    while(input>>kind>>x>>y>>z)
    {
        float floor=-10000;
        if(kind=="boat")
        {
            int tx=int(32-x/533.33333333f),ty=int(32-y/533.33333333f);
            if(tiles.emplace(tx,ty).second && vm.loadMap((root+"/vmaps/").c_str(),1475,tx,ty)!=VMAP::VMAP_LOAD_RESULT_OK)return 4;
            floor=vm.getHeight(1475,x,y,z+1.5f,100);
            if(!vm.isInLineOfSight(1475,x,y,z+35,x,y,z+0.3f,VMAP::ModelIgnoreFlags::Nothing))++failures;
        }
        else
        {
            float distance=100;
            if(ship->IntersectRay(G3D::Ray::fromOriginAndDirection({x,y,z+1.5f},{0,0,-1}),distance,false,VMAP::ModelIgnoreFlags::Nothing))floor=z+1.5f-distance;
            if(kind=="launch")
            {
                distance=35;
                if(ship->IntersectRay(G3D::Ray::fromOriginAndDirection({x,y,z+0.3f},{0,0,1}),distance,false,VMAP::ModelIgnoreFlags::Nothing))++failures;
            }
        }
        bool valid=std::isfinite(floor)&&floor>-1000&&z>=floor&&z-floor<1.0f;
        if(!valid)++failures;
        std::cout<<kind<<" "<<x<<" "<<y<<" feet="<<z<<" floor="<<floor<<" "<<(valid?"PASS":"FAIL")<<"\n";++points;
    }
    // The first jump exits the real model before descending towards a ship.
    for(float dy:{-1.0f,0.0f,1.0f})for(float dz:{0.0f,2.0f,4.0f})
    {
        G3D::Vector3 from(-40,12+dy,21.5f+dz),to(-40,12+dy,56.5f+dz);float distance=(to-from).length();
        if(ship->IntersectRay(G3D::Ray::fromOriginAndDirection(from,(to-from).direction()),distance,false,VMAP::ModelIgnoreFlags::Nothing))++failures;
    }
    std::cout<<points<<" model/landing points; "<<failures<<" collision/placement failures. Native model collision is not a client render/camera test.\n";
    return failures?1:0;
}
