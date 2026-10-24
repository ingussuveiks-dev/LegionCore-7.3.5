#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    char const* config=std::getenv("LEGION_DIALOGUE_CONFIG");
    std::string error;
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if(!readOnly||!readOnly->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!texts.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();texts.LoadFromDB();
    for(uint32 identity:{13u,197355u}){
        auto spell=spells.LookupEntry(identity);if(!spell)return 6;
        std::cout<<"SPELL\t"<<identity<<'\t'<<spell->Name->Str[0]<<'\n';
    }
    for(auto effect:effects)if(!effect->DifficultyID&&(effect->SpellID==13||effect->SpellID==197355))
        std::cout<<"EFFECT\t"<<effect->SpellID<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'
            <<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'
            <<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    for(auto text:texts){
        std::string first=text->Text->Str[0],second=text->Text1->Str[0];
        bool selected=false;
        for(char const* phrase:{"Beloved brother","shall be avenged","not healing"})
            selected=selected||first.find(phrase)!=std::string::npos||second.find(phrase)!=std::string::npos;
        if(selected)std::cout<<"CANDIDATE_TEXT\t"<<text->ID<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\t'<<first<<'\t'<<second<<'\n';
    }
    HotfixDatabase.Close();
}
