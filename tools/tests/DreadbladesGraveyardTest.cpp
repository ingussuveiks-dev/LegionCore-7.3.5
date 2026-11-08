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
struct WorldSafeLocsEntry{uint32 ID,MapID=1545;struct{float X,Y,Z,O;}Loc;};
struct Store{WorldSafeLocsEntry rows[4]={{5299,1545,{-1398.39929f,5891.40967f,.943334281f,350.412537f}},{5359,1545,{-1193.19788f,5828.75684f,5.50435305f,134.865784f}},{5360,1545,{-1093.40454f,6085.1582f,40.1628494f,152.406982f}},{5361,1545,{-1333.24475f,6268.80664f,9.48173618f,115.735718f}}};bool missing=false;unsigned reads=0;WorldSafeLocsEntry const* LookupEntry(uint32 id){++reads;assert(id==5299||id==5359||id==5360||id==5361);return missing?nullptr:&rows[id==5299?0:id==5359?1:id==5360?2:3];}}sWorldSafeLocsStore;
struct Scenario{uint32 id=1012,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Manager{Scenario data;bool missing=false;Scenario* GetScenario(uint32 id){assert(id==987);return missing?nullptr:&data;}}manager;auto sScenarioMgr=&manager;
struct Map{uint32 GetInstanceId(){return 987;}};
struct MapManager{static bool IsValidMapCoord(uint32 m,float x,float y,float z,float o){return m==1545&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o)&&std::fabs(x)<17066&&std::fabs(y)<17066;}};
struct Base{virtual WorldLocation* GetClosestGraveYard(float,float,float)=0;};
struct Probe:Base{Map map;Map* instance=&map;WorldLocation loc_res_pla;unsigned getScenarionStep(){return manager.data.step;}
#include "DreadbladesGraveyard.inc"
};
#ifndef DREADBLADES_GRAVEYARD_BASELINE
static unsigned Slot(unsigned step){return step>=4?3:step>=2?2:step==1?1:0;}
#endif
static void Reset(){manager={};sWorldSafeLocsStore={};}
static void Same(WorldLocation* loc,WorldSafeLocsEntry const& s,bool facing){assert(loc&&loc->map==1545&&loc->x==s.Loc.X&&loc->y==s.Loc.Y&&loc->z==s.Loc.Z&&std::fabs(loc->o-(facing?s.Loc.O*float(M_PI)/180:0))<.001f);}
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<6;++step)for(unsigned pose=0;pose<4;++pose)for(unsigned repeat=0;repeat<2;++repeat){Reset();manager.data.step=step;auto loc=p.GetClosestGraveYard(pose==0?-1398.0f:pose==1?-1333.0f:pose==2?836.16f:0.0f,pose==2?1035.4f:6268.0f,pose==3?-1000.0f:9.0f);
#ifdef DREADBLADES_GRAVEYARD_BASELINE
  Same(loc,sWorldSafeLocsStore.rows[step==3||step==4?2:0],false);
#else
  Same(loc,sWorldSafeLocsStore.rows[Slot(step)],true);
#endif
  assert(loc==&p.loc_res_pla&&manager.data.step==step&&!errors&&sWorldSafeLocsStore.reads==1);++cases;
 }
#ifndef DREADBLADES_GRAVEYARD_BASELINE
 for(unsigned step:{0u,1u,2u,4u})for(unsigned fault=0;fault<14;++fault){Reset();manager.data.step=step;auto& s=sWorldSafeLocsStore.rows[Slot(step)];auto before=errors;if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)s.ID=0;if(fault==2)s.MapID=1600;if(fault==3)s.Loc.X=s.Loc.Y=s.Loc.Z=0;if(fault>=4&&fault<10){float& v=fault<7?s.Loc.X:s.Loc.Y;auto k=(fault-4)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}if(fault>=10){float& v=fault<12?s.Loc.Z:s.Loc.O;v=fault%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}assert(!p.GetClosestGraveYard(1,2,3)&&errors==before+1);++cases;}
 for(unsigned kind=0;kind<2;++kind)for(unsigned pose=0;pose<4;++pose){Reset();if(!kind)manager.missing=true;else manager.data.id=1066;auto before=errors;assert(!p.GetClosestGraveYard(float(pose),4,-7)&&errors==before&&sWorldSafeLocsStore.reads==0);++cases;}
 for(unsigned step:{0u,4u}){Reset();manager.data.step=step;Same(p.GetClosestGraveYard(std::numeric_limits<float>::quiet_NaN(),99999,-1000),sWorldSafeLocsStore.rows[Slot(step)],true);++cases;}
 assert(cases==114);std::cout<<"PASS Dreadblades graveyard114 ASAN:all5 stages+completed,four native checkpoints/facing,56 diagnostic source refusals,repeated/four death contexts/foreign+missing scenario/nonfinite death;actual callback,controlled services,no progress or direct resurrection\n";
#else
 assert(cases==48);std::cout<<"COUNTERFACTUAL pinned9a8f9340d Dreadblades source48:stage1/2 use start,stage4 uses hill,explicit cached5 input returns start;all facing zero. Controlled cached-step inputs,not natural completion or historical executable\n";
#endif
}
