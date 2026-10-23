#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    char const* config = std::getenv("LEGION_DIALOGUE_CONFIG");
    std::string error;
    if (!config || !sConfigMgr->LoadInitial(config, {}, error))
        return 6;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements())
        return 7;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool())
        return 8;
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<SoundKitEntry> kits("SoundKit.db2", SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2", SoundKitEntryLoadInfo::Instance());
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2", ConversationLineLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2", CreatureDisplayInfoLoadInfo::Instance());
    if (!texts.Load(argv[1], 0) || !kits.Load(argv[1], 0) || !sounds.Load(argv[1], 0) ||
        !lines.Load(argv[1], 0) || !displays.Load(argv[1], 0))
        return 2;
    for (uint32 identity : {103289u, 103242u, 103441u, 106998u, 107282u, 103501u, 107281u})
        std::cout << "BASE_BROADCAST\t" << identity << '\t' << bool(texts.LookupEntry(identity)) << '\n';
    texts.LoadFromDB();
    displays.LoadFromDB();
    auto line = lines.LookupEntry(3566);
    auto display = displays.LookupEntry(65514);
    if (!line || !display)
        return 3;
    std::cout << "CONVERSATION_LINE\t" << line->ID << '\t' << line->BroadcastTextID << '\t'
        << line->SpellVisualKitID << '\t' << line->AnimKitID << '\t' << line->AdditionalDuration << '\n';
    for (uint32 identity : {28222u, 65514u})
    {
        auto actorDisplay = displays.LookupEntry(identity);
        if (!actorDisplay)
            return 9;
        std::cout << "DISPLAY\t" << actorDisplay->ID << '\t' << actorDisplay->ModelID << '\t'
            << uint32(actorDisplay->Gender) << '\n';
    }
    std::set<uint32> soundKits;
    for (uint32 identity : {103289u, 103242u, 103441u, 106998u, 107282u, 103501u, uint32(line->BroadcastTextID)})
    {
        auto text = texts.LookupEntry(identity);
        if (!text)
            return 4;
        std::cout << "BROADCAST\t" << identity << '\t' << text->SoundEntriesID[0] << '\t' << text->SoundEntriesID[1]
            << '\t' << text->Text->Str[0] << '\t' << text->Text1->Str[0] << '\n';
        for (uint32 sound : text->SoundEntriesID)
            if (sound)
            {
                if (!kits.LookupEntry(sound))
                    return 5;
                soundKits.insert(sound);
            }
    }
    std::set<uint32> resolvedKits;
    for (auto sound : sounds)
        if (soundKits.count(sound->SoundKitID))
        {
            if (!sound->FileDataID || !sound->Frequency || sound->Volume <= 0)
                return 10;
            resolvedKits.insert(sound->SoundKitID);
            std::cout << "AUDIO_FILE\t" << sound->SoundKitID << '\t' << sound->FileDataID << '\n';
        }
    if (resolvedKits != soundKits)
        return 11;
    HotfixDatabase.Close();
}
