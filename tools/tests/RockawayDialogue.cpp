#include <unordered_set>
#include <Windows.h>
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include "DB2Stores.h"
#include "DB2LoadInfo.h"
#include "SharedDefines.h"
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <cstring>
#include "CreatureTextEntry.inc"

static void Require(bool value, char const* message)
{
    if (!value)
        throw std::runtime_error(message);
}

// Only the two actor accessors are controlled. The production SendText sound
// selector executes against native typed DB2 stores; no packed-field reader.
struct Speaker
{
    uint32 display;
    uint8 gender = GENDER_MALE;
    uint32 GetDisplayId() const { return display; }
    uint8 getGender() const { return gender; }
};
static uint32 SelectedSound(Speaker* source, CreatureTextEntry const* text,
    BroadcastTextEntry const* bct, uint32 sound = 0)
{
#include "SoundSelection.inc"
    return fSound;
}

int Run(int argc, char** argv)
{
    Require(argc == 4, "Expected native DBC directory, SQL snapshot and private configuration");
    std::string error;
    Require(sConfigMgr->LoadInitial(argv[3], {}, error), "Private configuration failed");
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    Require(!HotfixDatabase.Open() && HotfixDatabase.PrepareStatements(), "Hotfix pool failed");
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    Require(readOnly && readOnly->Fetch()[0].GetBool(), "Hotfix session is not read-only");
    Require(sBroadcastTextStore.Load(argv[1], 0) && sCreatureDisplayInfoStore.Load(argv[1], 0),
        "Native broadcast/display catalogs failed");
    for (uint32 id : {98097u, 98105u, 123587u, 123609u, 123644u})
        std::cout << "BASE " << id << ' ' << bool(sBroadcastTextStore.LookupEntry(id)) << '\n';
    sBroadcastTextStore.LoadFromDB();
    sCreatureDisplayInfoStore.LoadFromDB();
    DB2Storage<SoundKitEntry> sounds("SoundKit.db2", SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> files("SoundKitEntry.db2", SoundKitEntryLoadInfo::Instance());
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2", ConversationLineLoadInfo::Instance());
    Require(sounds.Load(argv[1], 0) && files.Load(argv[1], 0) && lines.Load(argv[1], 0),
        "Native sound/line catalogs failed");
    std::map<uint32, std::vector<SoundKitEntryEntry const*>> bySound;
    for (auto row : files)
        bySound[row->SoundKitID].push_back(row);
    auto emitFiles = [&](std::string const& key, uint32 sound)
    {
        Require(sound && sounds.LookupEntry(sound), "Selected sound kit missing");
        auto const& entries = bySound[sound];
        Require(!entries.empty(), "Selected sound kit has no native files");
        for (auto file : entries)
        {
            Require(file->FileDataID && file->Frequency && file->Volume > 0,
                "Selected sound has an inaudible/invalid file entry");
            std::cout << "FILE " << key << ' ' << sound << ' ' << file->ID << ' '
                << file->FileDataID << ' ' << uint32(file->Frequency) << ' ' << file->Volume << '\n';
        }
    };

    std::ifstream snapshot(argv[2]);
    Require(bool(snapshot), "Missing SQL snapshot");
    std::set<std::pair<uint32, uint32>> expected = {{115132, 1}, {115132, 2}, {115133, 1}, {115133, 4}};
    std::map<std::pair<uint32, uint32>, uint32> expectedText = {
        {{115132, 1}, 98097}, {{115132, 2}, 98105}, {{115133, 1}, 123587}, {{115133, 4}, 123609}};
    unsigned rows = 0;
    while (snapshot)
    {
        uint32 entry, group, id, type, sqlSound, broadcast, duration, minTimer, maxTimer, display;
        if (!(snapshot >> entry >> group >> id >> type >> sqlSound >> broadcast >> duration >> minTimer >> maxTimer >> display))
            break;
        Require(expected.erase({entry, group}) == 1 && id == 0 &&
            type == (entry == 115133 ? CHAT_MSG_MONSTER_YELL : CHAT_MSG_MONSTER_SAY) &&
            broadcast == expectedText.at({entry, group}) && display == (entry == 115133 ? 66238u : 70526u),
            "Missing/duplicate/unexpected dialogue row");
        auto bct = sBroadcastTextStore.LookupEntry(broadcast);
        auto model = sCreatureDisplayInfoStore.LookupEntry(display);
        if (!bct || !model)
            std::cerr << "lookup entry=" << entry << " broadcast=" << broadcast << " exists=" << bool(bct)
                << " display=" << display << " exists=" << bool(model) << '\n';
        Require(bct && model, "Missing native text/display");
        auto gender = model->Gender;
        if (gender == GENDER_UNKNOWN) gender = GENDER_NONE;
        Require(gender <= GENDER_NONE, "Reinspect actual model gender normalization");
        Speaker actor{display, uint8(gender)};
        CreatureTextEntry text{}; text.sound = sqlSound;
        auto selected = SelectedSound(&actor, &text, bct);
        Require(SelectedSound(&actor, &text, nullptr) == sqlSound &&
            SelectedSound(&actor, &text, bct, 123) == 123, "Native fallback/explicit override changed");
        BroadcastTextEntry silent = *bct; silent.SoundEntriesID[0] = silent.SoundEntriesID[1] = 0;
        Require(SelectedSound(&actor, &text, &silent) == sqlSound, "Silent broadcast lost SQL fallback");
        auto value = DB2Manager::GetBroadcastTextValue(bct, LOCALE_enUS, uint8(gender));
        Require(value && value[0], "Native broadcast text empty");
        std::string key = std::to_string(entry) + ":" + std::to_string(group);
        std::cout << "DIALOG " << key << ' ' << type << ' ' << broadcast << ' ' << sqlSound << ' '
            << selected << ' ' << duration << ' ' << minTimer << ' ' << maxTimer << ' '
            << display << ' ' << uint32(gender) << ' ' << uint32(model->Gender) << ' '
            << bct->SoundEntriesID[0] << ' ' << bct->SoundEntriesID[1] << '\n';
        std::cout << "TEXT " << key << ' ' << value << '\n';
        emitFiles(key, selected);
        ++rows;
    }
    Require(rows == 4 && expected.empty(), "Incomplete dialogue snapshot");
    auto line = lines.LookupEntry(9050);
    auto bct = line ? sBroadcastTextStore.LookupEntry(line->BroadcastTextID) : nullptr;
    Require(bct && bct->ID == 123644 && bct->SoundEntriesID[0] == 77641,
        "Rockaway conversation sound changed");
    std::cout << "TEXT outro " << DB2Manager::GetBroadcastTextValue(bct, LOCALE_enUS, GENDER_MALE) << '\n';
    emitFiles("outro", bct->SoundEntriesID[0]);
    std::cout << "PASS: typed native dialogue/voice files, actual SendText sound selector and native localized text accessor. Actor getters are controlled; no packet delivery or client playback claim.\n";
    HotfixDatabase.Close();
    return 0;
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    try { return Run(argc, argv); }
    catch (std::exception const& e) { std::cerr << e.what() << '\n'; return 1; }
}
