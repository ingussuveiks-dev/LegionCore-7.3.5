#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ItemSparseEntry> sparse("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    DB2Storage<ItemEntry> items("Item.db2",ItemLoadInfo::Instance());
    DB2Storage<ItemEffectEntry> uses("ItemEffect.db2",ItemEffectLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<QuestLineXQuestEntry> lines("QuestLineXQuest.db2",QuestLineXQuestLoadInfo::Instance());
    if(!sparse.Load(argv[1],0)||!items.Load(argv[1],0)||!uses.Load(argv[1],0)||!locks.Load(argv[1],0)||!effects.Load(argv[1],0)||!objects.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!lines.Load(argv[1],0))return 2;
    auto s=sparse.LookupEntry(123887);auto item=items.LookupEntry(123887);if(!s||!item||s->Stackable<1||s->Bonding!=4)return 3;
    std::cout<<"item123887 "<<s->Display->Str[0]<<" max="<<s->MaxCount<<" stack="<<s->Stackable<<" bonding="<<unsigned(s->Bonding)<<" class="<<unsigned(item->ClassID)<<'\n';
    unsigned casts=0;for(auto u:uses)if(u->ItemID==123887)++casts;std::cout<<"item123887 use effects "<<casts<<'\n';
    auto lock=locks.LookupEntry(1690);if(!lock||lock->Type[1]!=2||lock->Index[1]!=5||lock->Skill[1])return 4;
    for(unsigned i=0;i<8;++i)if(i!=1&&(lock->Type[i]||lock->Index[i]||lock->Skill[i]))return 4;
    bool open=false;for(auto e:effects)if(e->SpellID==3365&&e->Effect==33&&e->EffectMiscValue[0]==5&&e->ImplicitTarget[0]==23)open=true;
    if(!open)return 5;std::cout<<"native lock1690 type5 and Open3365 effect33 target23\n";
    auto go=objects.LookupEntry(8644);if(!go||!go->FileDataID)return 6;
    std::cout<<"object8644 file "<<go->FileDataID<<" minZ "<<go->GeoBoxMin.Z<<" maxZ "<<go->GeoBoxMax.Z<<'\n';
    auto d=displays.LookupEntry(7028);if(!d)return 7;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 7;
    std::cout<<"Overspark7028 model "<<m->ID<<" file "<<m->FileDataID<<'\n';
    bool found=false;for(auto q:lines)if(q->QuestID==38558){if(q->QuestLineID!=128||q->OrderIndex!=3)return 8;found=true;std::cout<<"quest38558 line128 order3\n";}if(!found)return 8;
}
