#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2FileLoader.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <set>

struct WarswordsSceneFile : DB2FileSource
{
    std::string name;
    FILE* file;
    explicit WarswordsSceneFile(std::string path) : name(std::move(path)), file(std::fopen(name.c_str(), "rb")) { }
    ~WarswordsSceneFile() override { if (file) std::fclose(file); }
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
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2", SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2", SpellMiscLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> castTimes("SpellCastTimes.db2", SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2", SpellDurationLoadInfo::Instance());
    DB2Storage<LFGDungeonsEntry> dungeons("LFGDungeons.db2", LfgDungeonsLoadInfo::Instance());
    if (!steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0) || !spells.Load(argv[1], 0) || !effects.Load(argv[1], 0) || !dungeons.Load(argv[1], 0))
        return 5;
    steps.LoadFromDB();
    trees.LoadFromDB();
    criteria.LoadFromDB();
    spells.LoadFromDB();
    effects.LoadFromDB();
    if (!misc.Load(argv[1], 0) || !castTimes.Load(argv[1], 0) || !durations.Load(argv[1], 0))
        return 17;
    misc.LoadFromDB();
    castTimes.LoadFromDB();
    durations.LoadFromDB();
    DB2Storage<BroadcastTextEntry> textsStore("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<SoundKitEntry> soundKits("SoundKit.db2", SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> soundFiles("SoundKitEntry.db2", SoundKitEntryLoadInfo::Instance());
    if (!textsStore.Load(argv[1], 0) || !soundKits.Load(argv[1], 0) || !soundFiles.Load(argv[1], 0))
        return 18;
    textsStore.LoadFromDB();
    soundKits.LoadFromDB();
    soundFiles.LoadFromDB();
    for (uint32 identity : {100374, 106338, 100375})
    {
        auto text = textsStore.LookupEntry(identity);
        if (!text)
            return 19;
        std::cout << "BROADCAST\t" << identity << '\t' << text->SoundEntriesID[0] << '\t' << text->SoundEntriesID[1] << '\n';
        for (uint32 kit : text->SoundEntriesID)
            if (kit)
            {
                if (!soundKits.LookupEntry(kit))
                    return 20;
                bool found = false;
                for (auto sound : soundFiles)
                    if (uint32(sound->SoundKitID) == kit && sound->FileDataID && sound->Frequency && sound->Volume > 0)
                    {
                        found = true;
                        std::cout << "AUDIO_FILE\t" << identity << '\t' << kit << '\t' << sound->FileDataID << '\n';
                    }
                if (!found)
                    return 21;
            }
    }
    for (uint32 identity : {77359, 64464})
    {
        if (!soundKits.LookupEntry(identity))
            return 24;
        bool found = false;
        for (auto sound : soundFiles)
            if (uint32(sound->SoundKitID) == identity && sound->FileDataID && sound->Frequency && sound->Volume > 0)
            {
                found = true;
                std::cout << "SCENE_SOUND_FILE\t" << identity << '\t' << sound->FileDataID << '\n';
            }
        if (!found)
            return 25;
    }
    dungeons.LoadFromDB();
    DB2Storage<MapEntry> maps("Map.db2", MapLoadInfo::Instance());
    if (!maps.Load(argv[1], 0))
        return 11;
    maps.LoadFromDB();
    DB2Storage<WorldMapAreaEntry> worldAreas("WorldMapArea.db2", WorldMapAreaLoadInfo::Instance());
    if (!worldAreas.Load(argv[1], 0))
        return 22;
    worldAreas.LoadFromDB();
    auto areaPrecision = std::cout.precision(9);
    for (auto area : worldAreas)
        if (area->MapID == 1511 || area->AreaID == 7541)
            std::cout << "WORLD_AREA\t" << area->ID << '\t' << area->AreaID << '\t' << area->LocLeft << '\t'
                << area->LocRight << '\t' << area->LocTop << '\t' << area->LocBottom << '\t'
                << area->MapID << '\t' << area->ParentWorldMapID << '\n';
    std::cout.precision(areaPrecision);
    DB2Storage<LockEntry> locks("Lock.db2", LockLoadInfo::Instance());
    if (!locks.Load(argv[1], 0))
        return 15;
    locks.LoadFromDB();
    auto lock = locks.LookupEntry(1691);
    if (!lock)
        return 16;
    for (uint32 index = 0; index < MAX_LOCK_CASE; ++index)
        std::cout << "LOCK\t" << lock->ID << '\t' << index << '\t' << unsigned(lock->Type[index]) << '\t'
            << lock->Index[index] << '\t' << lock->Skill[index] << '\n';
    auto map = maps.LookupEntry(1511);
    if (!map)
        return 12;
    std::cout << "MAP\t" << map->ID << '\t' << unsigned(map->InstanceType) << '\t' << map->IsDungeon() << '\t'
        << map->IsScenario() << '\t' << unsigned(map->MaxPlayers) << '\n';
    DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2", ItemChildEquipmentLoadInfo::Instance());
    DB2Storage<SceneScriptEntry> scripts("SceneScript.db2", SceneScriptLoadInfo::Instance());
    DB2Storage<SceneScriptPackageMemberEntry> members("SceneScriptPackageMember.db2", SceneScriptPackageMemberLoadInfo::Instance());
    DB2FileLoader texts;
    WarswordsSceneFile sceneFile(std::string(argv[1]) + "SceneScriptText.db2");
    if (!children.Load(argv[1], 0) || !scripts.Load(argv[1], 0) || !members.Load(argv[1], 0) || !texts.Load(&sceneFile, SceneScriptTextLoadInfo::Instance()))
        return 8;
    children.LoadFromDB();
    scripts.LoadFromDB();
    members.LoadFromDB();
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    if (!items.Load(argv[1], 0))
        return 13;
    items.LoadFromDB();
    std::set<uint32> equipment{128908, 134553, 128289};
    for (auto child : children)
        if (child->ItemID == 128289)
            equipment.insert(child->ChildItemID);
    for (uint32 identity : equipment)
    {
        auto item = items.LookupEntry(identity);
        if (!item)
            return 14;
        std::cout << "ITEM\t" << identity << '\t' << unsigned(item->ArtifactID) << '\t' << unsigned(item->InventoryType)
            << '\t' << item->ItemLevel << '\t' << item->Display->Str[0] << '\n';
    }
    for (auto child : children)
        if (child->ItemID == 128908 || child->ItemID == 128289)
            std::cout << "CHILD\t" << child->ItemID << '\t' << child->ChildItemID << '\t' << unsigned(child->ChildItemEquipSlot) << '\n';
    std::set<uint32> packages{1580, 1575}, seenPackages, pendingScripts, seenScripts;
    while (!packages.empty())
    {
        uint32 package = *packages.begin();
        packages.erase(package);
        if (!seenPackages.insert(package).second)
            continue;
        for (auto member : members)
            if (member->SceneScriptPackageID == package)
            {
                std::cout << "SCENE_MEMBER\t" << package << '\t' << member->SceneScriptID << '\t' << member->ChildSceneScriptPackageID << '\n';
                if (member->ChildSceneScriptPackageID)
                    packages.insert(member->ChildSceneScriptPackageID);
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
            return 9;
        bool found = false;
        for (uint32 index = 0; index < texts.GetRecordCount(); ++index)
        {
            auto record = texts.GetRecord(index);
            if (record && record.GetId() == identity)
            {
                found = true;
                std::cout << "SCENE_TEXT\t" << identity << '\t' << record.GetString("Name") << '\n' << record.GetString("Script") << '\n';
                break;
            }
        }
        if (!found)
            return 10;
        if (script->NextSceneScriptID)
            pendingScripts.insert(script->NextSceneScriptID);
    }
    bool foundDungeon = false;
    for (auto dungeon : dungeons)
        if (dungeon->ScenarioID == 944)
        {
            foundDungeon = true;
            std::cout << "DUNGEON\t" << dungeon->ID << '\t' << dungeon->MapID << '\t' << unsigned(dungeon->DifficultyID)
                << '\t' << int(dungeon->Faction) << '\t' << dungeon->GetMinGroupSize() << '\t' << dungeon->GetMaxGroupSize() << '\n';
        }
    if (!foundDungeon)
        std::cout << "NO_NATIVE_DUNGEON\t944\n";
    for (auto step : steps)
        if (step->ScenarioID == 909)
        {
            std::cout << "EARTH_STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t'
                << step->IsBonusObjective() << '\t' << step->RewardQuestID << '\t' << step->Title->Str[0] << '\n';
            std::set<uint32> pending{uint32(step->Criteriatreeid)}, seen;
            while (!pending.empty())
            {
                uint32 identity = *pending.begin();
                pending.erase(identity);
                if (!seen.insert(identity).second)
                    continue;
                auto tree = trees.LookupEntry(identity);
                if (!tree)
                    return 23;
                if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                    std::cout << "EARTH_CRITERION\t" << unsigned(step->OrderIndex) << '\t' << criterion->ID << '\t'
                        << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t' << tree->Amount << '\n';
                for (auto descendant : trees)
                    if (uint32(descendant->Parent) == identity)
                        pending.insert(descendant->ID);
            }
        }
    for (uint32 identity : {128264, 195996, 197852, 195986, 197853, 205443, 194201, 208578, 192085, 225890, 195062, 205424, 205534, 189695, 205744, 205340, 188074, 208562, 225889})
    {
        auto spell = spells.LookupEntry(identity);
        if (!spell)
            return 7;
        std::cout << "SPELL\t" << identity << '\t' << spell->Name->Str[0] << '\n';
        for (auto data : misc)
            if (data->SpellID == identity && !data->DifficultyID)
            {
                auto cast = castTimes.LookupEntry(data->CastingTimeIndex);
                auto duration = durations.LookupEntry(data->DurationIndex);
                std::cout << "CAST\t" << identity << '\t' << data->CastingTimeIndex << '\t'
                    << (cast ? cast->Base : 0) << '\t' << (cast ? cast->Minimum : 0) << '\t'
                    << (cast ? cast->PerLevel : 0) << '\t' << data->LaunchDelay << '\n';
                std::cout << "DURATION\t" << identity << '\t' << data->DurationIndex << '\t'
                    << (duration ? duration->Duration : 0) << '\t' << (duration ? duration->MaxDuration : 0) << '\t'
                    << (duration ? duration->DurationPerLevel : 0) << '\n';
            }
        for (auto effect : effects)
            if (!effect->DifficultyID && effect->SpellID == identity)
                std::cout << "EFFECT\t" << identity << '\t' << effect->EffectIndex << '\t' << effect->Effect << '\t'
                    << effect->EffectAura << '\t' << effect->EffectMiscValue[0] << '\t' << effect->EffectTriggerSpell << '\t'
                    << effect->ImplicitTarget[0] << '\t' << effect->ImplicitTarget[1] << '\n';
    }
    for (auto step : steps)
        if (step->ScenarioID == 944)
        {
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t'
                << step->Title->Str[0] << '\t' << step->Description->Str[0] << '\n';
            std::cout << "STEP_KIND\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t' << step->IsBonusObjective() << '\n';
            std::cout << "STEP_REWARD\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t' << step->RewardQuestID << '\n';
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
                for (auto descendant : trees)
                    if (uint32(descendant->Parent) == identity)
                        pending.insert(descendant->ID);
            }
        }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
