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
    auto readOnly=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if(!readOnly||!readOnly->Fetch()[0].GetBool())return 4;
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    if(!lines.Load(argv[1],0)||!texts.Load(argv[1],0)||!spells.Load(argv[1],0)||
       !effects.Load(argv[1],0)||!sounds.Load(argv[1],0))return 5;
    lines.LoadFromDB();texts.LoadFromDB();spells.LoadFromDB();effects.LoadFromDB();sounds.LoadFromDB();
    auto line=lines.LookupEntry(13244);if(!line)return 6;
    std::cout<<"LINE\t"<<line->ID<<'\t'<<line->BroadcastTextID<<'\t'<<line->AdditionalDuration<<'\t'<<line->NextConversationLineID<<'\n';
    auto text=texts.LookupEntry(line->BroadcastTextID);if(!text)return 7;
    std::cout<<"TEXT\t"<<text->ID<<'\t'<<text->Text->Str[0]<<'\t'<<text->Text1->Str[0]<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\n';
    std::set<uint32> kits;
    for(auto kit:text->SoundEntriesID)if(kit)kits.insert(kit);
    for(auto sound:sounds)if(kits.count(sound->SoundKitID))
        std::cout<<"AUDIO_FILE\t"<<sound->SoundKitID<<'\t'<<sound->FileDataID<<'\n';
    for(auto effect:effects)if(effect->Effect==219&&effect->EffectMiscValue[0]==5912)
    {
        auto spell=spells.LookupEntry(effect->SpellID);if(!spell)return 8;
        std::cout<<"PRODUCER\t"<<effect->SpellID<<'\t'<<spell->Name->Str[0]<<'\t'<<unsigned(effect->DifficultyID)<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<effect->Effect<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    }
    std::cout<<"EOF\tconversation5912\n";
    HotfixDatabase.Close();MySQL::Library_End();
}
