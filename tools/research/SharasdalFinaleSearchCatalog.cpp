// Matching Legion DB2 + read-only hotfix closure, including incoming references.
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
#include <algorithm>
#include <cctype>
int main(int argc,char**argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!check||!check->Fetch()[0].GetBool())return 4;
 DB2Storage<SpellEntry>spells("Spell.db2",SpellLoadInfo::Instance());DB2Storage<SpellEffectEntry>effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());DB2Storage<SpellMiscEntry>misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());DB2Storage<SpellDurationEntry>durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());DB2Storage<SpellCastTimesEntry>times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0)||!times.Load(argv[1],0))return 5;
 spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();times.LoadFromDB();
 std::set<uint32> seed{209457,209487,209552,209796,209903,209983,210247};std::set<uint32>selected=seed;
 for(auto spell:spells){std::string name=spell->Name->Str[0],lower=name;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});if((spell->ID>=209300&&spell->ID<=210300)||lower.find("grash")!=std::string::npos||lower.find("zithreen")!=std::string::npos||lower.find("zitrin")!=std::string::npos||lower.find("ereen")!=std::string::npos)selected.insert(spell->ID);}
 for(bool changed=true;changed;){changed=false;for(auto effect:effects)if(selected.count(effect->SpellID)&&effect->EffectTriggerSpell)changed=selected.insert(effect->EffectTriggerSpell).second||changed;}
 for(auto spell:spells)if(selected.count(spell->ID))std::cout<<"SPELL\t"<<spell->ID<<'\t'<<spell->Name->Str[0]<<'\n';
 uint32 count=0;for(auto effect:effects){++count;bool incoming=seed.count(effect->EffectTriggerSpell);if(incoming)std::cout<<"INCOMING\t"<<effect->SpellID<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<effect->EffectTriggerSpell<<'\n';if(selected.count(effect->SpellID)||incoming)std::cout<<"EFFECT\t"<<effect->SpellID<<'\t'<<unsigned(effect->DifficultyID)<<'\t'<<unsigned(effect->EffectIndex)<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectBasePoints<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectMiscValue[1]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';}
 for(auto row:misc)if(selected.count(row->SpellID)){auto duration=durations.LookupEntry(row->DurationIndex);auto time=times.LookupEntry(row->CastingTimeIndex);std::cout<<"MISC\t"<<row->SpellID<<'\t'<<unsigned(row->DifficultyID)<<'\t'<<(duration?duration->Duration:-2)<<'\t'<<(time?time->Base:-2)<<'\n';}
 std::cout<<"EOF\tspells="<<selected.size()<<"\teffects_scanned="<<count<<'\n';HotfixDatabase.Close();MySQL::Library_End();return 0;
}
