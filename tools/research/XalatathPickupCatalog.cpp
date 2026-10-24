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
struct XalatathSceneFile : DB2FileSource
{
    std::string name;
    FILE* file;
    explicit XalatathSceneFile(std::string path) : name(std::move(path)), file(std::fopen(name.c_str(), "rb")) { }
    ~XalatathSceneFile() override { if (file) std::fclose(file); }
    bool IsOpen() const override { return file != nullptr; }
    bool Read(void* buffer, std::size_t size) override { return std::fread(buffer, size, 1, file) == 1; }
    std::size_t GetPosition() const override { return std::ftell(file); }
    std::size_t GetFileSize() const override { return std::filesystem::file_size(name); }
    char const* GetFileName() const override { return name.c_str(); }
};
int main(int argc,char** argv){
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!check||!check->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0))return 5;spells.LoadFromDB();effects.LoadFromDB();
    std::set<uint32> selected{201778,201780,204919,201917,205774,201791,201899};
    for(auto identity:selected){auto spell=spells.LookupEntry(identity);if(!spell)return 6;std::cout<<"SPELL\t"<<identity<<'\t'<<spell->Name->Str[0]<<'\n';}
    for(auto effect:effects)if(!effect->DifficultyID&&selected.count(effect->SpellID))
        std::cout<<"EFFECT\t"<<effect->SpellID<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\n';
    for (auto effect : effects)
        if (!effect->DifficultyID && (effect->SpellID == 201791 || effect->SpellID == 201899))
            std::cout << "TARGET\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
    DB2Storage<SpellMiscEntry> miscellaneous("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> castTimes("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    if (!miscellaneous.Load(argv[1], 0) || !castTimes.Load(argv[1], 0))
        return 14;
    miscellaneous.LoadFromDB(); castTimes.LoadFromDB();
    for (auto entry : miscellaneous)
        if (!entry->DifficultyID && entry->SpellID == 201899)
        {
            auto castTime = castTimes.LookupEntry(entry->CastingTimeIndex);
            if (!castTime)
                return 15;
            std::cout << "CAST\t" << entry->SpellID << '\t' << castTime->Base << '\t' << entry->Speed << '\t' << entry->LaunchDelay << '\n';
        }
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2", ItemChildEquipmentLoadInfo::Instance());
    if (!items.Load(argv[1], 0) || !children.Load(argv[1], 0))
        return 12;
    items.LoadFromDB(); children.LoadFromDB();
    for (uint32 identity : {128827, 133958})
    {
        auto item = items.LookupEntry(identity);
        if (!item)
            return 13;
        std::cout << "ITEM\t" << identity << '\t' << item->MaxCount << '\t' << item->Stackable << '\t' << item->AllowableClass << '\t' << unsigned(item->ArtifactID) << '\n';
    }
    for (auto child : children)
        if (child->ItemID == 128827)
            std::cout << "CHILD\t" << child->ItemID << '\t' << child->ChildItemID << '\t' << unsigned(child->ChildItemEquipSlot) << '\n';
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    DB2FileLoader texts;
    XalatathSceneFile textFile(std::string(argv[1]) + "SceneScriptText.db2");
    if (!scripts.Load(argv[1], 0) || !members.Load(argv[1], 0) || !texts.Load(&textFile, SceneScriptTextLoadInfo::Instance()))
        return 7;
    scripts.LoadFromDB(); members.LoadFromDB();
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    if (!steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0))
        return 10;
    steps.LoadFromDB(); trees.LoadFromDB(); criteria.LoadFromDB();
    for (auto step : steps)
        if (step->ScenarioID == 991)
        {
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t' << step->Title->Str[0] << '\t' << step->Description->Str[0] << '\n';
            std::set<uint32> pendingTrees{uint32(step->Criteriatreeid)}, seenTrees;
            while (!pendingTrees.empty())
            {
                uint32 identity = *pendingTrees.begin();
                pendingTrees.erase(identity);
                if (!seenTrees.insert(identity).second)
                    continue;
                auto tree = trees.LookupEntry(identity);
                if (!tree)
                    return 11;
                if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                    std::cout << "CRITERION\t" << unsigned(step->OrderIndex) << '\t' << criterion->ID << '\t' << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t' << tree->Amount << '\n';
                for (auto child : trees)
                    if (uint32(child->Parent) == identity)
                        pendingTrees.insert(child->ID);
            }
        }
    std::set<uint32> pendingPackages{1553}, seenPackages, pendingScripts, seenScripts;
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
            return 8;
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
            return 9;
        if (script->NextSceneScriptID)
            pendingScripts.insert(script->NextSceneScriptID);
    }
    HotfixDatabase.Close();
}
