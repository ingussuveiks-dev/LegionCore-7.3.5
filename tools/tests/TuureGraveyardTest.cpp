// Actual callback, with controlled services for missing/corrupt source paths.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
using uint32=std::uint32_t;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static unsigned errors=0;
#define TC_LOG_ERROR(...) (++errors)
struct WorldLocation {uint32 map=0;float x=0,y=0,z=0,o=0;void WorldRelocate(uint32 m,float a,float b,float c,float d=0){map=m;x=a;y=b;z=c;o=d;}};
struct WorldSafeLocsEntry {uint32 ID=5480,MapID=1489;struct {float X=-43.1440964f,Y=1160.33679f,Z=-74.353569f,O=27.7250366f;}Loc;};
struct Store {WorldSafeLocsEntry data,early{5433,1489,{236.392365f,1952.18921f,-55.7522888f,20}},late{5434,1489,{192.675354f,1435.95142f,-35.8320885f,254.1651f}};bool missing=false;uint32 reads=0;WorldSafeLocsEntry const* LookupEntry(uint32 id){assert(id==5480||id==5433||id==5434);++reads;return missing?nullptr:id==5480?&data:id==5433?&early:&late;}}sWorldSafeLocsStore;
struct Scenario {uint32 id=1085,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Manager {Scenario data;bool missing=false;Scenario* GetScenario(uint32 id){assert(id==987);return missing?nullptr:&data;}}manager;
auto sScenarioMgr=&manager;
struct Map {uint32 GetInstanceId(){return 987;}};
struct MapManager {static bool IsValidMapCoord(uint32 m,float x,float y,float z,float o){return m==1489&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o)&&std::fabs(x)<17066&&std::fabs(y)<17066;}};
struct Base {virtual WorldLocation* GetClosestGraveYard(float,float,float)=0;};
struct Probe:Base {Map map;Map* instance=&map;WorldLocation loc_res_pla;
#include "TuureGraveyard.inc"
};
static void Reset(){manager={};sWorldSafeLocsStore={};}
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<8;++step)for(unsigned place=0;place<4;++place)for(unsigned repeat=0;repeat<2;++repeat){Reset();manager.data.step=step;float x=place==0?-828.85f:place==1?71.9653f:place==2?236.392f:-43.1441f;float y=place?1218.51f:4372;float z=place==3?-1000.0f:place==0?738.64f:-74.2702f;auto before=errors;auto loc=p.GetClosestGraveYard(x,y,z);
  assert(loc==&p.loc_res_pla&&loc->map==1489&&errors==before);
#ifdef TUURE_GRAVEYARD_BASELINE
  assert(loc->x==x&&loc->y==y&&loc->z==z&&!sWorldSafeLocsStore.reads);
#else
  auto& s=sWorldSafeLocsStore.data;assert(loc->x==s.Loc.X&&loc->y==s.Loc.Y&&loc->z==s.Loc.Z&&std::fabs(loc->o-s.Loc.O*float(M_PI)/180)<0.001f&&sWorldSafeLocsStore.reads==1);
#endif
  assert(manager.data.step==step);++cases;
 }
#ifndef TUURE_GRAVEYARD_BASELINE
 for(unsigned fault=0;fault<16;++fault){Reset();auto& s=sWorldSafeLocsStore.data;auto before=errors;
  if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)s.ID=5487;if(fault==2)s.MapID=1220;if(fault==3)s.Loc.X=s.Loc.Y=s.Loc.Z=0;
  if(fault>=4&&fault<=9){float& v=fault<7?s.Loc.X:s.Loc.Y;unsigned k=(fault-4)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}
  if(fault>=10&&fault<=13){float& v=fault<12?s.Loc.Z:s.Loc.O;v=fault%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}
  if(fault==14){manager.data.id=1146;sWorldSafeLocsStore.missing=true;}if(fault==15){manager.missing=true;sWorldSafeLocsStore.missing=true;}
  auto loc=p.GetClosestGraveYard(3,4,5);if(fault<14)assert(!loc&&errors==before+1);else assert(loc&&loc->x==3&&loc->y==4&&loc->z==5&&errors==before&&!sWorldSafeLocsStore.reads);++cases;
 }
#endif
 for(unsigned kind=0;kind<3;++kind)for(unsigned place=0;place<4;++place){Reset();if(kind==0)manager.missing=true;else manager.data.id=kind==1?940:1146;auto before=errors;auto loc=p.GetClosestGraveYard(float(place),4,-7);assert(loc&&loc->x==place&&loc->y==4&&loc->z==-7&&errors==before&&!sWorldSafeLocsStore.reads);++cases;}
 for(unsigned kind=0;kind<4;++kind){Reset();float x=kind==0?std::numeric_limits<float>::quiet_NaN():kind==1?std::numeric_limits<float>::infinity():99999;auto loc=p.GetClosestGraveYard(x,-99999,-2000);
#ifdef TUURE_GRAVEYARD_BASELINE
  assert(loc&&(kind==0?std::isnan(loc->x):loc->x==x)&&loc->z==-2000);
#else
  assert(loc&&loc->x==sWorldSafeLocsStore.data.Loc.X&&loc->z==sWorldSafeLocsStore.data.Loc.Z);
#endif
  ++cases;
 }
#ifdef TUURE_GRAVEYARD_BASELINE
 assert(cases==80);std::cout<<"COUNTERFACTUAL pinned562084277 Tuure graveyard source80 ASAN:death and nonfinite/void positions echoed,shared/no-scenario controls;not successful recovery\n";
#else
 assert(cases==96);std::cout<<"PASS extracted Tuure graveyard96 ASAN:1085/all7steps+completed,5480/radians independent of death pose,14 corrupt-source refusals with diagnostics,shared940/1146/no-scenario unchanged,stable location;controlled services,no progression/direct resurrection\n";
#endif
 return 0;
}
