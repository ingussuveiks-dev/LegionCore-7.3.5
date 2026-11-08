#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <iomanip>
#include <set>
#include <string>
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    std::cout<<std::setprecision(9);
    DB2Storage<WorldSafeLocsEntry> safe("WorldSafeLocs.db2",WorldSafeLocsLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    if(!safe.Load(argv[1],0)||!spells.Load(argv[1],0)||!effects.Load(argv[1],0))return 2;
    for(auto e:safe)if(e->MapID==1600)std::cout<<"SAFE\t"<<e->ID<<'\t'<<e->Loc.X<<'\t'<<e->Loc.Y<<'\t'<<e->Loc.Z<<'\t'<<e->Loc.O<<'\t'<<e->Name->Str[0]<<'\n';
    std::set<unsigned> selected;
    for(auto e:spells){std::string name=e->Name->Str[0];if(name.find("Sharas")!=std::string::npos||e->ID==207683){selected.insert(e->ID);std::cout<<"SPELL\t"<<e->ID<<'\t'<<name<<'\n';}}
    for(auto e:effects)if((selected.count(e->SpellID)&&(e->Effect==227||e->Effect==252||e->Effect==45||e->Effect==90))||(e->Effect==252&&e->EffectMiscValue[0]==1229)||(e->Effect==227&&e->EffectMiscValue[0]==1600))std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<e->ImplicitTarget[0]<<'\t'<<e->ImplicitTarget[1]<<'\n';
    std::cout<<"COMPLETE\n";return 0;
}
