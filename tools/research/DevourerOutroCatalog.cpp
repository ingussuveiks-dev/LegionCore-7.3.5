#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc,char**argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");auto read=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!read||!read->Fetch()[0].GetBool())return 4;
#define STORE(T,N,F,I) DB2Storage<T> N(F ".db2",I::Instance());if(!N.Load(argv[1],0))return 5;N.LoadFromDB()
 STORE(SpellEntry,spells,"Spell",SpellLoadInfo);STORE(SpellEffectEntry,effects,"SpellEffect",SpellEffectLoadInfo);STORE(SpellMiscEntry,misc,"SpellMisc",SpellMiscLoadInfo);STORE(SpellDurationEntry,durations,"SpellDuration",SpellDurationLoadInfo);STORE(SpellCastTimesEntry,times,"SpellCastTimes",SpellCastTimesLoadInfo);STORE(SpellRangeEntry,ranges,"SpellRange",SpellRangeLoadInfo);
#undef STORE
 std::set<uint32> selected{209813,209814};for(bool changed=true;changed;){changed=false;for(auto e:effects)if(selected.count(e->SpellID)&&e->EffectTriggerSpell)changed=selected.insert(e->EffectTriggerSpell).second||changed;}
 for(auto id:selected){auto s=spells.LookupEntry(id);if(!s)return 6;std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';
  for(auto e:effects)if(e->SpellID==id)std::cout<<"EFFECT\t"<<id<<'\t'<<unsigned(e->DifficultyID)<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectAuraPeriod<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
  for(auto m:misc)if(m->SpellID==id){auto d=durations.LookupEntry(m->DurationIndex);auto t=times.LookupEntry(m->CastingTimeIndex);auto range=ranges.LookupEntry(m->RangeIndex);std::cout<<"MISC\t"<<id<<'\t'<<unsigned(m->DifficultyID)<<'\t'<<(d?d->Duration:-2)<<'\t'<<(t?t->Base:-2)<<'\t'<<(range?range->RangeMax[0]:-2);for(auto a:m->Attributes)std::cout<<'\t'<<uint32(a);std::cout<<'\n';}
 }
 std::cout<<"EOF\tdevourer-outro\n";HotfixDatabase.Close();MySQL::Library_End();return 0;
}
