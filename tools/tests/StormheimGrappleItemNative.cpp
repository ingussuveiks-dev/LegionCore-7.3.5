#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    if (!items.Load(argv[1], 0)) return 2;
    for (uint32 id : {138111u})
    {
        auto item = items.LookupEntry(id);
        if (!item) return 3;
        std::cout << id << ": MaxCount=" << item->MaxCount << ", Stackable=" << item->Stackable
            << ", Bonding=" << unsigned(item->Bonding) << ", LimitCategory=" << item->LimitCategory << ", Name=" << item->Display->Str[0] << '\n';
        if (item->Bonding != 1) return 4;
    }
    std::cout << "PASS: production sparse DB2 loader confirms native grapple launcher item138111.\n";
}
