#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "DB2FileLoader.h"
#include <cstdio>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <set>

struct DruidSceneFile : DB2FileSource
{
    std::string path;
    FILE* file;
    explicit DruidSceneFile(std::string name) : path(std::move(name)), file(std::fopen(path.c_str(), "rb")) { }
    ~DruidSceneFile() override { if (file) std::fclose(file); }
    bool IsOpen() const override { return file != nullptr; }
    bool Read(void* data, std::size_t size) override { return std::fread(data, size, 1, file) == 1; }
    std::size_t GetPosition() const override { return std::ftell(file); }
    std::size_t GetFileSize() const override { return std::filesystem::file_size(path); }
    char const* GetFileName() const override { return path.c_str(); }
};

// Matching client data and read-only hotfixes; no character/world writes.
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2", ItemChildEquipmentLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !items.Load(argv[1], 0) || !children.Load(argv[1], 0) ||
        !steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0)) return 4;
    spells.LoadFromDB(); effects.LoadFromDB(); items.LoadFromDB(); children.LoadFromDB();
    steps.LoadFromDB(); trees.LoadFromDB(); criteria.LoadFromDB();
    std::set<uint32> selected{189916,201012,201010,225863};
    for (auto id : selected)
    {
        auto spell = spells.LookupEntry(id);
        if (!spell) return 5;
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\n';
    }
    for (auto effect : effects)
        if (selected.count(effect->SpellID))
        {
            std::cout << "EFFECT\t" << effect->SpellID << '\t' << effect->DifficultyID << '\t' << effect->EffectIndex << '\t'
                << effect->Effect << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t'
                << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
            if (effect->Effect == 6)
                std::cout << "AURA\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t'
                    << effect->EffectAura << '\t' << effect->EffectAuraPeriod << '\n';
        }
    for (uint32 id : {127829u})
    {
        auto item = items.LookupEntry(id);
        if (!item) return 6;
        std::cout << "ITEM\t" << id << '\t' << item->MaxCount << '\t' << item->Stackable << '\t' << item->AllowableClass << '\n';
    }
    for (auto child : children)
        if (child->ItemID == 127829)
            std::cout << "CHILD\t" << child->ItemID << '\t' << child->ChildItemID << '\t' << unsigned(child->ChildItemEquipSlot) << '\n';
    for (auto step : steps)
        if (step->ScenarioID == 900)
        {
            std::cout << "STEP\t" << step->ScenarioID << '\t' << unsigned(step->OrderIndex) << '\t' << step->Title->Str[0] << '\t' << step->RewardQuestID << '\n';
            std::set<uint32> pending{uint32(step->Criteriatreeid)}, seen;
            while (!pending.empty())
            {
                uint32 id = *pending.begin(); pending.erase(pending.begin());
                if (!seen.insert(id).second) continue;
                auto tree = trees.LookupEntry(id);
                if (!tree) return 7;
                if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                    std::cout << "CRITERION\t" << step->ScenarioID << '\t' << unsigned(step->OrderIndex) << '\t'
                        << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t' << tree->Amount << '\n';
                for (auto child : trees) if (uint32(child->Parent) == id) pending.insert(child->ID);
            }
        }
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    DB2FileLoader texts;
    DruidSceneFile file(std::string(argv[1]) + "SceneScriptText.db2");
    if (!scripts.Load(argv[1], 0) || !members.Load(argv[1], 0) || !texts.Load(&file, SceneScriptTextLoadInfo::Instance())) return 8;
    scripts.LoadFromDB(); members.LoadFromDB();
    for (uint32 root : {1462u})
    {
        std::cout << "SCENE_PACKAGE\t" << root << '\n';
        std::set<uint32> packages{root}, seenPackages, pending, seen;
        while (!packages.empty())
        {
            uint32 id = *packages.begin(); packages.erase(packages.begin());
            if (!seenPackages.insert(id).second) continue;
            for (auto member : members)
                if (member->SceneScriptPackageID == id)
                {
                    if (member->ChildSceneScriptPackageID) packages.insert(member->ChildSceneScriptPackageID);
                    if (member->SceneScriptID) pending.insert(member->SceneScriptID);
                }
        }
        while (!pending.empty())
        {
            uint32 id = *pending.begin(); pending.erase(pending.begin());
            if (!seen.insert(id).second) continue;
            auto script = scripts.LookupEntry(id);
            if (!script) return 9;
            bool found = false;
            for (uint32 index = 0; index < texts.GetRecordCount(); ++index)
            {
                auto row = texts.GetRecord(index);
                if (row && row.GetId() == id)
                {
                    std::cout << "SCENE_TEXT\t" << root << '\t' << id << '\t' << row.GetString("Name") << '\n'
                        << row.GetString("Script") << '\n';
                    found = true;
                    break;
                }
            }
            if (!found) return 10;
            if (script->NextSceneScriptID) pending.insert(script->NextSceneScriptID);
        }
    }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
