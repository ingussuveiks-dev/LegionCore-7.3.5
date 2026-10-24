#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    char const* config = std::getenv("LEGION_DIALOGUE_CONFIG");
    std::string error;
    if (!config || !sConfigMgr->LoadInitial(config, {}, error))
        return 6;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements())
        return 7;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool())
        return 8;
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2", ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<ModifierTreeEntry> modifiers("ModifierTree.db2", ModifierTreeLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2", SoundKitEntryLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    if (!lines.Load(argv[1], 0) || !texts.Load(argv[1], 0) || !criteria.Load(argv[1], 0) ||
        !trees.Load(argv[1], 0) || !modifiers.Load(argv[1], 0) || !sounds.Load(argv[1], 0) ||
        !spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !misc.Load(argv[1], 0) || !durations.Load(argv[1], 0))
        return 2;
    lines.LoadFromDB();
    texts.LoadFromDB();
    criteria.LoadFromDB();
    trees.LoadFromDB();
    modifiers.LoadFromDB();
    sounds.LoadFromDB();
    spells.LoadFromDB();
    effects.LoadFromDB();
    misc.LoadFromDB();
    durations.LoadFromDB();
    std::set<uint32> selectedSpells{211690,211694,211731,211738,211820,211823,211824,211826,211827,
        211753,208447,211848,211851,211934};
    for (auto effect : effects)
        if (effect->Effect == 219 && (effect->EffectMiscValue[0] == 2167 || effect->EffectMiscValue[0] == 2173))
            selectedSpells.insert(effect->SpellID);
    for (size_t previous = 0; previous != selectedSpells.size();)
    {
        previous = selectedSpells.size();
        for (auto effect : effects)
            if (!effect->DifficultyID && selectedSpells.count(effect->SpellID) && effect->EffectTriggerSpell)
                selectedSpells.insert(effect->EffectTriggerSpell);
    }
    for (uint32 identity : selectedSpells)
    {
        auto spell = spells.LookupEntry(identity);
        if (!spell)
            return 9;
        std::cout << "SPELL\t" << identity << '\t' << spell->Name->Str[0] << '\n';
    }
    for (auto effect : effects)
        if (!effect->DifficultyID && selectedSpells.count(effect->SpellID))
            std::cout << "EFFECT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t'
                << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectMiscValue[0]
                << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t'
                << effect->ImplicitTarget[1] << '\n';
    for (auto entry : misc)
        if (!entry->DifficultyID && selectedSpells.count(entry->SpellID))
        {
            auto duration = durations.LookupEntry(entry->DurationIndex);
            std::cout << "DURATION\t" << entry->SpellID << '\t' << (duration ? duration->Duration : 0) << '\n';
        }
    std::set<uint32> soundKits;
    for (uint32 identity : {4555u, 4556u, 4557u, 4598u, 4599u, 4600u, 4601u, 4826u,
        4615u, 4619u, 4616u, 4620u, 4621u})
    {
        auto line = lines.LookupEntry(identity);
        if (!line)
            return 3;
        std::cout << "LINE\t" << identity << '\t' << line->BroadcastTextID << '\t'
            << line->AdditionalDuration << '\t' << line->NextConversationLineID << '\t'
            << line->SpellVisualKitID << '\t' << line->AnimKitID << '\n';
        auto text = texts.LookupEntry(line->BroadcastTextID);
        if (!text)
            return 4;
        std::cout << "TEXT\t" << text->ID << '\t' << text->SoundEntriesID[0] << '\t'
            << text->SoundEntriesID[1] << '\t' << text->Text->Str[0] << '\t' << text->Text1->Str[0] << '\n';
        for (uint32 kit : text->SoundEntriesID)
            if (kit)
                soundKits.insert(kit);
    }
    std::set<int32> selectedModifiers;
    for (uint32 identity : {4565u, 4566u, 4567u, 4568u})
    {
        auto line = lines.LookupEntry(identity);
        if (!line)
            return 10;
        auto text = texts.LookupEntry(line->BroadcastTextID);
        if (!text)
            return 11;
        std::cout << "COMBAT_LINE\t" << identity << '\t' << line->BroadcastTextID << '\t'
            << line->AdditionalDuration << '\t' << line->NextConversationLineID << '\n';
        std::cout << "COMBAT_TEXT\t" << text->ID << '\t' << text->SoundEntriesID[0] << '\t'
            << text->SoundEntriesID[1] << '\t' << text->Text->Str[0] << '\t' << text->Text1->Str[0] << '\n';
    }
    for (uint32 identity : {48019u, 48020u, 48094u, 48021u, 48022u})
    {
        auto tree = trees.LookupEntry(identity);
        if (!tree)
            return 5;
        std::cout << "TREE\t" << identity << '\t' << tree->Amount << '\t' << int(tree->Operator)
            << '\t' << tree->Flags << '\t' << tree->Parent << '\t' << tree->CriteriaID << '\n';
        if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
        {
            std::cout << "CRITERION\t" << criterion->ID << '\t' << int(criterion->Type) << '\t'
                << criterion->Asset << '\t' << criterion->ModifierTreeId << '\t'
                << int(criterion->StartEvent) << '\t' << criterion->StartAsset << '\t'
                << criterion->StartTimer << '\t' << int(criterion->Flags) << '\n';
            if (criterion->ModifierTreeId)
                selectedModifiers.insert(criterion->ModifierTreeId);
        }
    }
    for (size_t previous = 0; previous != selectedModifiers.size();)
    {
        previous = selectedModifiers.size();
        for (auto modifier : modifiers)
            if (selectedModifiers.count(modifier->Parent))
                selectedModifiers.insert(modifier->ID);
    }
    for (auto modifier : modifiers)
        if (selectedModifiers.count(modifier->ID))
            std::cout << "MODIFIER\t" << modifier->ID << '\t' << modifier->Parent << '\t'
                << int(modifier->Type) << '\t' << modifier->Asset << '\t' << modifier->SecondaryAsset
                << '\t' << int(modifier->Operator) << '\t' << int(modifier->Amount) << '\n';
    for (auto sound : sounds)
        if (soundKits.count(sound->SoundKitID))
            std::cout << "AUDIO_FILE\t" << sound->SoundKitID << '\t' << sound->FileDataID << '\t'
                << uint32(sound->Frequency) << '\t' << sound->Volume << '\n';
    HotfixDatabase.Close();
}
