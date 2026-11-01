#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>

// Matching 26972 lock/key records, with the actual read-only Hotfix overlay.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!locks.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0))return 4;
    locks.LoadFromDB();spells.LoadFromDB();effects.LoadFromDB();
    auto lock=locks.LookupEntry(2570);if(!lock)return 5;
    for(uint32 n=0;n<8;++n)
        std::cout<<"LOCK\t2570\t"<<n<<'\t'<<unsigned(lock->Type[n])<<'\t'<<lock->Index[n]<<'\t'<<lock->Skill[n]<<'\t'<<unsigned(lock->Action[n])<<'\n';
    for(uint32 id:{218174u,3365u})
    {
        auto spell=spells.LookupEntry(id);if(!spell)return 6;
        std::cout<<"SPELL\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto effect:effects)if(effect->SpellID==id)
            std::cout<<"EFFECT\t"<<id<<'\t'<<effect->DifficultyID<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    }
    HotfixDatabase.Close();MySQL::Library_End();
}
