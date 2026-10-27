#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <string>
#include <cmath>
using int32=int32_t;
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;
static uint32 roll, upper, calls;
uint32 urand(uint32 lo,uint32 hi){assert(lo==0&&roll<=hi);upper=hi;++calls;return roll;}
struct BattlePetDataStoreMgr{uint16 GetRandomBreedID(std::set<uint32>);uint8 GetWeightForBreed(uint16);uint16 Baseline(std::set<uint32>);};
#include "Weight.inc"
#include "Selection.inc"
#define GetRandomBreedID Baseline
#include "Baseline.inc"
#undef GetRandomBreedID
enum { BATTLEPET_STATE_Stat_Power=18,BATTLEPET_STATE_Stat_Stamina=19,BATTLEPET_STATE_Stat_Speed=20,BATTLEPET_STATE_Stat_Gender=78 };
struct State{uint32 BreedID,SpeciesID,BattlePetStateID,Value;};
std::vector<State*>sBattlePetBreedStateStore,sBattlePetSpeciesStateStore;
struct Quality{float StateMultiplier;};
struct Qualities{std::map<uint32,Quality>values;Quality*LookupEntry(uint32 id){auto i=values.find(id);return i==values.end()?nullptr:&i->second;}}sBattlePetBreedQualityStore;
struct BattlePet{uint32 Breed=8,Species=1532,Level=2,Quality=3,XP=17;int32 InfoPower=0,InfoMaxHealth=0,InfoSpeed=0,InfoGender=0,Health=0;bool needSave=false;void UpdateStats();};
#include "Stats.inc"
int main(int argc,char**argv){
 BattlePetDataStoreMgr mgr;
 roll=20;assert(mgr.Baseline({8})==0&&upper==20);
 std::cout<<"COUNTERFACTUAL old inclusive upper endpoint returns invalid breed0 for valid singleton8.\n";
 uint32 checked=0;
 for(uint32 mask=1;mask<1024;++mask){
  std::set<uint32> breeds;uint32 sum=0;
  for(uint32 i=0;i<10;++i)if(mask&(1u<<i)){breeds.insert(i+3);sum+=mgr.GetWeightForBreed(uint16(i+3));}
  std::map<uint32,uint32> counts;
  for(roll=0;roll<sum;++roll){auto b=mgr.GetRandomBreedID(breeds);assert(breeds.count(b)&&upper==sum-1);++counts[b];++checked;}
  for(auto b:breeds)assert(counts[b]==mgr.GetWeightForBreed(uint16(b)));
 }
 for(auto breeds:{std::set<uint32>{},std::set<uint32>{0,1,2,107}}){auto before=calls;assert(mgr.GetRandomBreedID(breeds)==0&&calls==before);}
 for(roll=0;roll<20;++roll)assert(mgr.GetRandomBreedID({0,8,107})==8);
 std::cout<<"PASS "<<checked<<" exhaustive weighted draws across all1023 valid breed subsets; exact probabilities, endpoints, singleton, mixed/empty/zero-weight input.\n";
 assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string type;uint32 a,b,c;float multiplier;
 std::vector<State> breedStates,speciesStates;
 while(input>>type){if(type=="BREED"){input>>a>>b>>c;breedStates.push_back({a,0,b,c});}else if(type=="SPECIES"){input>>a>>b>>c;speciesStates.push_back({0,a,b,c});}else{assert(type=="QUALITY");input>>a>>b>>multiplier;sBattlePetBreedQualityStore.values[a]={multiplier};}}
 for(auto& s:breedStates)sBattlePetBreedStateStore.push_back(&s);
 for(auto& s:speciesStates)sBattlePetSpeciesStateStore.push_back(&s);
 assert(breedStates.size()>20&&speciesStates.size()>2);
 BattlePet bad;bad.Breed=0;bad.UpdateStats();assert(bad.Health==159&&bad.InfoPower==15&&bad.InfoSpeed==11);
 BattlePet fixed;fixed.UpdateStats();assert(fixed.Health==197&&fixed.InfoPower==25&&fixed.InfoSpeed==21&&fixed.InfoGender==1&&fixed.XP==17&&fixed.Level==2&&fixed.Quality==3);
 fixed.Level=3;fixed.UpdateStats();assert(fixed.Health==246&&fixed.InfoPower==38&&fixed.InfoSpeed==32);
 fixed.Level=25;fixed.UpdateStats();assert(fixed.Health==1319&&fixed.InfoPower==321&&fixed.InfoSpeed==272);
 for(uint32 level=1;level<=25;++level){fixed.Level=level;fixed.UpdateStats();auto hp=fixed.Health;fixed.UpdateStats();assert(fixed.Health==hp&&fixed.XP==17&&fixed.Quality==3);}
 std::cout<<"PASS actual UpdateStats and native26972 DB2/hotfix: broken breed0 level2=159/15/11, P/S8 level2=197/25/21, level3=246/38/32, level25 HP1319; all25 levels/repeat/XP/quality retained. Existing integer/float truncation preserved.\n";
}
