#include "MMapManager.h"
#include "MapDefines.h"
#include "VMapManager2.h"
#include "GridMap.h"
#include "MovementTypedefs.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
struct Point{float x,y,z;bool jump=false;};
static bool Complete(dtStatus s){return dtStatusSucceed(s)&&!(s&DT_STATUS_DETAIL_MASK);}
static float Distance(Point a,Point b){return std::sqrt((b.x-a.x)*(b.x-a.x)+(b.y-a.y)*(b.y-a.y)+(b.z-a.z)*(b.z-a.z));}
int main(int argc,char** argv)
{
 if(argc!=3&&argc!=6)return 1;std::string root=argv[1];std::ifstream input(argv[2]);std::vector<Point> points;Point p;
 Point target=argc==6?Point{std::stof(argv[3]),std::stof(argv[4]),std::stof(argv[5])}:Point{2033.69f,2186.11f,465.141f};
 while(input>>p.x>>p.y>>p.z>>p.jump)points.push_back(p);if(points.size()<2)return 1;
 VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1220,{}}});MMAP::MMapManager mm;mm.InitializeThreadUnsafe({{1220,{}}});std::map<std::pair<int,int>,std::unique_ptr<GridMap>> grids;
 for(int x=27;x<=29;++x)for(int y=26;y<=28;++y){vm.loadMap((root+"/vmaps/").c_str(),1220,x,y);mm.loadMap(root+"/",1220,x,y);auto grid=std::make_unique<GridMap>();if(grid->loadData((root+"/maps/1220_"+std::to_string(x)+"_"+std::to_string(y)+".map").c_str())!=GridMap::LoadResult::Ok)return 2;grids[{x,y}]=std::move(grid);}
 auto terrain=[&](Point a){auto& grid=grids[{int(32-a.x/533.33333f),int(32-a.y/533.33333f)}];return grid?grid->getHeight(a.x,a.y):-100000.f;};
 auto floor=[&](Point a){float t=terrain(a),v=vm.getHeight(1220,a.x,a.y,a.z+.3f,2);return v>-10000&&(t>a.z+.3f||v>t)?v:t;};
 auto standing=[&](Point a){return std::isfinite(floor(a))&&std::abs(floor(a)-a.z)<.04f&&vm.isInLineOfSight(1220,a.x,a.y,a.z+.05f,a.x,a.y,a.z+1.8f,VMAP::ModelIgnoreFlags::Nothing);};
 auto ray=[&](Point a,Point b){for(float h:{.1f,1.f,1.7f})for(auto offset:{std::pair<float,float>{0,0},{.25f,0},{-.25f,0},{0,.25f},{0,-.25f}})if(!vm.isInLineOfSight(1220,a.x+offset.first,a.y+offset.second,a.z+h,b.x+offset.first,b.y+offset.second,b.z+h,VMAP::ModelIgnoreFlags::Nothing))return false;return true;};
 for(auto a:points)if(!standing(a)){std::cout<<"Bad support "<<a.x<<","<<a.y<<","<<a.z<<'\n';return 3;}
 unsigned jumps=0;float length=0;
 for(unsigned i=1;i<points.size();++i){auto a=points[i-1],b=points[i];length+=Distance(a,b);
  if(!b.jump){if(std::abs(b.z-a.z)>.35f||std::hypot(b.x-a.x,b.y-a.y)>.72f||!ray(a,b))return 4;continue;}
  ++jumps;float velocity=b.z<a.z?0.f:7.95555f,g=Movement::gravity,disc=velocity*velocity-2*g*(b.z-a.z);if(disc<0)return 4;float duration=(velocity+std::sqrt(disc))/g;if(duration<=0||std::hypot(b.x-a.x,b.y-a.y)/duration>7||b.z-a.z>1||b.z-a.z< -12)return 4;
  Point previous=a;for(int n=1;n<=40;++n){float f=n/40.f,t=duration*f;Point q{a.x+(b.x-a.x)*f,a.y+(b.y-a.y)*f,a.z+velocity*t-.5f*g*t*t};
   float ground=terrain(q),v=vm.getHeight(1220,q.x,q.y,q.z+.1f,20);
   if(!ray(previous,q)||(ground>q.z+.1f&&v<-10000)){std::cout<<"Blocked arc "<<i<<" sample "<<n<<'\n';return 4;}previous=q;
  }
 }
 auto mesh=mm.GetNavMesh(1220);if(!mesh)return 5;dtNavMeshQuery query;if(!Complete(query.init(mesh,65535)))return 5;dtQueryFilter filter;filter.setIncludeFlags(NAV_GROUND|NAV_WATER);
 float source[3]={2433.03f,485.375f,2182.78f},anchor[3]={points.front().y,points.front().z,points.front().x},start[3],end[3],startExt[3]={2,4,2},endExt[3]={.25f,.8f,.25f};dtPolyRef startRef=0,endRef=0;
 if(!Complete(query.findNearestPoly(source,startExt,&filter,&startRef,start))||!startRef||!Complete(query.findNearestPoly(anchor,endExt,&filter,&endRef,end))||!endRef)return 5;
 Point join{end[2],end[0],end[1]};join.z=floor(join);if(!standing(join)||Distance(join,points.front())>.36f||!ray(join,points.front())){std::cout<<"Bad join "<<join.x<<","<<join.y<<","<<join.z<<" first="<<points.front().x<<","<<points.front().y<<","<<points.front().z<<" distance="<<Distance(join,points.front())<<'\n';return 5;}
 dtPolyRef path[2048];int count=0;auto status=query.findPath(startRef,endRef,start,end,&filter,path,&count,2048);if(!Complete(status)||!count||path[count-1]!=endRef)return 5;
 for(int i=0;i<count;++i){dtMeshTile const* tile=nullptr;dtPoly const* poly=nullptr;if(!Complete(mesh->getTileAndPolyByRef(path[i],&tile,&poly))||!poly||!(poly->flags&filter.getIncludeFlags())||(poly->flags&~(NAV_GROUND|NAV_WATER)))return 5;}
 float straight[6144];unsigned char flags[2048];int corners=0;if(!Complete(query.findStraightPath(start,end,path,count,straight,flags,nullptr,&corners,2048))||corners<2||!(flags[corners-1]&DT_STRAIGHTPATH_END)||Distance({straight[3*(corners-1)+2],straight[3*(corners-1)],straight[3*(corners-1)+1]},{end[2],end[0],end[1]})>.01f)return 5;
 auto last=points.back();if(Distance(last,target)>5||!vm.isInLineOfSight(1220,last.x,last.y,last.z+2,target.x,target.y,target.z+2,VMAP::ModelIgnoreFlags::Nothing))return 6;
 std::cout<<"PASS: complete "<<count<<"-polygon native corridor to cave approach join; "<<points.size()<<" supported samples, "<<length<<"yd, "<<jumps<<" bounded jump/fall arcs with native gravity,15 body rays/40 samples, no terrain tunneling; final circle range5/LOS.\n";
 std::cout<<"Manual locomotion feasibility only: not a scripted player path or a client movement/animation verification.\n";
}
