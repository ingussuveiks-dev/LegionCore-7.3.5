#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<OverrideSpellDataEntry> overrides("OverrideSpellData.db2",OverrideSpellDataLoadInfo::Instance());
    if(!effects.Load(argv[1],0)||!overrides.Load(argv[1],0))return 2;
    for(auto e:effects)if(e->SpellID==198425)
        std::cout<<"effect "<<e->SpellID<<' '<<e->EffectIndex<<' '<<e->EffectAura<<' '<<e->EffectMiscValue[0]<<'\n';
    auto row=overrides.LookupEntry(641);if(!row)return 3;
    std::cout<<"override "<<row->ID;
    for(auto spell:row->Spells)if(spell)std::cout<<' '<<spell;
    std::cout<<'\n';
}
