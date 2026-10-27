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
int main(int argc,char** argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
 DB2Storage<ScenarioEntry> scenarios("Scenario.db2",ScenarioLoadInfo::Instance());DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());DB2Storage<LFGDungeonsEntry> lfg("LFGDungeons.db2",LfgDungeonsLoadInfo::Instance());DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
 if(!scenarios.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!lfg.Load(argv[1],0)||!effects.Load(argv[1],0)||!spells.Load(argv[1],0))return 5;
 scenarios.LoadFromDB();steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();lfg.LoadFromDB();effects.LoadFromDB();spells.LoadFromDB();
 std::set<uint32> selected;
 for(auto s:scenarios)if(std::string(s->Name->Str[0]).find("Nexus")!=std::string::npos||std::string(s->Name->Str[0]).find("Wrath")!=std::string::npos){selected.insert(s->ID);std::cout<<"SCENARIO\t"<<s->ID<<'\t'<<s->Name->Str[0]<<'\n';}
 for(auto d:lfg)if(selected.count(d->ScenarioID))std::cout<<"LFG\t"<<d->ID<<'\t'<<d->ScenarioID<<'\t'<<d->MapID<<'\t'<<unsigned(d->DifficultyID)<<'\n';
 for(auto s:steps)if(selected.count(s->ScenarioID)){
  std::cout<<"STEP\t"<<s->ScenarioID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<s->ID<<'\t'<<s->Criteriatreeid<<'\t'<<s->Title->Str[0]<<'\t'<<s->Description->Str[0]<<'\n';
  std::set<uint32> todo{uint32(s->Criteriatreeid)},seen;while(!todo.empty()){auto id=*todo.begin();todo.erase(id);if(!seen.insert(id).second)continue;if(auto t=trees.LookupEntry(id)){if(auto c=criteria.LookupEntry(t->CriteriaID))std::cout<<"CRITERION\t"<<s->ScenarioID<<'\t'<<unsigned(s->OrderIndex)<<'\t'<<t->ID<<'\t'<<c->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';for(auto child:trees)if(child->Parent==id)todo.insert(child->ID);}}
 }
 std::set<uint32> chosen{207949,207956,207996,208011,209863,209869,209874,225879};for(auto e:effects)if(e->EffectMiscValue[0]==248572)chosen.insert(e->SpellID);
 for(unsigned depth=0;depth<3;++depth)for(auto e:effects)if(chosen.count(e->SpellID)&&e->EffectTriggerSpell)chosen.insert(e->EffectTriggerSpell);
 for(auto e:effects)if((e->EffectTriggerSpell==207949||e->EffectTriggerSpell==207996)||e->EffectMiscValue[0]==104985)
  std::cout<<"SHARED_USE\t"<<e->SpellID<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\n';
 for(auto id:chosen){if(auto s=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<s->Name->Str[0]<<'\n';for(auto e:effects)if(e->SpellID==id&&!e->DifficultyID)std::cout<<"EFFECT\t"<<id<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<unsigned(e->ImplicitTarget[0])<<'\t'<<unsigned(e->ImplicitTarget[1])<<'\n';}
 for(auto e:effects)if(e->SpellID==209863)std::cout<<"PERIOD\t"<<e->SpellID<<'\t'<<e->DifficultyID<<'\t'<<e->EffectIndex<<'\t'<<e->EffectAuraPeriod<<'\t'<<e->EffectTriggerSpell<<'\n';
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());DB2Storage<SpellDurationEntry> duration("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 if(!misc.Load(argv[1],0)||!duration.Load(argv[1],0))return 6;misc.LoadFromDB();duration.LoadFromDB();
 for(auto m:misc)if(!m->DifficultyID&&chosen.count(m->SpellID)){
  std::cout<<"MISC\t"<<m->SpellID<<'\t'<<m->DurationIndex<<'\t'<<m->CastingTimeIndex<<'\t'<<m->Speed<<'\t'<<m->LaunchDelay<<'\n';
  if(auto d=duration.LookupEntry(m->DurationIndex))std::cout<<"DURATION\t"<<m->SpellID<<'\t'<<d->Duration<<'\t'<<d->MaxDuration<<'\n';
 }
 HotfixDatabase.Close();MySQL::Library_End();
}
