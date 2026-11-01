#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "MySQLThreading.h"
#include <cstdlib>
#include <iostream>
#include <set>

// Matching 26972 records and read-only hotfixes. No world/character writes.
int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    std::string error;
    auto config = std::getenv("LEGION_DIALOGUE_CONFIG");
    if (!config || !sConfigMgr->LoadInitial(config, {}, error)) return 2;
    MySQL::Library_Init();
    HotfixDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("HotfixDatabaseInfo", ""), 1, 1);
    if (HotfixDatabase.Open() || !HotfixDatabase.PrepareStatements()) return 3;
    HotfixDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    HotfixDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    if (!steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0)) return 4;
    steps.LoadFromDB(); trees.LoadFromDB(); criteria.LoadFromDB();
    for (auto step : steps)
        if (step->ScenarioID == 1137)
        {
            std::cout << "STEP\t" << step->ID << '\t' << unsigned(step->OrderIndex) << '\t'
                << step->Criteriatreeid << '\t' << step->Title->Str[0] << '\n';
            std::set<uint32> pending{uint32(step->Criteriatreeid)}, seen;
            while (!pending.empty())
            {
                uint32 id = *pending.begin(); pending.erase(pending.begin());
                if (!seen.insert(id).second) continue;
                auto tree = trees.LookupEntry(id);
                if (!tree) return 5;
                std::cout << "TREE\t" << id << '\t' << tree->Parent << '\t' << int(tree->Operator) << '\t'
                    << tree->Amount << '\t' << tree->CriteriaID << '\n';
                if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                    std::cout << "CRITERION\t" << id << '\t' << criterion->ID << '\t'
                        << unsigned(criterion->Type) << '\t' << criterion->Asset << '\t'
                        << criterion->ModifierTreeId << '\t' << unsigned(criterion->Flags) << '\n';
                for (auto child : trees) if (uint32(child->Parent) == id) pending.insert(child->ID);
            }
        }
    HotfixDatabase.Close();
    MySQL::Library_End();
}
