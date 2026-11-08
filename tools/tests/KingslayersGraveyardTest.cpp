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
struct WorldSafeLocsEntry{uint32 ID,MapID=1620;struct{float X,Y,Z,O;}Loc;};
struct Store{WorldSafeLocsEntry rows[4]={{5518,1620,{-9127.9375f,389.055573f,91.2150726f,35.15448f}},{5528,1620,{-8847.75879f,568.449646f,94.6873474f,134.798553f}},{5529,1620,{-8637.58203f,419.171875f,103.701897f,307.027008f}},{5530,1620,{-8346.0957f,268.420135f,155.347153f,38.0432129f}}};bool missing=false;unsigned reads=0;WorldSafeLocsEntry const* LookupEntry(uint32 id){++reads;assert(id==5518||id==5528||id==5529||id==5530);return missing?nullptr:&rows[id==5518?0:id==5528?1:id==5529?2:3];}}sWorldSafeLocsStore;
struct Scenario{uint32 id=1123,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Manager{Scenario data;bool missing=false;Scenario* GetScenario(uint32 id){assert(id==987);return missing?nullptr:&data;}}manager;auto sScenarioMgr=&manager;
struct Map{uint32 GetInstanceId(){return 987;}};
struct MapManager{static bool IsValidMapCoord(uint32 m,float x,float y,float z,float o){return m==1620&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o)&&std::fabs(x)<17066&&std::fabs(y)<17066;}};
struct Base{virtual WorldLocation* GetClosestGraveYard(float,float,float)=0;};
struct Probe:Base{Map map;Map* instance=&map;WorldLocation loc_res_pla;unsigned getScenarionStep(){return manager.data.step;}
#include "KingslayersGraveyard.inc"
};
#ifndef KINGSLAYERS_GRAVEYARD_BASELINE
static unsigned Slot(unsigned step){return step>=7?3:step>=5?2:step>=2?1:0;}
#endif
static void Reset(){manager={};sWorldSafeLocsStore={};}
static void Same(WorldLocation* loc,WorldSafeLocsEntry const& s,bool facing){assert(loc&&loc->map==1620&&loc->x==s.Loc.X&&loc->y==s.Loc.Y&&loc->z==s.Loc.Z&&std::fabs(loc->o-(facing?s.Loc.O*float(M_PI)/180:0))<.001f);}
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<11;++step)for(unsigned pose=0;pose<4;++pose)for(unsigned repeat=0;repeat<2;++repeat){Reset();manager.data.step=step;auto loc=p.GetClosestGraveYard(pose==0?-1398.0f:pose==1?-1333.0f:pose==2?836.16f:0.0f,pose==2?1035.4f:6268.0f,pose==3?-1000.0f:9.0f);
#ifdef KINGSLAYERS_GRAVEYARD_BASELINE
  Same(loc,sWorldSafeLocsStore.rows[step>=7&&step<10?3:step>=5&&step<7?2:step>=2&&step<5?1:0],false);
#else
  Same(loc,sWorldSafeLocsStore.rows[Slot(step)],true);
#endif
  assert(loc==&p.loc_res_pla&&manager.data.step==step&&!errors&&sWorldSafeLocsStore.reads==1);++cases;
 }
#ifndef KINGSLAYERS_GRAVEYARD_BASELINE
 for(unsigned step:{0u,2u,5u,7u})for(unsigned fault=0;fault<14;++fault){Reset();manager.data.step=step;auto& s=sWorldSafeLocsStore.rows[Slot(step)];auto before=errors;if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)s.ID=0;if(fault==2)s.MapID=1600;if(fault==3)s.Loc.X=s.Loc.Y=s.Loc.Z=0;if(fault>=4&&fault<10){float& v=fault<7?s.Loc.X:s.Loc.Y;auto k=(fault-4)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}if(fault>=10){float& v=fault<12?s.Loc.Z:s.Loc.O;v=fault%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}assert(!p.GetClosestGraveYard(1,2,3)&&errors==before+1);++cases;}
 for(unsigned kind=0;kind<2;++kind)for(unsigned pose=0;pose<4;++pose){Reset();if(!kind)manager.missing=true;else manager.data.id=1066;auto before=errors;assert(!p.GetClosestGraveYard(float(pose),4,-7)&&errors==before&&sWorldSafeLocsStore.reads==0);++cases;}
 for(unsigned step:{0u,10u}){Reset();manager.data.step=step;Same(p.GetClosestGraveYard(std::numeric_limits<float>::quiet_NaN(),99999,-1000),sWorldSafeLocsStore.rows[Slot(step)],true);++cases;}
 assert(cases==154);std::cout<<"PASS Kingslayers graveyard154 ASAN:all10 stages+completed,four native checkpoints/facing,56 diagnostic source refusals,repeated/four death contexts/foreign+missing scenario/nonfinite death;actual callback,controlled services,no progress or direct resurrection\n";
#else
 assert(cases==88);std::cout<<"COUNTERFACTUAL pinned148c610e4 Kingslayers source88:cached inputs0..9 retain native checkpoints,explicit cached10 input returns start;all facing zero. Controlled cached-step inputs,not natural completion or historical executable\n";
#endif
}
