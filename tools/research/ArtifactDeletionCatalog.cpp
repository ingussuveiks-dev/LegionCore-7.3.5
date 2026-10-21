#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 1;
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 if(!items.Load(argv[1],0))return 2;
 unsigned total=0,flagged=0;
 for(auto i:items)if(i->ArtifactID){
  ++total;flagged+=bool(i->Flags[0]&0x20);
  if(!(i->Flags[0]&0x20))std::cout<<"UNFLAGGED "<<i->ID<<" ARTIFACT "<<unsigned(i->ArtifactID)<<" INVENTORY "<<unsigned(i->InventoryType)<<'\n';
 }
 if(total<36)return 3;
 std::cout<<"ARTIFACTS "<<total<<" NATIVE_NO_DESTROY "<<flagged<<'\n';
 for(auto id:{128941,128942,128943}){
  auto i=items.LookupEntry(id);if(!i||!i->ArtifactID||!(i->Flags[0]&0x20))return 4;
  std::cout<<"WARLOCK "<<id<<" ARTIFACT "<<unsigned(i->ArtifactID)<<" NO_DESTROY 1\n";
 }
}
