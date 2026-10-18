// Full production LootItem constructor, deterministic RNG and init services.
// This verifies bounds propagation, not the historical origin of those bounds.
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
using uint32=uint32_t;
constexpr int LOOT_ITEM_TYPE_CURRENCY=1,ITEM_QUALITY_POOR=0;
uint32 selected=0,expectedMin=0,expectedMax=0,rngCalls=0;
uint32 urand(uint32 min,uint32 max){assert(min==expectedMin&&max==expectedMax&&selected>=min&&selected<=max);++rngCalls;return selected;}
struct Loot{};
struct LootStoreItem{uint32 itemid=142487,currencyid=0,type=0,mincount=0,maxcount=0;bool needs_quest=true;std::vector<int>conditions{9,41,58,60};};
struct LootItem{
 struct{uint32 ItemID=0,CurrencyID=0;}item;
 uint32 type=0,count=0,quality=0;std::vector<int>conditions;
 bool currency=false,needs_quest=false,is_looted=true,is_blocked=true,is_underthreshold=true,is_counted=true,initialized=false;
 LootItem(LootStoreItem const&,Loot*);void init(Loot*){initialized=true;}
};
#include "NativeFragmentLootItem.inc"
int main(int argc,char**argv){
 assert(argc==2);std::ifstream file(argv[1]);uint32 entry,item,min,max,quest,rows=0;
 while(file>>entry>>item>>min>>max>>quest){
  assert(item==142487&&quest==1);
  if(entry==229113)assert(min==10&&max==15);else{assert(entry==114537&&min==100&&max==150);}++rows;
  LootStoreItem row;row.itemid=item;row.mincount=expectedMin=min;row.maxcount=expectedMax=max;
  for(selected=min;selected<=max;++selected){LootItem result(row,nullptr);assert(result.count==selected&&result.item.ItemID==item&&result.initialized&&result.needs_quest&&!result.currency&&result.conditions==row.conditions&&!result.is_looted&&!result.is_blocked);}
 }
 assert(rows==2&&rngCalls==57);
 std::cout<<"PASS: actual LootItem constructor calls RNG with Rockaway10..15 and Helya100..150 and preserves quest/condition fields for all57 selected values. RNG/init are fixtures; retail bounds are reconstructed.\n";
}
