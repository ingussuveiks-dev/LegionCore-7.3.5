#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellVisualKitEntry> kits("SpellVisualKit.db2",SpellVisualKitLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!kits.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0))return 2;
    for(uint32 spellId:{227853u,152821u})
    {
        auto spell=spells.LookupEntry(spellId);if(!spell)return 3;
        std::cout<<"SPELL\t"<<spellId<<'\t'<<spell->Name->Str[0]<<'\n';
        for(auto effect:effects)if(effect->SpellID==spellId)
            std::cout<<"EFFECT\t"<<spellId<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectMiscValue[0]<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->ImplicitTarget[0]<<'\t'<<effect->ImplicitTarget[1]<<'\n';
    }
    std::cout<<"KIT\t61036\t"<<bool(kits.LookupEntry(61036))<<'\n';
    for(auto step:steps)if(step->ScenarioID==775 && step->OrderIndex==5)
    {
        std::cout<<"STEP\t"<<step->ID<<'\t'<<step->Criteriatreeid<<'\n';
        for(auto tree:trees)if(tree->Parent==step->Criteriatreeid)
            if(auto criterion=criteria.LookupEntry(tree->CriteriaID))
                std::cout<<"CRITERION\t"<<tree->ID<<'\t'<<criterion->ID<<'\t'<<unsigned(criterion->Type)<<'\t'<<criterion->Asset<<'\n';
    }
}
