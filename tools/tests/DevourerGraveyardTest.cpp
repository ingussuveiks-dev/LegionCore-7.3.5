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
struct WorldSafeLocsEntry{uint32 ID,MapID=1607;struct{float X,Y,Z,O;}Loc;};
struct Store{WorldSafeLocsEntry rows[4]={{5467,1607,{1854.599f,1409.84546f,91.729599f,268.272125f}},{5468,1607,{1847.91846f,1260.42542f,57.0953293f,69.0323181f}},{5469,1607,{1854.63892f,1351.53296f,66.3808899f,270.503601f}},{5470,1607,{1817.86462f,1288.27429f,91.7157059f,340.499573f}}};bool missing=false;unsigned reads=0;WorldSafeLocsEntry const* LookupEntry(uint32 id){++reads;assert(id==5467||id==5468||id==5469||id==5470);return missing?nullptr:&rows[id==5467?0:id==5468?1:id==5469?2:3];}}sWorldSafeLocsStore;
struct Scenario{uint32 id=1078,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Manager{Scenario data;bool missing=false;Scenario* GetScenario(uint32 id){assert(id==987);return missing?nullptr:&data;}}manager;auto sScenarioMgr=&manager;
struct Map{uint32 GetInstanceId(){return 987;}};
struct MapManager{static bool IsValidMapCoord(uint32 m,float x,float y,float z,float o){return m==1607&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o)&&std::fabs(x)<17066&&std::fabs(y)<17066;}};
struct Base{virtual WorldLocation* GetClosestGraveYard(float,float,float)=0;};
struct Probe:Base{Map map;Map* instance=&map;WorldLocation loc_res_pla;unsigned getScenarionStep(){return manager.data.step;}
#include "DevourerGraveyard.inc"
};
#ifndef DEVOURER_GRAVEYARD_BASELINE
static unsigned Slot(unsigned step){return step>=5?3:step>=3?2:step>=1?1:0;}
#endif
static void Reset(){manager={};sWorldSafeLocsStore={};}
static void Same(WorldLocation* loc,WorldSafeLocsEntry const& s,bool facing){assert(loc&&loc->map==1607&&loc->x==s.Loc.X&&loc->y==s.Loc.Y&&loc->z==s.Loc.Z&&std::fabs(loc->o-(facing?s.Loc.O*float(M_PI)/180:0))<.001f);}
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<8;++step)for(unsigned pose=0;pose<4;++pose)for(unsigned repeat=0;repeat<2;++repeat){Reset();manager.data.step=step;auto loc=p.GetClosestGraveYard(pose==0?-1398.0f:pose==1?-1333.0f:pose==2?836.16f:0.0f,pose==2?1035.4f:6268.0f,pose==3?-1000.0f:9.0f);
#ifdef DEVOURER_GRAVEYARD_BASELINE
  Same(loc,sWorldSafeLocsStore.rows[step>=5&&step<7?3:step>=3&&step<5?2:step>=1&&step<3?1:0],false);
#else
  Same(loc,sWorldSafeLocsStore.rows[Slot(step)],true);
#endif
  assert(loc==&p.loc_res_pla&&manager.data.step==step&&!errors&&sWorldSafeLocsStore.reads==1);++cases;
 }
#ifndef DEVOURER_GRAVEYARD_BASELINE
 for(unsigned step:{0u,1u,3u,5u})for(unsigned fault=0;fault<14;++fault){Reset();manager.data.step=step;auto& s=sWorldSafeLocsStore.rows[Slot(step)];auto before=errors;if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)s.ID=0;if(fault==2)s.MapID=1600;if(fault==3)s.Loc.X=s.Loc.Y=s.Loc.Z=0;if(fault>=4&&fault<10){float& v=fault<7?s.Loc.X:s.Loc.Y;auto k=(fault-4)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}if(fault>=10){float& v=fault<12?s.Loc.Z:s.Loc.O;v=fault%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}assert(!p.GetClosestGraveYard(1,2,3)&&errors==before+1);++cases;}
 for(unsigned kind=0;kind<2;++kind)for(unsigned pose=0;pose<4;++pose){Reset();if(!kind)manager.missing=true;else manager.data.id=1066;auto before=errors;assert(!p.GetClosestGraveYard(float(pose),4,-7)&&errors==before&&sWorldSafeLocsStore.reads==0);++cases;}
 for(unsigned step:{0u,7u}){Reset();manager.data.step=step;Same(p.GetClosestGraveYard(std::numeric_limits<float>::quiet_NaN(),99999,-1000),sWorldSafeLocsStore.rows[Slot(step)],true);++cases;}
 assert(cases==130);std::cout<<"PASS Devourer graveyard130 ASAN:all7 stages+completed,four native checkpoints/facing,56 diagnostic source refusals,repeated/four death contexts/foreign+missing scenario/nonfinite death;actual callback,controlled services,no progress or direct resurrection\n";
#else
 assert(cases==64);std::cout<<"COUNTERFACTUAL pinned3465a1d48 Devourer source64:cached inputs0..6 retain native checkpoints,explicit cached7 input returns start;all facing zero. Controlled cached-step inputs,not natural completion or historical executable\n";
#endif
}
