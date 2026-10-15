#include "VMapManager2.h"
#include "GridMap.h"
#include "Position.h"
#include "stormheim_cry_thunder_route.h"
#include <cmath>
#include <map>
#include <memory>
#include <iostream>
#include <algorithm>
int main(int argc,char**argv){
 if(argc!=2)return 2;std::string root=argv[1];VMAP::VMapManager2 vm;vm.InitializeThreadUnsafe({{1220,{}}});std::map<std::pair<int,int>,std::unique_ptr<GridMap>> maps;
 auto terrain=[&](float x,float y){int tx=int(32-x/533.33333f),ty=int(32-y/533.33333f);auto& g=maps[{tx,ty}];if(!g){g=std::make_unique<GridMap>();if(g->loadData((root+"/maps/1220_"+std::to_string(tx)+"_"+std::to_string(ty)+".map").c_str())!=GridMap::LoadResult::Ok||vm.loadMap((root+"/vmaps/").c_str(),1220,tx,ty)!=VMAP::VMAP_LOAD_RESULT_OK)throw std::runtime_error("Missing geometry");}return g->getHeight(x,y);};
 auto floor=[&](Position const& p){float ground=terrain(p.GetPositionX(),p.GetPositionY()),mesh=vm.getHeight(1220,p.GetPositionX(),p.GetPositionY(),p.GetPositionZ()+2,10);float f=p.GetPositionZ()+2>ground?ground:-100000;
 if(mesh>-10000&&(f<=-10000||mesh>f||std::fabs(f-p.GetPositionZ())>std::fabs(mesh-p.GetPositionZ())))f=mesh;return f;};
 unsigned failures=0,anchors=0;auto check=[&](Position const& p,float tolerance){float f=floor(p);bool valid=f>-10000&&std::isfinite(f)&&std::fabs(p.GetPositionZ()-f)<tolerance;if(!valid){++failures;std::cout<<"bad anchor "<<p.GetPositionX()<<","<<p.GetPositionY()<<","<<p.GetPositionZ()<<" floor="<<f<<"\n";}++anchors;};
 for(auto const& seed:StormheimCryThunder::Seeds)check(seed.pos,2);check(StormheimCryThunder::Source,1);check(StormheimCryThunder::Landing,1);check({3066.67f,2246.5f,224.916f,0},2);
 unsigned sky=0;float clearance=10000;
 for(float x=2350;x<=2720;x+=10)for(float y=2250;y<=2680;y+=10){float g=terrain(x,y),mesh=vm.getHeight(1220,x,y,850,1000);float top=std::max(g,mesh);clearance=std::min(clearance,850-top);if(!std::isfinite(top)||850-top<20)++failures;
 auto const& end=StormheimCryThunder::ReturnPath[0];for(int dx:{-2,0,2})for(int dy:{-2,0,2})if(!vm.isInLineOfSight(1220,x+dx,y+dy,850,end.GetPositionX()+dx,end.GetPositionY()+dy,850,VMAP::ModelIgnoreFlags::Nothing))++failures;++sky;}
 for(size_t i=1;i<3;++i){auto const& a=StormheimCryThunder::ReturnPath[i-1];auto const& b=StormheimCryThunder::ReturnPath[i];int n=int(std::ceil(a.GetExactDist(&b)))+1;
 for(int k=0;k<=n;++k)for(int dx:{-2,0,2})for(int dy:{-2,0,2}){float t=float(k)/n,x=a.GetPositionX()+(b.GetPositionX()-a.GetPositionX())*t+dx,y=a.GetPositionY()+(b.GetPositionY()-a.GetPositionY())*t+dy,z=a.GetPositionZ()+(b.GetPositionZ()-a.GetPositionZ())*t;float g=terrain(x,y),m=vm.getHeight(1220,x,y,z,1000);if(z-std::max(g,m)<1)++failures;}
 for(int dx:{-2,0,2})for(int dy:{-2,0,2})for(int dz:{0,4})if(!vm.isInLineOfSight(1220,a.GetPositionX()+dx,a.GetPositionY()+dy,a.GetPositionZ()+dz,b.GetPositionX()+dx,b.GetPositionY()+dy,b.GetPositionZ()+dz,VMAP::ModelIgnoreFlags::Nothing))++failures;
 }
 std::cout<<anchors<<" seed/source/landing/Huginn anchors; "<<sky<<" sky samples/return corridors, minimum sky clearance="<<clearance<<"; "<<failures<<" failures. Dynamic initial climb is runtime LOS-gated; full model/wings need client checks.\n";return failures?1:0;
}
