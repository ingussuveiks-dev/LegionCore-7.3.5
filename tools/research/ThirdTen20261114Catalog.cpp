#include <unordered_set>
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2Stores.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char**argv)
{
 if(argc!=2)return 1;
 auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 if(!sCreatureDisplayInfoStore.Load(argv[1],0)||!sPhaseStore.Load(argv[1],0)||!sCurrencyTypesStore.Load(argv[1],0))return 4;
 sCreatureDisplayInfoStore.LoadFromDB();sPhaseStore.LoadFromDB();sCurrencyTypesStore.LoadFromDB();
 uint32 displays=0,phases=0,plain=0,precise=0;
 for(auto entry:sCreatureDisplayInfoStore){if(!entry->ID)return 5;++displays;}
 for(auto entry:sPhaseStore){if(!entry->ID)return 6;++phases;}
 for(auto entry:sCurrencyTypesStore){float factor=DB2Manager::GetCurrencyPrecision(entry->ID);if(factor==1.f)++plain;else if(factor==100.f)++precise;else return 7;}
 if(!displays||!phases||!plain||!precise)return 8;
 std::cout<<"MATCHING26972\t"<<displays<<" displays\t"<<phases<<" phases\t"<<plain<<" plain currencies\t"<<precise<<" precision100 currencies\n";
 std::cout<<"PASS actual native DB2/Hotfix admissions and actual DB2Manager precision1/100;read-only Hotfix,no character/world mutation\n";
 HotfixDatabase.Close();return 0;
}
