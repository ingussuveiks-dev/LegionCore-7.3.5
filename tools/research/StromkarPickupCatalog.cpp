#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2FileLoader.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <cstdio>
#include <filesystem>
struct StromkarSceneFile : DB2FileSource
{
    std::string name;
    FILE* file;
    explicit StromkarSceneFile(std::string path) : name(std::move(path)), file(std::fopen(name.c_str(), "rb")) { }
    ~StromkarSceneFile() override { if (file) std::fclose(file); }
    bool IsOpen() const override { return file != nullptr; }
    bool Read(void* buffer, std::size_t size) override { return std::fread(buffer, size, 1, file) == 1; }
    std::size_t GetPosition() const override { return std::ftell(file); }
    std::size_t GetFileSize() const override { return std::filesystem::file_size(name); }
    char const* GetFileName() const override { return name.c_str(); }
};
int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error))
        return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements())
        return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool())
        return 4;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2", ItemChildEquipmentLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !items.Load(argv[1], 0) || !children.Load(argv[1], 0) ||
        !steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0))
        return 5;
    spells.LoadFromDB(); effects.LoadFromDB(); items.LoadFromDB(); children.LoadFromDB();
    steps.LoadFromDB(); trees.LoadFromDB(); criteria.LoadFromDB();
    std::set<uint32> selected{206436, 206439, 206448, 225891, 201904, 207454, 206455, 206456, 204394, 202014, 63048};
    for (uint32 identity : selected)
    {
        auto spell = spells.LookupEntry(identity);
        if (!spell)
            return 6;
        std::cout << "SPELL\t" << identity << '\t' << spell->Name->Str[0] << '\n';
    }
    for (auto effect : effects)
        if (!effect->DifficultyID && selected.count(effect->SpellID))
            std::cout << "EFFECT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t'
                << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
    auto item = items.LookupEntry(128910);
    if (!item)
        return 7;
    std::cout << "ITEM\t" << item->ID << '\t' << item->MaxCount << '\t' << item->Stackable << '\t' << item->AllowableClass << '\t' << unsigned(item->ArtifactID) << '\n';
    for (auto child : children)
        if (child->ItemID == 128910)
            std::cout << "CHILD\t" << child->ItemID << '\t' << child->ChildItemID << '\t' << unsigned(child->ChildItemEquipSlot) << '\n';
    for (auto step : steps)
        if (step->ScenarioID == 1037)
        {
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t' << step->Title->Str[0] << '\t' << step->Description->Str[0] << '\n';
            std::set<uint32> pending{uint32(step->Criteriatreeid)}, seen;
            while (!pending.empty())
            {
                uint32 identity = *pending.begin();
                pending.erase(identity);
                if (!seen.insert(identity).second)
                    continue;
                auto tree = trees.LookupEntry(identity);
                if (!tree)
                    return 8;
                if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                    std::cout << "CRITERION\t" << unsigned(step->OrderIndex) << '\t' << criterion->ID << '\t' << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t' << tree->Amount << '\n';
                for (auto descendant : trees)
                    if (uint32(descendant->Parent) == identity)
                        pending.insert(descendant->ID);
            }
        }
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    DB2FileLoader texts;
    StromkarSceneFile textFile(std::string(argv[1]) + "SceneScriptText.db2");
    if (!scripts.Load(argv[1], 0) || !members.Load(argv[1], 0) || !texts.Load(&textFile, SceneScriptTextLoadInfo::Instance()))
        return 9;
    scripts.LoadFromDB(); members.LoadFromDB();
    DB2Storage<OverrideSpellDataEntry> overrides("OverrideSpellData.db2", OverrideSpellDataLoadInfo::Instance());
    if (!overrides.Load(argv[1], 0))
        return 12;
    overrides.LoadFromDB();
    auto execution = overrides.LookupEntry(684);
    if (!execution)
        return 13;
    for (uint32 slot = 0; slot < 10; ++slot)
        if (execution->Spells[slot])
            std::cout << "OVERRIDE\t684\t" << slot << '\t' << execution->Spells[slot] << '\n';
    for (auto entry : overrides)
        for (int32 spell : entry->Spells)
            if (spell == 206455)
                std::cout << "EXECUTE_OVERRIDE\t" << entry->ID << '\n';
    for (auto effect : effects)
        if (effect->EffectTriggerSpell == 206455 || (effect->Effect == 6 && effect->EffectAura == 293 && effect->EffectMiscValue[0] == 684))
            std::cout << "EXECUTE_REFERENCE\t" << effect->SpellID << '\t' << effect->EffectIndex << '\n';
    for (auto effect : effects)
        if (effect->EffectTriggerSpell == 206456)
            std::cout << "ARRIVAL_REFERENCE\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->DifficultyID << '\n';
    for (auto effect : effects)
        if (effect->EffectMiscValue[0] == 104307 || effect->EffectMiscValue[0] == 103151)
        {
            auto spell = spells.LookupEntry(effect->SpellID);
            std::cout << "ACTOR_REFERENCE\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                << effect->EffectMiscValue[0] << '\t' << effect->EffectMiscValue[1] << '\t' << effect->EffectTriggerSpell << '\t'
                << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\t' << (spell ? spell->Name->Str[0] : "") << '\n';
        }
    DB2Storage<SpellMiscEntry> miscellaneous("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2", SpellRangeLoadInfo::Instance());
    if (!miscellaneous.Load(argv[1], 0) || !ranges.Load(argv[1], 0))
        return 14;
    miscellaneous.LoadFromDB(); ranges.LoadFromDB();
    for (auto entry : miscellaneous)
        if (!entry->DifficultyID && entry->SpellID == 206455)
        {
            auto range = ranges.LookupEntry(entry->RangeIndex);
            if (!range)
                return 15;
            std::cout << "EXECUTE_RANGE\t" << range->RangeMin[0] << '\t' << range->RangeMax[0] << '\t' << range->RangeMin[1] << '\t' << range->RangeMax[1] << '\n';
        }
    std::set<uint32> pendingPackages{1590}, seenPackages, pendingScripts, seenScripts;
    while (!pendingPackages.empty())
    {
        uint32 package = *pendingPackages.begin();
        pendingPackages.erase(package);
        if (!seenPackages.insert(package).second)
            continue;
        for (auto member : members)
            if (member->SceneScriptPackageID == package)
            {
                if (member->ChildSceneScriptPackageID)
                    pendingPackages.insert(member->ChildSceneScriptPackageID);
                if (member->SceneScriptID)
                    pendingScripts.insert(member->SceneScriptID);
            }
    }
    while (!pendingScripts.empty())
    {
        uint32 identity = *pendingScripts.begin();
        pendingScripts.erase(identity);
        if (!seenScripts.insert(identity).second)
            continue;
        auto script = scripts.LookupEntry(identity);
        if (!script)
            return 10;
        bool found = false;
        for (uint32 recordIndex = 0; recordIndex < texts.GetRecordCount(); ++recordIndex)
        {
            auto record = texts.GetRecord(recordIndex);
            if (record && record.GetId() == identity)
            {
                std::cout << "SCENE_TEXT\t" << identity << '\t' << record.GetString("Name") << '\n' << record.GetString("Script") << '\n';
                found = true;
                break;
            }
        }
        if (!found)
            return 11;
        if (script->NextSceneScriptID)
            pendingScripts.insert(script->NextSceneScriptID);
    }
    HotfixDatabase.Close();
}
