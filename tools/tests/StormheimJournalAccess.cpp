#include "MMapManager.h"
#include "MapDefines.h"
#include "VMapManager2.h"
#include "GridMap.h"
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "G3D/g3dmath.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

static DB2Storage<GameObjectDisplayInfoEntry> displays("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
struct AccessObject {
 struct Info{uint32 displayId=25948;} info;Info const* m_goInfo=&info;
 float x=3515.9f,y=1526.83f,z=8.7028f,o=6.25676f;
 float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}
 bool IsWithinDist3d(float px,float py,float pz,float radius)const{return std::sqrt((px-x)*(px-x)+(py-y)*(py-y)+(pz-z)*(pz-z))<=radius;}
 bool IsInRange(float,float,float,float)const;
};
#define sGameObjectDisplayInfoStore displays
#include "JournalAccessRange.inc"
#undef sGameObjectDisplayInfoStore

struct Point{float x,y,z;};
static bool Complete(dtStatus status){return dtStatusSucceed(status)&&!(status&DT_STATUS_DETAIL_MASK);}
static float Distance(Point const& a,Point const& b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
static float Floor(VMAP::VMapManager2& vm,Point const& p){return vm.getHeight(1220,p.x,p.y,p.z+.3f,1.5f);}
static bool FloorMatches(VMAP::VMapManager2& vm,Point const& p){float z=Floor(vm,p);return std::isfinite(z)&&z>-1000&&std::abs(z-p.z)<.025f;}
static bool BodyClear(VMAP::VMapManager2& vm,Point const& a,Point const& b)
{
 if(std::abs(b.z-a.z)>.35f||std::hypot(b.x-a.x,b.y-a.y)>.36f)return false;
 for(float height:{.4f,1.f,1.7f})for(auto offset:{std::pair<float,float>{0.f,0.f},{.25f,0.f},{-.25f,0.f},{0.f,.25f},{0.f,-.25f}})
  if(!vm.isInLineOfSight(1220,a.x+offset.first,a.y+offset.second,a.z+height,b.x+offset.first,b.y+offset.second,b.z+height,VMAP::ModelIgnoreFlags::Nothing))return false;
 return true;
}

int main(int argc,char** argv)
{
 if(argc!=3)return 1;std::string root=argv[1];
 if(!displays.Load((root+"/dbc/enUS/").c_str(),0)||!displays.LookupEntry(15405)||!displays.LookupEntry(25948))return 2;
 VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1220,{}}});MMAP::MMapManager mm;mm.InitializeThreadUnsafe({{1220,{}}});
 for(int x=24;x<=26;++x)for(int y=28;y<=30;++y){vm.loadMap((root+"/vmaps/").c_str(),1220,x,y);mm.loadMap(root+"/",1220,x,y);}

 // Authored Kjell repair: actual native support and model lower bound determine
 // Z. Check a slightly larger footprint than its tiny rotated model bounds.
 Point kjell{3438.79f,1560.27f,7.01219f};auto paper=displays.LookupEntry(15405);
 float floor=vm.getHeight(1220,kjell.x,kjell.y,kjell.z,2);
 float clearance=kjell.z+paper->GeoBoxMin.Z-floor;
 if(!std::isfinite(floor)||std::abs(floor-6.439294f)>.025f||clearance<.015f||clearance>.03f)return 3;
 for(float dx:{-.4f,0.f,.4f})for(float dy:{-.45f,0.f,.45f}){
  float support=vm.getHeight(1220,kjell.x+dx,kjell.y+dy,floor+.3f,1);
  if(!std::isfinite(support)||std::abs(support-floor)>.15f||!vm.isInLineOfSight(1220,kjell.x+dx,kjell.y+dy,floor+.05f,kjell.x+dx,kjell.y+dy,floor+1.1f,VMAP::ModelIgnoreFlags::Nothing))return 3;
 }
 for(float h:{.1f,.5f,1.05f})for(int direction:{-1,1})
  if(!vm.isInLineOfSight(1220,kjell.x-.4f,kjell.y+direction*.45f,floor+h,kjell.x+.4f,kjell.y-direction*.45f,floor+h,VMAP::ModelIgnoreFlags::Nothing))return 3;
 std::cout<<"PASS: Kjell native floor="<<floor<<" model-bottom clearance="<<clearance<<"; nine support/vertical rays and six footprint diagonals clear.\n";

 std::ifstream input(argv[2]);if(!input)return 4;std::vector<Point> points;std::string line;
 while(std::getline(input,line)){if(line.empty()||line[0]=='#')continue;std::istringstream in(line);Point p;if(!(in>>p.x>>p.y>>p.z))return 4;points.push_back(p);}
 if(points.size()<2)return 4;
 for(auto const& p:points){if(!FloorMatches(vm,p)||!vm.isInLineOfSight(1220,p.x,p.y,p.z+.05f,p.x,p.y,p.z+1.8f,VMAP::ModelIgnoreFlags::Nothing))return 5;}
 float length=0;for(unsigned i=1;i<points.size();++i){if(!BodyClear(vm,points[i-1],points[i])){std::cout<<"Blocked walk segment "<<i<<'\n';return 5;}length+=Distance(points[i-1],points[i]);}

 // Require a complete native corridor to the exact beginning of the collision
 // walk, rather than accepting the old partial corridor to the isolated island.
 auto mesh=mm.GetNavMesh(1220);if(!mesh)return 6;dtNavMeshQuery query;if(!Complete(query.init(mesh,65535)))return 6;
 dtQueryFilter filter;filter.setIncludeFlags(NAV_GROUND|NAV_WATER);
 float source[3]={1377.97f,97.9798f,3296.72f},start[3],end[3],startExt[3]={2,4,2},endExt[3]={.25f,.8f,.25f},anchor[3]={points.front().y,points.front().z,points.front().x};dtPolyRef startRef=0,endRef=0;
 if(!Complete(query.findNearestPoly(source,startExt,&filter,&startRef,start))||!startRef||!Complete(query.findNearestPoly(anchor,endExt,&filter,&endRef,end))||!endRef)return 6;
 if(Distance({start[2],start[0],start[1]},{source[2],source[0],source[1]})>5)return 6;
 Point navFloor{end[2],end[0],end[1]};navFloor.z=vm.getHeight(1220,navFloor.x,navFloor.y,end[1]+.3f,1.5f);
 if(!FloorMatches(vm,navFloor)||std::abs(navFloor.z-end[1])>1||!BodyClear(vm,navFloor,points.front())){std::cout<<"Navmesh/walk join rejected "<<navFloor.x<<","<<navFloor.y<<","<<navFloor.z<<'\n';return 6;}
 dtPolyRef path[2048];int count=0;auto status=query.findPath(startRef,endRef,start,end,&filter,path,&count,2048);
 if(!Complete(status)||!count||path[count-1]!=endRef)return 6;
 float straight[2048*3];unsigned char flags[2048];int corners=0;
 if(!Complete(query.findStraightPath(start,end,path,count,straight,flags,nullptr,&corners,2048))||corners<2||!(flags[corners-1]&DT_STRAIGHTPATH_END)||Distance({straight[3*(corners-1)+2],straight[3*(corners-1)],straight[3*(corners-1)+1]},{end[2],end[0],end[1]})>.01f)return 6;
 for(int i=0;i<count;++i){dtMeshTile const* tile=nullptr;dtPoly const* poly=nullptr;
  if(!Complete(mesh->getTileAndPolyByRef(path[i],&tile,&poly))||!poly||!(poly->flags&filter.getIncludeFlags())||(poly->flags&~(NAV_GROUND|NAV_WATER)))return 6;
 }
 AccessObject roster;auto model=displays.LookupEntry(25948);auto const& last=points.back();
 bool visible=vm.isInLineOfSight(1220,last.x,last.y,last.z+2,roster.x,roster.y,roster.z+model->GeoBoxMax.Z,VMAP::ModelIgnoreFlags::Nothing);
 if(!roster.IsInRange(last.x,last.y,last.z,5)||!visible)return 7;
 std::cout<<"PASS: complete native corridor "<<count<<" polygons to exact Halvir walk join; "<<points.size()<<" collision-floor samples, "<<length<<"yd, fifteen body rays per segment, no jumps; native model-box range5 and model-top LOS pass.\n";
 std::cout<<"Endpoint "<<last.x<<","<<last.y<<","<<last.z<<"; static collision evidence only, client movement/rendering still pending.\n";
}
