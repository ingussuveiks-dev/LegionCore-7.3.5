#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    if (!effects.Load(argv[1], 0) || !misc.Load(argv[1], 0) || !times.Load(argv[1], 0) || !items.Load(argv[1], 0))
        return 2;
    uint32 matches = 0;
    for (auto effect : effects)
        if (effect->SpellID == 200262)
        {
            ++matches;
            std::cout << "EFFECT\t" << effect->ID << '\t' << effect->DifficultyID << '\t' << effect->EffectIndex
                << '\t' << effect->Effect << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell
                << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
        }
    uint32 spellRows = 0;
    for (auto entry : misc)
        if (entry->SpellID == 200262)
        {
            ++spellRows;
            auto cast = times.LookupEntry(entry->CastingTimeIndex);
            if (!cast)
                return 3;
            std::cout << "MISC\t" << entry->ID << '\t' << uint32(entry->DifficultyID) << '\t' << entry->CastingTimeIndex
                << '\t' << cast->Base << '\t' << entry->Speed << '\t' << entry->LaunchDelay << '\n';
        }
    auto item = items.LookupEntry(128826);
    if (!item || matches != 2 || spellRows != 1)
        return 4;
    std::cout << "ITEM\t" << item->ID << '\t' << uint32(item->InventoryType) << '\t' << uint32(item->ArtifactID) << '\n';
}
