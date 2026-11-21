// Read matching 7.3.5 catalog records through the production DB2 reader.
#include "DB2LoadInfo.h"
#include "DB2Store.h"
#include "DB2Structure.h"

#include <iostream>
#include <set>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;

    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ItemEffectEntry> itemEffects("ItemEffect.db2", ItemEffectLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2", LockLoadInfo::Instance());
    if (!items.Load(argv[1], 0) || !itemEffects.Load(argv[1], 0) ||
        !effects.Load(argv[1], 0) || !locks.Load(argv[1], 0))
        return 2;

    std::set<uint32> const selectedItems{
        780, 5339, 5411, 7627, 22473, 22693, 23614, 23654, 24081, 24337, 33009, 58362
    };
    std::set<uint32> const selectedSpells{
        4975, 27907, 28247, 29866, 31614, 42222, 71725, 80199, 80208
    };
    for (uint32 id : selectedItems)
    {
        auto item = items.LookupEntry(id);
        if (!item)
            return 3;
        std::cout << "ITEM\t" << id << '\t' << item->Display->Str[0] << '\t'
                  << item->StartQuestID << '\t' << int(item->RequiredLevel) << '\n';
    }
    for (auto row : items)
        if (row->StartQuestID)
            std::cout << "START\t" << row->ID << '\t' << row->StartQuestID << '\n';
    for (auto row : itemEffects)
        if (selectedItems.count(row->ItemID))
            std::cout << "USE\t" << row->ID << '\t' << row->ItemID << '\t' << row->SpellID << '\t'
                      << unsigned(row->TriggerType) << '\t' << row->Charges << '\n';
    for (auto row : effects)
        if (selectedSpells.count(row->SpellID))
            std::cout << "EFFECT\t" << row->ID << '\t' << row->SpellID << '\t' << row->Effect << '\t'
                      << row->EffectIndex << '\t' << row->DifficultyID << '\t' << row->EffectItemType << '\t'
                      << row->EffectTriggerSpell << '\t' << row->EffectMiscValue[0] << '\t'
                      << row->EffectMiscValue[1] << '\t' << row->ImplicitTarget[0] << '\t'
                      << row->ImplicitTarget[1] << '\t' << row->EffectAura << '\n';
    for (uint32 id : {259, 1673, 1691})
    {
        auto lock = locks.LookupEntry(id);
        if (!lock)
            return 4;
        for (unsigned slot = 0; slot < MAX_LOCK_CASE; ++slot)
            if (lock->Type[slot])
                std::cout << "LOCK\t" << id << '\t' << unsigned(lock->Type[slot]) << '\t'
                          << lock->Index[slot] << '\t' << lock->Skill[slot] << '\n';
    }
    return 0;
}
