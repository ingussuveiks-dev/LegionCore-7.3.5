#include "VMapManager2.h"
#include <cmath>
#include <iostream>
#include <string>
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1539,{}}});
    std::string path=std::string(argv[1])+"/vmaps/";
    for(int x=27;x<=29;++x)for(int y=26;y<=28;++y)vm.loadMap(path.c_str(),1539,x,y);
    unsigned supported=0;
    for(float x:{1875.f,1883.f,1890.f})for(float y:{2175.f,2180.f,2185.f})
    {
        float z=vm.getHeight(1539,x,y,45.f,20.f);
        if(z<0)return 2;
        float angle=std::atan2(y-2163.35f,x-1882.18f);
        float dx=x+30.f*std::cos(angle),dy=y+30.f*std::sin(angle);
        float dz=vm.getHeight(1539,dx,dy,z+6.f,15.f);
        float hx=dx,hy=dy,hz=dz+1.5f;
        bool hit=vm.getObjectHitPos(1539,x,y,z+2.f,dx,dy,dz+1.5f,hx,hy,hz,-0.5f);
        std::cout<<"FLOOR\t"<<x<<'\t'<<y<<'\t'<<z<<'\t'<<dx<<'\t'<<dy<<'\t'<<dz<<'\t'<<hit<<'\n';
        if(dz>0&&!hit&&std::fabs(dz-z)<=6.f)++supported;
    }
    std::cout<<"SUPPORTED_UNOBSTRUCTED\t"<<supported<<"/9; others require actual native first-collision shortening or rejection\n";
    return supported?0:3;
}
