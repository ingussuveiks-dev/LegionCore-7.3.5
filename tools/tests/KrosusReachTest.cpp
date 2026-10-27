#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include "KrosusReach.inc"
using uint32=uint32_t;
enum {UNIT_FIELD_COMBAT_REACH, REACT_PASSIVE};
struct Display {uint32 ModelID=8578; float CreatureModelScale=1;};
struct Model {float GeoBox[6]={-19.0081f,-34.4843f,-42.4556f,25.5332f,34.4019f,42.832f}; float ModelScale=1;};
template<class T> struct Store {T value; bool missing=false; T* LookupEntry(uint32){return missing?nullptr:&value;}};
Store<Display> sCreatureDisplayInfoStore;
Store<Model> sCreatureModelDataStore;
struct Unit {
 float x=0,y=0,z=0,reach=2,scale=1; uint32 map=1460,entry=90544,display=71701,phase=1;
 uint32 GetMapId()const{return map;} uint32 GetEntry()const{return entry;} uint32 GetDisplayId()const{return display;}
 float GetObjectScale()const{return scale;} float GetCombatReach()const{return reach;}
 float GetMeleeReach()const{return std::max(2.0f,reach);}
 float GetPositionX()const{return x;} float GetPositionY()const{return y;} float GetPositionZ()const{return z;}
 float GetPositionZMinusOffset()const{return z;}
 bool IsInMap(Unit const* u)const{return map==u->map;} bool InSamePhase(Unit const* u)const{return phase==u->phase;}
 bool IsWithinMeleeRange(Unit const*,float)const; bool IsWithinCombatRange(Unit const*,float)const;
 void SetFloatValue(int,float f){reach=f;} void SetAnimKitId(int){} void setFaction(int){}
};
#include "NativeRange.inc"
struct EventMap {void Reset(){}};
struct Base {virtual void Reset(){}};
struct AI:Base {Unit* me; bool intro=false,below_75=true,below_50=true,below_25=true; EventMap events;
 explicit AI(Unit* u):me(u){}
#include "Reset.inc"
};
int main(){
 Unit boss,player; boss.reach=30; boss.x=1494.573f;boss.y=1780.335f;boss.z=36.86848f;
 player.x=1445.5f;player.y=1778.5f;player.z=34.5f;
 assert(!player.IsWithinMeleeRange(&boss,5)); // pinned ordinary old sphere counterfactual
 AI ai(&boss);ai.Reset();assert(boss.reach>42.90f&&boss.reach<42.92f);
 assert(player.IsWithinMeleeRange(&boss,5));assert(player.IsWithinCombatRange(&boss,5));
 float reach=boss.reach;ai.Reset();assert(boss.reach==reach);
 boss.x=1455.05f;boss.y=1748.94f;assert(player.IsWithinMeleeRange(&boss,5));
 boss.x=1494.573f;boss.y=1780.335f;
 player.x=1400;assert(!player.IsWithinMeleeRange(&boss,5));
 player.x=1445.5f; player.z=100;assert(!player.IsWithinMeleeRange(&boss,5));
 player.z=34.5f;player.phase=2;assert(!player.IsWithinMeleeRange(&boss,5));
 player.phase=1;player.map=1;assert(!player.IsWithinMeleeRange(&boss,5));
 assert(!player.IsWithinMeleeRange(nullptr,5));
 for(int variant=0;variant<5;++variant){
  Unit other;other.reach=30;
  if(variant==0)other.map=1;if(variant==1)other.entry=101002;if(variant==2)other.display=68323;
  sCreatureDisplayInfoStore.missing=variant==3;sCreatureModelDataStore.missing=variant==4;
  AI(&other).Reset();assert(other.reach==30);
 }
 sCreatureDisplayInfoStore.missing=false;sCreatureModelDataStore.missing=false;
 auto box=sCreatureModelDataStore.value.GeoBox;
 assert(BrokenShoreKrosus::CombatReach(60,box,1)==60);
 assert(BrokenShoreKrosus::CombatReach(30,nullptr,1)==30);
 assert(BrokenShoreKrosus::CombatReach(30,box,-1)==30);
 assert(BrokenShoreKrosus::CombatReach(30,box,std::numeric_limits<float>::infinity())==30);
 box[0]=std::numeric_limits<float>::quiet_NaN();assert(BrokenShoreKrosus::CombatReach(30,box,1)==30);
 std::cout<<"PASS production Reset/reach and native melee/combat range: old shore rejection, both positions, repeat, actor scope, missing assets, distant/vertical/map/phase rejection\n";
}
