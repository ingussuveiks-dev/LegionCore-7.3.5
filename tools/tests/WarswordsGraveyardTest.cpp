// Extracted production callback; source/scenario/coordinate services controlled.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
using uint32=std::uint32_t;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
unsigned errors=0;
#define TC_LOG_ERROR(...) (++errors)
struct WorldLocation {uint32 map=0;float x=0,y=0,z=0,o=0;void WorldRelocate(uint32 m,float a,float b,float c,float d){map=m;x=a;y=b;z=c;o=d;}};
struct WorldSafeLocsEntry {uint32 MapID=1511;struct {float X=3380.29f,Y=1401.9f,Z=69.5355f,O=45.7539f;}Loc;};
struct Store {WorldSafeLocsEntry data;bool missing=false;WorldSafeLocsEntry const* LookupEntry(uint32 id){assert(id==5228);return missing?nullptr:&data;}}sWorldSafeLocsStore;
struct Scenario {uint32 id=944;uint32 GetScenarioId(){return id;}};
struct Manager {Scenario data;bool missing=false;Scenario* GetScenario(uint32 id){assert(id==987);return missing?nullptr:&data;}}manager;
auto sScenarioMgr=&manager;
struct Map {uint32 GetInstanceId(){return 987;}};
struct MapManager {static bool IsValidMapCoord(uint32 m,float x,float y,float z,float o){return m==1511&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o)&&std::fabs(x)<17066&&std::fabs(y)<17066;}};
struct Base {virtual WorldLocation* GetClosestGraveYard(float,float,float)=0;};
struct Probe:Base {Map map;Map* instance=&map;WorldLocation graveyard;
#include "FuryGraveyard.inc"
};
int main(){Probe p;unsigned cases=0;
 for(unsigned repeat=0;repeat<2;++repeat){auto loc=p.GetClosestGraveYard(999,999,999);assert(loc==&p.graveyard&&loc->map==1511&&loc->x==sWorldSafeLocsStore.data.Loc.X&&std::fabs(loc->o-0.798555f)<0.001f);assert(!errors);++cases;}
 for(unsigned kind=0;kind<16;++kind){sWorldSafeLocsStore=Store{};manager=Manager{};auto& s=sWorldSafeLocsStore.data;auto before=errors;
  if(kind==0)manager.missing=true;
  if(kind==1)manager.data.id=973;
  if(kind==2)sWorldSafeLocsStore.missing=true;
  if(kind==3)s.MapID=1539;
  if(kind==4)s.Loc.X=s.Loc.Y=s.Loc.Z=0;
  if(kind>=5&&kind<11){float* value=kind<8?&s.Loc.X:&s.Loc.Y;unsigned v=(kind-5)%3;*value=v==0?std::numeric_limits<float>::quiet_NaN():v==1?std::numeric_limits<float>::infinity():2000000.0f;}
  if(kind>=11&&kind<15){float* value=kind<13?&s.Loc.Z:&s.Loc.O;*value=(kind-11)%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}
  if(kind==15){manager.data.id=973;sWorldSafeLocsStore.missing=true;}
  assert(!p.GetClosestGraveYard(0,0,0));assert(errors==before+(kind>=2&&kind!=15));++cases;
 }
 assert(cases==18);std::cout<<"PASS extracted Fury graveyard18 ASAN:stable native entrance,missing/foreign scenario,missing/wrong/zero/nonfinite/out-of-bounds source,contextual error endpoint and native-degree orientation;services controlled"<<std::endl;
}
