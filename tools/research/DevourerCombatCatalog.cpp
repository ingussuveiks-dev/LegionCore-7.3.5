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
    for(auto step:steps)if(step->ScenarioID==1078)
        std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\n';
    for(auto tree:trees)if(auto criterion=criteria.LookupEntry(tree->CriteriaID))
        if((criterion->Asset==50261||criterion->Asset==50298||criterion->Asset==50300||criterion->Asset==50301)&&criterion->Type==92)
            std::cout<<"CRITERION\t"<<tree->ID<<'\t'<<tree->Parent<<'\t'<<tree->Amount<<'\t'<<tree->Flags<<'\t'<<criterion->ID<<'\t'<<unsigned(criterion->Type)<<'\t'<<criterion->Asset<<'\n';
    bool zirus=false,akaari=false;
    for(auto tree:trees)if(auto c=criteria.LookupEntry(tree->CriteriaID)) {
        if(tree->ID==47365&&tree->Parent==47364&&c->Type==92&&c->Asset==50298)zirus=true;
        if(tree->ID==47369&&tree->Parent==47368&&c->Type==92&&c->Asset==50300)akaari=true;
    }
    return zirus&&akaari?0:6;
}
