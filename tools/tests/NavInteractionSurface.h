#pragma once
#include "VMapManager2.h"
#include "GridMap.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
// Static endpoint diagnostics for walking/swimming interaction surveys. Values
// use Detour y,z,x coordinates. No character placement or runtime override.
class NavInteractionSurface
{
    uint32 map;
    std::string root;
    VMAP::VMapManager2 vm;
    std::map<std::pair<int,int>,std::unique_ptr<GridMap>> grids;
public:
    NavInteractionSurface(uint32 id,std::string path):map(id),root(std::move(path)){vm.InitializeThreadUnsafe({{map,{}}});}
    bool Check(float const* p,float const* object,bool report)
    {
        int tx=int(32-p[2]/533.33333f),ty=int(32-p[0]/533.33333f);auto& grid=grids[{tx,ty}];
        if(!grid){grid=std::make_unique<GridMap>();
            auto terrain=grid->loadData((root+"/maps/"+std::to_string(map)+"_"+std::to_string(tx)+"_"+std::to_string(ty)+".map").c_str());
            auto collision=vm.loadMap((root+"/vmaps/").c_str(),map,tx,ty);
            if(terrain!=GridMap::LoadResult::Ok||collision!=VMAP::VMAP_LOAD_RESULT_OK){grids.erase({tx,ty});return false;}}
        float terrain=grid->getHeight(p[2],p[0]);float floor=p[1]+2>terrain?terrain:-100000;
        float mesh=vm.getHeight(map,p[2],p[0],p[1]+2,10);
        if(std::isfinite(mesh)&&mesh>-10000&&(floor<=-10000||mesh>floor||std::abs(floor-p[1])>std::abs(mesh-p[1])))floor=mesh;
        float water=grid->getLiquidLevel(p[2],p[0]),vWater=-100000,vFloor=-100000;uint32 type=0;
        bool vLiquid=vm.GetLiquidLevel(map,p[2],p[0],p[1]+2,0,vWater,vFloor,type);
        float error=std::abs(floor-p[1]);
        // -2000 is the terrain no-liquid sentinel, not a water surface.
        if(std::isfinite(water)&&water>-1000&&water>=terrain&&p[1]>=terrain-2)error=std::min(error,std::abs(water-p[1]));
        if(vLiquid&&vWater>vFloor&&p[1]>vFloor-2)error=std::min(error,std::abs(vWater-p[1]));
        bool los=vm.isInLineOfSight(map,p[2],p[0],p[1]+2,object[2],object[0],object[1]+2,VMAP::ModelIgnoreFlags::Nothing);
        bool pass=error<=1&&los;
        if(report)std::cout<<"endpoint surfaceZ="<<p[1]<<" floor="<<floor<<" terrain="<<terrain<<" terrainLiquid="<<water<<" vmapLiquid="<<(vLiquid?vWater:-100000)
            <<" surfaceError="<<error<<" staticLOS="<<los<<" "<<(pass?"PASS":"FAIL")<<'\n';
        return pass;
    }
};
