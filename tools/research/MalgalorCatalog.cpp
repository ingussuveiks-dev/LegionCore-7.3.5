#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <iostream>
#include <cstdlib>
#include <set>
int main(int argc,char**argv){if(argc!=2)return 1;std::string err;auto c=std::getenv("LEGION_DIALOGUE_CONFIG");if(!c||!sConfigMgr->LoadInitial(c,{},err))return 2;MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
DB2Storage<ScenarioEntry> scenarios("Scenario.db2",ScenarioLoadInfo::Instance());DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());DB2Storage<LFGDungeonsEntry> dungeons("LFGDungeons.db2",LfgDungeonsLoadInfo::Instance());
if(!scenarios.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!dungeons.Load(argv[1],0))return 4;scenarios.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();dungeons.LoadFromDB();std::set<uint32> roots;
for(auto s:scenarios)if(s->ID==775||s->ID==933||s->ID==961)std::cout<<"SCENARIO\t"<<s->ID<<'\t'<<s->Name->Str[0]<<'\n';
for(auto s:steps)if(s->ScenarioID==933)std::cout<<"STEP_REWARD\t"<<unsigned(s->OrderIndex)<<'\t'<<s->RewardQuestID<<'\n';for(auto s:steps)if(s->ScenarioID==775||s->ScenarioID==933||s->ScenarioID==961){roots.insert(s->Criteriatreeid);std::cout<<"STEP\t"<<s->ScenarioID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<s->Criteriatreeid<<'\t'<<s->Title->Str[0]<<'\t'<<s->Description->Str[0]<<'\n';}
for(int i=0;i<8;++i)for(auto t:trees)if(roots.count(t->Parent))roots.insert(t->ID);
for(auto t:trees)if(roots.count(t->ID))if(auto c=criteria.LookupEntry(t->CriteriaID))std::cout<<"CRITERIA\t"<<t->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';
for(auto d:dungeons)if(d->MapID==1500)std::cout<<"LFG\t"<<d->ID<<'\t'<<d->ScenarioID<<'\t'<<unsigned(d->DifficultyID)<<'\t'<<d->Name->Str[0]<<'\n';DB2Storage<MapDifficultyEntry> difficulty("MapDifficulty.db2",MapDifficultyLoadInfo::Instance());if(!difficulty.Load(argv[1],0))return 5;difficulty.LoadFromDB();for(auto d:difficulty)if(d->MapID==1500)std::cout<<"MAP_DIFFICULTY\t"<<d->MapID<<'\t'<<unsigned(d->DifficultyID)<<'\n';DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());DB2Storage<SpellDurationEntry> durations("SpellDuration.db2",SpellDurationLoadInfo::Instance());
if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!misc.Load(argv[1],0)||!durations.Load(argv[1],0))return 6;spells.LoadFromDB();effects.LoadFromDB();misc.LoadFromDB();durations.LoadFromDB();std::set<uint32> wanted={197593,186382,191979,185714};
for(int pass=0;pass<4;++pass)for(auto e:effects)if(wanted.count(e->SpellID)&&e->EffectTriggerSpell>0)wanted.insert(e->EffectTriggerSpell);for(auto e:spells)if(wanted.count(e->ID))std::cout<<"SPELL\t"<<e->ID<<'\t'<<e->Name->Str[0]<<'\n';
for(auto e:effects)if(wanted.count(e->SpellID))std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectMiscValue[1]<<'\t'<<e->EffectTriggerSpell<<'\t'<<unsigned(e->ImplicitTarget[0])<<'\t'<<unsigned(e->ImplicitTarget[1])<<'\n';
for(auto e:misc)if(wanted.count(e->SpellID)&&!e->DifficultyID)if(auto d=durations.LookupEntry(e->DurationIndex))std::cout<<"DURATION\t"<<e->SpellID<<'\t'<<d->Duration<<'\t'<<d->MaxDuration<<'\n';HotfixDatabase.Close();MySQL::Library_End();return 0;}