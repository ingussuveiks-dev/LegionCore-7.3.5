#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include <string>

// Read-only evidence from the runtime's typed, packed 7.3.5 DB2 reader.
// A client record establishes spell/item data, not retail availability,
// a server-side loot source, a forge spawn, or observed client behavior.
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
#define STORE(Type, Name, File, Info) DB2Storage<Type> Name(File ".db2", Info::Instance()); if (!Name.Load(argv[1], 0)) return 2
    STORE(ItemSparseEntry, items, "ItemSparse", ItemSparseLoadInfo);
    STORE(ItemEffectEntry, uses, "ItemEffect", ItemEffectLoadInfo);
    STORE(SpellEntry, spells, "Spell", SpellLoadInfo);
    STORE(SpellEffectEntry, effects, "SpellEffect", SpellEffectLoadInfo);
    STORE(SpellMiscEntry, misc, "SpellMisc", SpellMiscLoadInfo);
    STORE(SpellCastTimesEntry, times, "SpellCastTimes", SpellCastTimesLoadInfo);
    STORE(SpellRangeEntry, ranges, "SpellRange", SpellRangeLoadInfo);
    STORE(SpellReagentsEntry, reagents, "SpellReagents", SpellReagentsLoadInfo);
    STORE(SpellCastingRequirementsEntry, requirements, "SpellCastingRequirements", SpellCastingRequirementsLoadInfo);
    STORE(GameObjectsEntry, objects, "GameObjects", GameobjectsLoadInfo);
#undef STORE
    auto item = items.LookupEntry(122612), key = items.LookupEntry(114054);
    if (!item || !key || item->Stackable != 20) return 3;
    std::cout << "item " << item->ID << " " << item->Display->Str[0] << " stack " << item->Stackable << '\n';
    std::cout << "key " << key->ID << " " << key->Display->Str[0] << '\n';
    unsigned useCount = 0, reagentCount = 0;
    for (auto use : uses)
        if (use->ItemID == 122612 || use->SpellID == 183149)
        {
            std::cout << "item-effect " << use->ID << " item " << use->ItemID << " spell " << use->SpellID << " trigger " << unsigned(use->TriggerType) << '\n';
            if (use->ItemID != 122612 || use->SpellID != 183149 || use->TriggerType) return 4;
            ++useCount;
        }
    for (auto reagent : reagents)
        for (unsigned i = 0; i < 8; ++i)
            if (reagent->Reagent[i] == 122612)
            {
                std::cout << "reagent spell " << reagent->SpellID << " item " << reagent->Reagent[i] << " count " << reagent->ReagentCount[i] << '\n';
                if (reagent->SpellID != 181858 || reagent->ReagentCount[i] != 1) return 5;
                ++reagentCount;
            }
    if (useCount != 1 || reagentCount != 1) return 6;
    std::set<uint32> ids{181858, 181859, 183149, 193965, 193971};
    unsigned contractEffects = 0, timedCast = 0;
    for (uint32 id : ids)
    {
        auto spell = spells.LookupEntry(id);
        if (!spell) return 7;
        std::cout << "spell " << id << " " << spell->Name->Str[0] << '\n';
        for (auto m : misc) if (m->SpellID == id)
        {
            auto time = times.LookupEntry(m->CastingTimeIndex);
            auto range = ranges.LookupEntry(m->RangeIndex);
            if (!time || !range) return 8;
            std::cout << "cast " << id << " difficulty " << unsigned(m->DifficultyID) << " time-index " << m->CastingTimeIndex
                << " milliseconds " << time->Base << " range-index " << m->RangeIndex << " max " << range->RangeMax[0] << " " << range->RangeMax[1] << '\n';
            if (id == 181858 && !m->DifficultyID)
            {
                if (time->Base != 1500 || range->RangeMax[0] != 20 || range->RangeMax[1] != 20) return 9;
                ++timedCast;
            }
        }
        for (auto r : requirements) if (r->SpellID == int32(id))
            std::cout << "requirement " << id << " focus " << r->RequiresSpellFocus << " area " << r->RequiredAreasID << '\n';
        for (auto e : effects) if (e->SpellID == id)
        {
            std::cout << "effect " << id << " " << e->EffectIndex << " type " << e->Effect << " aura " << e->EffectAura
                << " target " << e->ImplicitTarget[0] << " " << e->ImplicitTarget[1] << " misc " << e->EffectMiscValue[0]
                << " item " << e->EffectItemType << " trigger " << e->EffectTriggerSpell << '\n';
            if (id == 181858 && e->EffectIndex == 0 && e->Effect == 134 && e->EffectMiscValue[0] == 92240 && e->ImplicitTarget[0] == 1) ++contractEffects;
            if (id == 181859 && e->EffectIndex == 0 && e->Effect == 160 && e->EffectTriggerSpell == 181858) ++contractEffects;
            if (id == 183149 && e->EffectIndex == 0 && e->Effect == 33 && e->ImplicitTarget[0] == 23 && e->EffectMiscValue[0] == 10) ++contractEffects;
            if (id == 183149 && e->EffectIndex == 1 && e->Effect == 169 && e->EffectItemType == 114054 && e->ImplicitTarget[0] == 1) ++contractEffects;
        }
    }
    if (contractEffects != 4 || timedCast != 1) return 10;
    unsigned itemEffects = 0, forgeNames = 0;
    for (auto e : effects)
    {
        if (e->EffectItemType == 122612)
        {
            ++itemEffects;
            std::cout << "item-type-reference spell " << e->SpellID << " effect " << e->EffectIndex << " type " << e->Effect << '\n';
        }
        if (e->EffectTriggerSpell == 181858)
            std::cout << "credit-parent " << e->SpellID << " type " << e->Effect << '\n';
    }
    for (auto object : objects)
        if (std::string(object->Name->Str[0]) == "Smoldering Forge") ++forgeNames;
    std::cout << "native-item-type-references " << itemEffects << " native-exact-forge-names " << forgeNames << '\n';
    std::cout << "PASS: typed item, use, timed cast, reagent and credit links. Availability/spawn/loot/client remain separate evidence.\n";
}
