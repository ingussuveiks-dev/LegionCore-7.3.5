#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <string>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> properties("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    DB2Storage<UnitPowerBarEntry> bars("UnitPowerBar.db2",UnitPowerBarLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    DB2Storage<SoundKitEntryEntry> sounds("SoundKitEntry.db2",SoundKitEntryLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!properties.Load(argv[1],0)||!locks.Load(argv[1],0)||!bars.Load(argv[1],0)||!texts.Load(argv[1],0)||!sounds.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();properties.LoadFromDB();locks.LoadFromDB();bars.LoadFromDB();texts.LoadFromDB();sounds.LoadFromDB();
    std::cout.precision(9);std::set<uint32> selected{3365},seen,props;
    for(auto spell:spells)if(spell->ID>=217200&&spell->ID<=217350)selected.insert(spell->ID);
    for(auto effect:effects)if(effect->Effect==90&&effect->EffectMiscValue[0]==109191)selected.insert(effect->SpellID);
    while(seen.size()!=selected.size())
    {
        if(selected.size()>500)return 6;uint32 id=0;for(auto value:selected)if(!seen.count(value)){id=value;break;}seen.insert(id);
        if(auto spell=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';else return 7;
        for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID)
        {
            std::cout<<"EFFECT\t"<<id<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectAuraPeriod<<'\n';
            if(e->EffectTriggerSpell)selected.insert(e->EffectTriggerSpell);if(e->Effect==28)props.insert(e->EffectMiscValue[1]);
        }
        for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\n';}
    }
    for(auto id:props)if(auto p=properties.LookupEntry(id))std::cout<<"PROPERTIES\t"<<id<<'\t'<<p->Control<<'\t'<<p->Title<<'\t'<<p->Slot<<'\t'<<p->Flags<<'\n';
    auto lock=locks.LookupEntry(1691);if(!lock)return 8;
    for(uint32 n=0;n<8;++n)std::cout<<"LOCK\t1691\t"<<n<<'\t'<<unsigned(lock->Type[n])<<'\t'<<lock->Index[n]<<'\t'<<lock->Skill[n]<<'\t'<<unsigned(lock->Action[n])<<'\n';
    for(uint32 id:{380u,433u})if(auto bar=bars.LookupEntry(id))std::cout<<"BAR\t"<<id<<'\t'<<bar->StartPower<<'\t'<<bar->MinPower<<'\t'<<bar->MaxPower<<'\t'<<bar->RegenerationPeace<<'\t'<<bar->RegenerationCombat<<'\t'<<bar->Name->Str[0]<<'\n';
    for(uint32 id=115330;id<=115365;++id)if(auto text=texts.LookupEntry(id))std::cout<<"TEXT\t"<<id<<'\t'<<text->Text->Str[0]<<'\t'<<text->SoundEntriesID[0]<<'\t'<<text->SoundEntriesID[1]<<'\n';
    std::set<uint32> kits{64429};
    for(auto text:texts){std::string t=text->Text->Str[0];
     if(t.find("flavor's improvin")!=std::string::npos||t.find("smelly brew")!=std::string::npos||t.find("Smelly Brew")!=std::string::npos||t.find("flavors improv")!=std::string::npos)std::cout<<"RELATED_TEXT\t"<<text->ID<<'\t'<<t<<'\t'<<text->SoundEntriesID[0]<<'\n';
     if(text->ID>=115345&&text->ID<=115360)for(auto kit:text->SoundEntriesID)if(kit)kits.insert(kit);
    }
    for(auto kit:kits)for(auto sound:sounds)if(sound->SoundKitID==kit)std::cout<<"AUDIO_FILE\t"<<kit<<'\t'<<sound->FileDataID<<'\t'<<unsigned(sound->Frequency)<<'\n';
    HotfixDatabase.Close();std::cout<<"EOF\tFuZanBrewingCatalog\n";
}
