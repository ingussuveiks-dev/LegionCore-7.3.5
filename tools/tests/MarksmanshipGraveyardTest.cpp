// Reuse the whole-callback host; native tests separately establish4->5.
#define main HolyGraveyardUnusedMain
#include "TuureGraveyardTest.cpp"
#undef main
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<9;++step)for(unsigned place=0;place<4;++place)for(unsigned repeat=0;repeat<2;++repeat){Reset();manager.data.id=972;manager.data.step=step;float x=place==0?236.392f:place==1?192.675f:place==2?-828.85f:0;float y=place==0?1952.19f:place==1?1435.95f:place==2?4372:0;float z=place==3?-1000.0f:-35.832f;auto before=errors;auto loc=p.GetClosestGraveYard(x,y,z);assert(loc==&p.loc_res_pla&&loc->map==1489&&errors==before);
#ifdef MARKSMAN_GRAVEYARD_BASELINE
  assert(loc->x==x&&loc->y==y&&loc->z==z&&!sWorldSafeLocsStore.reads);
#else
  auto& s=step<5?sWorldSafeLocsStore.early:sWorldSafeLocsStore.late;assert(loc->x==s.Loc.X&&loc->y==s.Loc.Y&&loc->z==s.Loc.Z&&std::fabs(loc->o-s.Loc.O*float(M_PI)/180)<.001f&&sWorldSafeLocsStore.reads==1);
#endif
  assert(manager.data.step==step);++cases;
 }
#ifndef MARKSMAN_GRAVEYARD_BASELINE
 for(unsigned step:{4u,5u})for(unsigned fault=0;fault<14;++fault){Reset();manager.data.id=972;manager.data.step=step;auto& s=step==4?sWorldSafeLocsStore.early:sWorldSafeLocsStore.late;auto before=errors;
  if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)s.ID=5480;if(fault==2)s.MapID=1220;if(fault==3)s.Loc.X=s.Loc.Y=s.Loc.Z=0;
  if(fault>=4&&fault<=9){float& v=fault<7?s.Loc.X:s.Loc.Y;unsigned k=(fault-4)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}
  if(fault>=10){float& v=fault<12?s.Loc.Z:s.Loc.O;v=fault%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}
  assert(!p.GetClosestGraveYard(3,4,5)&&errors==before+1);++cases;
 }
#endif
 for(unsigned kind=0;kind<3;++kind)for(unsigned place=0;place<4;++place){Reset();if(kind==0)manager.missing=true;else manager.data.id=kind==1?1145:1085;auto loc=p.GetClosestGraveYard(float(place),4,-7);if(kind==2)assert(loc&&loc->x==sWorldSafeLocsStore.data.Loc.X&&loc->z==sWorldSafeLocsStore.data.Loc.Z);else assert(loc&&loc->x==place&&loc->y==4&&loc->z==-7);++cases;}
#ifdef MARKSMAN_GRAVEYARD_BASELINE
 assert(cases==84);std::cout<<"COUNTERFACTUAL pinnedf3c39b87a MM graveyard source84 ASAN:all8 stages+completed echo corpse positions,retained Holy/other/no-scenario;not safe recovery\n";
#else
 assert(cases==112);std::cout<<"PASS extracted MM graveyard112 ASAN:972/all8stages+completed,5433 before5/5434 from5,28 invalid-source refusals with diagnostics,repeated selection independent of death pose,retained Holy/other/no-scenario;controlled services,no progression/direct resurrection\n";
#endif
}
