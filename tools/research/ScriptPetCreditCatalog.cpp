#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
#include <map>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!effects.Load(argv[1],0))return 2;
    std::set<uint32> spells;
    std::set<uint32> mixed;
    for(auto e:effects)
        if(e->Effect==90 || e->Effect==134)spells.insert(e->SpellID);
        else if(e->Effect)mixed.insert(e->SpellID);
    for(auto id:spells)if(!mixed.count(id))std::cout<<id<<'\n';
}
