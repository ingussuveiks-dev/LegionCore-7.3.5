#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2", LockLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> displays("GameObjectDisplayInfo.db2", GameobjectDisplayInfoLoadInfo::Instance());
    if (!items.Load(argv[1], 0) || !locks.Load(argv[1], 0) || !displays.Load(argv[1], 0)) return 2;
    auto item = items.LookupEntry(124504);
    auto lock = locks.LookupEntry(1690);
    auto display = displays.LookupEntry(28223);
    if (!item || !lock || !display) return 3;
    std::cout << "item124504 name=" << item->Display->Str[0] << " max=" << item->MaxCount
        << " stack=" << item->Stackable << " bonding=" << unsigned(item->Bonding)
        << " flags=" << item->Flags[0] << ',' << item->Flags[1] << ',' << item->Flags[2] << '\n';
    for (unsigned i = 0; i < 8; ++i)
        std::cout << "lock1690 slot=" << i << " type=" << unsigned(lock->Type[i])
            << " index=" << lock->Index[i] << " skill=" << lock->Skill[i]
            << " action=" << unsigned(lock->Action[i]) << '\n';
    std::cout << "display28223 file=" << display->FileDataID << '\n';
    if (item->MaxCount != 0 || item->Stackable != 10 || item->Bonding != 4) return 4;
    if (display->FileDataID != 1237525) return 5;
    for (unsigned i = 0; i < 8; ++i)
        if (lock->Type[i] != (i == 1 ? 2 : 0) || lock->Index[i] != (i == 1 ? 5 : 0)
            || lock->Skill[i] != 0) return 6;
    std::cout << "PASS: native sparse Soulcap permits eight items; Lock1690 is ordinary Open/type5, no profession; actual Soulthorn display exists.\n";
    return 0;
}
