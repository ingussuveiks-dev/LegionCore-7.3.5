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

int main(int argc,char** argv){
 if(argc!=2)return 1;
 DB2Storage<SceneScriptEntry> scripts("SceneScript.db2",SceneScriptLoadInfo::Instance());
 DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2",SceneScriptPackageMemberLoadInfo::Instance());
 DB2FileLoader texts;
 if(!scripts.Load(argv[1],0)||!members.Load(argv[1],0))return 2;
 SceneFile textFile(std::string(argv[1])+"SceneScriptText.db2");
 if(!texts.Load(&textFile,SceneScriptTextLoadInfo::Instance()))return 3;
    std::set<uint32> pendingPackages{1515}, seenPackages, pendingScripts, seenScripts;
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
