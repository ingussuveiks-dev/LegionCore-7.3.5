#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 2;
    for(auto step:steps)if(step->ScenarioID==988&&step->OrderIndex>=2&&step->OrderIndex<=6)
        for(auto tree:trees)if(tree->Parent==step->Criteriatreeid)
            if(auto criterion=criteria.LookupEntry(tree->CriteriaID))
                std::cout<<"criterion "<<unsigned(step->OrderIndex)<<' '<<unsigned(criterion->Type)<<' '<<criterion->Asset<<'\n';
}
