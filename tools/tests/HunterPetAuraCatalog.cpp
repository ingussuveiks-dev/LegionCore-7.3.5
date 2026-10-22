#include <cassert>
#include <iostream>
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
int main(int argc,char** argv){
    assert(argc==2);
    DB2Storage<SpellAuraRestrictionsEntry> restrictions("SpellAuraRestrictions.db2",SpellAuraRestrictionsLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    assert(restrictions.Load(argv[1],0)&&effects.Load(argv[1],0));
    bool requirement=false,ownerAura=false;
    for(auto row:restrictions)if(row->SpellID==34026){assert(row->CasterAuraSpell==166615);requirement=true;}
    for(auto row:effects)if(row->SpellID==166615&&row->EffectIndex==0){assert(row->Effect==119&&row->ImplicitTarget[0]==1);ownerAura=true;}
    assert(requirement&&ownerAura);
    std::cout<<"PASS: native 7.3.5 DB2 requires Pet Active 166615 for Kill Command 34026 and retains its area-aura effect\n";
}
