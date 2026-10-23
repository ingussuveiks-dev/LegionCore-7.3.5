#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2FileLoader.h"
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <set>

struct SceneFile : DB2FileSource
{
    std::string name;
    FILE* file;
    explicit SceneFile(std::string path) : name(std::move(path)), file(std::fopen(name.c_str(), "rb")) { }
    ~SceneFile() override { if (file) std::fclose(file); }
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
    DB2Storage<ScenarioEntry> scenarios("Scenario.db2", ScenarioLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2", QuestLineXQuestLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2FileLoader texts;
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    if (!scenarios.Load(argv[1], 0) || !steps.Load(argv[1], 0) || !lines.Load(argv[1], 0) || !effects.Load(argv[1], 0))
        return 2;
    if (!trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0))
        return 2;
    if (!scripts.Load(argv[1], 0))
        return 3;
    SceneFile textFile(std::string(argv[1]) + "SceneScriptText.db2");
    if (!texts.Load(&textFile, SceneScriptTextLoadInfo::Instance()))
        return 3;
    if (!members.Load(argv[1], 0))
        return 3;
    for (auto scenario : scenarios)
        if (scenario->ID == 972 || scenario->ID == 973 || scenario->ID == 1099 || scenario->ID == 1136)
        {
            std::cout << "SCENARIO\t" << scenario->ID << '\t' << scenario->Name->Str[0] << '\n';
            for (auto step : steps)
                if (step->ScenarioID == scenario->ID)
                {
                    std::cout << "STEP\t" << scenario->ID << '\t' << unsigned(step->OrderIndex) << '\t'
                        << step->ID << '\t' << step->Criteriatreeid << '\t' << step->Title->Str[0] << '\t'
                        << step->Description->Str[0] << '\n';
                    if (scenario->ID == 973 || scenario->ID == 972)
                    {
                        std::set<uint32> pendingTrees{uint32(step->Criteriatreeid)}, seenTrees;
                        while (!pendingTrees.empty())
                        {
                            uint32 id = *pendingTrees.begin();
                            pendingTrees.erase(id);
                            if (!seenTrees.insert(id).second)
                                continue;
                            auto tree = trees.LookupEntry(id);
                            if (!tree)
                                return 6;
                            if (id == 46289)
                                std::cout << "DAKARR_ROOT_FLAGS\t" << tree->Flags << '\n';
                            if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                            {
                                std::cout << (scenario->ID == 973 ? "CRITERION\t" : "MARKSMANSHIP_CRITERION\t") << unsigned(step->OrderIndex) << '\t' << id << '\t'
                                    << criterion->ID << '\t' << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t' << tree->Amount << '\n';
                                if (criterion->ID == 29263)
                                    std::cout << "DAKARR_RULE\t" << criterion->ModifierTreeId << '\t' << unsigned(criterion->StartEvent) << '\t'
                                        << unsigned(criterion->FailEvent) << '\t' << criterion->EligibilityWorldStateId << '\t'
                                        << unsigned(criterion->Flags) << '\t' << tree->Flags << '\n';
                            }
                            for (auto child : trees)
                                if (uint32(child->Parent) == id)
                                    pendingTrees.insert(child->ID);
                        }
                    }
                }
        }
    std::set<uint32> survivalLines;
    for (auto line : lines)
        if (line->QuestID == 41542 || line->QuestID == 39427 || line->QuestID == 40385 || line->QuestID == 41540)
            survivalLines.insert(line->QuestLineID);
    for (auto line : lines)
        if (survivalLines.count(line->QuestLineID))
            std::cout << "QUEST\t" << line->QuestLineID << '\t' << unsigned(line->OrderIndex) << '\t' << line->QuestID << '\n';
    for (auto effect : effects)
        if (effect->SpellID == 205589 || effect->SpellID == 197651 || effect->SpellID == 208601 || effect->SpellID == 200279 || effect->SpellID == 200262)
            std::cout << "EFFECT\t" << effect->SpellID << '\t' << effect->EffectIndex << '\t'
                << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t'
                << effect->EffectTriggerSpell << '\t' << effect->EffectItemType << '\n';
    std::set<uint32> pendingPackages{1583, 1535}, seenPackages, pendingScripts, seenScripts;
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
        uint32 id = *pendingScripts.begin();
        pendingScripts.erase(id);
        if (!seenScripts.insert(id).second)
            continue;
        auto script = scripts.LookupEntry(id);
        if (!script)
            return 4;
        bool found = false;
        for (uint32 recordIndex = 0; recordIndex < texts.GetRecordCount(); ++recordIndex)
        {
            auto record = texts.GetRecord(recordIndex);
            if (record && record.GetId() == id)
            {
                std::cout << "SCENE_TEXT\t" << id << '\t' << record.GetString("Name") << '\n'
                    << record.GetString("Script") << '\n';
                found = true;
                break;
            }
        }
        if (!found)
            return 5;
        if (script->NextSceneScriptID)
            pendingScripts.insert(script->NextSceneScriptID);
    }
}
