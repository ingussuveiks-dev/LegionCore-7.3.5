#include "VMapManager2.h"
#include "GridMap.h"
#include "Position.h"
#include "stormheim_heart_route.h"
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <algorithm>
int main(int argc,char**argv)
{
    if(argc!=2)return 2;
    std::string root=argv[1];VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1220,{}}});
    std::map<std::pair<int,int>,std::unique_ptr<GridMap>> maps;
    auto terrain=[&](float x,float y){int tx=int(32-x/533.33333f),ty=int(32-y/533.33333f);auto& g=maps[{tx,ty}];if(!g){g=std::make_unique<GridMap>();if(g->loadData((root+"/maps/1220_"+std::to_string(tx)+"_"+std::to_string(ty)+".map").c_str())!=GridMap::LoadResult::Ok||vm.loadMap((root+"/vmaps/").c_str(),1220,tx,ty)!=VMAP::VMAP_LOAD_RESULT_OK)throw std::runtime_error("Missing geometry");}return g->getHeight(x,y);};
    // The authored sleeping origin is 1.901yd below the nearby floor. Preserve
    // that pose and begin the clearance probe 3.5yd above it, not below the floor.
    Position start{2026.87f,2629.76f,470.871f,0};unsigned failures=0,samples=0;
    for(auto const& end:StormheimHeart::Departure){
        int n=int(std::ceil(std::hypot(std::hypot(end.m_positionX-start.m_positionX,end.m_positionY-start.m_positionY),end.m_positionZ-start.m_positionZ)))+1;
        bool clear=true;float minFloor=10000;
        for(int s=0;s<=n;++s)for(int dx:{-2,0,2})for(int dy:{-2,0,2}){
            float t=float(s)/n,x=start.m_positionX+(end.m_positionX-start.m_positionX)*t+dx,y=start.m_positionY+(end.m_positionY-start.m_positionY)*t+dy,z=start.m_positionZ+(end.m_positionZ-start.m_positionZ)*t;
            float ground=terrain(x,y),mesh=vm.getHeight(1220,x,y,z,200);
            // The heightfield spans this cave mouth. Its VMAP floor/ceiling defines the opening; use terrain only outside the WMO floor.
            if(mesh<=-10000 && ground<z)mesh=ground;
            if(mesh<=-10000||!std::isfinite(mesh)||z-mesh<0.5f){clear=false;std::cout<<"bad floor "<<x<<","<<y<<","<<z<<" mesh="<<mesh<<" terrain="<<ground<<"\n";}
            minFloor=std::min(minFloor,z-mesh);++samples;
        }
        for(int dx:{-2,0,2})for(int dy:{-2,0,2})for(int dz:{0,3,6})
            if(!vm.isInLineOfSight(1220,start.m_positionX+dx,start.m_positionY+dy,start.m_positionZ+dz,end.m_positionX+dx,end.m_positionY+dy,end.m_positionZ+dz,VMAP::ModelIgnoreFlags::Nothing))clear=false;
        std::cout<<"segment "<<(&end-StormheimHeart::Departure)<<" minimum floor clearance="<<minFloor<<" "<<(clear?"PASS":"FAIL")<<"\n";
        if(!clear)++failures;start=end;
    }
    std::cout<<samples<<" floor samples, 27 collision rays per segment; "<<failures<<" failures. Native collision envelope checked; full animated wings/model clipping and flight presentation need the client.\n";
    bool verticalBlocked=!vm.isInLineOfSight(1220,2026.87f,2629.76f,470.871f,2026.87f,2629.76f,680.0f,VMAP::ModelIgnoreFlags::Nothing);
    std::cout<<"Direct vertical shortcut rejected="<<verticalBlocked<<"\n";
    return failures||!verticalBlocked?1:0;
}
