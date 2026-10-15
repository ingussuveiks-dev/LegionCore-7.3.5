#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 2;
    std::set<int32> selected;
    for(auto s:steps)if(s->ScenarioID==1097){selected.insert(s->Criteriatreeid);std::cout<<"step "<<s->ID<<' '<<unsigned(s->OrderIndex)<<' '<<s->Criteriatreeid<<'\n';}
    for(size_t old=0;old!=selected.size();){old=selected.size();for(auto t:trees)if(selected.count(t->Parent))selected.insert(t->ID);}
    for(auto t:trees)if(selected.count(t->ID)){
        auto c=criteria.LookupEntry(t->CriteriaID);
        std::cout<<"tree "<<t->ID<<' '<<t->Parent<<' '<<t->Amount<<' '<<unsigned(t->Operator)<<' '<<t->CriteriaID<<' '<<(c?unsigned(c->Type):0)<<' '<<(c?c->Asset:0)<<'\n';
    }
}
