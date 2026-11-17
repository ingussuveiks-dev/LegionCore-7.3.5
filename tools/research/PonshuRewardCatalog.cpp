#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    if(!lines.Load(argv[1],0)||!texts.Load(argv[1],0))return 6;lines.LoadFromDB();texts.LoadFromDB();
    for(uint32 id:{9550u,9551u}){
        auto line=lines.LookupEntry(id);if(!line)return 7;auto text=texts.LookupEntry(line->BroadcastTextID);if(!text)return 8;
        std::cout<<"CONVERSATION_LINE\t"<<line->ID<<'\t'<<line->BroadcastTextID<<'\t'<<line->NextConversationLineID<<'\t'<<text->Text->Str[0]<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\n';
    }
    HotfixDatabase.Close();
}