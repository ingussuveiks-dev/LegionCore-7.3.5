#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

// Read matching client scripts as evidence, without inferring server bindings
// from numeric matches belonging to other namespaces.
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string error; auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!ro || !ro->Fetch()[0].GetBool()) return 4;
    DB2Storage<SceneScriptTextEntry> texts("SceneScriptText.db2", SceneScriptTextLoadInfo::Instance());
    DB2Storage<SceneScriptGlobalTextEntry> globals("SceneScriptGlobalText.db2", SceneScriptGlobalTextLoadInfo::Instance());
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2Storage<SceneScriptPackageEntry> packages("SceneScriptPackage.db2", SceneScriptPackageLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    if (!texts.Load(argv[1], 0) || !globals.Load(argv[1], 0) || !scripts.Load(argv[1], 0) || !packages.Load(argv[1], 0) || !members.Load(argv[1], 0)) return 5;
    texts.LoadFromDB(); globals.LoadFromDB(); scripts.LoadFromDB(); packages.LoadFromDB(); members.LoadFromDB();
    std::vector<std::string> const words{"yulon", "yu'lon", "yu lon", "yigthregor", "korithis", "fu zan", "fuzan", "wandering isle", "trial at the temple"};
    std::vector<std::string> const numbers{"5699","5700","5701","5719","5720","5721","5727","5728","5729","52731","109820","109391","109440","109441","217818","217819","217820","101430","103151","202014"};
    auto matches = [&](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        std::vector<std::string> found;
        for (auto const& word : words) if (value.find(word) != std::string::npos) found.push_back(word);
        for (auto const& number : numbers) for (size_t at = value.find(number); at != std::string::npos; at = value.find(number, at + 1))
            if ((at == 0 || !std::isdigit(static_cast<unsigned char>(value[at - 1]))) &&
                (at + number.size() == value.size() || !std::isdigit(static_cast<unsigned char>(value[at + number.size()])))) { found.push_back(number); break; }
        return found;
    };
    std::set<uint32> textIds, scriptIds, packageIds{1668};
    uint32 textCount = 0, globalCount = 0, scriptCount = 0, packageCount = 0, memberCount = 0;
    auto emit = [&](char const* kind, auto row, bool force = false) {
        std::string name = row->Name->Str[0], source = row->Script->Str[0];
        auto found = matches(name + '\n' + source);
        if (found.empty() && !force) return false;
        std::cout << kind << '\t' << row->ID << '\t' << name;
        for (auto const& term : found) std::cout << '\t' << term;
        std::cout << "\nSCRIPT_BEGIN\t" << row->ID << '\t' << source.size() << '\n' << source << "\nSCRIPT_END\t" << row->ID << '\n';
        return true;
    };
    for (auto row : texts) { ++textCount; if (emit("TEXT", row)) textIds.insert(row->ID); }
    for (auto row : globals) { ++globalCount; emit("GLOBAL", row); }
    for (auto row : scripts) { ++scriptCount; if (textIds.count(row->ID) || textIds.count(row->FirstSceneScriptID) || textIds.count(row->NextSceneScriptID)) {
        scriptIds.insert(row->ID); std::cout << "SCRIPT_RECORD\t" << row->ID << '\t' << row->FirstSceneScriptID << '\t' << row->NextSceneScriptID << '\n'; } }
    for (auto row : packages) { ++packageCount; if (!matches(row->Name->Str[0]).empty()) packageIds.insert(row->ID); }
    for (auto row : members) { ++memberCount; if (scriptIds.count(row->SceneScriptID)) packageIds.insert(row->SceneScriptPackageID); }
    // Read the complete selected packages, including unlabelled initializers
    // and child helpers, rather than treating keyword excerpts as a sequence.
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto row : members) if (packageIds.count(row->SceneScriptPackageID)) {
            if (row->ChildSceneScriptPackageID) changed |= packageIds.insert(row->ChildSceneScriptPackageID).second;
            if (row->SceneScriptID) changed |= scriptIds.insert(row->SceneScriptID).second;
        }
        for (auto row : scripts) if (scriptIds.count(row->ID)) {
            if (row->FirstSceneScriptID) changed |= scriptIds.insert(row->FirstSceneScriptID).second;
            if (row->NextSceneScriptID) changed |= scriptIds.insert(row->NextSceneScriptID).second;
        }
    }
    for (auto id : scriptIds) {
        auto row = scripts.LookupEntry(id);
        if (!row || !texts.LookupEntry(id)) return 6;
        std::cout << "SCRIPT_SELECTED\t" << id << '\t' << row->FirstSceneScriptID << '\t' << row->NextSceneScriptID << '\n';
    }
    for (auto row : texts) if (scriptIds.count(row->ID) && !textIds.count(row->ID)) emit("PACKAGE_TEXT", row, true);
    for (auto row : packages) if (packageIds.count(row->ID)) std::cout << "PACKAGE\t" << row->ID << '\t' << row->Name->Str[0] << '\n';
    for (auto row : members) if (packageIds.count(row->SceneScriptPackageID) || packageIds.count(row->ChildSceneScriptPackageID))
        std::cout << "MEMBER\t" << row->ID << '\t' << row->SceneScriptPackageID << '\t' << row->SceneScriptID << '\t' << row->ChildSceneScriptPackageID << '\t' << unsigned(row->OrderIndex) << '\n';
    std::cout << "COMPLETE\t" << textCount << '\t' << globalCount << '\t' << scriptCount << '\t' << packageCount << '\t' << memberCount << '\n';
    std::cout << "SELECTION\t" << textIds.size() << '\t' << scriptIds.size() << '\t' << packageIds.size() << '\n';
    std::cout << "EOF\tFuZanSceneBindingCatalog\n";
    HotfixDatabase.Close();
}
