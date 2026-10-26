#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <vector>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;
    auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 4;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 5;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if(!check||!check->Fetch()[0].GetBool())return 6;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<OverrideSpellDataEntry> overrides("OverrideSpellData.db2",OverrideSpellDataLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!overrides.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0))return 2;
    spells.LoadFromDB();effects.LoadFromDB();overrides.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();
    std::vector<uint32> spellIds={180862u,216683u,216687u,216690u,216691u,216813u,217042u,52391u};
    auto actionBar=overrides.LookupEntry(735);if(!actionBar)return 7;
    for(auto spellId:actionBar->Spells)if(spellId){std::cout<<"OVERRIDE\t735\t"<<spellId<<'\n';spellIds.push_back(spellId);}
    for(uint32 spellId:spellIds)
    {
        auto spell=spells.LookupEntry(spellId);
        if(!spell)
        {
            std::cout<<"SPELL_MISSING\t"<<spellId<<'\n';
            if(spellId!=216687)return 3;
        }
        else std::cout<<"SPELL\t"<<spellId<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto effect:effects)if(effect->SpellID==spellId)
            std::cout<<"EFFECT\t"<<spellId<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
        for(auto entry:misc)if(entry->SpellID==spellId)
        {
            auto duration=durations.LookupEntry(entry->DurationIndex);
            std::cout<<"MISC\t"<<spellId<<'\t'<<unsigned(entry->DifficultyID)<<'\t'<<entry->DurationIndex<<'\t'<<(duration?duration->Duration:0)<<'\t'<<entry->Attributes[0]<<'\t'<<entry->Attributes[1]<<'\n';
        }
    }
}
