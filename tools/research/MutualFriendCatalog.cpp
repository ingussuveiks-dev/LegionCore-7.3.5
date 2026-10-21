#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<AreaTableEntry> areas("AreaTable.db2", AreaTableLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!areas.Load(argv[1], 0) || !effects.Load(argv[1], 0)) return 2;
    auto city = areas.LookupEntry(7581);
    auto sewers = areas.LookupEntry(8392);
    if (!city || city->ContinentID != 1220 || city->ParentAreaID != 7502 ||
        !sewers || sewers->ParentAreaID == 7502) return 3;
    unsigned summons = 0, markers = 0;
    for (auto row : effects)
        if (row->SpellID == 224338)
        {
            // Native SpellEffect 28 summons the messenger; aura 4 is its marker.
            if (row->Effect == 28 && row->EffectMiscValue[0] == 112693) ++summons;
            if (row->Effect == 6 && row->EffectAura == 4) ++markers;
        }
    if (summons != 1 || markers != 1) return 4;
    std::cout << "PASS: native DB2 routes 224338 to messenger 112693 and its marker in "
        << city->AreaName->Str[0] << "; " << sewers->AreaName->Str[0] << " is a separate area\n";
}
