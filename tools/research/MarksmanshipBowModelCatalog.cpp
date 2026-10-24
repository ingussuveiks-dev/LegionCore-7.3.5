#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    DB2Storage<GameObjectDisplayInfoEntry> displays("GameObjectDisplayInfo.db2", GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2", LockLoadInfo::Instance());
    if (!displays.Load(argv[1], 0) || !locks.Load(argv[1], 0))
        return 2;
    auto display = displays.LookupEntry(9806);
    auto lock = locks.LookupEntry(1691);
    if (!display || !display->FileDataID || !lock)
        return 3;
    std::cout << "BOW_MODEL\t" << display->ID << '\t' << display->FileDataID << '\t' << display->ObjectEffectPackageID << '\n';
    for (unsigned index = 0; index < 8; ++index)
        if (lock->Type[index])
            std::cout << "LOCK\t" << lock->ID << '\t' << unsigned(lock->Type[index]) << '\t'
                << lock->Index[index] << '\t' << lock->Skill[index] << '\t' << unsigned(lock->Action[index]) << '\n';
}
