#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using DamageEffectType=uint32;
#include "Constants.inc"
static void Require(bool b,char const*m){if(!b)throw std::runtime_error(m);}
struct Unit;using Creature=Unit;
struct Unit{uint64 hp=100,relativeHp=10;std::set<uint32>auras;uint64 GetHealth(Unit*u=nullptr){return u?relativeHp:hp;}void SetHealth(uint64 n){hp=n;relativeHp=std::max<uint64>(1,n/10);}bool HasAura(uint32 n){return auras.count(n)>0;}};
struct Base{Creature*me;uint32 casts=0;explicit Base(Creature*c):me(c){}bool HealthBelowPct(uint32 n){return me->hp<n;}void DoCast(Creature*u,uint32 n,bool=false){++casts;u->auras.insert(n);}};
#include "Selected.inc"
template<class F>static void Hit(F&f,Unit*p,uint32&d){
#ifdef COUNTERFACTUAL
 d=std::max<uint32>(1,d/10);f.DamageTaken(p,d,0);d*=10;
#else
 f.DamageTakenAfterScaling(p,d,0);
#endif
}
int main(){try{Unit me,p;uint32 handlers=0;Each(&me,[&](auto&f,uint32){++handlers;me.SetHealth(100);me.auras.clear();uint32 d=10;Hit(f,&p,d);bool fixed=false;
#if defined(CASE_FELFIREFISSION)
 fixed=d==10&&f.casts==1;
#elif defined(CASE_KALECGOSBANISH)
 fixed=!d&&me.hp==1;
#else
 fixed=!d;
#endif
#ifdef COUNTERFACTUAL
 Require(!fixed,"each pinned raw guard must expose its normalized-lethal defect");
#else
 Require(fixed,"each repaired production body handles final lethal damage");
#if defined(CASE_FELFIREFISSION)
 me.SetHealth(100);uint32 n=f.casts;d=1;Hit(f,&p,d);Require(d==1&&f.casts==n,"nonlethal cannot cause fission");
#else
 me.SetHealth(100);d=100;Hit(f,&p,d);Require(!d,"one-hit/oversized damage cannot bypass guard");
 me.SetHealth(100);d=1;Hit(f,&p,d);Require(d==1,"ordinary nonlethal damage retained");
 me.SetHealth(1);d=1;Hit(f,&p,d);Require(!d,"one health remains protected");
 me.SetHealth(100);d=0;f.DamageTakenAfterScaling(&p,d,0);Require(!d,"zero damage stays zero");
 me.hp=uint64(UINT32_MAX)+1;me.relativeHp=me.hp;d=UINT32_MAX;Hit(f,&p,d);
#if defined(CASE_AYSASURVIVAL)
 Require(d==1,"Aysa keeps native one-damage training cap");
#else
 Require(d==UINT32_MAX,"64bit health cannot be truncated into lethal");
#endif
#if defined(CASE_KALECGOSBANISH)
 me.SetHealth(100);d=100;Hit(f,&me,d);Require(d==100,"native scripted self-kill exemption retained");
#endif
#endif
#endif
 });
#ifdef COUNTERFACTUAL
 std::cout<<"PASS expected-broken controls for "<<handlers<<" pinned actor handlers\n";
#else
 std::cout<<"PASS repaired ASAN production bodies for "<<handlers<<" actor handlers;not full client encounter traversal\n";
#endif
 return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
