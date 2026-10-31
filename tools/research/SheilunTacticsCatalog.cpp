#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

// Read matching client records and readonly Hotfix; do not infer tactics from
// a spell's name alone. Retain descriptions, numeric aura policy and triggers.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<SpellAuraOptionsEntry> options("SpellAuraOptions.db2",SpellAuraOptionsLoadInfo::Instance());
    DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
    DB2Storage<SpellRangeEntry> ranges("SpellRange.db2",SpellRangeLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!options.Load(argv[1],0)||!times.Load(argv[1],0)||!ranges.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();options.LoadFromDB();times.LoadFromDB();ranges.LoadFromDB();
    auto text=[](char const* value){std::string result;for(char c:std::string(value?value:""))if(c=='\n')result+="\\n";else if(c=='\r')result+="\\r";else if(c=='\t')result+="\\t";else result+=c;return result;};
    std::set<uint32> selected{60958,203836,204624,204593,203832,203831,203869,203867,203865,131995,80016};
    uint32 total=0,matched=0;
    for(auto spell:spells)
    {
        ++total;std::string content=std::string(spell->Name->Str[0])+" "+spell->Description->Str[0]+" "+spell->AuraDescription->Str[0];
        std::transform(content.begin(),content.end(),content.begin(),[](unsigned char c){return char(std::tolower(c));});
        bool named=content.find("fei li")!=std::string::npos||content.find("nurong")!=std::string::npos||content.find("taoshi")!=std::string::npos||content.find("shado-plan")!=std::string::npos;
        bool window=spell->ID>=202000&&spell->ID<=205000&&(content.find("defensiv")!=std::string::npos||content.find("aggress")!=std::string::npos||content.find("reckless")!=std::string::npos||content.find("offensiv")!=std::string::npos);
        if(named||window){++matched;selected.insert(spell->ID);std::cout<<"CANDIDATE\t"<<spell->ID<<'\t'<<named<<'\t'<<window<<'\n';}
    }
    for(unsigned depth=0;depth<4;++depth)for(auto effect:effects)if(selected.count(effect->SpellID)&&effect->EffectTriggerSpell)selected.insert(effect->EffectTriggerSpell);
    for(auto id:selected)
    {
        if(auto spell=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<text(spell->Name->Str[0])<<'\t'<<text(spell->NameSubtext->Str[0])<<'\t'<<text(spell->Description->Str[0])<<'\t'<<text(spell->AuraDescription->Str[0])<<'\n';
        for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID)
            std::cout<<"EFFECT\t"<<id<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectAuraPeriod<<'\n';
        for(auto m:misc)if(m->SpellID==id&&!m->DifficultyID)
        {
            if(auto d=durations.LookupEntry(m->DurationIndex))std::cout<<"DURATION\t"<<id<<'\t'<<d->Duration<<'\t'<<d->MaxDuration<<'\n';
            if(auto t=times.LookupEntry(m->CastingTimeIndex))std::cout<<"CAST\t"<<id<<'\t'<<t->Base<<'\t'<<t->Minimum<<'\n';
            if(auto p=ranges.LookupEntry(m->RangeIndex))std::cout<<"RANGE\t"<<id<<'\t'<<p->RangeMin[0]<<'\t'<<p->RangeMax[0]<<'\t'<<p->RangeMin[1]<<'\t'<<p->RangeMax[1]<<'\n';
            std::cout<<"ATTRIBUTES\t"<<id;for(auto a:m->Attributes)std::cout<<'\t'<<uint32(a);std::cout<<'\n';
        }
        for(auto o:options)if(o->SpellID==id&&!o->DifficultyID)std::cout<<"AURA_OPTIONS\t"<<id<<'\t'<<o->CumulativeAura<<'\t'<<o->ProcCharges<<'\t'<<o->ProcChance<<'\t'<<o->ProcTypeMask<<'\n';
    }
    std::cout<<"EOF\tsheilun-tactics\t"<<total<<'\t'<<matched<<'\t'<<selected.size()<<'\n';
    HotfixDatabase.Close();MySQL::Library_End();
}
