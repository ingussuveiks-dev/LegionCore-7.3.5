// Matching26972 DB2/Hotfix and read-only world runestone evidence;no character access.
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
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();
 WorldDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("WorldDatabaseInfo",""),1,1);
 HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(WorldDatabase.Open()||!WorldDatabase.PrepareStatements()||HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");WorldDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 for(auto query:{"SELECT entry,type,name,Data0,Data1,Data2,Data3,Data4,Data5,Data6,Data7,Data8,Data9,Data10,Data11,Data12,flags,ScriptName FROM gameobject_template WHERE entry=244940",
 "SELECT guid,id,map,spawnMask,phaseMask,PhaseId,state FROM gameobject WHERE id=244940 AND map=1511",
 "SELECT entryorguid,source_type,event_type,event_param1,event_param2,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid IN(244940,98524) AND source_type IN(0,1)",
 "SELECT id,delay,command,datalong,datalong2,dataint FROM event_scripts WHERE id=46970",
 "SELECT id,delay,command,datalong,datalong2,dataint FROM spell_scripts WHERE id IN(194802,8386)",
 "SELECT spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(194802,8386)"}){
 auto rows=WorldDatabase.Query(query);if(!rows){std::cout<<"EMPTY_OPTIONAL_QUERY\n";continue;}
 do{auto f=rows->Fetch();std::cout<<"ROW";for(uint32 n=0;n<rows->GetFieldCount();++n)std::cout<<'\t'<<f[n].GetString();std::cout<<'\n';}while(rows->NextRow());
 }
 DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());DB2Storage<SpellCastTimesEntry> times("SpellCastTimes.db2",SpellCastTimesLoadInfo::Instance());
 if(!locks.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 5;
 locks.LoadFromDB();spells.LoadFromDB();effects.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
 auto usage=WorldDatabase.Query("SELECT COUNT(*) FROM quest_objectives WHERE Type=2 AND ObjectID=244940");if(!usage)return 9;std::cout<<"OTHER_QUEST_GO_OBJECTIVES\t"<<usage->Fetch()[0].GetUInt32()<<'\n';
 auto bindings=WorldDatabase.Query("SELECT entry,type FROM gameobject_template WHERE ScriptName='go_prison_runestone'");if(!bindings)return 10;do{auto b=bindings->Fetch();std::cout<<"BINDING\t"<<b[0].GetUInt32()<<'\t'<<b[1].GetUInt32()<<'\n';}while(bindings->NextRow());
 std::set<uint32> parents;for(auto t:trees)if(t->CriteriaID==29081){auto p=t;while(p&&parents.insert(p->ID).second)p=trees.LookupEntry(p->Parent);}
 for(auto step:steps)if(parents.count(step->Criteriatreeid))std::cout<<"EVENT_USE\t"<<step->ScenarioID<<'\t'<<step->ID<<'\n';
 if(!misc.Load(argv[1],0)||!times.Load(argv[1],0))return 8;misc.LoadFromDB();times.LoadFromDB();
 std::set<uint32> ids{3365};for(auto e:effects)if(e->Effect==33&&e->EffectMiscValue[0]==14)ids.insert(e->SpellID);
 auto data=WorldDatabase.Query("SELECT Data0,Data10 FROM gameobject_template WHERE entry=244940");if(!data)return 6;
 auto f=data->Fetch();if(f[1].GetUInt32())ids.insert(f[1].GetUInt32());auto lock=locks.LookupEntry(f[0].GetUInt32());
 if(lock)for(uint32 n=0;n<8;++n){std::cout<<"LOCK\t"<<lock->ID<<'\t'<<n<<'\t'<<unsigned(lock->Type[n])<<'\t'<<lock->Index[n]<<'\t'<<lock->Skill[n]<<'\n';if(lock->Type[n]==3)ids.insert(lock->Index[n]);}
 for(auto id:ids){auto s=spells.LookupEntry(id);if(!s)return 7;std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id)std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';for(auto m:misc)if(m->SpellID==id){auto t=times.LookupEntry(m->CastingTimeIndex);std::cout<<"MISC\t"<<id<<'\t'<<uint32(m->Attributes[2])<<'\t'<<m->CastingTimeIndex<<'\t'<<(t?t->Base:-1)<<'\n';}}
 for(auto step:steps)if(step->ScenarioID==944&&step->OrderIndex==3){std::cout<<"STEP\t"<<step->ID<<'\t'<<step->Criteriatreeid<<'\n';for(auto tree:trees)if(tree->Parent==step->Criteriatreeid||tree->ID==step->Criteriatreeid)if(auto c=criteria.LookupEntry(tree->CriteriaID))std::cout<<"CRITERIA\t"<<tree->ID<<'\t'<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\t'<<tree->Amount<<'\n';}
 WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
}
