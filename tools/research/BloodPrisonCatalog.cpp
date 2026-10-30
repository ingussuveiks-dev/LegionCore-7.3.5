#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>
int main(int argc,char** argv) {
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
 if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
 if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
 DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 DB2Storage<SummonPropertiesEntry> properties("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
 DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
 DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!properties.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 4;
 spells.LoadFromDB();effects.LoadFromDB();properties.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
 for(uint32 id:{203481u,203480u}){auto s=spells.LookupEntry(id);if(!s)return 5;std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';
  for(auto e:effects)if(e->SpellID==id)std::cout<<"EFFECT\t"<<id<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
 }
 for(auto s:steps)if(s->ScenarioID==940){std::cout<<"STEP\t"<<s->ID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<s->Criteriatreeid<<'\n';
  for(auto t:trees)if(t->CriteriaID){std::set<uint32> visited;uint32 id=t->ID;while(id&&visited.insert(id).second){if(id==s->Criteriatreeid){auto c=criteria.LookupEntry(t->CriteriaID);if(!c)return 6;std::cout<<"CRITERION\t"<<unsigned(s->OrderIndex)<<'\t'<<t->ID<<'\t'<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';break;}auto p=trees.LookupEntry(id);if(!p)break;id=p->Parent;}}
 }
 auto p=properties.LookupEntry(64);if(!p)return 7;
 std::cout<<"SUMMON_PROPERTIES\t"<<p->ID<<'\t'<<p->Control<<'\t'<<p->Faction<<'\t'<<p->Title<<'\t'<<p->Slot<<'\t'<<p->Flags<<'\n';
 HotfixDatabase.Close();
}
