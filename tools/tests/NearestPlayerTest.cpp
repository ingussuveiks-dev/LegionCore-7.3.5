#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32=uint32_t;
struct Player;
using PlayerMapType=std::vector<Player*>;
struct WorldObject{
 int map=1;uint32 phase=1;float x=0,y=0,z=0,visibility=10;mutable uint32 visibilityReads=0;
 float GetDistance(WorldObject const* other)const{return std::sqrt((x-other->x)*(x-other->x)+(y-other->y)*(y-other->y)+(z-other->z)*(z-other->z));}
 bool IsWithinDistInMap(WorldObject const* other,float range)const{return map==other->map&&GetDistance(other)<=range;}
 float GetVisibilityRange()const{++visibilityReads;return visibility;}
 uint32 GetPhaseMask()const{return phase;}
 Player* FindNearestPlayer(float range,bool alive=true);
};
struct Player:WorldObject{bool living=true;bool IsAlive()const{return living;}bool InSamePhase(uint32 mask)const{return (phase&mask)!=0;}};
namespace Trinity{
#include "NearestCheck.inc"
template<class Check>
#include "NearestSearcher.inc"
}
#include "NearestVisit.inc"
namespace Trinity{
#include "NearestMethods.inc"
static std::vector<PlayerMapType> cells;
template<class Searcher>void VisitNearbyWorldObject(WorldObject*,float,Searcher& searcher){for(auto& cell:cells)searcher.Visit(cell);}
}
#include "NearestWorld.inc"
int main(){uint32 cases=0;
 for(uint32 mode=0;mode<8;++mode)for(bool alive:{true,false}){
  std::array<int,3> order{0,1,2};do{
   WorldObject source;std::array<Player,3> players;players[0].x=3;players[1].x=7;players[2].x=40;Player* expected=&players[0];float range=15;
   if(mode==1){players[0].living=false;if(alive)expected=&players[1];}
   if(mode==2){players[0].phase=2;expected=&players[1];}
   if(mode==3){players[0].map=2;expected=&players[1];}
   if(mode==4){players[0].x=20;players[1].x=30;expected=nullptr;}
   if(mode==5){for(auto& p:players)p.living=false;if(alive)expected=nullptr;}
   if(mode==6){players[0].x=0;players[0].z=20;expected=&players[1];}
   if(mode==7){players[0].x=30;players[1].x=40;players[2].x=50;range=60;}
   Trinity::cells={{&players[order[0]]},{&players[order[1]]},{&players[order[2]]}};
   assert(source.FindNearestPlayer(range,alive)==expected);assert(!source.visibilityReads);++cases;
  }while(std::next_permutation(order.begin(),order.end()));
 }
 {WorldObject source;Player dead,live;dead.living=false;dead.x=3;live.x=7;Trinity::NearestPlayerInObjectRangeCheck check(&source,15);assert(!check(&dead)&&check(&live));++cases;}
 {Player source,other;other.x=3;Trinity::cells={{&other},{&source}};assert(source.FindNearestPlayer(15)==&source);++cases;}
 Trinity::cells.clear();std::cout<<"PASS "<<cases<<" actual helper/check/last-visitor ASAN contexts;96 order/map/phase/3D/radius/life contexts, existing2-argument live-only predicate and Player self. Geometry/cells modeled; native unit/grid tests separate\n";
}
