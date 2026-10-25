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
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2", BroadcastTextLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2", SoundKitEntryLoadInfo::Instance());
    if (!texts.Load(argv[1], 0) || !sounds.Load(argv[1], 0))
        return 5;
    texts.LoadFromDB(); sounds.LoadFromDB();
    for (uint32 identity : {98692u, 98324u, 98325u})
    {
        auto text = texts.LookupEntry(identity);
        if (!text)
            return 6;
        uint32 count = 0;
        for (uint32 kit : text->SoundEntriesID)
            if (kit)
                for (auto sound : sounds)
                    if (uint32(sound->SoundKitID) == kit)
                    {
                        ++count;
                        std::cout << "VOICE\t" << text->ID << '\t' << kit << '\t' << sound->FileDataID << '\n';
                    }
        if (count != 1)
        {
            std::cerr << "Expected one native voice for BroadcastText " << identity << '\n';
            return 7;
        }
    }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
