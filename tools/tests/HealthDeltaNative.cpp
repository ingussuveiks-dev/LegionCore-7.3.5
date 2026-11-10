// Actual native ModifyHealth/GetHealthGain/DealDamage; no player saves.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "CombatPackets.h"
#include <boost/multiprecision/cpp_int.hpp>
struct HealthActor:Actor{
 using Actor::Actor;
#ifdef HEALTH_DELTA_BASELINE
#include "HealthBefore.inc"
#endif
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999764,DIFFICULTY_N_SCENARIO,&parent);
 HealthActor target(&map,299,23001,{50,803,805,0}),attacker(&map,299,23002,{51,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&target))&&map.AddToMap(static_cast<Creature*>(&attacker)),"Native health actors missing");
 uint64 const maximum=std::numeric_limits<uint64>::max();uint32 cases=0;
#ifdef HEALTH_DELTA_BASELINE
 target.SetMaxHealth(maximum);target.SetFullHealth();target.BeforeModifyHealth(-1,nullptr,0);Require(target.GetHealth()!=maximum-1,"Original signed current-health narrowing no longer reproduces");
 target.SetMaxHealth(1000);target.SetHealth(100);Require(target.BeforeGetHealthGain(INT64_MAX)!=900,"Original predicted signed-add overflow no longer reproduces");
 target.SetMaxHealth(1099511627793ull);target.SetFullHealth();attacker.BeforeNonlethalDamage(&target,UINT32_MAX);Require(target.GetHealth()!=1099511627793ull-UINT32_MAX,"Original nonlethal damage int32 narrowing no longer reproduces");
 std::cout<<"PASS expected-broken pinned full health methods and nonlethal callsite component with native Creature endpoints:UINT64_MAX minus1 loses HP,signed prediction overflow,and UINT32_MAX damage fails to subtract;ordinary full Unit damage tested in repaired mode\n";
#else
 for(uint64 limit:{1ull,1000ull,4294967296ull,1099511627793ull,9223372036854775807ull,9223372036854775808ull,18446744073709551615ull})for(uint64 health:{0ull,1ull,limit/2,limit-1,limit})for(int64 delta:{INT64_MIN,INT64_MIN+1,int64(-4294967295ll),int64(-1),int64(0),int64(1),int64(4294967295ll),INT64_MAX}){
  target.SetMaxHealth(limit);target.SetHealth(health);
  boost::multiprecision::int128_t exact=boost::multiprecision::int128_t(health)+delta;if(exact<0)exact=0;if(exact>limit)exact=limit;
  uint64 expected=uint64(exact);int64 gain=int64(exact-health);
  Require(target.GetHealthGain(delta)==gain&&target.GetHealth()==health,"Native health prediction changed state or returned wrong bounded delta");
  Require(target.ModifyHealth(delta)==gain&&target.GetHealth()==expected&&target.IsAlive(),"Native signed delta lost full-width current health,overflowed or changed death semantics");++cases;
 }
 Require(cases==280,"Expected280fullwidth native health deltas");
 for(auto sample:{std::pair<uint32,uint32>{1u,1u},{2147483647u,2147483648u},{2147483648u,2147483648u},{4294967295u,4294967295u}}){
  uint32 damage=sample.first,normalized=sample.second; // Existing native float normalization/upper clamp.
  target.SetMaxHealth(1099511627793ull);target.SetFullHealth();uint32 dealt=attacker.DealDamage(&target,damage,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
  Require(target.IsAlive()&&target.GetHealth()==1099511627793ull-normalized&&dealt==normalized,"Native nonlethal damage narrowed to signed32 or altered native normalized return amount");
 }
 for(auto sample:{std::pair<uint64,uint64>{1000,100},{1000,950},{maximum,maximum-1000}}){
  target.SetMaxHealth(sample.first);target.SetHealth(sample.second);uint32 expected=uint32(std::min<uint64>(100,sample.first-sample.second));
  Require(attacker.DealHeal(&target,100,nullptr)==expected&&target.GetHealth()==sample.second+expected,"Actual native heal/overheal lost unsigned current health or final gain");
 }
 std::cout<<"PASS actual native280ModifyHealth/GetHealthGain matrices:zero,ordinary,32/40/63/64bit current/max,INT64_MIN/MAX signed delta,exact prediction/application and original alive-zero semantics;four real normalized DealDamage boundaries throughUINT32_MAX;three native ordinary/overheal/highhealth DealHeal gains;no player/character writes\n";
#endif
 target.CombatStop(true);attacker.CombatStop(true);CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
