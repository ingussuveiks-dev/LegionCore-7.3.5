#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "SharedDefines.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include "DB2FileLoader.h"
#include <cstdio>
#include <filesystem>
struct SilverHandSceneFile : DB2FileSource
{
    std::string name;
    FILE* file;
    explicit SilverHandSceneFile(std::string path) : name(std::move(path)), file(std::fopen(name.c_str(), "rb")) { }
    ~SilverHandSceneFile() override { if (file) std::fclose(file); }
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
    DB2Storage<MapEntry> maps("Map.db2", MapLoadInfo::Instance());
    if (!maps.Load(argv[1], 0))
        return 17;
    maps.LoadFromDB();
    auto acquisitionMap = maps.LookupEntry(1539);
    if (!acquisitionMap || !acquisitionMap->IsDungeon())
        return 18;
    std::cout << "MAP_DUNGEON\t" << acquisitionMap->ID << '\t' << acquisitionMap->IsDungeon() << '\n';
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    if (!steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0))
        return 5;
    steps.LoadFromDB();
    trees.LoadFromDB();
    criteria.LoadFromDB();
    for (auto step : steps)
        if (step->ScenarioID == 1092)
        {
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t'
                << step->Criteriatreeid << '\t' << step->IsBonusObjective() << '\t' << step->Title->Str[0] << '\n';
            std::cout << "STEP_REWARD\t" << unsigned(step->OrderIndex) << '\t' << step->RewardQuestID << '\n';
            std::set<uint32> pending{uint32(step->Criteriatreeid)}, seen;
            while (!pending.empty())
            {
                uint32 identity = *pending.begin();
                pending.erase(identity);
                if (!seen.insert(identity).second)
                    continue;
                auto tree = trees.LookupEntry(identity);
                if (!tree)
                    return 6;
                if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                    std::cout << "CRITERION\t" << unsigned(step->OrderIndex) << '\t' << criterion->ID << '\t'
                        << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t' << tree->Amount << '\n';
                for (auto child : trees)
                    if (uint32(child->Parent) == identity)
                        pending.insert(child->ID);
            }
        }
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!effects.Load(argv[1], 0))
        return 7;
    effects.LoadFromDB();
    for (auto effect : effects)
        if (!effect->DifficultyID && (effect->SpellID == 210985 || effect->SpellID == 211838 ||
            effect->SpellID == 211842 || effect->SpellID == 211862 || effect->SpellID == 211867 || effect->SpellID == 225877))
            std::cout << "EFFECT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t'
                << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
    DB2Storage<SpellMiscEntry> miscellaneous("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    if (!miscellaneous.Load(argv[1], 0))
        return 12;
    miscellaneous.LoadFromDB();
    bool pickupMiscFound = false;
    for (auto entry : miscellaneous)
        if (entry->SpellID == 211838 && !entry->DifficultyID)
        {
            std::cout << "PICKUP_MISC\t" << entry->SpellID << '\t' << entry->CastingTimeIndex << '\t'
                << entry->DurationIndex << '\t' << entry->Attributes[1] << '\n';
            if (entry->Attributes[1] & (SPELL_ATTR1_CHANNELED_1 | SPELL_ATTR1_CHANNELED_2))
                return 13;
            pickupMiscFound = true;
        }
    if (!pickupMiscFound)
        return 14;
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> castTimes("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    if (!durations.Load(argv[1], 0) || !castTimes.Load(argv[1], 0))
        return 15;
    durations.LoadFromDB();
    castTimes.LoadFromDB();
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    if (!spells.Load(argv[1], 0))
        return 19;
    spells.LoadFromDB();
    for (auto entry : miscellaneous)
        if ((entry->SpellID == 211862 || entry->SpellID == 211867) && !entry->DifficultyID)
        {
            auto spell = spells.LookupEntry(entry->SpellID);
            auto duration = durations.LookupEntry(entry->DurationIndex);
            auto castTime = castTimes.LookupEntry(entry->CastingTimeIndex);
            std::cout << "COLLAPSE_SPELL\t" << entry->SpellID << '\t' << (spell ? spell->Name->Str[0] : "missing") << '\t'
                << entry->CastingTimeIndex << '\t' << (castTime ? castTime->Base : 0) << '\t'
                << entry->DurationIndex << '\t' << (duration ? duration->Duration : 0) << '\t' << entry->Attributes[1] << '\n';
        }
    auto pickupDuration = durations.LookupEntry(105);
    auto pickupCastTime = castTimes.LookupEntry(1);
    if (!pickupDuration || !pickupCastTime)
        return 16;
    std::cout << "PICKUP_DURATION\t" << pickupDuration->ID << '\t' << pickupDuration->Duration << '\t'
        << pickupDuration->MaxDuration << '\t' << pickupDuration->DurationPerLevel << '\n';
    std::cout << "PICKUP_CAST_TIME\t" << pickupCastTime->ID << '\t' << pickupCastTime->Base << '\t'
        << pickupCastTime->Minimum << '\t' << pickupCastTime->PerLevel << '\n';
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2", ItemChildEquipmentLoadInfo::Instance());
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    DB2FileLoader texts;
    SilverHandSceneFile sceneFile(std::string(argv[1]) + "SceneScriptText.db2");
    if (!items.Load(argv[1], 0) || !children.Load(argv[1], 0) || !scripts.Load(argv[1], 0) ||
        !members.Load(argv[1], 0) || !texts.Load(&sceneFile, SceneScriptTextLoadInfo::Instance()))
        return 8;
    items.LoadFromDB(); children.LoadFromDB(); scripts.LoadFromDB(); members.LoadFromDB();
    auto item = items.LookupEntry(128823);
    if (!item)
        return 9;
    std::cout << "ITEM\t" << item->ID << '\t' << unsigned(item->ArtifactID) << '\t'
        << unsigned(item->InventoryType) << '\t' << item->Display->Str[0] << '\n';
    for (auto child : children)
        if (child->ItemID == 128823)
            std::cout << "CHILD\t" << child->ItemID << '\t' << child->ChildItemID << '\n';
    std::set<uint32> packages{1618, 1619}, seenPackages, pending, seen;
    while (!packages.empty())
    {
        uint32 package = *packages.begin();
        packages.erase(package);
        if (!seenPackages.insert(package).second)
            continue;
        for (auto member : members)
            if (member->SceneScriptPackageID == package)
            {
                std::cout << "MEMBER\t" << package << '\t' << member->SceneScriptID << '\t' << member->ChildSceneScriptPackageID << '\n';
                if (member->SceneScriptID)
                    pending.insert(member->SceneScriptID);
                if (member->ChildSceneScriptPackageID)
                    packages.insert(member->ChildSceneScriptPackageID);
            }
    }
    while (!pending.empty())
    {
        uint32 identity = *pending.begin();
        pending.erase(identity);
        if (!seen.insert(identity).second)
            continue;
        auto script = scripts.LookupEntry(identity);
        if (!script)
            return 10;
        if (auto replacement = HotfixDatabase.PQuery("SELECT Name, Script FROM scene_script_text WHERE ID = %u", identity))
        {
            auto fields = replacement->Fetch();
            std::cout << "SCENE_ORIGIN\t" << identity << "\thotfix\nSCENE_TEXT\t" << identity << '\t'
                << fields[0].GetString() << '\n' << fields[1].GetString() << '\n';
        }
        else
        {
            bool found = false;
            for (uint32 index = 0; index < texts.GetRecordCount(); ++index)
            {
                auto record = texts.GetRecord(index);
                if (record && record.GetId() == identity)
                {
                    std::cout << "SCENE_ORIGIN\t" << identity << "\tdb2\nSCENE_TEXT\t" << identity << '\t'
                        << record.GetString("Name") << '\n' << record.GetString("Script") << '\n';
                    found = true;
                    break;
                }
            }
            if (!found)
                return 11;
        }
        if (script->NextSceneScriptID)
            pending.insert(script->NextSceneScriptID);
    }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
