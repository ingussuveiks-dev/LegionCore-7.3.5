#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "PathGenerator.h"
#include "WaypointManager.h"
#include "DisableMgr.h"
#include <filesystem>
#include <cstdlib>

int main(int argc,char** argv)
{
    try
    {
        std::cout<<std::unitbuf;
        Require(argc==2,"Expected native dbc directory");
        auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
        Require(config,"Private native config missing");
        auto directory=std::filesystem::path(argv[1]).lexically_normal();
        if(directory.filename().empty()) directory=directory.parent_path();
        std::string runtime=directory.parent_path().parent_path().string()+'/';
        char* arguments[]={argv[0],config,runtime.data()};
        InitializeNative(3,arguments);
        sWaypointMgr->Load();
        auto path=sWaypointMgr->GetPathScript(9859006);
        Require(path&&path->size()==9,"Actual nine-node9859006 missing");
        Require(!sWaypointMgr->GetPath(9859006),"Unexpected ordinary addon route shadow");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1014,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1014,{}}});
        Require(DisableMgr::IsPathfindingEnabled(1014),"Native configured1014 pathfinding disabled; no geometry certification");
        {
            Map map(1014,300,9999993,DIFFICULTY_N_SCENARIO);
            Actor chen(&map,97778,10002,{3867.67f,1726.28f,932.66f,0.f});
            chen.SetPhaseId({7000,7001},false);
            map.LoadGrid(chen.GetPositionX(),chen.GetPositionY());
            Require(map.AddToMap(static_cast<Creature*>(&chen)),"Native Hait admission failed");
            for(auto const& node:*path)
            {
                map.LoadGrid(node.x,node.y);
                PathGenerator generated(&chen);
                bool ok=generated.CalculatePath(node.x,node.y,node.z,false);
                auto end=generated.GetActualEndPosition();
                float error=chen.GetExactDist(node.x,node.y,node.z);
                float endpoint=std::sqrt((end.x-node.x)*(end.x-node.x)+(end.y-node.y)*(end.y-node.y)+(end.z-node.z)*(end.z-node.z));
                std::cout<<"SEGMENT\t"<<node.id<<'\t'<<ok<<'\t'<<uint32(generated.GetPathType())<<'\t'<<generated.GetPath().size()<<'\t'<<end.x<<'\t'<<end.y<<'\t'<<end.z<<'\t'<<endpoint<<'\t'<<error<<'\n';
                Require(ok&&(generated.GetPathType()&PATHFIND_NORMAL)&&!(generated.GetPathType()&(PATHFIND_NOPATH|PATHFIND_INCOMPLETE|PATHFIND_NOT_USING_PATH)),"Native configured segment is not an ordinary complete mmap path");
                Require(endpoint<=0.5f,"Actual native path end differs from the original node by more than0.5yd");
                chen.Relocate(end.x,end.y,end.z,chen.GetOrientation());
            }
            std::cout<<"PASS actual matching1014 nine ordinary native PathGenerator mmap legs, no forced/ignored pathfinding, complete route and bounded0.5yd endpoints; isolated NullCreatureAI/phase7000+7001/static geometry, no Players/character saves. Not actual movement callbacks/dynamic-object/client traversal.\n";
        }
        WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
    }
    catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
