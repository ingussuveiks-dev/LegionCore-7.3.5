// Actual whole callback with controlled native-store/error paths.
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
struct WorldLocation{uint32 map=0;float x=0,y=0,z=0,o=0;void WorldRelocate(uint32 m,float a,float b,float c,float d=0){map=m;x=a;y=b;z=c;o=d;}};
struct WorldSafeLocsEntry{uint32 ID,MapID=1539;struct{float X,Y,Z,O;}Loc;};
struct Store{WorldSafeLocsEntry general{5281,1539,{2038.49133f,2442.02271f,135.075638f,196.582993f}},early{5339,1539,{1934.17883f,2513.69019f,130.607132f,341.615692f}},middle{5335,1539,{2072.02002f,2348.32007f,66.4023972f,198.197998f}},late{5705,1539,{1877.68994f,2249,37.5471001f,269.972992f}},outside{5488,1539,{2116.3999f,2428.46997f,119.737f,314.596985f}};bool missing=false;unsigned reads=0;WorldSafeLocsEntry const* LookupEntry(uint32 id){assert(id==5281||id==5339||id==5335||id==5705||id==5488);++reads;return missing?nullptr:id==5281?&general:id==5339?&early:id==5335?&middle:id==5488?&outside:&late;}}sWorldSafeLocsStore;
struct Scenario{uint32 id=1037,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Manager{Scenario data;bool missing=false;Scenario* GetScenario(uint32 id){assert(id==987);return missing?nullptr:&data;}}manager;auto sScenarioMgr=&manager;
struct Map{uint32 GetInstanceId(){return 987;}};
struct MapManager{static bool IsValidMapCoord(uint32 m,float x,float y,float z,float o){return m==1539&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o)&&std::fabs(x)<17066&&std::fabs(y)<17066;}};
struct Base{virtual WorldLocation* GetClosestGraveYard(float,float,float)=0;};
struct Probe:Base{Map map;Map* instance=&map;WorldLocation loc_res_pla;
#include "ArmsGraveyard.inc"
};
static void Reset(){manager={};sWorldSafeLocsStore={};}
#ifndef ARMS_GRAVEYARD_BASELINE
static auto const& Expected(unsigned step){return step<4?sWorldSafeLocsStore.early:step<7?sWorldSafeLocsStore.middle:sWorldSafeLocsStore.late;}
#endif
static void Same(WorldLocation* loc,WorldSafeLocsEntry const& s,bool facing=true){assert(loc&&loc->map==s.MapID&&loc->x==s.Loc.X&&loc->y==s.Loc.Y&&loc->z==s.Loc.Z&&std::fabs(loc->o-(facing?s.Loc.O*float(M_PI)/180:0))<.001f);}
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<11;++step)for(unsigned pose=0;pose<4;++pose)for(unsigned repeat=0;repeat<2;++repeat){Reset();manager.data.step=step;auto loc=p.GetClosestGraveYard(pose==0?1934.0f:pose==1?1877.0f:pose==2?-828.85f:0.0f,pose==2?4372.0f:2249.0f,pose==3?-1000.0f:37.0f);assert(loc==&p.loc_res_pla&&manager.data.step==step&&!errors&&sWorldSafeLocsStore.reads==1);
#ifdef ARMS_GRAVEYARD_BASELINE
  Same(loc,sWorldSafeLocsStore.general,false);
#else
  Same(loc,Expected(step));
#endif
  ++cases;
 }
#ifndef ARMS_GRAVEYARD_BASELINE
 for(unsigned step:{0u,4u,7u})for(unsigned fault=0;fault<14;++fault){Reset();manager.data.step=step;auto& s=step<4?sWorldSafeLocsStore.early:step<7?sWorldSafeLocsStore.middle:sWorldSafeLocsStore.late;auto before=errors;if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)s.ID=5281;if(fault==2)s.MapID=1220;if(fault==3)s.Loc.X=s.Loc.Y=s.Loc.Z=0;if(fault>=4&&fault<10){float& v=fault<7?s.Loc.X:s.Loc.Y;auto k=(fault-4)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}if(fault>=10){float& v=fault<12?s.Loc.Z:s.Loc.O;v=fault%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}assert(!p.GetClosestGraveYard(1,2,3)&&errors==before+1);++cases;}
#endif
 for(unsigned kind=0;kind<4;++kind)for(unsigned pose=0;pose<4;++pose){Reset();if(!kind)manager.missing=true;else manager.data.id=kind==1?991:kind==2?1092:1146;auto before=errors;auto loc=p.GetClosestGraveYard(float(pose),4,-7);
#ifndef ARMS_GRAVEYARD_BASELINE
 if(kind==2)Same(loc,sWorldSafeLocsStore.outside);else
#endif
 Same(loc,sWorldSafeLocsStore.general,false);assert(errors==before&&sWorldSafeLocsStore.reads==1);++cases;}
 for(unsigned step:{0u,4u,7u}){Reset();manager.data.step=step;auto loc=p.GetClosestGraveYard(std::numeric_limits<float>::quiet_NaN(),99999,-1000);
#ifdef ARMS_GRAVEYARD_BASELINE
  Same(loc,sWorldSafeLocsStore.general,false);
#else
  Same(loc,Expected(step));
#endif
  ++cases;
 }
#ifdef ARMS_GRAVEYARD_BASELINE
 assert(cases==107);std::cout<<"COUNTERFACTUAL pinned96ca7646c Arms source107 ASAN:all stages use generic5281/zero-facing instead of class checkpoints;retained shared/no-scenario,not repaired class recovery\n";
#else
 assert(cases==149);std::cout<<"PASS Arms graveyard149 ASAN:all10 stages+completed,5339 before4/5335 from4/5705 from7,42 diagnostic source refusals,repeat/nonfinite-death/retained991/1092/other/no-scenario controls;actual callback,controlled services,no progression/direct resurrection\n";
#endif
}
