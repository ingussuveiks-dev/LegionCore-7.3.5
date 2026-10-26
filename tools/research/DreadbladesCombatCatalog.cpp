#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!check||!check->Fetch()[0].GetBool())return 4;
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 5;
    steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();
    for(auto step:steps)if(step->ScenarioID==1012)
        std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\n';
    for(auto tree:trees)if(auto criterion=criteria.LookupEntry(tree->CriteriaID))
        if(criterion->Asset>=48449&&criterion->Asset<=48582&&criterion->Type==92)
            std::cout<<"CRITERION\t"<<tree->ID<<'\t'<<tree->Parent<<'\t'<<tree->Amount<<'\t'<<tree->Flags<<'\t'<<criterion->ID<<'\t'<<unsigned(criterion->Type)<<'\t'<<criterion->Asset<<'\n';
    if(auto root=trees.LookupEntry(46690))
        std::cout<<"ROOT\t"<<root->ID<<'\t'<<root->Amount<<'\t'<<unsigned(root->Operator)<<'\t'<<root->Flags<<'\n';
}
