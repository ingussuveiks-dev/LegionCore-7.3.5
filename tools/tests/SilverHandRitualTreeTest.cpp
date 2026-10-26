#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using uint8=std::uint8_t;
using uint32=std::uint32_t;
using uint64=std::uint64_t;
#include "TreeOperators.inc"
struct CriteriaTreeEntry {uint32 ID=0,Parent=0,Amount=0,CriteriaID=0,Flags=0,Operator=0;};
struct Criteria {};
struct CriteriaTree
{
    std::vector<CriteriaTree const*> Children;
    CriteriaTreeEntry const* Entry=nullptr;
    struct Criteria const* Criteria=nullptr;
    uint32 ID=0;
};
struct CriteriaProgress {uint64 Counter=0;};
struct AchievementGlobalMgr
{
    template<typename Func> static void WalkCriteriaTree(CriteriaTree const*,Func const&);
};
template<class Owner> struct AchievementMgr
{
    std::map<uint32,CriteriaProgress> storedProgress;
    CriteriaProgress const* GetCriteriaProgress(uint32 identity)
    {auto found=storedProgress.find(identity);return found==storedProgress.end()?nullptr:&found->second;}
    bool IsCompletedCriteria(CriteriaTree const* tree,uint64 required)
    {auto found=GetCriteriaProgress(tree->ID);return found && found->Counter>=required;}
    bool IsCompletedCriteriaTree(CriteriaTree const*);
};
#include "TreeCompletion.inc"
int main(int argc,char** argv)
{
    assert(argc==2);
    std::ifstream input(argv[1]);assert(input.good());
    std::map<uint32,CriteriaTreeEntry> entries;
    std::string line;
    while(std::getline(input,line))
    {
        std::istringstream fields(line);std::string label;uint32 step=0;
        fields>>label>>step;
        if(label!="TREE" || step!=9)continue;
        CriteriaTreeEntry entry;
        fields>>entry.ID>>entry.Parent>>entry.Amount>>entry.Operator>>entry.Flags>>entry.CriteriaID;
        assert(fields);entries[entry.ID]=entry;
    }
    assert(entries.size()==3);
    assert(entries.at(48055).Amount==180 && entries.at(48055).Operator==CRITERIA_TREE_OPERATOR_ALL);
    assert(entries.at(53744).Parent==48055 && entries.at(53744).Amount==180 && entries.at(53744).Operator==CRITERIA_TREE_OPERATOR_SCENARIO);
    assert(entries.at(53745).Parent==53744 && entries.at(53745).Amount==1 && entries.at(53745).CriteriaID==30110);
    std::map<uint32,CriteriaTree> trees;
    Criteria criterion;
    for(auto const& reference:entries)
    {
        auto& tree=trees[reference.first];tree.ID=reference.first;tree.Entry=&reference.second;
        if(reference.second.CriteriaID)tree.Criteria=&criterion;
    }
    for(auto const& reference:entries)
        if(reference.second.Parent)trees[reference.second.Parent].Children.push_back(&trees.at(reference.first));
    AchievementMgr<int> manager;
    assert(!manager.IsCompletedCriteriaTree(nullptr));
    assert(!manager.IsCompletedCriteriaTree(&trees.at(48055)));
    for(uint64 pulses=1;pulses<=180;++pulses)
    {
        manager.storedProgress[53745].Counter=pulses;
        assert(manager.IsCompletedCriteriaTree(&trees.at(48055))==(pulses==180));
    }
    std::cout<<"PASS actual native ALL/scenario-weighted completion and WalkCriteriaTree over fresh26972 metadata: one/179 pulses do not complete;180 do. Counter storage/input controlled, not live character progress or client timing.\n";
}
