#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    std::string error;auto config=std::getenv("LEGION_DIALOGUE_CONFIG");
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto check=HotfixDatabase.Query("SELECT @@session.transaction_read_only");if(!check||!check->Fetch()[0].GetBool())return 4;
    DB2Storage<TaxiNodesEntry> nodes("TaxiNodes.db2",TaxiNodesLoadInfo::Instance());
    DB2Storage<TaxiPathEntry> paths("TaxiPath.db2",TaxiPathLoadInfo::Instance());
    DB2Storage<TaxiPathNodeEntry> points("TaxiPathNode.db2",TaxiPathNodeLoadInfo::Instance());
    DB2Storage<PlayerConditionEntry> conditions("PlayerCondition.db2",PlayerConditionLoadInfo::Instance());
    DB2Storage<GarrTalentEntry> talents("GarrTalent.db2",GarrTalentLoadInfo::Instance());
    if(!nodes.Load(argv[1],0)||!paths.Load(argv[1],0)||!points.Load(argv[1],0)||!conditions.Load(argv[1],0)||!talents.Load(argv[1],0))return 5;
    nodes.LoadFromDB();paths.LoadFromDB();points.LoadFromDB();conditions.LoadFromDB();talents.LoadFromDB();
    std::set<uint32> selected;
    for(auto node:nodes)if(node->ContinentID==1220&&std::hypot(node->Pos.X-4630.38f,node->Pos.Y-5312.53f)<150.f)selected.insert(node->ID);
    for(auto node:nodes)if(node->ContinentID==1220&&std::hypot(node->Pos.X-375.352f,node->Pos.Y-4362.82f)<100.f)selected.insert(node->ID);
    for(auto node:nodes)if(node->ContinentID==1220&&std::hypot(node->Pos.X-3790.49f,node->Pos.Y-2369.96f)<100.f)selected.insert(node->ID);
    for(auto id:selected)
    {
        auto node=nodes.LookupEntry(id);
        std::cout<<"NODE\t"<<id<<'\t'<<node->Name->Str[0]<<'\t'<<node->Pos.X<<'\t'<<node->Pos.Y<<'\t'<<node->Pos.Z<<'\t'<<unsigned(node->Flags)<<'\t'<<node->ConditionID<<'\t'<<node->MountCreatureID[0]<<'\t'<<node->MountCreatureID[1]<<'\n';
        if(auto condition=conditions.LookupEntry(node->ConditionID))std::cout<<"CONDITION\t"<<condition->ID<<'\t'<<condition->ClassMask<<'\t'<<condition->RaceMask<<'\t'<<condition->MinLevel<<'\n';
        else if(node->ConditionID)std::cout<<"CONDITION_MISSING\t"<<node->ConditionID<<'\n';
    }
    for(auto path:paths)if(selected.count(path->FromTaxiNode)||selected.count(path->ToTaxiNode))
    {
        uint32 count=0;for(auto point:points)if(point->PathID==path->ID)++count;
        std::cout<<"PATH\t"<<path->ID<<'\t'<<path->FromTaxiNode<<'\t'<<path->ToTaxiNode<<'\t'<<path->Cost<<'\t'<<count<<'\n';
        if(selected.count(path->FromTaxiNode)&&!selected.count(path->ToTaxiNode))if(auto destination=nodes.LookupEntry(path->ToTaxiNode))std::cout<<"DESTINATION\t"<<destination->ID<<'\t'<<destination->Name->Str[0]<<'\n';
    }
    for(auto talent:talents)
    {
        std::string name=talent->Name->Str[0];
        if(name.find("Eagle")!=std::string::npos||talent->PerkPlayerConditionID==43356||talent->PlayerConditionID==43356)
            std::cout<<"TALENT\t"<<talent->ID<<'\t'<<name<<'\t'<<talent->GarrTalentTreeID<<'\t'<<talent->PerkSpellID<<'\t'<<talent->PerkPlayerConditionID<<'\t'<<talent->PlayerConditionID<<'\t'<<talent->Description->Str[0]<<'\n';
    }
}
