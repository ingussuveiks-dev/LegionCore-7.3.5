#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char**argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<LFGDungeonsEntry> lfg("LFGDungeons.db2",LfgDungeonsLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!lfg.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 2;
    std::set<unsigned> ids={199985,201674,201678,210180,216680,217100,217812};
    for(auto e:effects)if(e->Effect==227&&(e->EffectMiscValue[0]==1059||e->EffectMiscValue[0]==1243))ids.insert(e->SpellID);
    for(auto e:effects)if(ids.count(e->EffectTriggerSpell))ids.insert(e->SpellID);
    for(auto e:effects)if(ids.count(e->SpellID))
        std::cout<<"EFFECT\t"<<e->SpellID<<'\t'<<unsigned(e->EffectIndex)<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectTriggerSpell<<'\t'<<unsigned(e->ImplicitTarget[0])<<'\t'<<unsigned(e->ImplicitTarget[1])<<'\n';
    for(unsigned id:ids)if(auto e=spells.LookupEntry(id))std::cout<<"SPELL\t"<<id<<'\t'<<e->Name->Str[0]<<'\n';
    for(auto e:lfg)if(e->MapID==1489)std::cout<<"LFG\t"<<e->ID<<'\t'<<e->ScenarioID<<'\t'<<unsigned(e->DifficultyID)<<'\t'<<e->Flags<<'\n';
    for(auto e:steps)if(e->ScenarioID==940||e->ScenarioID==1085)std::cout<<"STEP\t"<<e->ScenarioID<<'\t'<<unsigned(e->OrderIndex)<<'\t'<<e->Criteriatreeid<<'\t'<<e->Title->Str[0]<<'\n';
    for(auto e:trees)if(e->Parent==46836&&e->CriteriaID)if(auto c=criteria.LookupEntry(e->CriteriaID))std::cout<<"FINAL\t"<<e->ID<<'\t'<<unsigned(c->Type)<<'\t'<<c->Asset<<'\n';
    std::cout<<"COMPLETE\n";return 0;
}
