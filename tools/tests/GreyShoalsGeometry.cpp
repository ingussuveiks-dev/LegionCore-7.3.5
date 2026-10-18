#include "VMapManager2.h"
#include "GridMap.h"
#include "MMapManager.h"
#include "MapDefines.h"
#include "NativePathLimits.inc"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

static void Require(bool ok,char const* reason){if(!ok)throw std::runtime_error(reason);}
static bool Complete(dtStatus status){return dtStatusSucceed(status)&&!(status&DT_STATUS_DETAIL_MASK);}
static float Distance(float const* a,float const* b){return std::sqrt((a[0]-b[0])*(a[0]-b[0])+(a[1]-b[1])*(a[1]-b[1])+(a[2]-b[2])*(a[2]-b[2]));}
struct Point {
 std::string kind;uint64 guid;float x,y,z;
 dtPolyRef ref=0,groundRef=0;float near[3]={},groundNear[3]={};
};
int Run(int argc,char** argv){
 if(argc==2&&std::string(argv[1])=="--self-test"){
  Require(Complete(DT_SUCCESS)&&!Complete(DT_FAILURE)&&!Complete(DT_SUCCESS|DT_PARTIAL_RESULT)&&
   !Complete(DT_SUCCESS|DT_OUT_OF_NODES)&&!Complete(DT_SUCCESS|DT_BUFFER_TOO_SMALL),"Incomplete query passed");
  std::cout<<"PASS: partial, exhausted, truncated and failed queries are rejected.\n";return 0;
 }
 if(argc!=3)return 2;std::string root=argv[1];std::ifstream input(argv[2]);std::vector<Point> points;Point p;
 std::set<uint64> guids;while(input>>p.kind>>p.guid>>p.x>>p.y>>p.z){
  Require((p.kind=="npc"||p.kind=="rack")&&guids.insert(p.guid).second,"Invalid or duplicate point");points.push_back(p);
 }
 Require(input.eof()&&points.size()==24,"Expected all20 actors and four racks");
 VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1220,{}}});
 MMAP::MMapManager mm;mm.InitializeThreadUnsafe({{1220,{}}});
 std::set<std::pair<int,int>> navTiles;
 std::map<std::pair<int,int>,std::unique_ptr<GridMap>> maps;
 for(auto const& point:points){int tx=int(32-point.x/533.33333f),ty=int(32-point.y/533.33333f);
  for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)
   if(navTiles.insert({tx+dx,ty+dy}).second)mm.loadMap(root+"/",1220,tx+dx,ty+dy);
  auto& grid=maps[{tx,ty}];if(!grid){grid=std::make_unique<GridMap>();
   auto file=root+"/maps/1220_"+std::to_string(tx)+"_"+std::to_string(ty)+".map";
   Require(grid->loadData(file.c_str())==GridMap::LoadResult::Ok,"Missing terrain tile");
   Require(vm.loadMap((root+"/vmaps/").c_str(),1220,tx,ty)==VMAP::VMAP_LOAD_RESULT_OK,"Missing VMAP tile");}
 }
 Require(mm.loadMapInstance(root+"/",1220,0),"Missing native navmesh instance");
 auto query=mm.GetNavMeshQuery(1220,0);auto mesh=mm.GetNavMesh(1220);Require(query&&mesh,"Missing native query");
 // Use the real per-instance query (1024 nodes), native path/corner buffers and
 // native low search extents. No enlarged query budget or forced shortcut.
 dtQueryFilter filter;filter.setIncludeFlags(NAV_GROUND|NAV_WATER);
 dtQueryFilter ground;ground.setIncludeFlags(NAV_GROUND);
 unsigned actors=0,racks=0;float maxActorError=0,maxProjection=0;
 for(auto& point:points){int tx=int(32-point.x/533.33333f),ty=int(32-point.y/533.33333f);auto& grid=maps[{tx,ty}];
  float terrain=grid->getHeight(point.x,point.y),floor=point.z+2>terrain?terrain:-100000;
  float collision=vm.getHeight(1220,point.x,point.y,point.z+2,10);
  if(std::isfinite(collision)&&collision>-10000&&(floor<-10000||collision>floor||std::abs(point.z-floor)>std::abs(point.z-collision)))floor=collision;
  Require(grid->getArea(point.x,point.y)==8297&&std::isfinite(floor)&&floor>-10000,"Invalid Grey Shoals supporting surface");
  float error=std::abs(point.z-floor);
  // Rack origins are reconstructed and their native model extends below Z0.
  // This 1m diagnostic bound proves neither model contact nor original placement.
  Require(error<=(point.kind=="npc"?.02f:1.0f),"Spawn too far from supporting surface");
  if(point.kind=="npc"){++actors;maxActorError=std::max(maxActorError,error);}else ++racks;
  float probe[3]={point.y,point.z,point.x},ext[3]={3,5,3};
  Require(Complete(query->findNearestPoly(probe,ext,&filter,&point.ref,point.near))&&point.ref,"Missing ground/water start polygon");
  float projection=Distance(probe,point.near);maxProjection=std::max(maxProjection,projection);
  Require(projection<=1.5f&&std::abs(probe[0]-point.near[0])<=.01f&&std::abs(probe[2]-point.near[2])<=.01f,"Distant or sideways projection");
  if(point.kind=="rack"){
   Require(projection<5.0f&&vm.isInLineOfSight(1220,point.near[2],point.near[0],point.near[1]+2,point.x,point.y,point.z+2,VMAP::ModelIgnoreFlags::Nothing),"No nearby static-LOS rack approach");
  }
  auto status=query->findNearestPoly(probe,ext,&ground,&point.groundRef,point.groundNear);
  Require(Complete(status),"Ground diagnostic query incomplete");
  std::cout<<point.kind<<' '<<point.guid<<" storedZ="<<point.z<<" floor="<<floor<<" delta="<<point.z-floor<<" navProjection="<<projection<<" groundStart="<<bool(point.groundRef)<<'\n';
 }
 Require(actors==20&&racks==4,"Changed actor/rack inventory");
 unsigned routes=0,waterRoutes=0,groundRoutes=0,groundExhausted=0,groundTruncated=0,maxPolys=0,maxCorners=0;
 for(auto const& a:points)for(auto const& b:points){if(a.guid==b.guid)continue;
  dtPolyRef path[MAX_PATH_LENGTH];int count=0;
  Require(Complete(query->findPath(a.ref,b.ref,a.near,b.near,&filter,path,&count,MAX_PATH_LENGTH))&&count>0&&path[count-1]==b.ref,"Incomplete native corridor");
  maxPolys=std::max(maxPolys,unsigned(count));bool water=false;
  for(int i=0;i<count;++i){dtMeshTile const* tile=nullptr;dtPoly const* poly=nullptr;
   Require(Complete(mesh->getTileAndPolyByRef(path[i],&tile,&poly))&&poly&&poly->flags&&!(poly->flags&~(NAV_GROUND|NAV_WATER)),"Invalid/hazardous path polygon");water|=(poly->flags&NAV_WATER)!=0;}
  float straight[MAX_POINT_PATH_LENGTH*3];unsigned char flags[MAX_POINT_PATH_LENGTH];int corners=0;
  Require(Complete(query->findStraightPath(a.near,b.near,path,count,straight,flags,nullptr,&corners,MAX_POINT_PATH_LENGTH))&&corners>=2&&
   (flags[corners-1]&DT_STRAIGHTPATH_END)&&Distance(straight+3*(corners-1),b.near)<.01f,"Incomplete straight-path endpoint");
  maxCorners=std::max(maxCorners,unsigned(corners));++routes;if(water)++waterRoutes;
  if(a.groundRef&&b.groundRef){int n=0;auto status=query->findPath(a.groundRef,b.groundRef,a.groundNear,b.groundNear,&ground,path,&n,MAX_PATH_LENGTH);
   if(dtStatusDetail(status,DT_OUT_OF_NODES))++groundExhausted;
   if(dtStatusDetail(status,DT_BUFFER_TOO_SMALL))++groundTruncated;
   if(Complete(status)&&n&&path[n-1]==b.groundRef)++groundRoutes;}
 }
 Require(routes==552,"Not every ordered pair checked");
 std::cout<<"PASS:20 NPC and4 rack surfaces/nearby approaches;552 complete directed ground/water corridors and straight endpoints, native1024-node query; maxPolys="<<maxPolys<<" maxCorners="<<maxCorners<<" waterRoutes="<<waterRoutes<<" groundOnlyRoutes="<<groundRoutes<<" groundExhausted="<<groundExhausted<<" groundTruncated="<<groundTruncated<<" maxNPCFloorError="<<maxActorError<<" maxProjection="<<maxProjection<<"\n";
 std::cout<<"No runtime movement, smoothing, dynamic GO collision, client visual contact or exact retail placement claim.\n";
 return 0;
}
int main(int argc,char** argv){try{return Run(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
