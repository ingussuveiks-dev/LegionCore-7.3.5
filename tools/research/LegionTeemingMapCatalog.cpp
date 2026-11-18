#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    auto config=std::getenv("LEGION_DIALOGUE_CONFIG");std::string error;
    if(!config||!sConfigMgr->LoadInitial(config,{},error))return 2;
    MySQL::Library_Init();HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo",""),1,1);
    if(HotfixDatabase.Open()||!HotfixDatabase.PrepareStatements())return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<WorldMapAreaEntry> areas("WorldMapArea.db2",WorldMapAreaLoadInfo::Instance());
    DB2Storage<DungeonMapEntry> floors("DungeonMap.db2",DungeonMapLoadInfo::Instance());
    DB2Storage<DungeonMapChunkEntry> chunks("DungeonMapChunk.db2",DungeonMapChunkLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    if(!areas.Load(argv[1],0)||!floors.Load(argv[1],0)||!chunks.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!steps.Load(argv[1],0))return 4;
    areas.LoadFromDB();floors.LoadFromDB();chunks.LoadFromDB();trees.LoadFromDB();criteria.LoadFromDB();steps.LoadFromDB();
    std::set<uint32> maps={1456,1458,1466,1477,1492,1493,1501,1516,1571,1651,1677,1753};
    std::set<uint32> scenarios={1046,1166,1169,1172,1173,1174,1175,1177,1178,1308,1309,1335,1428};
    std::set<uint32> selectedTrees;
    std::cout.precision(10);
    for(auto row:areas)if(maps.count(row->MapID))
        std::cout<<"AREA\t"<<row->ID<<'\t'<<row->MapID<<'\t'<<row->AreaID<<'\t'<<row->LocLeft<<'\t'<<row->LocRight<<'\t'<<row->LocTop<<'\t'<<row->LocBottom<<'\t'<<row->DefaultDungeonFloor<<'\t'<<row->AreaName->Str[0]<<'\n';
    for(auto row:floors)if(maps.count(row->MapID))
        std::cout<<"FLOOR\t"<<row->ID<<'\t'<<row->MapID<<'\t'<<row->ParentWorldMapID<<'\t'<<unsigned(row->FloorIndex)<<'\t'<<unsigned(row->RelativeHeightIndex)<<'\t'<<unsigned(row->Flags)<<'\t'<<row->Min[0]<<'\t'<<row->Min[1]<<'\t'<<row->Max[0]<<'\t'<<row->Max[1]<<'\n';
    for(auto row:chunks)if(maps.count(row->MapID))
        std::cout<<"CHUNK\t"<<row->ID<<'\t'<<row->MapID<<'\t'<<row->DungeonMapID<<'\t'<<row->MinZ<<'\t'<<row->DoodadPlacementID<<'\t'<<row->WmoGroupID<<'\n';
    for(auto row:steps)if(scenarios.count(row->ScenarioID))
    {
        std::cout<<"STEP\t"<<row->ID<<'\t'<<row->ScenarioID<<'\t'<<unsigned(row->OrderIndex)<<'\t'<<row->Criteriatreeid<<'\n';selectedTrees.insert(row->Criteriatreeid);
    }
    bool changed=true;
    while(changed){changed=false;for(auto row:trees)if(selectedTrees.count(row->Parent)&&selectedTrees.insert(row->ID).second)changed=true;}
    for(auto row:trees)if(selectedTrees.count(row->ID))
    {
        auto criterion=criteria.LookupEntry(row->CriteriaID);
        std::cout<<"TREE\t"<<row->ID<<'\t'<<row->Parent<<'\t'<<row->Amount<<'\t'<<int(row->Operator)<<'\t'<<row->CriteriaID<<'\t'<<(criterion?int(criterion->Type):-1)<<'\t'<<(criterion?criterion->Asset:0)<<'\t'<<(criterion?criterion->ModifierTreeId:0)<<'\t'<<row->Description->Str[0]<<'\n';
    }
    std::cout<<"EOF\tteeming-map-catalog\n";HotfixDatabase.Close();MySQL::Library_End();return 0;
}
