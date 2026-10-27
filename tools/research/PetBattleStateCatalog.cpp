#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char**argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<BattlePetStateEntry> states("BattlePetState.db2",BattlePetStateLoadInfo::Instance());
 if(!states.Load(argv[1],0))return 4;states.LoadFromDB();
 auto dead=states.LookupEntry(1);if(!dead||!dead->Flags)return 5;
 for(uint32 id:{1u,18u,19u,20u,42u,50u})if(auto s=states.LookupEntry(id))std::cout<<"STATE\t"<<s->ID<<'\t'<<s->Flags<<'\t'<<s->BattlePetVisualID<<'\t'<<s->LuaName->Str[0]<<'\n';
 HotfixDatabase.Close();MySQL::Library_End();return 0;
}
