#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
// ItemSparse uses sparse records. Read them with the production DB2 loader.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    if(!items.Load(argv[1],0))return 2;
    auto item=items.LookupEntry(122746);if(!item)return 3;
    std::cout<<"Head 122746: MaxCount="<<item->MaxCount<<", Stackable="<<item->Stackable<<", Bonding="<<unsigned(item->Bonding)<<", LimitCategory="<<item->LimitCategory<<".\n";
    if(item->Stackable!=1 || item->Bonding!=4 || item->LimitCategory!=0)return 4;
    std::cout<<"PASS: production sparse DB2 reader confirms the quest trophy item.\n";
}
