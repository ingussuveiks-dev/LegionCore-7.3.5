#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2Meta.h"
#include "DB2FileLoader.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct MagnarIntroVisualEvent { SpellVisualEventEntry event; int32 visual; };
#pragma pack(pop)

struct MagnarSceneFile : DB2FileSource
{
    std::string name;
    FILE* file;
    explicit MagnarSceneFile(std::string path) : name(std::move(path)), file(std::fopen(name.c_str(), "rb")) { }
    ~MagnarSceneFile() override { if (file) std::fclose(file); }
    bool IsOpen() const override { return file != nullptr; }
    bool Read(void* buffer, std::size_t size) override { return std::fread(buffer, size, 1, file) == 1; }
    std::size_t GetPosition() const override { return std::ftell(file); }
    std::size_t GetFileSize() const override { return std::filesystem::file_size(name); }
    char const* GetFileName() const override { return name.c_str(); }
};

bool MatchesMagnar(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) { return char(std::tolower(character)); });
    for (auto term : {"magnar reveal", "magnar intro", "earthward", "189718", "189721", "190588", "46083"})
        if (value.find(term) != std::string::npos)
            return true;
    return false;
}

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
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool())
        return 4;
    DB2Storage<SceneScriptPackageEntry> packages("SceneScriptPackage.db2", SceneScriptPackageLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2FileLoader texts;
    MagnarSceneFile sceneFile(std::string(argv[1]) + "SceneScriptText.db2");
    if (!packages.Load(argv[1], 0) || !members.Load(argv[1], 0) || !scripts.Load(argv[1], 0) ||
        !texts.Load(&sceneFile, SceneScriptTextLoadInfo::Instance()))
        return 5;
    packages.LoadFromDB();
    members.LoadFromDB();
    scripts.LoadFromDB();
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<SoundKitEntry> soundKits("SoundKit.db2", SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> soundFiles("SoundKitEntry.db2", SoundKitEntryLoadInfo::Instance());
    if (!spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !misc.Load(argv[1], 0) ||
        !durations.Load(argv[1], 0) || !soundKits.Load(argv[1], 0) || !soundFiles.Load(argv[1], 0))
        return 8;
    spells.LoadFromDB(); effects.LoadFromDB(); misc.LoadFromDB(); durations.LoadFromDB(); soundKits.LoadFromDB(); soundFiles.LoadFromDB();
    auto spell = spells.LookupEntry(189721);
    if (!spell)
        return 9;
    std::cout << "SPELL\t189721\t" << spell->Name->Str[0] << '\n';
    for (auto effect : effects)
        if (effect->SpellID == 189721)
            std::cout << "EFFECT\t" << unsigned(effect->EffectIndex) << '\t' << effect->Effect << '\t'
                << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t'
                << unsigned(effect->ImplicitTarget[0]) << '\t' << unsigned(effect->ImplicitTarget[1]) << '\n';
    for (auto setting : misc)
        if (setting->SpellID == 189721)
        {
            auto duration = durations.LookupEntry(setting->DurationIndex);
            if (!duration)
                return 10;
            std::cout << "DURATION\t" << duration->Duration << '\t' << duration->DurationPerLevel << '\t' << duration->MaxDuration << '\n';
        }
    for (uint32 identity : {77391u, 77396u, 77393u, 73289u, 60406u})
    {
        if (!soundKits.LookupEntry(identity))
            return 11;
        bool found = false;
        for (auto sound : soundFiles)
            if (uint32(sound->SoundKitID) == identity && sound->FileDataID && sound->Frequency && sound->Volume > 0)
            {
                found = true;
                std::cout << "SOUND\t" << identity << '\t' << sound->FileDataID << '\n';
            }
        if (!found)
            return 12;
    }
    DB2Storage<CreatureEntry> creatures("Creature.db2", CreatureLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2", CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2", CreatureModelDataLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> broadcasts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<SpellVisualEntry> visuals("SpellVisual.db2", SpellVisualLoadInfo::Instance());
    DB2Storage<SpellVisualKitEntry> kits("SpellVisualKit.db2", SpellVisualKitLoadInfo::Instance());
    auto eventInfo = SpellVisualEventLoadInfo::Instance();
    std::vector<DB2FieldMeta> eventFields(eventInfo->Fields, eventInfo->Fields + eventInfo->FieldCount);
    eventFields.push_back({true, FT_INT, "SpellVisualID"});
    DB2LoadInfo fullEventInfo(eventFields.data(), eventFields.size(), SpellVisualEventMeta::Instance(), HOTFIX_SEL_SPELL_VISUAL_EVENT);
    DB2Storage<MagnarIntroVisualEvent> events("SpellVisualEvent.db2", &fullEventInfo);
    if (sizeof(MagnarIntroVisualEvent) != fullEventInfo.Meta->GetRecordSize() || !creatures.Load(argv[1], 0) ||
        !displays.Load(argv[1], 0) || !models.Load(argv[1], 0) || !broadcasts.Load(argv[1], 0) ||
        !visuals.Load(argv[1], 0) || !kits.Load(argv[1], 0) || !events.Load(argv[1], 0))
        return 13;
    creatures.LoadFromDB(); displays.LoadFromDB(); models.LoadFromDB(); broadcasts.LoadFromDB(); visuals.LoadFromDB(); kits.LoadFromDB();
    auto actor = creatures.LookupEntry(94699);
    auto broadcast = broadcasts.LookupEntry(97071);
    if (!actor || !broadcast || !visuals.LookupEntry(49290))
        return 14;
    std::cout << "ACTOR\t94699\t" << actor->Name->Str[0] << '\n';
    for (int32 displayIdentity : actor->DisplayID)
        if (displayIdentity)
        {
            auto display = displays.LookupEntry(displayIdentity);
            auto model = display ? models.LookupEntry(display->ModelID) : nullptr;
            if (!model)
                return 15;
            std::cout << "MODEL\t" << displayIdentity << '\t' << display->ModelID << '\t' << model->FileDataID << '\n';
        }
    std::cout << "BROADCAST\t97071\t" << broadcast->Text->Str[0] << '\t' << broadcast->SoundEntriesID[0] << '\n';
    bool foundVisualEvent = false;
    for (auto event : events)
        if (event->visual == 49290)
        {
            auto kit = kits.LookupEntry(event->event.SpellVisualKitID);
            if (!kit)
                return 16;
            foundVisualEvent = true;
            std::cout << "VISUAL_EVENT\t49290\t" << event->event.StartEvent << '\t' << event->event.EndEvent << '\t'
                << kit->ID << '\t' << kit->Flags << '\n';
        }
    if (!foundVisualEvent)
        return 17;
    std::cout << "SOURCE\troot-tables-base-plus-hotfix\tscene-text-and-visual-parent-records-base-only\n";
    std::set<uint32> matchingScripts;
    for (uint32 index = 0; index < texts.GetRecordCount(); ++index)
    {
        auto record = texts.GetRecord(index);
        if (record && (MatchesMagnar(record.GetString("Name")) || MatchesMagnar(record.GetString("Script"))))
        {
            matchingScripts.insert(record.GetId());
            std::cout << "SCENE_TEXT\t" << record.GetId() << '\t' << record.GetString("Name") << '\n'
                << record.GetString("Script") << '\n';
        }
    }
    std::set<uint32> matchingPackages;
    for (auto package : packages)
        if (MatchesMagnar(package->Name->Str[0]))
            matchingPackages.insert(package->ID);
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (auto script : scripts)
            if (matchingScripts.count(script->NextSceneScriptID) || matchingScripts.count(script->FirstSceneScriptID))
                changed = matchingScripts.insert(script->ID).second || changed;
        for (auto member : members)
            if (matchingScripts.count(member->SceneScriptID) || matchingPackages.count(member->ChildSceneScriptPackageID))
                changed = matchingPackages.insert(member->SceneScriptPackageID).second || changed;
    }
    for (uint32 identity : matchingPackages)
    {
        auto package = packages.LookupEntry(identity);
        if (!package)
            return 6;
        std::cout << "PACKAGE\t" << identity << '\t' << package->Name->Str[0] << '\n';
        for (auto member : members)
            if (uint32(member->SceneScriptPackageID) == identity)
                std::cout << "MEMBER\t" << identity << '\t' << member->SceneScriptID << '\t'
                    << member->ChildSceneScriptPackageID << '\t' << unsigned(member->OrderIndex) << '\n';
    }
    return matchingScripts.empty() ? 7 : 0;
}
