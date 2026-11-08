// Reuse the whole-callback host; native tests separately establish2->3.
#define main HolyGraveyardUnusedMain
#include "TuureGraveyardTest.cpp"
#undef main
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<7;++step)for(unsigned place=0;place<4;++place)for(unsigned repeat=0;repeat<2;++repeat){Reset();manager.data.id=1146;manager.data.step=step;float x=place==0?236.392f:place==1?192.675f:place==2?-828.85f:0;float y=place==0?1952.19f:place==1?1435.95f:place==2?4372:0;float z=place==3?-1000.0f:-35.832f;auto before=errors;auto loc=p.GetClosestGraveYard(x,y,z);assert(loc==&p.loc_res_pla&&loc->map==1489&&errors==before);
#ifdef HODIR_GRAVEYARD_BASELINE
  assert(loc->x==x&&loc->y==y&&loc->z==z&&!sWorldSafeLocsStore.reads);
#else
  auto& s=step<3?sWorldSafeLocsStore.hodir:sWorldSafeLocsStore.hodirLate;assert(loc->x==s.Loc.X&&loc->y==s.Loc.Y&&loc->z==s.Loc.Z&&std::fabs(loc->o-s.Loc.O*float(M_PI)/180)<.001f&&sWorldSafeLocsStore.reads==1);
#endif
  assert(manager.data.step==step);++cases;
 }
#ifndef HODIR_GRAVEYARD_BASELINE
 for(unsigned step:{2u,3u})for(unsigned fault=0;fault<14;++fault){Reset();manager.data.id=1146;manager.data.step=step;auto& s=step==2?sWorldSafeLocsStore.hodir:sWorldSafeLocsStore.hodirLate;auto before=errors;
  if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)s.ID=5480;if(fault==2)s.MapID=1220;if(fault==3)s.Loc.X=s.Loc.Y=s.Loc.Z=0;
  if(fault>=4&&fault<=9){float& v=fault<7?s.Loc.X:s.Loc.Y;unsigned k=(fault-4)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}
  if(fault>=10){float& v=fault<12?s.Loc.Z:s.Loc.O;v=fault%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}
  assert(!p.GetClosestGraveYard(3,4,5)&&errors==before+1);++cases;
 }
#endif
 for(unsigned kind=0;kind<5;++kind)for(unsigned place=0;place<4;++place){Reset();if(kind==0)manager.missing=true;else manager.data.id=kind==1?1085:kind==2?972:kind==3?940:1145;auto loc=p.GetClosestGraveYard(float(place),4,-7);if(kind==1)assert(loc&&loc->x==sWorldSafeLocsStore.data.Loc.X);else if(kind==2)assert(loc&&loc->x==sWorldSafeLocsStore.early.Loc.X);else if(kind==3)assert(loc&&loc->x==graveyardSpells.data.target_X);else assert(loc&&loc->x==place&&loc->y==4&&loc->z==-7);++cases;}
 for(unsigned step:{0u,2u,3u}){Reset();manager.data.id=1146;manager.data.step=step;auto loc=p.GetClosestGraveYard(std::numeric_limits<float>::quiet_NaN(),99999,-1000);
#ifdef HODIR_GRAVEYARD_BASELINE
  assert(loc&&std::isnan(loc->x)&&loc->z==-1000);
#else
  auto& safe=step>=3?sWorldSafeLocsStore.hodirLate:sWorldSafeLocsStore.hodir;assert(loc&&loc->x==safe.Loc.X&&loc->y==safe.Loc.Y&&loc->z==safe.Loc.Z);
#endif
  ++cases;
 }
#ifdef HODIR_GRAVEYARD_BASELINE
 assert(cases==79);std::cout<<"COUNTERFACTUAL pinnedd2f2840b2 Hodir graveyard source79 ASAN:all6 stages+completed echo corpse positions,retained Holy/MM/Blood/other/no-scenario;not safe recovery\n";
#else
 assert(cases==107);std::cout<<"PASS extracted Hodir graveyard107 ASAN:1146/all6stages+completed,5654 before3/5655 from3,28 invalid-source refusals with diagnostics,repeated selection independent of death pose,retained Holy/MM/Blood/other/no-scenario;controlled services,no progression/direct resurrection\n";
#endif
}
