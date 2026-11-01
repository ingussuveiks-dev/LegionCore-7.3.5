#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

// Matching26972 spell/Hotfix records, read-only; no World/character writes.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();
    std::set<uint32> selected{217817,218883,218882},seen;
    while(seen.size()!=selected.size())
    {
        if(selected.size()>100)return 5;
        uint32 id=0;for(auto candidate:selected)if(!seen.count(candidate)){id=candidate;break;}seen.insert(id);
        auto spell=spells.LookupEntry(id);if(!spell)return 6;
        std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id)
        {
            std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'
                <<e->EffectBasePoints<<'\t'<<e->EffectAuraPeriod<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'
                <<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
            if(e->EffectTriggerSpell)selected.insert(e->EffectTriggerSpell);
        }
        for(auto m:misc)if(m->SpellID==id)
        {
            auto d=durations.LookupEntry(m->DurationIndex);
            std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\t'<<m->CastingTimeIndex<<'\t'<<m->RangeIndex;
            for(auto flags:m->Attributes)std::cout<<'\t'<<uint32(flags);
            std::cout<<'\n';
        }
    }
    HotfixDatabase.Close();MySQL::Library_End();
}
