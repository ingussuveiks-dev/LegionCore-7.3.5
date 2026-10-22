#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ScenarioEntry> scenarios("Scenario.db2",ScenarioLoadInfo::Instance());
    DB2Storage<ScenarioStepEntry> steps("ScenarioStep.db2",ScenarioStepLoadInfo::Instance());
    DB2Storage<CriteriaTreeEntry> trees("CriteriaTree.db2",CriteriaTreeLoadInfo::Instance());
    DB2Storage<CriteriaEntry> criteria("Criteria.db2",CriteriaLoadInfo::Instance());
    DB2Storage<LFGDungeonsEntry> dungeons("LFGDungeons.db2",LfgDungeonsLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<QuestPackageItemEntry> packages("QuestPackageItem.db2",QuestPackageItemLoadInfo::Instance());
    if(!scenarios.Load(argv[1],0)||!steps.Load(argv[1],0)||!trees.Load(argv[1],0)||!criteria.Load(argv[1],0)||!dungeons.Load(argv[1],0)||!effects.Load(argv[1],0)||!packages.Load(argv[1],0))return 2;
    for(auto id:{786,1189}){
        auto scenario=scenarios.LookupEntry(id);if(!scenario)return 3;
        std::cout<<"scenario "<<id<<' '<<unsigned(scenario->Type)<<'\n';
        for(auto d:dungeons)if(d->ScenarioID==id)std::cout<<"lfg "<<id<<' '<<d->ID<<' '<<d->MapID<<' '<<unsigned(d->DifficultyID)<<'\n';
        std::set<int32> selected;
        for(auto s:steps)if(s->ScenarioID==id){selected.insert(s->Criteriatreeid);std::cout<<"step "<<id<<' '<<unsigned(s->OrderIndex)<<' '<<s->ID<<' '<<s->Criteriatreeid<<" reward="<<s->RewardQuestID<<'\n';}
        for(size_t old=0;old!=selected.size();){old=selected.size();for(auto t:trees)if(selected.count(t->Parent))selected.insert(t->ID);}
        for(auto t:trees)if(selected.count(t->ID)){std::cout<<"tree "<<id<<' '<<t->ID<<' '<<t->Parent<<' '<<t->Amount<<' '<<unsigned(t->Operator)<<'\n';if(auto c=criteria.LookupEntry(t->CriteriaID))std::cout<<"criteria "<<id<<' '<<t->ID<<' '<<t->Parent<<' '<<t->Amount<<' '<<c->ID<<' '<<unsigned(c->Type)<<' '<<c->Asset<<' '<<c->StartTimer<<'\n';}
    }
    for(auto e:effects)if(e->SpellID==208513||e->SpellID==208516||e->SpellID==208514||e->SpellID==208517)
        std::cout<<"effect "<<e->SpellID<<' '<<unsigned(e->DifficultyID)<<' '<<unsigned(e->EffectIndex)<<' '<<e->Effect<<' '<<e->EffectMiscValue[0]<<' '<<unsigned(e->ImplicitTarget[0])<<' '<<unsigned(e->ImplicitTarget[1])<<'\n';
    for(auto p:packages)if(p->PackageID==9719||p->PackageID==9720)
        std::cout<<"package "<<p->PackageID<<' '<<p->ItemID<<' '<<p->ItemQuantity<<' '<<unsigned(p->DisplayType)<<'\n';
}
