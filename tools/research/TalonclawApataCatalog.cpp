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
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!texts.Load(argv[1],0)||!sounds.Load(argv[1],0)||!displays.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();texts.LoadFromDB();sounds.LoadFromDB();displays.LoadFromDB();
    for(uint32 identity:{13u,197355u,197615u,198883u}){
        auto spell=spells.LookupEntry(identity);if(!spell)return 6;
        std::cout<<"SPELL\t"<<identity<<'\t'<<spell->Name->Str[0]<<'\n';
    }
    std::set<uint32> selectedSpells{13,197355,197615,198883};
    for(auto spell:spells){
        std::string name=spell->Name->Str[0];
        if(name.find("Apata")!=std::string::npos||name.find("Cat Boss")!=std::string::npos){
            selectedSpells.insert(spell->ID);
            std::cout<<"SCENE_SPELL\t"<<spell->ID<<'\t'<<name<<'\n';
        }
    }
    for(auto effect:effects)if(!effect->DifficultyID&&selectedSpells.count(effect->SpellID))
        std::cout<<"EFFECT\t"<<effect->SpellID<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'
            <<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'
            <<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    for(auto text:texts){
        std::string first=text->Text->Str[0],second=text->Text1->Str[0];
        bool selected=false;
        for(char const* phrase:{"Beloved brother","shall be avenged","not healing","over for me","Not again, beast","drop a flare","hold them off","set the trap"})
            selected=selected||first.find(phrase)!=std::string::npos||second.find(phrase)!=std::string::npos;
        if(selected)std::cout<<"CANDIDATE_TEXT\t"<<text->ID<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\t'<<first<<'\t'<<second<<'\n';
        if(text->SoundEntriesID[1]>=62694&&text->SoundEntriesID[1]<=62697){
            std::cout<<"VOICE_TEXT\t"<<text->ID<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\t'<<uint32(text->LanguageID)<<'\t'<<text->ConditionID<<'\t'<<text->EmotesID<<'\t'<<second<<'\n';
            for(uint32 index=0;index<3;++index)std::cout<<"VOICE_EMOTE\t"<<text->ID<<'\t'<<index<<'\t'<<text->EmoteID[index]<<'\t'<<text->EmoteDelay[index]<<'\n';
        }
        if((text->SoundEntriesID[0]>=62685&&text->SoundEntriesID[0]<=62710)||(text->SoundEntriesID[1]>=62685&&text->SoundEntriesID[1]<=62710))
            std::cout<<"NEIGHBOR_TEXT\t"<<text->ID<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\t'<<first<<'\t'<<second<<'\n';
    }
    for(auto sound:sounds)if(sound->SoundKitID>=62694&&sound->SoundKitID<=62697)
        std::cout<<"AUDIO_FILE\t"<<sound->SoundKitID<<'\t'<<sound->FileDataID<<'\t'<<uint32(sound->Frequency)<<'\t'<<sound->Volume<<'\n';
    for(auto sound:sounds)if(sound->SoundKitID>=62685&&sound->SoundKitID<=62710)
        std::cout<<"NEIGHBOR_FILE\t"<<sound->SoundKitID<<'\t'<<sound->FileDataID<<'\t'<<uint32(sound->Frequency)<<'\t'<<sound->Volume<<'\n';
    auto display=displays.LookupEntry(66135);if(!display)return 7;
    std::cout<<"DISPLAY\t66135\t"<<uint32(display->Gender)<<'\n';
    HotfixDatabase.Close();
}
