#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2FileLoader.h"
#include <filesystem>
#include <cstdio>
#include "MovementPackets.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

struct GauntletSceneFile : DB2FileSource
{
    std::string path;
    FILE* file;
    explicit GauntletSceneFile(std::string name) : path(std::move(name)), file(std::fopen(path.c_str(), "rb")) { }
    ~GauntletSceneFile() override { if (file) std::fclose(file); }
    bool IsOpen() const override { return file != nullptr; }
    bool Read(void* data, std::size_t size) override { return std::fread(data, size, 1, file) == 1; }
    std::size_t GetPosition() const override { return std::ftell(file); }
    std::size_t GetFileSize() const override { return std::filesystem::file_size(path); }
    char const* GetFileName() const override { return path.c_str(); }
};

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
    auto check = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!check || !check->Fetch()[0].GetBool()) return 4;
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
    if (!spells.Load(argv[1],0) || !effects.Load(argv[1],0) || !misc.Load(argv[1],0) || !times.Load(argv[1],0) || !ranges.Load(argv[1],0)) return 5;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();times.LoadFromDB();ranges.LoadFromDB();
    std::set<uint32> selected{208580,209132};
    for(auto effect:effects) if(effect->Effect==61 && (effect->EffectMiscValue[0]==50336 || effect->EffectMiscValue[0]==50314)) selected.insert(effect->SpellID);
    for(bool changed=true;changed;)
    {
        changed=false;
        for(auto effect:effects)
            if(selected.count(effect->SpellID) && effect->EffectTriggerSpell)
                changed=selected.insert(effect->EffectTriggerSpell).second || changed;
    }
    for (uint32 id : selected)
    {
        auto spell=spells.LookupEntry(id);
        if (!spell) return 6;
        std::cout << "SPELL\t" << id << '\t' << spell->Name->Str[0] << '\n';
        for(auto effect : effects) if(effect->SpellID==id)
        {
            std::cout << "EFFECT\t" << id << '\t' << unsigned(effect->DifficultyID) << '\t' << unsigned(effect->EffectIndex) << '\t' << effect->Effect << '\t' << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t' << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
            SpellImplicitTargetInfo target(effect->ImplicitTarget[0]);
            std::cout<<"TARGET\t"<<id<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<target.GetCheckType()<<'\t'<<target.GetReferenceType()<<'\n';
        }
        for(auto row:misc) if(row->SpellID==id)
        {
            auto time=times.LookupEntry(row->CastingTimeIndex);auto range=ranges.LookupEntry(row->RangeIndex);
            std::cout<<"MISC\t"<<id<<'\t'<<unsigned(row->DifficultyID)<<'\t'<<row->CastingTimeIndex<<'\t'<<(time?time->Base:-1)<<'\t'<<row->RangeIndex<<'\t'<<(range?range->RangeMax[0]:-1)<<'\t'<<(range?range->RangeMax[1]:-1)<<'\n';
        }
    }
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    DB2FileLoader texts;
    GauntletSceneFile file(std::string(argv[1]) + "SceneScriptText.db2");
    if (!scripts.Load(argv[1], 0) || !members.Load(argv[1], 0) || !texts.Load(&file, SceneScriptTextLoadInfo::Instance())) return 8;
    scripts.LoadFromDB(); members.LoadFromDB();
    for (uint32 root : {1604u})
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
    std::cout<<"EOF\tgauntlet-native\n";
    HotfixDatabase.Close(); MySQL::Library_End();
}
