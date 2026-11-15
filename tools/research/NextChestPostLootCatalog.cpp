#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <array>

// Read-only matching735 evidence for native chest post-loot spell references.
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0))return 5;
    spells.LoadFromDB();effects.LoadFromDB();
    for(uint32 id:std::array<uint32,4>{179109,240362,240402,254576})
    {
        auto spell=spells.LookupEntry(id);size_t count=0;
        for(auto effect:effects)if(effect->SpellID==id)++count;
        std::cout<<"SPELL\t"<<id<<'\t'<<(spell?"PRESENT":"ABSENT")<<'\t'<<count<<'\t'<<(spell?spell->Name->Str[0]:"<missing>")<<'\n';
    }
    HotfixDatabase.Close();std::cout<<"EOF\tNextChestPostLootCatalog\n";return 0;
}
