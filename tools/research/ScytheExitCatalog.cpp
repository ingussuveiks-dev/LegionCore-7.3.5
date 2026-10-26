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
 DB2Storage<QuestObjectiveEntry> objectives("QuestObjective.db2",QuestObjectiveLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
 DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 if(!objectives.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0))return 5;
 objectives.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();
 uint32 count=0;for(auto obj:objectives)if(obj->QuestID==37660)++count;std::cout<<"OBJECTIVE_COUNT\t"<<count<<'\n';
 for(auto e:effects)if(!e->DifficultyID&&(e->SpellID==179183||e->SpellID==179182))std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
 for(auto m:misc)if(!m->DifficultyID&&(m->SpellID==179183||m->SpellID==179182)){auto d=durations.LookupEntry(m->DurationIndex);std::cout<<"MISC\t"<<m->SpellID<<'\t'<<m->DurationIndex<<'\t'<<(d?d->Duration:0)<<'\t'<<(d?d->MaxDuration:0)<<'\n';}
}
