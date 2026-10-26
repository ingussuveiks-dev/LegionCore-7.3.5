#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellXSpellVisualEntry> links("SpellXSpellVisual.db2",SpellXSpellVisualLoadInfo::Instance());
    DB2Storage<SpellVisualEntry> visuals("SpellVisual.db2",SpellVisualLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!links.Load(argv[1],0)||!visuals.Load(argv[1],0))return 2;
    for(uint32 spellId:{190666u,190644u})
    {
        auto spell=spells.LookupEntry(spellId);
        if(!spell)return 3;
        std::cout<<"SPELL\t"<<spell->ID<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto effect:effects)if(effect->SpellID==spellId)
            std::cout<<"EFFECT\t"<<spellId<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
        for(auto link:links)if(link->SpellID==spellId)
            std::cout<<"VISUAL\t"<<spellId<<'\t'<<link->SpellVisualID<<'\t'<<bool(visuals.LookupEntry(link->SpellVisualID))<<'\n';
    }
}
