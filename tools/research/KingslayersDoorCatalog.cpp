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
 DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
 if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 5;steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
 for(auto step:steps)if(step->ScenarioID==1123)std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\n';
 for(auto tree:trees)if(tree->Parent==48576||tree->Parent==48584)std::cout<<"CHILD\t"<<tree->ID<<'\t'<<tree->Parent<<'\t'<<tree->CriteriaID<<'\t'<<tree->Amount<<'\t'<<unsigned(tree->Operator)<<'\n';
 for(auto tree:trees)if(auto criterion=criteria.LookupEntry(tree->CriteriaID))if(criterion->Type==92&&(criterion->Asset==51549||criterion->Asset==51654)){
  std::cout<<"LEAF\t"<<tree->ID<<'\t'<<tree->Parent<<'\t'<<tree->Amount<<'\t'<<tree->Flags<<'\t'<<unsigned(tree->Operator)<<'\t'<<criterion->ID<<'\t'<<criterion->Asset<<'\n';
  if(auto parent=trees.LookupEntry(tree->Parent))std::cout<<"ROOT\t"<<parent->ID<<'\t'<<parent->Parent<<'\t'<<parent->Amount<<'\t'<<parent->Flags<<'\t'<<unsigned(parent->Operator)<<'\n';
 }
 HotfixDatabase.Close();MySQL::Library_End();std::cout<<"PASS native26972 door criteria catalog\n";
}
