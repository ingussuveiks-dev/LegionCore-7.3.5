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
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellInterruptsEntry> interrupts("SpellInterrupts.db2",SpellInterruptsLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!times.Load(argv[1],0)||!texts.Load(argv[1],0)||!interrupts.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();times.LoadFromDB();texts.LoadFromDB();interrupts.LoadFromDB();
    std::set<uint32> selected;
    for(auto e:effects)if(e->EffectMiscValue[0]==105332||e->EffectMiscValue[0]==105333||e->EffectMiscValue[0]==105434){selected.insert(e->SpellID);std::cout<<"CONTACT\t"<<e->SpellID<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\n';}
    for(auto s:spells){std::string name=s->Name->Str[0];if(name.find("Search")!=std::string::npos||name.find("Desmond")!=std::string::npos||name.find("Gravesorrow")!=std::string::npos)std::cout<<"NAMED\t"<<s->ID<<'\t'<<name<<'\n';}
    std::set<uint32> seen;
    while(seen.size()!=selected.size()){
        if(selected.size()>100)return 5;uint32 id=0;for(auto value:selected)if(!seen.count(value)){id=value;break;}seen.insert(id);
        auto s=spells.LookupEntry(id);if(!s){std::cout<<"MISSING\t"<<id<<'\n';continue;}
        std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id){std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';if(e->EffectTriggerSpell)selected.insert(e->EffectTriggerSpell);}
        for(auto m:misc)if(m->SpellID==id){auto t=times.LookupEntry(m->CastingTimeIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(t?t->Base:0)<<'\t'<<m->RangeIndex<<'\n';std::cout<<"ATTRIBUTES\t"<<id;for(auto a:m->Attributes)std::cout<<'\t'<<a;std::cout<<'\n';}
        for(auto i:interrupts)if(i->SpellID==id)std::cout<<"INTERRUPTS\t"<<id<<'\t'<<unsigned(i->DifficultyID)<<'\t'<<i->InterruptFlags<<'\n';
    }
    for(uint32 id:{108073u,108074u,108075u,108087u,108099u,108103u,108105u,108106u,108107u,108108u,108119u})if(auto t=texts.LookupEntry(id))std::cout<<"TEXT\t"<<id<<'\t'<<t->Text->Str[0]<<'\t'<<t->ConditionID<<'\n';
    HotfixDatabase.Close();
}
