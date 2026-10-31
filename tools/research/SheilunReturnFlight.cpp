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

// Functional route candidates, not a recovered retail1541 spline or a proof
// that retail binds5619 to202831. Evaluate actual native smooth interpolation.
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
            if(!std::isfinite(height)||height< -10000.f||p.z-height< -0.05f)clear=false;
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
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();
    WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(WorldDatabase.Open()||!WorldDatabase.PrepareStatements()||HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=WorldDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 5;
    DB2Storage<TaxiPathNodeEntry> nodes("TaxiPathNode.db2",TaxiPathNodeLoadInfo::Instance());
    DB2Storage<MapDifficultyEntry> difficulties("MapDifficulty.db2",MapDifficultyLoadInfo::Instance());
    if(!nodes.Load(argv[1],0)||!difficulties.Load(argv[1],0))return 6;
    nodes.LoadFromDB();difficulties.LoadFromDB();
    for(auto difficulty:difficulties)if(difficulty->MapID==1541)
        std::cout<<"MAP_ADMISSION\t"<<difficulty->ID<<'\t'<<unsigned(difficulty->DifficultyID)<<'\t'<<unsigned(difficulty->MaxPlayers)<<'\t'<<unsigned(difficulty->Flags)<<'\n';
    auto row=WorldDatabase.Query("SELECT target_map,target_position_x,target_position_y,target_position_z FROM spell_target_position WHERE id=202831");
    if(!row||row->Fetch()[0].GetUInt32()!=1514)return 7;
    std::vector<G3D::Vector3> second{{row->Fetch()[1].GetFloat(),row->Fetch()[2].GetFloat(),row->Fetch()[3].GetFloat()}};
    std::map<uint32,G3D::Vector3> ordered;
    for(auto node:nodes)if(node->PathID==5619)
    {
        if(node->ContinentID!=1514||node->Flags||node->Delay||node->ArrivalEventID||node->DepartureEventID)return 8;
        ordered.emplace(node->NodeIndex,G3D::Vector3(node->Loc.X,node->Loc.Y,node->Loc.Z));
    }
    if(ordered.size()!=8)return 9;
    for(uint32 i=0;i<8;++i){auto p=ordered.find(i);if(p==ordered.end())return 10;second.push_back(p->second);}
    std::cout.precision(9);
    auto root=std::filesystem::path(argv[1]).lexically_normal();if(root.filename().empty())root=root.parent_path();root=root.parent_path().parent_path();
    bool okay=true;
    for(auto start:{G3D::Vector3(-1017.29f,-2772.8f,38.57111f),G3D::Vector3(-1019.15f,-2795.95f,38.91f)})
    {
        std::vector<G3D::Vector3> first{start,{start.x,start.y,start.z+10.f},{-1017.29f,-2745.f,68.57111f},{-1032.f,-2705.f,95.f},{-1060.f,-2660.f,135.f}};
        std::cout<<"FUNCTIONAL_1541_START\t"<<start.x<<'\t'<<start.y<<'\t'<<start.z<<'\n';okay=Check(root,1541,first)&&okay;
    }
    for(auto p:second)std::cout<<"CANDIDATE_1514_POINT\t"<<p.x<<'\t'<<p.y<<'\t'<<p.z<<'\n';
    okay=Check(root,1514,second)&&okay;
    HotfixDatabase.Close();WorldDatabase.Close();MySQL::Library_End();
    std::cout<<"EOF\tsheilun-functional-flight-candidate\t"<<okay<<'\n';return okay?0:11;
}
