#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 2;
    for(uint32 spellId:{180850u,181257u,181511u})
    {
        auto spell=spells.LookupEntry(spellId);if(!spell)return 3;
        std::cout<<"SPELL\t"<<spellId<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto effect:effects)if(effect->SpellID==spellId)
            std::cout<<"EFFECT\t"<<spellId<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    }
    for(auto step:steps)if(step->ScenarioID==775)
        std::cout<<"STEP\t"<<unsigned(step->OrderIndex)<<'\t'<<step->ID<<'\t'<<step->Criteriatreeid<<'\t'<<step->IsBonusObjective()<<'\n';
    for(uint32 treeId:{45146u,45792u,45278u})
    {
        auto tree=trees.LookupEntry(treeId);if(!tree)return 4;
        auto criterion=criteria.LookupEntry(tree->CriteriaID);if(!criterion)return 5;
        std::cout<<"CRITERION\t"<<treeId<<'\t'<<tree->Parent<<'\t'<<criterion->ID<<'\t'<<unsigned(criterion->Type)<<'\t'<<criterion->Asset<<'\n';
    }
}
