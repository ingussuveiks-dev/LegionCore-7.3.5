// Use the production DB2 reader for packed signed fields and array columns.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<AdventureMapPOIEntry> pois("AdventureMapPOI.db2",AdventureMapPOILoadInfo::Instance());
    DB2Storage<PlayerConditionEntry> conditions("PlayerCondition.db2",PlayerConditionLoadInfo::Instance());
    DB2Storage<ModifierTreeEntry> trees("ModifierTree.db2",ModifierTreeLoadInfo::Instance());
    if(!pois.Load(argv[1],0)||!conditions.Load(argv[1],0)||!trees.Load(argv[1],0))return 2;
    std::set<int32> conditionIds,treeIds;
    for(auto poi:pois)
        if(poi->Type==1 && (poi->QuestID==39718 || poi->QuestID==39731 || poi->QuestID==39733 || poi->QuestID==39735 || poi->QuestID==39864))
        {
            std::cout<<"poi\t"<<poi->ID<<'\t'<<poi->QuestID<<'\t'<<poi->PlayerConditionID<<'\t'<<poi->Title->Str[0]<<'\n';
            conditionIds.insert(poi->PlayerConditionID);
        }
    for(int32 id:conditionIds)
    {
        auto c=conditions.LookupEntry(id);if(!c)return 3;
        std::cout<<"condition\t"<<id<<'\t'<<c->RaceMask<<'\t'<<c->MinLevel<<'\t'<<c->MaxLevel<<'\t'<<c->ClassMask<<'\t'<<int(c->Gender)<<'\t'<<int(c->NativeGender)<<'\t'<<c->ModifierTreeID;
        for(auto skill:c->SkillID)std::cout<<'\t'<<skill;
        std::cout<<'\n';treeIds.insert(c->ModifierTreeID);
    }
    bool changed=true;
    while(changed){changed=false;for(auto t:trees)if(treeIds.count(t->Parent)&&treeIds.insert(t->ID).second)changed=true;}
    for(auto t:trees)if(treeIds.count(t->ID))
        std::cout<<"tree\t"<<t->ID<<'\t'<<t->Parent<<'\t'<<unsigned(t->Operator)<<'\t'<<unsigned(t->Type)<<'\t'<<t->Asset<<'\t'<<t->SecondaryAsset<<'\t'<<unsigned(t->TertiaryAsset)<<'\t'<<unsigned(t->Amount)<<'\n';
}
