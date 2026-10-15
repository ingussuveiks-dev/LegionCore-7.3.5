#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    if(!items.Load(argv[1],0)||!locks.Load(argv[1],0)||!objects.Load(argv[1],0)||!actors.Load(argv[1],0))return 2;
    auto item=items.LookupEntry(124512);auto lock=locks.LookupEntry(1690);auto iounn=actors.LookupEntry(69637);
    if(!item||!lock||!iounn)return 3;
    std::cout<<"item124512 name="<<item->Display->Str[0]<<" max="<<item->MaxCount<<" stack="<<item->Stackable<<" bonding="<<unsigned(item->Bonding)<<" races="<<item->AllowableRace<<" flags="<<item->Flags[0]<<","<<item->Flags[1]<<","<<item->Flags[2]<<'\n';
    if((item->MaxCount&&item->MaxCount<8)||item->Stackable<1||item->Bonding!=4)return 4;
    for(unsigned i=0;i<8;++i)if(lock->Type[i]!=(i==1?2:0)||lock->Index[i]!=(i==1?5:0)||lock->Skill[i])return 5;
    std::cout<<"lock1690 ordinary Open/type5, no skill\n";
    for(uint32 id:{20764u,17934u,18974u,25607u,18988u}){auto row=objects.LookupEntry(id);if(!row||!row->FileDataID)return 6;std::cout<<"display"<<id<<" file="<<row->FileDataID<<'\n';}
    std::cout<<"Iounn69637 model="<<iounn->ModelID<<'\n';
    std::cout<<"PASS: native sparse Ashildir bones item permits eight, lock1690 and all five bone displays/Iounn exist; no rendering certification.\n";
}
