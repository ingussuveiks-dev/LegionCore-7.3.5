#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
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
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> summons("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!criteria.Load(argv[1],0)||!summons.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();criteria.LoadFromDB();summons.LoadFromDB();
    for(auto effect:effects)if(!effect->DifficultyID&&effect->Effect==28&&effect->EffectMiscValue[0]==99431){
        auto spell=spells.LookupEntry(effect->SpellID);if(!spell)return 6;
        std::cout<<"SUMMON\t"<<effect->SpellID<<'\t'<<spell->Name->Str[0]<<'\t'<<effect->EffectIndex<<'\t'
            <<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\t'<<effect->EffectMiscValue[1]<<'\n';
    }
    auto criterion=criteria.LookupEntry(29205);if(!criterion)return 7;
    std::cout<<"CRITERION\t"<<criterion->ID<<'\t'<<uint32(criterion->Type)<<'\t'<<criterion->Asset<<'\n';
    auto summon=summons.LookupEntry(61);if(!summon)return 8;
    std::cout<<"BOARDING_SUMMON\t61\t"<<summon->Control<<'\t'<<summon->Title<<'\t'<<summon->Flags<<'\n';
    auto harpoon=summons.LookupEntry(64);if(!harpoon)return 9;
    std::cout<<"HARPOON_SUMMON\t64\t"<<harpoon->Control<<'\t'<<harpoon->Title<<'\t'<<harpoon->Flags<<'\n';
    HotfixDatabase.Close();
}
