#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!check||!check->Fetch()[0].GetBool())return 4;
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SummonPropertiesEntry> properties("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
 DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());
 DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());
 if(!effects.Load(argv[1],0)||!properties.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!seats.Load(argv[1],0))return 5;
 effects.LoadFromDB();properties.LoadFromDB();vehicles.LoadFromDB();seats.LoadFromDB();
 for(auto e:effects)if(!e->DifficultyID&&(e->SpellID==178923||e->SpellID==180768||e->SpellID==46598))std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->ImplicitTarget[0]<<'\n';
 if(auto p=properties.LookupEntry(3635))std::cout<<"PROPERTIES\t"<<p->ID<<'\t'<<p->Control<<'\t'<<p->Flags<<'\n';
 if(auto v=vehicles.LookupEntry(4103))for(unsigned i=0;i<8;++i)if(v->SeatID[i])if(auto s=seats.LookupEntry(v->SeatID[i]))std::cout<<"SEAT\t"<<i<<'\t'<<s->ID<<'\t'<<s->Flags<<'\t'<<s->FlagsB<<'\n';
}
