#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    DB2Storage<LockTypeEntry> types("LockType.db2",LockTypeLoadInfo::Instance());
    DB2Storage<MapEntry> maps("Map.db2",MapLoadInfo::Instance());
    if(!locks.Load(argv[1],0)||!types.Load(argv[1],0)||!maps.Load(argv[1],0))return 5;
    locks.LoadFromDB();types.LoadFromDB();maps.LoadFromDB();
    auto map=maps.LookupEntry(1541);auto lock=locks.LookupEntry(1691);if(!map||!lock)return 6;
    std::cout<<"MAP\t"<<map->ID<<'\t'<<unsigned(map->InstanceType)<<'\t'<<map->IsDungeon()<<'\t'<<map->IsNonRaidDungeon()<<'\t'<<map->IsScenario()<<'\t'<<map->Instanceable()<<'\n';
    for(unsigned i=0;i<8;++i)
    {
        std::cout<<"LOCK\t1691\t"<<i<<'\t'<<unsigned(lock->Type[i])<<'\t'<<lock->Index[i]<<'\t'<<lock->Skill[i]<<'\t'<<unsigned(lock->Action[i])<<'\n';
        if(lock->Type[i]==2)if(auto type=types.LookupEntry(lock->Index[i]))
            std::cout<<"LOCK_TYPE\t"<<type->ID<<'\t'<<type->Name->Str[0]<<'\t'<<type->ResourceName->Str[0]<<'\n';
    }
    std::cout<<"EOF\tsheilun-loot\n";HotfixDatabase.Close();MySQL::Library_End();
}
