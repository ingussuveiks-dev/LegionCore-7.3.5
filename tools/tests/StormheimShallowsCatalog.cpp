#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    DB2Storage<ItemEffectEntry> uses("ItemEffect.db2",ItemEffectLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!items.Load(argv[1],0)||!uses.Load(argv[1],0)||!objects.Load(argv[1],0)||!locks.Load(argv[1],0))return 2;
    auto i=items.LookupEntry(129174);
    if(!i||i->Stackable!=1||i->MaxCount||i->Bonding!=4||i->Flags[0]!=2048||i->Flags[1]!=12288||i->StartQuestID)return 3;
    for(auto u:uses)if(u->ItemID==129174)return 3;
    std::cout<<"item129174 "<<i->Display->Str[0]<<" stack1 max0 quest-bonding4 flags2048/12288 no use or start quest\n";
    unsigned credit=0,opening=0;
    for(auto id:{227818u,3365u}){
        auto s=spells.LookupEntry(id);if(!s)return 4;
        std::cout<<"spell"<<id<<" "<<s->Name->Str[0]<<'\n';
        for(auto e:effects)if(e->SpellID==id){
            std::cout<<" effect="<<e->Effect<<" misc="<<e->EffectMiscValue[0]<<" item="<<e->EffectItemType<<" trigger="<<e->EffectTriggerSpell<<" targets="<<e->ImplicitTarget[0]<<","<<e->ImplicitTarget[1]<<'\n';
            if(id==227818){if(e->Effect!=90||e->EffectMiscValue[0]!=114487||e->EffectItemType||e->EffectTriggerSpell||e->ImplicitTarget[0]!=1||e->ImplicitTarget[1])return 4;++credit;}
            else {if(e->Effect!=33||e->EffectMiscValue[0]!=5||e->ImplicitTarget[0]!=23||e->ImplicitTarget[1])return 4;++opening;}
        }
    }
    if(credit!=1||opening!=1)return 4;
    auto d=objects.LookupEntry(35915);if(!d||d->FileDataID!=1103594)return 5;
    std::cout<<"object35915 file="<<d->FileDataID<<" unscaled bounds="<<d->GeoBoxMin.X<<","<<d->GeoBoxMin.Y<<","<<d->GeoBoxMin.Z<<".."<<d->GeoBoxMax.X<<","<<d->GeoBoxMax.Y<<","<<d->GeoBoxMax.Z<<'\n';
    auto l=locks.LookupEntry(1691);if(!l)return 6;unsigned routes=0;
    for(unsigned n=0;n<8;++n)if(l->Type[n]){if(l->Type[n]!=2||l->Index[n]!=5||l->Skill[n])return 6;++routes;}
    if(routes!=1)return 6;
    std::cout<<"lock1691 has one skill-key route: generic Opening5, zero required skill. No custom spell/script needed.\n";
}
