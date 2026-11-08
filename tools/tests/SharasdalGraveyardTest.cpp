// Actual whole cemetery callback; native services/fault injection controlled.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
using uint32=std::uint32_t;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static unsigned errors=0;
#define TC_LOG_ERROR(...) (++errors)
struct WorldLocation{uint32 map=0;float x=0,y=0,z=0,o=0;void WorldRelocate(uint32 m,float a,float b,float c,float d=0){map=m;x=a;y=b;z=c;o=d;}};
struct WorldSafeLocsEntry{uint32 ID,MapID=1600;struct{float X,Y,Z,O;}Loc;};
struct Store{WorldSafeLocsEntry lower{5443,1600,{-622.690979f,805.760437f,245.953827f,0}},upper{5473,1600,{-184.494003f,801.723999f,796.705017f,359.64801f}};bool missing=false;unsigned reads=0;WorldSafeLocsEntry const* LookupEntry(uint32 id){assert(id==5443||id==5473);++reads;return missing?nullptr:id==5443?&lower:&upper;}}sWorldSafeLocsStore;
struct Progress{unsigned Counter=0;bool operator==(Progress const& other)const{return Counter==other.Counter;}};
struct Achievement{std::map<uint32,Progress> rows;auto GetCriteriaProgressMap(){return &rows;}};
struct Scenario{uint32 id=1066,step=0;Achievement progress;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}auto& GetAchievementMgr(){return progress;}};
struct Manager{Scenario data;bool missing=false;Scenario* GetScenario(uint32 id){assert(id==987);return missing?nullptr:&data;}}manager;auto sScenarioMgr=&manager;
struct Map{uint32 GetInstanceId(){return 987;}};
struct MapManager{static bool IsValidMapCoord(uint32 m,float x,float y,float z,float o){return m==1600&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o)&&std::fabs(x)<17066&&std::fabs(y)<17066;}};
struct Base{virtual WorldLocation* GetClosestGraveYard(float,float,float)=0;};
struct Probe:Base{Map map;Map* instance=&map;WorldLocation loc_res_pla;
#include "SharasdalGraveyard.inc"
};
static void Reset(){manager={};sWorldSafeLocsStore={};}
#ifdef SHARASDAL_GRAVEYARD_BASELINE
static void Death(WorldLocation* loc,float x,float y,float z){assert(loc&&loc->map==1600&&(loc->x==x||(std::isnan(x)&&std::isnan(loc->x)))&&loc->y==y&&loc->z==z&&loc->o==0);}
#endif
#ifndef SHARASDAL_GRAVEYARD_BASELINE
static void Same(WorldLocation* loc,bool upper){auto const& s=upper?sWorldSafeLocsStore.upper:sWorldSafeLocsStore.lower;assert(loc&&loc->map==1600&&loc->x==s.Loc.X&&loc->y==s.Loc.Y&&loc->z==s.Loc.Z&&std::fabs(loc->o-s.Loc.O*float(M_PI)/180)<.001f);}
#endif
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<7;++step)for(unsigned receipt=0;receipt<3;++receipt)for(unsigned pose=0;pose<4;++pose)for(unsigned repeat=0;repeat<2;++repeat){Reset();manager.data.step=step;if(receipt)manager.data.progress.rows[47131].Counter=receipt-1;manager.data.progress.rows[47378].Counter=1;auto before=manager.data.progress.rows;float x=pose==0?-622.0f:pose==1?-184.0f:pose==2?836.16f:0;float z=pose==3?-1000.0f:pose==0?245.0f:796.0f;auto loc=p.GetClosestGraveYard(x,805,z);assert(loc==&p.loc_res_pla&&manager.data.step==step&&manager.data.progress.rows==before&&!errors);
#ifdef SHARASDAL_GRAVEYARD_BASELINE
  Death(loc,x,805,z);assert(!sWorldSafeLocsStore.reads);
#else
  Same(loc,step>=4||(step==3&&receipt==2));assert(sWorldSafeLocsStore.reads==1);
#endif
  ++cases;
 }
#ifndef SHARASDAL_GRAVEYARD_BASELINE
 for(unsigned step:{0u,4u})for(unsigned fault=0;fault<14;++fault){Reset();manager.data.step=step;auto& s=step?sWorldSafeLocsStore.upper:sWorldSafeLocsStore.lower;auto before=errors;if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)s.ID=0;if(fault==2)s.MapID=1220;if(fault==3)s.Loc.X=s.Loc.Y=s.Loc.Z=0;if(fault>=4&&fault<10){float& v=fault<7?s.Loc.X:s.Loc.Y;auto k=(fault-4)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}if(fault>=10){float& v=fault<12?s.Loc.Z:s.Loc.O;v=fault%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}assert(!p.GetClosestGraveYard(1,2,3)&&errors==before+1);++cases;}
#endif
 for(unsigned missing=0;missing<2;++missing)for(unsigned pose=0;pose<4;++pose){Reset();manager.missing=missing;manager.data.id=1092;auto before=errors;auto loc=p.GetClosestGraveYard(float(pose),4,-7);
#ifdef SHARASDAL_GRAVEYARD_BASELINE
  Death(loc,float(pose),4,-7);
#else
  assert(!loc&&!sWorldSafeLocsStore.reads);
#endif
  assert(errors==before);++cases;
 }
 for(unsigned step:{0u,4u}){Reset();manager.data.step=step;auto loc=p.GetClosestGraveYard(std::numeric_limits<float>::quiet_NaN(),99999,-1000);
#ifdef SHARASDAL_GRAVEYARD_BASELINE
  Death(loc,std::numeric_limits<float>::quiet_NaN(),99999,-1000);
#else
  Same(loc,step==4);
#endif
  ++cases;
 }
#ifdef SHARASDAL_GRAVEYARD_BASELINE
 assert(cases==178);std::cout<<"COUNTERFACTUAL pinnedd8cc8bb9e Sharasdal source178 ASAN:death pose returned verbatim,including below-map/foreign/nonfinite;no native source selection\n";
#else
 assert(cases==206);std::cout<<"PASS Sharasdal graveyard206 ASAN:5443 before real stage3 ascent47131,5473 after receipt/from4,all6 stages+completed,missing/zero/positive receipt and unrelated root,28 source refusals,foreign/missing scenario null,repeat/nonfinite-death/no progression controls;actual callback,controlled services\n";
#endif
}
