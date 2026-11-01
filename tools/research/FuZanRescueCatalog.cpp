#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

// Matching26972 combat/control and sanctuary lock records; read-only Hotfix.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    DB2Storage<LockTypeEntry> kinds("LockType.db2",LockTypeLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||
       !locks.Load(argv[1],0)||!kinds.Load(argv[1],0))return 4;
    spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();locks.LoadFromDB();kinds.LoadFromDB();
    auto lock=locks.LookupEntry(2570);if(!lock)return 5;
    for(uint32 n=0;n<8;++n)
    {
        std::cout<<"LOCK\t2570\t"<<n<<'\t'<<unsigned(lock->Type[n])<<'\t'<<lock->Index[n]<<'\t'<<lock->Skill[n]<<'\t'<<unsigned(lock->Action[n]);
        auto kind=lock->Type[n]==2?kinds.LookupEntry(lock->Index[n]):nullptr;
        std::cout<<'\t'<<(kind?kind->Name->Str[0]:"")<<'\n';
    }
    std::set<uint32> selected{3365,213527,218174,205660,203373,202703,202821,219583,219426},seen;
    for(auto e:effects)if(e->Effect==61&&(e->EffectMiscValue[0]==52435||e->EffectMiscValue[0]==52443||e->EffectMiscValue[0]==52609))selected.insert(e->SpellID);
    while(seen.size()!=selected.size())
    {
        if(selected.size()>100)return 6;
        uint32 id=0;for(auto candidate:selected)if(!seen.count(candidate)){id=candidate;break;}seen.insert(id);
        auto spell=spells.LookupEntry(id);if(!spell)return 7;
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
            std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:0)<<'\t'<<m->CastingTimeIndex<<'\t'<<m->RangeIndex<<'\t'<<m->Speed<<'\t'<<m->LaunchDelay;
            for(auto flags:m->Attributes)std::cout<<'\t'<<uint32(flags);
            std::cout<<'\n';
        }
    }
    HotfixDatabase.Close();MySQL::Library_End();
}
