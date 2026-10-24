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
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();
    std::set<uint32> selected{198941,198950,195476,182737};
    for(uint32 identity:selected){auto spell=spells.LookupEntry(identity);if(!spell)return 6;std::cout<<"SPELL\t"<<identity<<'\t'<<spell->Name->Str[0]<<'\n';}
    for(auto effect:effects)if(!effect->DifficultyID&&selected.count(effect->SpellID))
        std::cout<<"EFFECT\t"<<effect->SpellID<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'
            <<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'
            <<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\t'<<effect->EffectMiscValue[1]<<'\n';
    HotfixDatabase.Close();
}
