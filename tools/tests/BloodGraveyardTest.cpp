#define main HolyGraveyardUnusedMain
#include "TuureGraveyardTest.cpp"
#undef main
#ifndef BLOOD_GRAVEYARD_BASELINE
static void VerifyBlood(WorldLocation const* loc,unsigned step){
 assert(loc&&loc->map==1489);
 if(step<3){auto& s=graveyardSpells.data;assert(loc->x==s.target_X&&loc->y==s.target_Y&&loc->z==s.target_Z&&std::fabs(loc->o-s.target_Orientation)<.001f);}
 else{auto& s=step<7?sWorldSafeLocsStore.blood:sWorldSafeLocsStore.bloodLate;assert(loc->x==s.Loc.X&&loc->y==s.Loc.Y&&loc->z==s.Loc.Z&&std::fabs(loc->o-s.Loc.O*float(M_PI)/180)<.001f);}
}
#endif
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<12;++step)for(unsigned place=0;place<4;++place)for(unsigned repeat=0;repeat<2;++repeat){Reset();manager.data.id=940;manager.data.step=step;float x=place==0?240.9f:place==1?311.418f:place==2?196.017f:0;float y=place==0?1953.44f:place==1?1669.007f:1524.392f;float z=place==3?-1000.0f:-55.29f;auto before=errors;auto loc=p.GetClosestGraveYard(x,y,z);assert(loc==&p.loc_res_pla&&errors==before);
#ifdef BLOOD_GRAVEYARD_BASELINE
  assert(loc->map==1489&&loc->x==x&&loc->y==y&&loc->z==z&&!graveyardSpells.reads&&!sWorldSafeLocsStore.reads);
#else
  VerifyBlood(loc,step);assert(graveyardSpells.reads==unsigned(step<3)&&sWorldSafeLocsStore.reads==unsigned(step>=3));
#endif
  assert(manager.data.step==step);++cases;
 }
#ifndef BLOOD_GRAVEYARD_BASELINE
 for(unsigned fault=0;fault<13;++fault){Reset();manager.data.id=940;auto& s=graveyardSpells.data;auto before=errors;
  if(fault==0)graveyardSpells.missing=true;if(fault==1)s.target_mapId=1220;if(fault==2)s.target_X=s.target_Y=s.target_Z=0;
  if(fault>=3&&fault<=8){float& v=fault<6?s.target_X:s.target_Y;unsigned k=(fault-3)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}
  if(fault>=9){float& v=fault<11?s.target_Z:s.target_Orientation;v=fault%2?std::numeric_limits<float>::quiet_NaN():std::numeric_limits<float>::infinity();}
  assert(!p.GetClosestGraveYard(1,2,3)&&errors==before+1&&!sWorldSafeLocsStore.reads);++cases;
 }
 for(unsigned step:{3u,7u})for(unsigned fault=0;fault<14;++fault){Reset();manager.data.id=940;manager.data.step=step;auto& s=step==3?sWorldSafeLocsStore.blood:sWorldSafeLocsStore.bloodLate;auto before=errors;
  if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)s.ID=5480;if(fault==2)s.MapID=1220;if(fault==3)s.Loc.X=s.Loc.Y=s.Loc.Z=0;
  if(fault>=4&&fault<=9){float& v=fault<7?s.Loc.X:s.Loc.Y;unsigned k=(fault-4)%3;v=k==0?std::numeric_limits<float>::quiet_NaN():k==1?std::numeric_limits<float>::infinity():2000000;}
  if(fault>=10){float& v=fault<12?s.Loc.Z:s.Loc.O;v=fault%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}
  assert(!p.GetClosestGraveYard(1,2,3)&&errors==before+1&&!graveyardSpells.reads);++cases;
 }
#endif
 for(unsigned kind=0;kind<4;++kind)for(unsigned place=0;place<4;++place){Reset();if(kind==0)manager.missing=true;else manager.data.id=kind==1?1085:kind==2?972:1145;auto loc=p.GetClosestGraveYard(float(place),4,-7);if(kind==1)assert(loc&&loc->x==sWorldSafeLocsStore.data.Loc.X);else if(kind==2)assert(loc&&loc->x==sWorldSafeLocsStore.early.Loc.X);else assert(loc&&loc->x==place&&loc->y==4&&loc->z==-7);++cases;}
 for(unsigned step:{0u,3u,7u}){Reset();manager.data.id=940;manager.data.step=step;auto loc=p.GetClosestGraveYard(std::numeric_limits<float>::quiet_NaN(),99999,-1000);
#ifdef BLOOD_GRAVEYARD_BASELINE
  assert(loc&&std::isnan(loc->x)&&loc->z==-1000);
#else
  VerifyBlood(loc,step);
#endif
  ++cases;
 }
#ifdef BLOOD_GRAVEYARD_BASELINE
 assert(cases==115);std::cout<<"COUNTERFACTUAL pinned9d0d0d1bb Blood source115 ASAN:eleven stages+completed echo corpse/nonfinite positions;retained Holy/MM/other/no-scenario,not safe recovery\n";
#else
 assert(cases==156);std::cout<<"PASS extracted Blood graveyard156 ASAN:940/all11stages+completed,201678 before3/5440 from3/5441 from7,41 invalid-source refusals with diagnostics,radians only for DB2 sources,repeat/nonfinite death pose controls,retained Holy/MM/other/no-scenario;controlled services,no progression/direct resurrection\n";
#endif
 return 0;
}
