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
    if(argc!=2) return 1;
    std::string error; auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config || !sConfigMgr->LoadInitial(config,{},error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)) return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();
    std::set<uint32> selected{198902,196500,202475,202481};
    for(auto spell:spells)
    {
        std::string name=spell->Name->Str[0];
        if(name.find("Uldum")!=std::string::npos || name.find("Li Li")!=std::string::npos || name.find("Crane")!=std::string::npos || name.find("Kite")!=std::string::npos)
            std::cout<<"NAMED\t"<<spell->ID<<'\t'<<name<<'\n';
    }
    std::set<uint32> seen;
    while(seen.size()!=selected.size())
    {
        if(selected.size()>100) return 5;
        uint32 id=0;for(auto value:selected)if(!seen.count(value)){id=value;break;}seen.insert(id);
        auto spell=spells.LookupEntry(id);if(!spell){std::cout<<"MISSING\t"<<id<<'\n';continue;}
        std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id)
        {
            std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'
                <<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\t'<<e->EffectAuraPeriod<<'\n';
            if(e->EffectTriggerSpell) selected.insert(e->EffectTriggerSpell);
        }
        for(auto m:misc)if(m->SpellID==id){auto duration=durations.LookupEntry(m->DurationIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(duration?duration->Duration:0)<<'\n';}
    }
    HotfixDatabase.Close();
}
