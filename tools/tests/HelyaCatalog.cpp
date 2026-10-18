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
    auto root=trees.LookupEntry(55482),leaf=trees.LookupEntry(55483);
    assert(root&&leaf&&root->Parent==0&&root->Operator==4&&root->Flags==4096);
    assert(leaf->Parent==root->ID&&leaf->CriteriaID==34647&&leaf->Operator==0);
    auto criterion=criteria.LookupEntry(leaf->CriteriaID);
    assert(criterion&&criterion->Type==165&&criterion->Asset==2008&&!criterion->ModifierTreeId);
    auto raid=encounters.LookupEntry(criterion->Asset),dungeon=encounters.LookupEntry(1809);
    assert(raid&&raid->MapID==1648&&dungeon&&dungeon->MapID==1477);
    auto reward=items.LookupEntry(142534);assert(reward);
    STORE(SpellEffectEntry,effects,"SpellEffect",SpellEffectLoadInfo);
    STORE(CreatureDisplayInfoEntry,displays,"CreatureDisplayInfo",CreatureDisplayInfoLoadInfo);
    STORE(WorldSafeLocsEntry,locations,"WorldSafeLocs",WorldSafeLocsLoadInfo);
    STORE(ConversationLineEntry,lines,"ConversationLine",ConversationLineLoadInfo);
    bool immunity=false,missile=false,jump=false;
    for(auto e:effects){
        if(e->SpellID==228847&&e->EffectIndex==0){assert(e->EffectAura==40&&e->EffectMiscValue[0]==127);immunity=true;}
        if(e->SpellID==232580&&e->EffectIndex==0){assert(e->Effect==42&&e->EffectTriggerSpell==232581&&e->ImplicitTarget[1]==138);missile=true;}
        if(e->SpellID==232581&&e->EffectIndex==0){assert(e->Effect==98);jump=true;}
    }
    assert(immunity&&missile&&jump&&displays.LookupEntry(66238)&&displays.LookupEntry(67230));
    auto exit=locations.LookupEntry(5779);assert(exit&&exit->MapID==1220);
    for(auto id:{9528,9529,9530})assert(lines.LookupEntry(id));
    std::ofstream out(argv[2]);
    out<<"constexpr unsigned JudgmentRoot="<<root->ID<<",JudgmentType="<<unsigned(criterion->Type)<<",JudgmentEncounter="<<criterion->Asset<<";\n";
    if(!out)return 3;
    std::cout<<"PASS: native7.3.5 tree55482/55483 -> criterion34647/type165/encounter2008/map1648; dungeon1809/map1477 differs; native immunity228847, Odyn missile232580 -> jump232581, displays66238/67230; reward142534 "<<reward->Display->Str[0]<<".\n";
}
