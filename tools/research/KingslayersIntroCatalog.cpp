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
    std::string error;
    auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 4;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 5;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");
    if(!check||!check->Fetch()[0].GetBool())return 6;
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<MapEntry> maps("Map.db2",MapLoadInfo::Instance());
    if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!maps.Load(argv[1],0))return 2;
    steps.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();maps.LoadFromDB();
    auto map=maps.LookupEntry(1620);if(!map)return 3;
    std::cout<<"MAP\t1620\t"<<map->Instanceable()<<'\t'<<map->IsDungeon()<<'\n';
    for(auto step:steps)if(step->ScenarioID==1123)
        std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\t'<<step->IsBonusObjective()<<'\n';
    for(auto tree:trees)
    {
        auto criterion=criteria.LookupEntry(tree->CriteriaID);
        if(criterion&&(criterion->Asset==51418||criterion->Asset==52672))
            std::cout<<"CRITERION\t"<<tree->ID<<'\t'<<tree->Parent<<'\t'<<criterion->ID<<'\t'<<unsigned(criterion->Type)<<'\t'<<criterion->Asset<<'\n';
    }
}
