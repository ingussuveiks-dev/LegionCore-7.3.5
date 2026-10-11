#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>

// Use the production WDC1 reader: ItemSparse uses the sparse-record layout.
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    if (!items.Load(argv[1], 0)) return 2;
    auto item = items.LookupEntry(128380);
    if (!item || item->MaxCount != 30 || item->Stackable != 30 || item->LimitCategory != 0) return 3;
    std::cout << "PASS: production DB2 reader confirms raw Hag Feather MaxCount=30, Stackable=30, LimitCategory=0.\n";
    auto focus = items.LookupEntry(128343);
    if (!focus || focus->MaxCount != 0 || focus->Stackable != 1 || focus->LimitCategory != 0) return 4;
    std::cout << "PASS: production DB2 reader confirms raw Stonedark Focus MaxCount=0, Stackable=1, LimitCategory=0.\n";
}
