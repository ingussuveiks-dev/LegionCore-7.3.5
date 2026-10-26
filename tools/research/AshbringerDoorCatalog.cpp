#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2", ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2", CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2", CriteriaLoadInfo::Instance());
    if (!steps.Load(argv[1], 0) || !trees.Load(argv[1], 0) || !criteria.Load(argv[1], 0))
        return 2;
    std::set<int32> selected;
    for (auto step : steps)
        if (step->ScenarioID == 775)
        {
            selected.insert(step->Criteriatreeid);
            std::cout << "STEP\t" << unsigned(step->OrderIndex) << '\t' << step->ID << '\t' << step->Criteriatreeid << '\n';
        }
    for (size_t previous = 0; previous != selected.size();)
    {
        previous = selected.size();
        for (auto tree : trees)
            if (selected.count(tree->Parent))
                selected.insert(tree->ID);
    }
    for (auto tree : trees)
        if (selected.count(tree->ID))
            if (auto criterion = criteria.LookupEntry(tree->CriteriaID))
                std::cout << "CRITERION\t" << tree->ID << '\t' << tree->Parent << '\t' << tree->Amount << '\t'
                    << criterion->ID << '\t' << unsigned(criterion->Type) << '\t' << criterion->Asset << '\n';
}
