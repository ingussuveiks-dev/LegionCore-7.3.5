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
 DB2Storage<BattlePetBreedStateEntry> breeds("BattlePetBreedState.db2",BattlePetBreedStateLoadInfo::Instance());
 DB2Storage<BattlePetSpeciesStateEntry> species("BattlePetSpeciesState.db2",BattlePetSpeciesStateLoadInfo::Instance());
 DB2Storage<BattlePetBreedQualityEntry> quality("BattlePetBreedQuality.db2",BattlePetBreedQualityLoadInfo::Instance());
 if(!breeds.Load(argv[1],0)||!species.Load(argv[1],0)||!quality.Load(argv[1],0))return 4;
 breeds.LoadFromDB();species.LoadFromDB();quality.LoadFromDB();
 for(auto s:breeds)if(s->BreedID<13)std::cout<<"BREED "<<unsigned(s->BreedID)<<' '<<unsigned(s->BattlePetStateID)<<' '<<s->Value<<'\n';
 for(auto s:species)if(s->SpeciesID==1532)std::cout<<"SPECIES "<<s->SpeciesID<<' '<<unsigned(s->BattlePetStateID)<<' '<<s->Value<<'\n';
 for(auto s:quality)std::cout<<"QUALITY "<<s->ID<<' '<<unsigned(s->QualityEnum)<<' '<<s->StateMultiplier<<'\n';
 HotfixDatabase.Close();MySQL::Library_End();return 0;
}
