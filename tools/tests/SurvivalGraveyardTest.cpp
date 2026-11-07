// Same actual callback; controlled services independently check both sides of
// the native harpoon boundary and both Hunter cemetery source identities.
#define main FuryGraveyardSourceUnusedMain
#include "WarswordsGraveyardTest.cpp"
#undef main
int main(){Probe p;unsigned cases=0;
 for(unsigned step=0;step<13;++step)for(unsigned repeat=0;repeat<2;++repeat){
  manager=Manager{};manager.data.id=973;manager.data.step=step;sWorldSafeLocsStore=Store{};
  auto& data=sWorldSafeLocsStore.data;data.Loc={3369.16f,1920.99f,0.628784f,320.087f};if(step>=7)data.Loc={3593.84f,1803.5f,2.06259f,308.119f};auto before=errors;
  auto loc=p.GetClosestGraveYard(0,0,0);assert(loc==&p.graveyard&&loc->map==1511&&loc->x==data.Loc.X&&loc->y==data.Loc.Y&&loc->z==data.Loc.Z&&std::fabs(loc->o-data.Loc.O*float(M_PI)/180)<0.001f&&sWorldSafeLocsStore.lastId==(step>=7?5296u:5295u)&&errors==before);++cases;
 }
 for(unsigned step:{6u,7u})for(unsigned kind=0;kind<16;++kind){
  manager=Manager{};manager.data.id=973;manager.data.step=step;sWorldSafeLocsStore=Store{};auto& s=sWorldSafeLocsStore.data;auto before=errors;
  if(kind==0)manager.missing=true;if(kind==1)manager.data.id=999;if(kind==2)sWorldSafeLocsStore.missing=true;if(kind==3)s.MapID=1539;if(kind==4)s.Loc.X=s.Loc.Y=s.Loc.Z=0;
  if(kind>=5&&kind<11){float* value=kind<8?&s.Loc.X:&s.Loc.Y;unsigned v=(kind-5)%3;*value=v==0?std::numeric_limits<float>::quiet_NaN():v==1?std::numeric_limits<float>::infinity():2000000.0f;}
  if(kind>=11&&kind<15){float* value=kind<13?&s.Loc.Z:&s.Loc.O;*value=(kind-11)%2?std::numeric_limits<float>::infinity():std::numeric_limits<float>::quiet_NaN();}
  if(kind==15){manager.data.id=999;sWorldSafeLocsStore.missing=true;}
  assert(!p.GetClosestGraveYard(0,0,0)&&errors==before+(kind>=2&&kind!=15));if(kind>=2&&kind!=15)assert(sWorldSafeLocsStore.lastId==(step==6?5295u:5296u));++cases;
 }
 assert(cases==58);std::cout<<"PASS extracted Survival graveyard58 ASAN:973 stages0-12,pre/post harpoon5295/5296,stable pose/radians,both missing/invalid source guards and diagnostics,no stale pointer result;services controlled"<<std::endl;return 0;
}
