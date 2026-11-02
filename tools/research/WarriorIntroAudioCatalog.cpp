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
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<SoundKitEntry> kits("SoundKit.db2",SoundKitLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    if(!texts.Load(argv[1],0)||!lines.Load(argv[1],0)||!kits.Load(argv[1],0)||!sounds.Load(argv[1],0))return 4;
    texts.LoadFromDB();lines.LoadFromDB();kits.LoadFromDB();sounds.LoadFromDB();
    std::set<uint32> broadcastIds{},soundKits,resolved;
    for(uint32 id:{1831u,3171u,3172u,3201u,3202u,3214u})
    {
        auto line=lines.LookupEntry(id);if(!line)return 5;
        broadcastIds.insert(line->BroadcastTextID);
        std::cout<<"LINE\t"<<id<<'\t'<<line->BroadcastTextID<<'\t'<<line->AdditionalDuration<<'\t'<<line->NextConversationLineID<<'\n';
    }
    for(auto id:broadcastIds)
    {
        auto text=texts.LookupEntry(id);if(!text)return 6;
        std::cout<<"TEXT\t"<<id<<'\t'<<text->Text->Str[0]<<'\t'<<text->Text1->Str[0]<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\n';
        for(auto kit:text->SoundEntriesID)if(kit){if(!kits.LookupEntry(kit))return 7;soundKits.insert(kit);}
    }
    for(auto sound:sounds)if(soundKits.count(sound->SoundKitID))
    {
        if(!sound->FileDataID||!sound->Frequency||sound->Volume<=0)return 8;
        resolved.insert(sound->SoundKitID);
        std::cout<<"AUDIO_FILE\t"<<sound->SoundKitID<<'\t'<<sound->FileDataID<<'\n';
    }
    if(resolved!=soundKits)return 9;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!effects.Load(argv[1],0))return 10;
    effects.LoadFromDB();
    for(auto effect:effects)
        if(effect->Effect==219 && (effect->EffectMiscValue[0]==770 || effect->EffectMiscValue[0]==1386 || effect->EffectMiscValue[0]==1405 || effect->EffectMiscValue[0]==1412))
            std::cout<<"CONVERSATION_SPELL_USAGE\t"<<effect->SpellID<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<effect->EffectMiscValue[0]<<'\n';
    HotfixDatabase.Close();MySQL::Library_End();
}
