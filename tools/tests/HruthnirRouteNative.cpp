// Actual original spawn/authored points against matching1495 static navigation.
// Relocations sample PathGenerator endpoints; this is not client traversal.
#define ROCKAWAY_OFFENSIVE_NATIVE
#include "RockawayChannelNative.cpp"
#include "PathGenerator.h"
#include "WaypointManager.h"
#include "DisableMgr.h"
#include <filesystem>
int main(int argc,char** argv){try{
 Require(argc==2,"Expected native dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Private config missing");
 auto directory=std::filesystem::path(argv[1]).lexically_normal();if(directory.filename().empty())directory=directory.parent_path();
 std::string runtime=directory.parent_path().parent_path().string()+'/';char* args[]={argv[0],config,runtime.data()};InitializeNative(3,args);
 sWaypointMgr->Load();auto path=sWaypointMgr->GetPathScript(9859004);Require(path&&path->size()==7,"Actual original7-node9859004 missing");
 auto row=WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=361306 AND id=96468 AND map=1495 AND PhaseId='6999'");Require(bool(row),"Actual original spawn missing");auto fields=row->Fetch();Position spawn{fields[0].GetFloat(),fields[1].GetFloat(),fields[2].GetFloat(),fields[3].GetFloat()};
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});Require(DisableMgr::IsPathfindingEnabled(1495),"Configured1495 pathfinding disabled");
 {
 Map map(1495,300,9995900,DIFFICULTY_N_SCENARIO);Actor companion(&map,96468,13001,spawn);companion.SetPhaseId({6999},false);map.LoadGrid(spawn.GetPositionX(),spawn.GetPositionY());Require(map.AddToMap(static_cast<Creature*>(&companion)),"Native actor publication failed");
 auto inspect=[&](char const* label,float x,float y,float z,bool advance){map.LoadGrid(x,y);PathGenerator generated(&companion);bool ok=generated.CalculatePath(x,y,z,false);auto end=generated.GetActualEndPosition();float error=std::sqrt((end.x-x)*(end.x-x)+(end.y-y)*(end.y-y)+(end.z-z)*(end.z-z));std::cout<<"SEGMENT\t"<<label<<'\t'<<ok<<'\t'<<uint32(generated.GetPathType())<<'\t'<<generated.GetPath().size()<<'\t'<<end.x<<'\t'<<end.y<<'\t'<<end.z<<'\t'<<error<<std::endl;if(advance){Require(ok&&(generated.GetPathType()&PATHFIND_NORMAL)&&!(generated.GetPathType()&(PATHFIND_NOPATH|PATHFIND_INCOMPLETE|PATHFIND_NOT_USING_PATH)),"Authored route segment lacks ordinary complete mmap path");Require(error<=1.0f,"Authored native endpoint differs by more than1yd");companion.Relocate(end.x,end.y,end.z,companion.GetOrientation());}};
 inspect("direct-cold-offer",4763.73f,340.87f,-37.19f,false);
 inspect("point1",4864.44f,-33.53f,48.68f,true);
 for(auto const& node:*path)inspect(("route"+std::to_string(node.id)).c_str(),node.x,node.y,node.z,true);
 inspect("point2",4809.14f,164.68f,-11.20f,true);inspect("point4",4824.81f,246.52f,-24.29f,true);inspect("point5",4822.31f,328.56f,-30.14f,true);inspect("point6",4763.73f,340.87f,-37.19f,true);
 companion.Relocate(spawn);inspect("direct-after-authored-tiles",4763.73f,340.87f,-37.19f,false);
 std::cout<<"PASS original spawn361306/96468/1495/phase6999,authored point1/seven9859004/point2/4/5/6 ordinary matching mmap segments; no forced pathfinding/player/quest writes; sampled native endpoints/static geometry,not actual motion callbacks or client traversal.\n";
 }
 WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
