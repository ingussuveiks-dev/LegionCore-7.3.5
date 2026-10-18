#include <map>
#include "Errors.h"
#include "Optional.h"
#include "VMapManager2.h"
#include "MapTree.h"
#include "ModelInstance.h"
#include "GridMap.h"
#include "MMapManager.h"
#include "MapDefines.h"
#include "DetourCommon.h"
#include "DetourMath.h"
#include "Position.h"
class WorldObject;
#include "MoveSpline.h"
#include "NativePathLimits.inc"
#include "VrektFlightSettings.inc"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

static void Require(bool value,char const* reason){if(!value)throw std::runtime_error(reason);}
static bool Complete(dtStatus status){return dtStatusSucceed(status)&&!(status&DT_STATUS_DETAIL_MASK);}
static float Distance(float const* a,float const* b){return dtVdist(a,b);}
struct NativePathSmoothing {
 dtNavMesh const* _navMesh;dtNavMeshQuery const* _navMeshQuery;dtQueryFilter _filter;
 uint32 FixupCorridor(dtPolyRef*,uint32,uint32,dtPolyRef const*,uint32);
 bool GetSteerTarget(float const*,float const*,float,dtPolyRef const*,uint32,float*,unsigned char&,dtPolyRef&);
 dtStatus FindSmoothPath(float const*,float const*,dtPolyRef const*,uint32,float*,int*,uint32);
 bool InRangeYZX(float const*,float const*,float,float)const;
};
// Only log expressions are fixture leaves. These production methods query the
// actual loaded native mesh; no replacement smoothing/world-height algorithm.
#undef TC_LOG_DEBUG
#define TC_LOG_DEBUG(...) ((void)0)
#include "VrektNativeSmoothing.inc"
struct Point{std::string kind;uint64 guid;unsigned step;float x,y,z;dtPolyRef ref=0,groundRef=0;float raw[3],navPoint[3],groundNear[3];};
int Run(int argc,char** argv){
 if(argc==2&&std::string(argv[1])=="--self-test"){
  Require(Complete(DT_SUCCESS)&&!Complete(DT_FAILURE)&&!Complete(DT_SUCCESS|DT_PARTIAL_RESULT)&&!Complete(DT_SUCCESS|DT_OUT_OF_NODES)&&!Complete(DT_SUCCESS|DT_BUFFER_TOO_SMALL),"Incomplete status admitted");
  std::cout<<"PASS: partial/exhausted/truncated/failed status rejection.\n";return 0;
 }
 if(argc!=3)return 2;std::string root=argv[1];std::ifstream input(argv[2]);std::vector<Point> points;Point p;
 std::set<std::pair<uint64,unsigned>> keys;
 while(input>>p.kind>>p.guid>>p.step>>p.x>>p.y>>p.z){Require((p.kind=="npc"||p.kind=="wp")&&keys.insert({p.guid,p.step}).second,"Invalid/duplicate point key");points.push_back(p);}
 Require(input.eof()&&points.size()==100,"Expected34 anchors and66 patrol points");
 VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1220,{}}});MMAP::MMapManager mm;mm.InitializeThreadUnsafe({{1220,{}}});
 std::set<std::pair<int,int>> navTiles;std::map<std::pair<int,int>,std::unique_ptr<GridMap>> maps;
 auto gridAt=[&](float x,float y)->GridMap*{
  int tx=int(32-x/533.33333f),ty=int(32-y/533.33333f);auto& grid=maps[{tx,ty}];
  if(!grid){grid=std::make_unique<GridMap>();auto file=root+"/maps/1220_"+std::to_string(tx)+"_"+std::to_string(ty)+".map";
   Require(grid->loadData(file.c_str())==GridMap::LoadResult::Ok,"Missing terrain tile");Require(vm.loadMap((root+"/vmaps/").c_str(),1220,tx,ty)==VMAP::VMAP_LOAD_RESULT_OK,"Missing VMAP tile");
   for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(navTiles.insert({tx+dx,ty+dy}).second)mm.loadMap(root+"/",1220,tx+dx,ty+dy);
   std::cout<<"TILE "<<tx<<' '<<ty<<'\n';}
  return grid.get();
 };
 for(auto const& point:points)gridAt(point.x,point.y);
 Require(mm.loadMapInstance(root+"/",1220,0),"Missing native query instance");auto query=mm.GetNavMeshQuery(1220,0);auto mesh=mm.GetNavMesh(1220);Require(query&&mesh,"Missing mesh/query");
 dtQueryFilter filter;filter.setIncludeFlags(NAV_GROUND|NAV_WATER);dtQueryFilter ground;ground.setIncludeFlags(NAV_GROUND);
 std::vector<Point const*> anchors;std::map<uint64,std::vector<Point const*>> patrols;Point const* drake=nullptr;
 float maxFloor=0,minDelta=0,maxDelta=0,maxProjection=0,maxHorizontal=0;
 for(auto& point:points){auto grid=gridAt(point.x,point.y);float terrain=grid->getHeight(point.x,point.y),floor=point.z+2>terrain?terrain:-100000;
  float collision=vm.getHeight(1220,point.x,point.y,point.z+2,10);
  if(std::isfinite(collision)&&collision>-10000&&(floor<-10000||collision>floor||std::abs(point.z-floor)>std::abs(point.z-collision)))floor=collision;
  Require(grid->getArea(point.x,point.y)==8362&&std::isfinite(floor)&&floor>-10000,"Invalid supporting surface/area");
  float delta=point.z-floor;maxFloor=std::max(maxFloor,std::abs(delta));minDelta=std::min(minDelta,delta);maxDelta=std::max(maxDelta,delta);
  // A regression guard, not proof of exact model-foot contact on this slope.
  Require(std::abs(delta)<=.5f,"Supporting-surface offset changed; re-audit placement");
  point.raw[0]=point.y;point.raw[1]=point.z;point.raw[2]=point.x;float ext[3]={3,5,3};
  Require(Complete(query->findNearestPoly(point.raw,ext,&filter,&point.ref,point.navPoint))&&point.ref,"Missing low-box start polygon");
  float projection=Distance(point.raw,point.navPoint),horizontal=std::hypot(point.navPoint[0]-point.y,point.navPoint[2]-point.x);
  Require(projection<=NativeAllowedPolyDist&&horizontal<=.01f,"Far/sideways start projection");
  maxProjection=std::max(maxProjection,projection);maxHorizontal=std::max(maxHorizontal,horizontal);
  Require(Complete(query->findNearestPoly(point.raw,ext,&ground,&point.groundRef,point.groundNear))&&point.groundRef,"Missing ground-only point");
  if(point.kind=="npc"){anchors.push_back(&point);if(point.guid==293175)drake=&point;}else patrols[point.guid].push_back(&point);
  std::cout<<"POINT "<<point.kind<<' '<<point.guid<<' '<<point.step<<" delta="<<delta<<" nav="<<projection<<" terrain="<<terrain<<" vmap="<<collision<<'\n';
 }
 Require(anchors.size()==34&&patrols.size()==10&&drake,"Changed actor/patrol inventory");
 NativePathSmoothing smoothing{mesh,query,filter};unsigned maxPolys=0,maxCorners=0,maxSmooth=0,waterRoutes=0,smoothComplete=0,smoothLimited=0,groundComplete=0;float maxPreNormalizeEndZ=0;
 auto route=[&](Point const& a,Point const& b,bool requireSmooth){
  dtPolyRef path[MAX_PATH_LENGTH];int count=0;auto status=query->findPath(a.ref,b.ref,a.raw,b.raw,&filter,path,&count,MAX_PATH_LENGTH);
  Require(Complete(status)&&count&&path[count-1]==b.ref,"Incomplete native corridor");maxPolys=std::max(maxPolys,unsigned(count));bool water=false;
  for(int i=0;i<count;++i){dtMeshTile const* tile=nullptr;dtPoly const* poly=nullptr;Require(Complete(mesh->getTileAndPolyByRef(path[i],&tile,&poly))&&poly&&poly->flags&&!(poly->flags&~(NAV_GROUND|NAV_WATER)),"Hazardous/invalid polygon");water|=bool(poly->flags&NAV_WATER);}
  float straight[MAX_POINT_PATH_LENGTH*3];unsigned char flags[MAX_POINT_PATH_LENGTH];int corners=0;
  Require(Complete(query->findStraightPath(a.raw,b.raw,path,count,straight,flags,nullptr,&corners,MAX_POINT_PATH_LENGTH))&&corners>=1&&(flags[corners-1]&DT_STRAIGHTPATH_END)&&Distance(straight+3*(corners-1),b.raw)<.01f,"Incomplete straight endpoint");maxCorners=std::max(maxCorners,unsigned(corners));if(water)++waterRoutes;
  float smooth[MAX_POINT_PATH_LENGTH*3];int n=0;auto result=smoothing.FindSmoothPath(a.raw,b.raw,path,count,smooth,&n,MAX_POINT_PATH_LENGTH);
  bool full=Complete(result)&&n>0&&std::hypot(smooth[3*(n-1)]-b.y,smooth[3*(n-1)+2]-b.x)<SMOOTH_PATH_SLOP&&std::abs(smooth[3*(n-1)+1]-b.z)<=NativeAllowedPolyDist;
  if(full){++smoothComplete;maxSmooth=std::max(maxSmooth,unsigned(n));maxPreNormalizeEndZ=std::max(maxPreNormalizeEndZ,std::abs(smooth[3*(n-1)+1]-b.z));}else{++smoothLimited;std::cout<<"SMOOTH_LIMIT "<<a.guid<<':'<<a.step<<" -> "<<b.guid<<':'<<b.step<<" count="<<n<<" status="<<result<<" xy="<<(n?std::hypot(smooth[3*(n-1)]-b.y,smooth[3*(n-1)+2]-b.x):-1)<<" dz="<<(n?smooth[3*(n-1)+1]-b.z:-1)<<'\n';}
  Require(!requireSmooth||full,"Patrol smoothing did not reach endpoint");
 };
 unsigned directed=0;for(auto a:anchors)for(auto b:anchors)if(a!=b){route(*a,*b,false);++directed;dtPolyRef path[MAX_PATH_LENGTH];int n=0;auto s=query->findPath(a->groundRef,b->groundRef,a->raw,b->raw,&ground,path,&n,MAX_PATH_LENGTH);if(Complete(s)&&n&&path[n-1]==b->groundRef)++groundComplete;}
 unsigned patrolRoutes=0,duplicates=0;
 for(auto& item:patrols){auto& nodes=item.second;std::sort(nodes.begin(),nodes.end(),[](Point const* a,Point const* b){return a->step<b->step;});auto start=std::find_if(anchors.begin(),anchors.end(),[&](Point const* a){return a->guid==item.first;});Require(start!=anchors.end(),"Patrol missing its spawn");route(**start,*nodes.front(),true);++patrolRoutes;
  for(size_t i=0;i<nodes.size();++i){Require(nodes[i]->step==i+1,"Changed patrol order");auto next=nodes[(i+1)%nodes.size()];if(Distance(nodes[i]->raw,next->raw)<.01f)++duplicates;route(*nodes[i],*next,true);++patrolRoutes;}
 }
 Require(directed==1122&&groundComplete==1122&&patrolRoutes==76,"Not every required route completed");
 std::cout<<"SUMMARY anchors="<<anchors.size()<<" patrolPoints=66 patrols="<<patrols.size()<<" directed="<<directed<<" patrolRoutes="<<patrolRoutes<<" duplicateEdges="<<duplicates<<" maxPolys="<<maxPolys<<" maxCorners="<<maxCorners<<" maxSmooth="<<maxSmooth<<" smoothComplete="<<smoothComplete<<" smoothLimited="<<smoothLimited<<" groundComplete="<<groundComplete<<" waterRoutes="<<waterRoutes<<" minFloorDelta="<<minDelta<<" maxFloorDelta="<<maxDelta<<" maxAbsFloorDelta="<<maxFloor<<" maxProjection="<<maxProjection<<" maxHorizontal="<<maxHorizontal<<" maxPreNormalizeEndZ="<<maxPreNormalizeEndZ<<'\n';
 float hx,hy,hz;bool oldHit=vm.getObjectHitPos(1220,drake->x,drake->y,drake->z+OldRise,drake->x+OldX,drake->y+OldY,drake->z+OldZ,hx,hy,hz,0);
 Require(oldHit,"Pinned old escape no longer reproduces collision; re-audit assets");std::cout<<"OLD_FLIGHT_HIT "<<hx<<' '<<hy<<' '<<hz<<'\n';
 G3D::Vector3 start(drake->x,drake->y,drake->z),lift(start.x,start.y,start.z+FlightRise),end(start.x+FlightX,start.y+FlightY,start.z+FlightZ);
 Require(vm.isInLineOfSight(1220,start.x,start.y,start.z,lift.x,lift.y,lift.z,VMAP::ModelIgnoreFlags::Nothing)&&vm.isInLineOfSight(1220,lift.x,lift.y,lift.z,end.x,end.y,end.z,VMAP::ModelIgnoreFlags::Nothing),"Current flight centreline crosses static collision");
 unsigned samples=0;float minGap=1e9f;int duration=0;
 for(unsigned leg=0;leg<2;++leg){Movement::MoveSplineInitArgs args;args.path={leg?lift:start,leg?end:lift};args.flags.EnableFlying();if(leg){args.flags.EnableCatmullRom();args.flags.uncompressedPath=true;}args.velocity=leg?FlightSpeed:TakeoffSpeed;args.HasVelocity=true;Require(args.Validate(),"Invalid native spline arguments");Movement::MoveSpline spline;spline.Initialize(args);duration+=spline.Duration();auto previous=args.path.front();
  while(!spline.Finalized()){spline.updateState(std::min(10,spline.Duration()-spline.GetTimePassed()));auto v=spline.ComputePosition();auto grid=gridAt(v.x,v.y);float gap=v.z-grid->getHeight(v.x,v.y);minGap=std::min(minGap,gap);Require(gap>=0&&vm.isInLineOfSight(1220,previous.x,previous.y,previous.z,v.x,v.y,v.z,VMAP::ModelIgnoreFlags::Nothing),"Native flight sample intersects terrain/static collision");previous=v;++samples;}
  auto actual=spline.ComputePosition();Require((actual-args.path.back()).length()<.01f,"Native spline endpoint mismatch");
 }
 Require(duration+ReleaseDelay<FlightTimeout,"Flight cannot finish before fallback timeout");std::cout<<"FLIGHT samples="<<samples<<" splineDurationMs="<<duration<<" minTerrainGap="<<minGap<<" releaseDelayMs="<<ReleaseDelay<<" timeoutMs="<<FlightTimeout<<'\n';
 VMAP::InstanceTreeMap trees;vm.getInstanceMapTree(trees);std::set<std::string> models;for(auto const& tree:trees){VMAP::ModelInstance* instances=nullptr;uint32 count=0;tree.second->getModelInstances(instances,count);for(uint32 i=0;i<count;++i)if(instances[i].getWorldModel())models.insert(instances[i].name);}
 for(auto const& model:models)std::cout<<"MODEL "<<model<<".vmo\n";
 std::cout<<"PASS: native Vrekt surfaces/navmesh/patrol smoothing and collision-free flight centreline. Floor offsets are diagnostics; dynamic objects, full wing clearance and client movement/rendering remain unverified.\n";return 0;
}
int main(int argc,char** argv){try{return Run(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
