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
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    DB2Storage<SoundKitEntry> kits("SoundKit.db2",SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitNameEntry> names("SoundKitName.db2",SoundKitNameLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    if(!texts.Load(argv[1],0)||!kits.Load(argv[1],0)||!names.Load(argv[1],0)||!sounds.Load(argv[1],0))return 4;
    texts.LoadFromDB();kits.LoadFromDB();names.LoadFromDB();sounds.LoadFromDB();
    // Inspect the existing98074 explicit override as evidence, rather than
    // assuming a valid kit is the right speaker for its BroadcastText.
    std::set<uint32> soundKits{62044},resolved;
    if(!kits.LookupEntry(62044))return 6;
    for(uint32 id:{99965u,99966u,99967u,99968u,99969u,99970u,100187u})
    {
        auto text=texts.LookupEntry(id);if(!text)return 5;
        std::cout<<"TEXT\t"<<id<<'\t'<<text->Text->Str[0]<<'\t'<<text->Text1->Str[0]<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\n';
        for(auto kit:text->SoundEntriesID)if(kit){if(!kits.LookupEntry(kit))return 6;soundKits.insert(kit);}
    }
    for(auto kit:soundKits)if(auto name=names.LookupEntry(kit))std::cout<<"SOUND_NAME\t"<<kit<<'\t'<<name->Name->Str[0]<<'\n';else return 9;
    for(auto text:texts)if(text->SoundEntriesID[0]==62044||text->SoundEntriesID[1]==62044)
        std::cout<<"OVERRIDE_NATIVE_OWNER\t"<<text->ID<<'\t'<<text->Text->Str[0]<<'\t'<<text->Text1->Str[0]<<'\n';
    for(auto sound:sounds)if(soundKits.count(sound->SoundKitID))
    {
        if(!sound->FileDataID||!sound->Frequency||sound->Volume<=0)return 7;
        resolved.insert(sound->SoundKitID);
        std::cout<<"AUDIO_FILE\t"<<sound->SoundKitID<<'\t'<<sound->FileDataID<<'\n';
    }
    if(resolved!=soundKits)return 8;
    std::cout<<"COMPLETE\t7\t"<<soundKits.size()<<'\n';
    HotfixDatabase.Close();MySQL::Library_End();
}
