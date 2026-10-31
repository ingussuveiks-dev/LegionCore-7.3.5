#include "VMapManager2.h"
#include <filesystem>
#include <iostream>
#include <cmath>

// Read actual static7.3.5 collision at the recorded artifact location. This
// does not model a rendered client, dynamic objects or an entire jump arc.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    auto root=std::filesystem::path(argv[1]).lexically_normal();
    if(root.filename().empty())root=root.parent_path();
    root=root.parent_path().parent_path();
    VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1600,{}}});
    auto path=(root/"vmaps").string()+"/";
    constexpr float x=50.5781f,y=802.392f,z=805.731f;
    int tx=int(32-x/533.33333f),ty=int(32-y/533.33333f);
    std::cout<<"VMAP_LOAD\t"<<vm.loadMap(path.c_str(),1600,tx,ty)<<'\n';
    std::cout.precision(9);
    bool valid=true;
    for(float dx:{-1.f,-0.5f,0.f,0.5f,1.f})for(float dy:{-1.f,-0.5f,0.f,0.5f,1.f})
    {
        float floor=vm.getHeight(1600,x+dx,y+dy,z+2,5);
        bool safe=std::isfinite(floor)&&std::fabs(z-floor)<0.5f;
        std::cout<<"SURFACE\t"<<dx<<'\t'<<dy<<'\t'<<floor<<'\t'<<safe<<'\n';
        valid=valid&&safe;
    }
    return valid?0:2;
}
