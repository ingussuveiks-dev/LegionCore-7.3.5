#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 1;
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> effects("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 if(!items.Load(argv[1],0)||!effects.Load(argv[1],0))return 2;
 for(auto id:{147853,147854}){
  auto i=items.LookupEntry(id);if(!i)return 3;
  std::cout<<"BOOK "<<id<<" name "<<i->Display->Str[0]<<" description "<<i->Description->Str[0]<<" level "<<int(i->RequiredLevel)<<" flags "<<i->Flags[0]<<','<<i->Flags[1]<<'\n';
  int n=0;for(auto e:effects)if(e->ItemID==id){++n;std::cout<<" EFFECT "<<e->SpellID<<'\n';}std::cout<<" EFFECT_COUNT "<<n<<'\n';
 }
}
