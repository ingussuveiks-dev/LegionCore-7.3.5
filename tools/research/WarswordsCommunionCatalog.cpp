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
    for (uint32 identity : {2066u, 2067u})
    {
        auto line = lines.LookupEntry(identity);
        auto text = line ? texts.LookupEntry(line->BroadcastTextID) : nullptr;
        if (!line || !text)
            return 6;
        std::cout << "LINE\t" << line->ID << '\t' << line->BroadcastTextID << '\t' << line->SpellVisualKitID << '\t'
            << line->AdditionalDuration << '\t' << line->NextConversationLineID << '\t' << line->AnimKitID << '\t'
            << unsigned(line->SpeechType) << '\t' << unsigned(line->StartAnimation) << '\t' << unsigned(line->EndAnimation) << '\n';
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
