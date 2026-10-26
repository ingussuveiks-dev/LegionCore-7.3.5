#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 4;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 5;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if (!readOnly || !readOnly->Fetch()[0].GetBool()) return 6;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2", SpellEffectLoadInfo::Instance());
    if (!effects.Load(argv[1], 0)) return 2;
    effects.LoadFromDB();


    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> duration("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!misc.Load(argv[1],0)||!duration.Load(argv[1],0))return 7;
    spells.LoadFromDB();misc.LoadFromDB();duration.LoadFromDB();
    std::set<uint32> selected{209231,209238,209264,211686,211684,189170};
    for(unsigned depth=0;depth<4;++depth)for(auto e:effects)if(selected.count(e->SpellID)&&e->EffectTriggerSpell)selected.insert(e->EffectTriggerSpell);
    for(auto id:selected)if(auto spell=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';else return 8;
    for(auto e:effects)if(selected.count(e->SpellID)&&!e->DifficultyID)
        std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
    for(auto m:misc)if(selected.count(m->SpellID)&&!m->DifficultyID){auto d=duration.LookupEntry(m->DurationIndex);std::cout<<"DURATION\t"<<m->SpellID<<'\t'<<(d?d->Duration:0)<<'\n';}
    return 0;
}
