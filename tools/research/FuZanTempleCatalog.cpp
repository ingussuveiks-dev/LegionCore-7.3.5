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
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<ConversationLineEntry> lines("ConversationLine.db2",ConversationLineLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||
        !lines.Load(argv[1],0)||!texts.Load(argv[1],0)||!sounds.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();
    lines.LoadFromDB();texts.LoadFromDB();sounds.LoadFromDB();
    std::set<uint32> selected{217775,217786};
    for(auto effect:effects)
        if(effect->Effect==61&&(effect->EffectMiscValue[0]==52411||effect->EffectMiscValue[0]==52452||effect->EffectMiscValue[0]==52477))selected.insert(effect->SpellID);
    std::set<uint32> seen;
    while(seen.size()!=selected.size())
    {
        if(selected.size()>100)return 5;
        uint32 id=0;for(auto value:selected)if(!seen.count(value)){id=value;break;}seen.insert(id);
        auto spell=spells.LookupEntry(id);if(!spell)return 6;
        std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id)
        {
            std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'
                <<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'
                <<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
            if(e->EffectTriggerSpell)selected.insert(e->EffectTriggerSpell);
        }
        for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\n';}
    }
    std::set<uint32> wanted;
    for(uint32 id:{5759u,5938u,5760u,5761u,5775u,5762u,5763u,5764u,5765u})
    {
        auto line=lines.LookupEntry(id);if(!line)return 7;
        std::cout<<"LINE\t"<<id<<'\t'<<line->BroadcastTextID<<'\t'<<line->AdditionalDuration<<'\t'<<line->NextConversationLineID<<'\n';
        auto text=texts.LookupEntry(line->BroadcastTextID);
        if(!text){if(line->BroadcastTextID){std::cout<<"MISSING_TEXT\t"<<line->BroadcastTextID<<'\n';return 8;}continue;}
        std::cout<<"TEXT\t"<<text->ID<<'\t'<<text->Text->Str[0]<<'\t'<<text->Text1->Str[0]<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\n';
        for(auto kit:text->SoundEntriesID)if(kit)wanted.insert(kit);
    }
    for(auto sound:sounds)if(wanted.count(sound->SoundKitID))std::cout<<"SOUND\t"<<sound->SoundKitID<<'\t'<<sound->FileDataID<<'\n';
    HotfixDatabase.Close();MySQL::Library_End();
}
