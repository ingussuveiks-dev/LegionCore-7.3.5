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
 for(auto e:effects)if(e->SpellID==214645||e->SpellID==214683||e->SpellID==214734||e->SpellID==214304||e->SpellID==227061)std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectAura<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 5;steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
 for(auto step:steps)if(step->ScenarioID==1123)std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\n';
 for(auto step:steps)if(step->ScenarioID==1123)for(auto tree:trees)if(tree->Parent==step->Criteriatreeid)if(auto c=criteria.LookupEntry(tree->CriteriaID))std::cout<<"LEAF\t"<<unsigned(step->OrderIndex)<<'\t'<<tree->ID<<'\t'<<tree->CriteriaID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\t'<<tree->Amount<<'\n';
 DB2Storage<ItemEffectEntry> items("ItemEffect.db2",ItemEffectLoadInfo::Instance());if(!items.Load(argv[1],0))return 8;items.LoadFromDB();for(auto e:items)if(e->SpellID==214645)std::cout<<"ITEM_EFFECT\t"<<e->ID<<'\t'<<e->ItemID<<'\t'<<e->Charges<<'\t'<<unsigned(e->TriggerType)<<'\n';
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS native26972 smoke catalog\n";
}
