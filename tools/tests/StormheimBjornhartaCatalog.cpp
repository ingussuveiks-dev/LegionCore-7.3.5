#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>

int main(int argc,char** argv)
{
 if(argc!=2)return 1;
 DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
 DB2Storage<ItemEffectEntry> uses("ItemEffect.db2",ItemEffectLoadInfo::Instance());
 DB2Storage<QuestPackageItemEntry> rewards("QuestPackageItem.db2",QuestPackageItemLoadInfo::Instance());
 DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
 DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
 if(!items.Load(argv[1],0)||!uses.Load(argv[1],0)||!rewards.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0))return 2;
 auto item=items.LookupEntry(124503);if(!item)return 3;
 std::cout<<"item124503 "<<item->Display->Str[0]<<" max="<<item->MaxCount<<" stack="<<item->Stackable<<" bonding="<<unsigned(item->Bonding)<<" flags="<<item->Flags[0]<<","<<item->Flags[1]<<'\n';
 if(item->MaxCount||item->Stackable!=10||item->Bonding!=4||item->StartQuestID||item->Flags[0]!=67584||item->Flags[1]!=8192)return 3;
 for(auto u:uses)if(u->ItemID==124503)return 3;
 unsigned count=0;for(auto r:rewards)if(r->PackageID==650){if(!items.LookupEntry(r->ItemID)||!r->ItemQuantity)return 4;++count;std::cout<<"package650 item="<<r->ItemID<<" quantity="<<r->ItemQuantity<<" display="<<unsigned(r->DisplayType)<<'\n';}
 if(!count)return 4;
 for(auto id:{33920,65138}){auto d=displays.LookupEntry(id);if(!d)return 5;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"display"<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
 std::cout<<"PASS: native quest heart supports four items and has no use spell; reward package and bear/Vydhar model records exist.\n";
}
