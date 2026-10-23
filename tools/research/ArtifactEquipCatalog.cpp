#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    if (!effects.Load(argv[1], 0) || !items.Load(argv[1], 0))
        return 2;
    for (auto effect : effects)
        if (effect->Effect == 249)
        {
            auto item = items.LookupEntry(effect->EffectMiscValue[0]);
            if (!item)
                return 3;
            std::cout << effect->SpellID << '\t' << effect->EffectMiscValue[0] << '\t'
                << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\t'
                << unsigned(item->ArtifactID) << '\n';
        }
}
