#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<AreaTableEntry> areas("AreaTable.db2", AreaTableLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !texts.Load(argv[1], 0) || !areas.Load(argv[1], 0))
        return 2;
    auto dalaran = areas.LookupEntry(7502);
    if (!dalaran)
        return 3;
    std::cout << "AREA\t" << dalaran->ID << '\t' << dalaran->ContinentID << '\t' << dalaran->AreaName->Str[0] << '\n';
    std::set<uint32> selected{197651};
    for (uint32 id = 205540; id <= 205600; ++id)
        selected.insert(id);
    for (uint32 id = 205290; id <= 205320; ++id)
        selected.insert(id);
    for (auto spell : spells)
    {
        std::string name = spell->Name->Str[0];
        for (std::string const& token : {"Dorro", "Talonclaw", "Spear in", "Highmountain", "Dalaran", "Survival Artifact", "Eagle Spirit"})
            if (name.find(token) != std::string::npos)
                selected.insert(spell->ID);
    }
    for (auto effect : effects)
        if ((effect->Effect == 90 && effect->EffectMiscValue[0] == 103798) ||
            (effect->Effect == 28 && (effect->EffectMiscValue[0] == 100005 || effect->EffectMiscValue[0] == 103798 ||
                effect->EffectMiscValue[0] == 101155 || effect->EffectMiscValue[0] == 101484)))
            selected.insert(effect->SpellID);
    for (size_t old = 0; old != selected.size();)
    {
        old = selected.size();
        for (auto effect : effects)
            if (!effect->DifficultyID && selected.count(effect->SpellID) && effect->EffectTriggerSpell)
                selected.insert(effect->EffectTriggerSpell);
    }
    for (uint32 id : selected)
        if (auto spell = spells.LookupEntry(id))
            std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\t' << spell->Description->Str[0] << '\n';
    for (auto effect : effects)
        if (!effect->DifficultyID && selected.count(effect->SpellID))
            std::cout << "EFFECT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t'
                << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\t'
                << effect->EffectMiscValue[1] << '\n';
    for (auto text : texts)
    {
        std::string line = text->Text->Str[0];
        if (text->ID == 103407 || (text->ID > 90000 && text->ID < 130000 &&
            (line.find("return to Dalaran") != std::string::npos || line.find("Dorro") != std::string::npos)))
            std::cout << "TEXT\t" << text->ID << '\t' << line << '\t' << text->SoundEntriesID[0] << '\n';
    }
}
