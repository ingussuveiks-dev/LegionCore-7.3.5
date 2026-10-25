#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>

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
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2", ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2", SoundKitEntryLoadInfo::Instance());
    if (!lines.Load(argv[1], 0) || !texts.Load(argv[1], 0) || !sounds.Load(argv[1], 0))
        return 5;
    lines.LoadFromDB(); texts.LoadFromDB(); sounds.LoadFromDB();
    for (uint32 identity : {2066u, 2067u, 1324u, 1325u, 1326u, 1327u, 1328u, 1329u, 1330u, 1331u, 1332u, 1334u, 1335u, 1336u, 1337u, 1341u, 1342u, 1343u, 1344u})
    {
        auto line = lines.LookupEntry(identity);
        auto text = line ? texts.LookupEntry(line->BroadcastTextID) : nullptr;
        if (!line)
        {
            std::cerr << "Missing ConversationLine " << identity << '\n';
            return 6;
        }
        std::cout << "LINE\t" << line->ID << '\t' << line->BroadcastTextID << '\t' << line->SpellVisualKitID << '\t'
            << line->AdditionalDuration << '\t' << line->NextConversationLineID << '\t' << line->AnimKitID << '\t'
            << unsigned(line->SpeechType) << '\t' << unsigned(line->StartAnimation) << '\t' << unsigned(line->EndAnimation) << '\n';
        if (identity == 1334 && !line->BroadcastTextID && !line->SpellVisualKitID && !line->AdditionalDuration &&
            !line->NextConversationLineID && !line->AnimKitID && !line->SpeechType && line->StartAnimation == 60 && line->EndAnimation == 60)
        {
            std::cout << "NONVERBAL\t" << identity << '\n';
            continue;
        }
        if (!text)
        {
            std::cerr << "Missing BroadcastText " << line->BroadcastTextID << " for ConversationLine " << identity << '\n';
            return 6;
        }
        std::cout << "TEXT\t" << text->ID << '\t' << text->Text->Str[0] << '\t' << text->Text1->Str[0] << '\n';
        for (uint32 kit : text->SoundEntriesID)
            if (kit)
                for (auto sound : sounds)
                    if (uint32(sound->SoundKitID) == kit)
                        std::cout << "SOUND\t" << text->ID << '\t' << kit << '\t' << sound->FileDataID << '\n';
    }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
