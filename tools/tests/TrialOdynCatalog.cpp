#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <cassert>
#include <fstream>
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=3)return 1;
#define STORE(T,N,F,I) DB2Storage<T> N(F ".db2",I::Instance());if(!N.Load(argv[1],0))return 2
    STORE(CriteriaTreeEntry,trees,"CriteriaTree",CriteriaTreeLoadInfo);
    STORE(CriteriaEntry,criteria,"Criteria",CriteriaLoadInfo);
    STORE(DungeonEncounterEntry,encounters,"DungeonEncounter",DungeonEncounterLoadInfo);
    STORE(ItemSparseEntry,items,"ItemSparse",ItemSparseLoadInfo);
    auto root=trees.LookupEntry(55330),leaf=trees.LookupEntry(55331);
    assert(root&&leaf&&root->Parent==0&&root->Operator==4&&root->Flags==4096);
    assert(leaf->Parent==root->ID&&leaf->CriteriaID==34579&&leaf->Operator==0);
    auto criterion=criteria.LookupEntry(leaf->CriteriaID);
    assert(criterion&&criterion->Type==165&&criterion->Asset==1958&&!criterion->ModifierTreeId);
    auto raid=encounters.LookupEntry(criterion->Asset),dungeon=encounters.LookupEntry(1809);
    assert(raid&&raid->MapID==1648&&dungeon&&dungeon->MapID==1477);
    auto reward=items.LookupEntry(142533);assert(reward);
    std::ofstream out(argv[2]);
    out<<"constexpr unsigned FavorRoot="<<root->ID<<",FavorType="<<unsigned(criterion->Type)<<",FavorEncounter="<<criterion->Asset<<";\n";
    if(!out)return 3;
    std::cout<<"PASS: native7.3.5 tree55330/55331 -> criterion34579/type165/encounter1958/map1648; dungeon1809/map1477 differs; reward142533 "<<reward->Display->Str[0]<<".\n";
}
