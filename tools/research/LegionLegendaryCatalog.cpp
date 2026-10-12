// Read the sparse ItemSparse table with the same DB2 reader as worldserver.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <map>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ItemEffectEntry> effects("ItemEffect.db2", ItemEffectLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    if (!items.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !spells.Load(argv[1], 0))
        return 2;

    std::map<uint32, std::vector<int32>> itemSpells;
    for (auto effect : effects)
        itemSpells[effect->ItemID].push_back(effect->SpellID);
    std::cout << "ID\tName\tRequiredLevel\tLimitCategory\tInventoryType\tAllowableClass\tSpellIDs\tMissingSpellIDs\n";
    bool missing = false;
    for (auto item : items)
    {
        if (item->OverallQualityID != 5 || item->ExpansionID != 6 || !item->InventoryType ||
            (item->LimitCategory != 357 && item->LimitCategory != 362))
            continue;
        std::cout << item->ID << '\t' << item->Display->Str[0] << '\t' << int(item->RequiredLevel)
            << '\t' << item->LimitCategory << '\t' << unsigned(item->InventoryType) << '\t' << item->AllowableClass << '\t';
        char const* separator = "";
        for (auto spell : itemSpells[item->ID])
        {
            std::cout << separator << spell;
            separator = ",";
        }
        std::cout << '\t';
        separator = "";
        for (auto spell : itemSpells[item->ID])
            if (!spells.LookupEntry(spell))
            {
                std::cout << separator << spell;
                separator = ",";
                missing = true;
            }
        std::cout << '\n';
    }
    return missing ? 3 : 0;
}
