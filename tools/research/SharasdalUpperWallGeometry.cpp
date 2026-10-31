#include "VMapManager2.h"
#include <filesystem>
#include <cmath>
#include <iostream>

// Read-only matching static collision around the authored final Grash point.
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    auto root=std::filesystem::path(argv[1]).lexically_normal();
    if(root.filename().empty())root=root.parent_path();
    root=root.parent_path().parent_path();
    VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1600,{}}});
    auto path=(root/"vmaps").string()+"/";
    std::cout<<"LOAD\t"<<vm.loadMap(path.c_str(),1600,32,30)<<'\n';
    std::cout.precision(9);
    for(float x:{-181.168f,-164.214f,-44.47f,-17.96f,-10.3385f,0.f})
        std::cout<<"ROUTE_FLOOR\t"<<x<<'\t'<<vm.getHeight(1600,x,802.05f,812.f,30.f)<<'\n';
    for(float dx:{-1.f,0.f,1.f})for(float dy:{-1.f,0.f,1.f})
        std::cout<<"FINAL_FLOOR\t"<<dx<<'\t'<<dy<<'\t'<<vm.getHeight(1600,-17.96f+dx,802.05f+dy,810.f,10.f)<<'\n';
    return 0;
}
