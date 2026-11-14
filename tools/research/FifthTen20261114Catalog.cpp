#include <unordered_set>
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "DB2Stores.h"
#include "GridDefines.h"
#include "ObjectGuid.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <limits>
int main(int argc,char**argv)
{
 if(argc!=2)return 1;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 if(!sWorldSafeLocsStore.Load(argv[1],0)||!sTaxiNodesStore.Load(argv[1],0)||!sAreaTriggerStore.Load(argv[1],0)||!sAreaTableStore.Load(argv[1],0)||!sMapStore.Load(argv[1],0))return 4;
 sWorldSafeLocsStore.LoadFromDB();sTaxiNodesStore.LoadFromDB();sAreaTriggerStore.LoadFromDB();sAreaTableStore.LoadFromDB();sMapStore.LoadFromDB();
 ObjectGuid::LowType const wide=(uint64(1)<<32)+7;
 static_assert(sizeof(ObjectGuid::LowType)==8,"Native spawn counter width");
 if(ObjectGuid::Create<HighGuid::Creature>(1,99,wide).GetCounter()!=wide)return 7;
 uint32 safe=0,taxi=0,triggers=0,areas=0,maps=0,validParents=0;
 for(auto e:sWorldSafeLocsStore)++safe;for(auto e:sTaxiNodesStore)++taxi;for(auto e:sAreaTriggerStore)++triggers;for(auto e:sAreaTableStore){++areas;if(e->ParentAreaID&&sAreaTableStore.LookupEntry(e->ParentAreaID))++validParents;}for(auto e:sMapStore)++maps;
 if(!safe||!taxi||!triggers||!areas||!maps||!validParents)return 5;
 if(!Trinity::IsValidMapCoord(1.f,2.f,20000.f)||Trinity::IsValidMapCoord(1.f,2.f,std::numeric_limits<float>::quiet_NaN())||Trinity::IsValidMapCoord(20000.f,2.f))return 6;
 std::cout<<"MATCHING26972\t"<<safe<<" safe locations\t"<<taxi<<" taxi nodes\t"<<triggers<<" triggers\t"<<areas<<" areas\t"<<maps<<" maps\t"<<validParents<<" resolved child areas\n";
 std::cout<<"PASS native33-bit Creature GUID counter through64-bit LowType and actual coordinate overloads accept finite high Z,limit XY and reject NaN;matching native stores read-only Hotfix. No real travel/character/world mutation\n";
 HotfixDatabase.Close();return 0;
}
