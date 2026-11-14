#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "VMapManager2.h"
#include "ModelIgnoreFlags.h"
#include "GridMap.h"
#include "Spline.h"
#include <G3D/Vector3.h>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <array>
#include "../../src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_return_route.h"

// Functional Fu Zan departure reconstruction; not a recovered retail spline.
// Evaluate the actual native smooth interpolation against matching geometry.
static bool Check(std::filesystem::path const& root,uint32 map,std::vector<G3D::Vector3> const& path)
{
    VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{map,{}}});
    std::map<std::pair<int,int>,std::unique_ptr<GridMap>> tiles;
    auto ground=[&](float x,float y)->GridMap*
    {
        int tx=int(32-x/533.33333f),ty=int(32-y/533.33333f);
        auto& tile=tiles[{tx,ty}];
        if(!tile)
        {
            tile.reset(new GridMap);
            auto file=root/"maps"/(std::to_string(map)+"_"+std::to_string(tx)+"_"+std::to_string(ty)+".map");
            if(tile->loadData(file.string().c_str())!=GridMap::LoadResult::Ok ||
                vm.loadMap((root/"vmaps").string().append("/").c_str(),map,tx,ty)!=VMAP::VMAP_LOAD_RESULT_OK)
            { std::cerr<<"GEOMETRY_LOAD_FAILED "<<map<<' '<<tx<<' '<<ty<<'\n';return nullptr; }
        }
        return tile.get();
    };
    Movement::SplineBase spline;
    spline.init_spline(path.data(),int(path.size()),Movement::SplineBase::ModeCatmullrom);
    std::vector<std::vector<G3D::Vector3>> segments;
    // A2yd horizontal corridor and0..4yd vertical profile are declared probes,
    // not a reconstructed oriented kite/passenger collision hull.
    for(int segment=spline.first();segment<spline.last();++segment)
    {
        int steps=std::max(64,int(std::ceil(spline.SegLength(segment)*4.f)));
        if(steps>8192)return false;
        std::vector<G3D::Vector3> samples;
        for(int i=0;i<=steps;++i)
        {
            G3D::Vector3 point;spline.evaluate_percent(segment,float(i)/steps,point);samples.push_back(point);
            for(float dx:{-2.f,0.f,2.f})for(float dy:{-2.f,0.f,2.f})
                if(!ground(point.x+dx,point.y+dy))return false;
        }
        segments.push_back(std::move(samples));
    }
    bool okay=true;unsigned total=0;
    for(size_t segment=0;segment<segments.size();++segment)
    {
        auto const& samples=segments[segment];bool clear=true;float minimum=10000.f;
        for(size_t i=0;i<samples.size();++i)for(float dx:{-2.f,0.f,2.f})for(float dy:{-2.f,0.f,2.f})
        {
            auto p=samples[i];p.x+=dx;p.y+=dy;
            float height=std::max(ground(p.x,p.y)->getHeight(p.x,p.y),vm.getHeight(map,p.x,p.y,p.z+4.f,10000.f));
            minimum=std::min(minimum,p.z-height);++total;
            if(!std::isfinite(height)||height< -10000.f||p.z-height< -0.05f){if(clear)std::cout<<"BLOCKED "<<p.x<<" "<<p.y<<" "<<p.z<<" ground "<<height<<"\n";clear=false;}
            if(i)
            {
                auto a=samples[i-1];a.x+=dx;a.y+=dy;
                for(float dz:{0.1f,2.f,4.f})
                    if(!vm.isInLineOfSight(map,a.x,a.y,a.z+dz,p.x,p.y,p.z+dz,VMAP::ModelIgnoreFlags::Nothing))clear=false;
            }
        }
        std::cout<<"SMOOTH_SEGMENT\t"<<map<<'\t'<<segment<<'\t'<<samples.size()<<'\t'<<minimum<<'\t'<<clear<<'\n';
        okay&=clear;
    }
    std::cout<<"SAMPLES\t"<<map<<'\t'<<total<<'\n';return okay;
}

int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    auto root=std::filesystem::path(argv[1]).lexically_normal();if(root.filename().empty())root=root.parent_path();root=root.parent_path().parent_path();
    bool okay=true;
    for(auto start:{G3D::Vector3(881.767f,-2742.08f,192.236f)})
        okay=Check(root,1625,FuZanReturnRoute::Departure(start))&&okay;

    std::cout<<"EOF\tFuZanReturnFlight\t"<<okay<<'\n';return okay?0:1;
}