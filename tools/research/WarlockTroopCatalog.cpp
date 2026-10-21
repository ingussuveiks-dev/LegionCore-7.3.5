#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<CharShipmentEntry> shipments("CharShipment.db2",CharShipmentLoadInfo::Instance());
    DB2Storage<CharShipmentContainerEntry> containers("CharShipmentContainer.db2",CharShipmentContainerLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<SpellReagentsCurrencyEntry> costs("SpellReagentsCurrency.db2",SpellReagentsCurrencyLoadInfo::Instance());
    if(!shipments.Load(argv[1],0)||!containers.Load(argv[1],0)||!effects.Load(argv[1],0)||!costs.Load(argv[1],0))return 2;
    std::set<int> spells;
    for(auto s:shipments)if(s->ContainerID==129){
        std::cout<<"S\t"<<s->ID<<'\t'<<s->ContainerID<<'\t'<<s->Duration<<'\t'<<s->SpellID<<'\t'<<s->OnCompleteSpellID<<'\t'<<s->GarrFollowerID<<'\t'<<unsigned(s->MaxShipments)<<'\t'<<unsigned(s->Flags)<<'\t'<<s->DummyItemID<<'\n';
        spells.insert(s->SpellID);spells.insert(s->OnCompleteSpellID);
    }
    auto c=containers.LookupEntry(129);if(!c)return 3;
    std::cout<<"C\t"<<c->ID<<'\t'<<unsigned(c->GarrTypeID)<<'\t'<<unsigned(c->GarrBuildingType)<<'\t'<<unsigned(c->BaseCapacity)<<'\n';
    for(auto e:effects)if(spells.count(e->SpellID))std::cout<<"E\t"<<e->SpellID<<'\t'<<e->Effect<<'\t'<<e->EffectMiscValue[0]<<'\t'<<e->EffectBasePoints<<'\t'<<e->EffectTriggerSpell<<'\n';
    for(auto r:costs)if(spells.count(r->SpellID))std::cout<<"R\t"<<r->SpellID<<'\t'<<r->CurrencyTypesID<<'\t'<<r->CurrencyCount<<'\n';
}
