#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char**argv){
 if(argc!=2)return 1;std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
 MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
 HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");auto ro=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!ro||!ro->Fetch()[0].GetBool())return 4;
 DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
 if(!effects.Load(argv[1],0))return 7;effects.LoadFromDB();
 for(auto e:effects)if(e->SpellID==209626)std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 5;steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
 for(auto step:steps)if(step->ScenarioID==1078)std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\t'<<step->Title->Str[0]<<'\t'<<step->Description->Str[0]<<'\n';
 for(auto step:steps)if(step->ScenarioID==1078)for(auto tree:trees)if(tree->Parent==step->Criteriatreeid)if(auto c=criteria.LookupEntry(tree->CriteriaID))std::cout<<"LEAF\t"<<unsigned(step->OrderIndex)<<'\t'<<tree->ID<<'\t'<<tree->CriteriaID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\t'<<tree->Amount<<'\t'<<tree->Description->Str[0]<<'\n';
 DB2Storage<ItemEffectEntry> items("ItemEffect.db2",ItemEffectLoadInfo::Instance());if(!items.Load(argv[1],0))return 8;items.LoadFromDB();for(auto e:items)if(e->SpellID==209626)std::cout<<"ITEM_EFFECT\t"<<e->ID<<'\t'<<e->ItemID<<'\t'<<e->Charges<<'\t'<<unsigned(e->TriggerType)<<'\n';
 DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());DB2Storage<SpellDurationEntry> duration("SpellDuration.db2",SpellDurationLoadInfo::Instance());
 if(!misc.Load(argv[1],0)||!duration.Load(argv[1],0))return 9;misc.LoadFromDB();duration.LoadFromDB();for(auto row:misc)if(row->SpellID==209626){auto d=duration.LookupEntry(row->DurationIndex);std::cout<<"DURATION\t209626\t"<<(d?d->Duration:0)<<'\n';}
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS native26972 Devourer entrance catalog\n";
}
