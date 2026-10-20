#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellTargetRestrictionsEntry> targets("SpellTargetRestrictions.db2",SpellTargetRestrictionsLoadInfo::Instance());
    DB2Storage<SummonPropertiesEntry> summons("SummonProperties.db2",SummonPropertiesLoadInfo::Instance());
    if(!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!effects.Load(argv[1],0)||!targets.Load(argv[1],0)||!summons.Load(argv[1],0))return 2;
    if(auto p=summons.LookupEntry(3730))std::cout<<"summon3730 "<<p->Flags<<' '<<p->Control<<' '<<p->Title<<' '<<p->Slot<<'\n';
    for(auto step:steps)if(step->ScenarioID==988&&step->OrderIndex<=6)
        for(auto tree:trees)if(tree->Parent==step->Criteriatreeid)
            if(auto criterion=criteria.LookupEntry(tree->CriteriaID))
                std::cout<<"criterion "<<unsigned(step->OrderIndex)<<' '<<unsigned(criterion->Type)<<' '<<criterion->Asset<<'\n';
    for(auto effect:effects)if(effect->SpellID==199625||effect->SpellID==199626||effect->SpellID==199627)
        std::cout<<"effect "<<effect->SpellID<<' '<<unsigned(effect->EffectIndex)<<' '<<unsigned(effect->Effect)<<' '<<unsigned(effect->EffectAura)<<' '<<effect->EffectMiscValue[0]<<' '<<effect->EffectMiscValue[1]<<' '<<effect->EffectTriggerSpell<<' '<<unsigned(effect->ImplicitTarget[0])<<'\n';
    for(uint32 spell:{199626u,199627u}){
        int32 mask=0;
        for(auto target:targets)if(target->SpellID==spell&&target->DifficultyID==0)mask|=target->Targets;
        std::cout<<"explicit "<<spell<<' '<<mask<<'\n';
    }
}
