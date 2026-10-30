#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>
using uint32=uint32_t;
struct Unit {
 float multiplier=1;
 float GetHealthMultiplierForTarget(Unit*)const{return multiplier;}
 uint32 Scale(Unit* victim,uint32 damage){
#include "Scaling.inc"
  return damage;
 }
};
int main(){
 Unit attacker,victim;
 std::vector<uint32> raw{0,1,31,127,16777215,16777216,16777217,1073741823,2147483647,2147483648u,4294966784u,4294967040u,4294967167u,4294967294u,4294967295u};
 uint32 cases=0;
#ifdef BASELINE
 uint32 overflowCounterfactuals=0;
#endif
 for(float scale:{0.0f,0.000001f,0.015625f,0.1f,0.25f,0.7f,1.0f,2.0f,16.0f,1000000.0f}){
  victim.multiplier=scale;
#ifndef BASELINE
  uint32 previous=0;
#endif
  for(uint32 value:raw){
   if(scale==0&&!value)continue; // Actual positive-health native multiplier contract; do not infer 0/0 behavior.
   float floating=float(value)/scale;
   bool overflow=double(floating)>double(UINT32_MAX);
   uint32 actual=attacker.Scale(&victim,value);
#ifdef BASELINE
   if(overflow&&actual!=UINT32_MAX)++overflowCounterfactuals;
   if(!overflow)assert(actual==uint32(floating));
#else
   if(overflow)assert(actual==UINT32_MAX);
   else assert(actual==uint32(floating)); // Preserve defined ordinary float-rounding/truncation behavior.
   assert(actual>=previous);previous=actual;
   if(value)assert(actual||double(floating)<1.0);
#endif
   ++cases;
  }
 }
 victim.multiplier=1;
#ifdef BASELINE
 assert(attacker.Scale(&victim,UINT32_MAX)!=UINT32_MAX&&overflowCounterfactuals);
 std::cout<<"PASS "<<cases<<" pinned a148a34b5 numeric counterfactuals; "<<overflowCounterfactuals<<" out-of-range native float conversions lose saturated damage. Expected-broken baseline, not ordinary success\n";
#else
 assert(attacker.Scale(&victim,UINT32_MAX)==UINT32_MAX);
 for(uint32 value:raw){
  uint32 previous=UINT32_MAX;
  for(float scale:{0.015625f,0.25f,1.0f,16.0f}){victim.multiplier=scale;uint32 actual=attacker.Scale(&victim,value);assert(actual<=previous);previous=actual;++cases;}
 }
 std::cout<<"PASS "<<cases<<" actual numeric-prefix cases: saturation before cast at2^32/large normalized hits, native in-range float behavior, monotonic raw damage/scaling. Positive native multipliers plus nonzero/zero boundary; invalid negative/NaN assets not certified\n";
#endif
}
