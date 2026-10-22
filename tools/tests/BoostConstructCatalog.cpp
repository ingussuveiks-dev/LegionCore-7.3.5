#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <unordered_set>
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "AchievementMgr.h"
constexpr uint32 CRITERIA_CONSTRUCTS_DESTROYED=50571;
#include "Criterion.inc"
#include "Needs.inc"
int main(int argc,char** argv){
    assert(argc==2);
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    assert(criteria.Load(argv[1],0)&&trees.Load(argv[1],0)&&steps.Load(argv[1],0)&&displays.Load(argv[1],0));
    assert(displays.LookupEntry(56111));
    std::map<int32,Criteria> records;
    std::map<int32,CriteriaTree> nodes;
    for(auto c:criteria)records[c->ID].Entry=c;
    for(auto t:trees){auto& node=nodes[t->ID];node.Entry=t;if(t->CriteriaID)node.Criteria=&records.at(t->CriteriaID);}
    for(auto t:trees)if(t->Parent)nodes.at(t->Parent).Children.push_back(&nodes.at(t->ID));
    std::set<uint32> scenarios{1073,1083,1084,1090,1091,1093,1094,1095,1096,1132,1133,1181,1182,1214};
    std::set<int32> found;
    bool hunter=false;
    for(auto s:steps)if(scenarios.count(s->ScenarioID)){
        auto tree=&nodes.at(s->Criteriatreeid);
        if(NeedsArcaneConstructs(tree)){
            found.insert(s->Criteriatreeid);
            std::cout<<"construct-lesson "<<s->ScenarioID<<' '<<unsigned(s->OrderIndex)<<' '<<s->Criteriatreeid<<' '<<s->Title->Str[0]<<'\n';
        }
        if(s->ScenarioID==1083&&s->OrderIndex==7){
            assert(s->ID==2413&&s->Criteriatreeid==47514);
            assert(HasLessonCriterion(tree,CRITERIA_TYPE_CAST_SPELL,2643));
            assert(HasLessonCriterion(tree,CRITERIA_TYPE_SCRIPT_EVENT_2,50571));
            assert(trees.LookupEntry(50125)->Amount==1);
            hunter=true;
        }
        if(s->ScenarioID==1084)assert(!NeedsArcaneConstructs(tree));
    }
    assert(hunter&&(found==std::set<int32>{47514,47793,48073,48081,48841,51837,55422,55639}));
    assert(criteria.LookupEntry(29840)->Type==CRITERIA_TYPE_SCRIPT_EVENT_2&&criteria.LookupEntry(29840)->Asset==50571);
    assert(criteria.LookupEntry(30919)->Asset==48772);
    std::cout<<"PASS: production lesson selection over native 7.3.5 ScenarioStep/CriteriaTree/Criteria; all eight construct lessons, independent hunter cast/event, ordinary Warlock and sparring event preserved\n";
}
