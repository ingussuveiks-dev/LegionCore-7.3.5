#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    if (!items.Load(argv[1], 0)) return 2;
    for (uint32 id : {128493u,128495u,128496u,128497u})
    {
        auto item = items.LookupEntry(id);
        if (!item) return 3;
        std::cout << id << ": MaxCount=" << item->MaxCount << ", Stackable=" << item->Stackable
            << ", Bonding=" << unsigned(item->Bonding) << ", LimitCategory=" << item->LimitCategory << '\n';
        if (item->Stackable != 1 || item->Bonding != 4 || item->LimitCategory != 0) return 4;
    }
    std::cout << "PASS: production sparse DB2 loader confirms all four watcher fragment items.\n";
}
