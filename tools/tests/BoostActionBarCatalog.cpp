#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "BoostActionBars.h"
#include <cassert>
#include <iostream>
int main(int argc,char** argv)
{
    assert(argc==2);
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellMiscEntry> misc("SpellMisc.db2",SpellMiscLoadInfo::Instance());
    DB2Storage<SpellShapeshiftFormEntry> forms("SpellShapeshiftForm.db2",SpellShapeshiftFormLoadInfo::Instance());
    assert(spells.Load(argv[1],0)&&misc.Load(argv[1],0)&&forms.Load(argv[1],0));
    for(auto form:forms)if(form->BonusActionBar)std::cout<<"FORM "<<form->ID<<' '<<unsigned(form->BonusActionBar)<<' '<<form->Name->Str[0]<<'\n';
    assert(forms.LookupEntry(1)->BonusActionBar==1&&forms.LookupEntry(5)->BonusActionBar==3&&forms.LookupEntry(31)->BonusActionBar==4);
    assert(forms.LookupEntry(30)->BonusActionBar==1&&forms.LookupEntry(13)->BonusActionBar==2);
    std::set<uint32_t> ids;
    for(auto spec:{71,72,73,65,66,70,253,254,255,259,260,261,256,257,258,250,251,252,262,263,264,62,63,64,265,266,267,268,269,270,102,103,104,105,577,581})
        for(auto id:BoostActionBars::Priority(spec))ids.insert(id);
    for(int c=1;c<=12;++c)for(auto id:BoostActionBars::Utility(c))ids.insert(id);
    int missing=0;
    for(auto id:ids)
    {
        auto s=spells.LookupEntry(id);
        if(!s){std::cout<<"MISSING "<<id<<'\n';++missing;continue;}
        uint32_t attrs=0;
        for(auto m:misc)if(m->SpellID==id){attrs=m->Attributes[0];break;}
        std::cout<<id<<'\t'<<s->Name->Str[0]<<'\t'<<attrs<<'\n';
    }
    return missing?1:0;
}
