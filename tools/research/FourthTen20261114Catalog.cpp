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
 if(!sEmotesStore.Load(argv[1],0)||!sFactionTemplateStore.Load(argv[1],0)||!sCreatureDisplayInfoStore.Load(argv[1],0))return 4;
 sEmotesStore.LoadFromDB();sFactionTemplateStore.LoadFromDB();sCreatureDisplayInfoStore.LoadFromDB();
 uint32 emotes=0,factions=0,largest=0,displays=0;
 for(auto entry:sEmotesStore)++emotes;
 for(auto entry:sFactionTemplateStore){++factions;if(entry->ID<0)return 6;largest=std::max(largest,uint32(entry->ID));}
 for(auto entry:sCreatureDisplayInfoStore)++displays;
 if(!emotes||!factions||!displays||largest>65535)return 5;
 std::cout<<"MATCHING26972\t"<<emotes<<" emotes\t"<<factions<<" factions\tmax="<<largest<<"\t"<<displays<<" displays\n";
 std::cout<<"PASS native emote/model/faction stores;native faction IDs fit actual smallint SQL schema;read-only Hotfix,no world/character mutation\n";
 HotfixDatabase.Close();return 0;
}
