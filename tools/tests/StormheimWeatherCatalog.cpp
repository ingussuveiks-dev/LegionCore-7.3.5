#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>
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
    DB2Storage<QuestPackageItemEntry> packages("QuestPackageItem.db2",QuestPackageItemLoadInfo::Instance());
    if(!sparse.Load(argv[1],0)||!items.Load(argv[1],0)||!uses.Load(argv[1],0)||!locks.Load(argv[1],0)||!effects.Load(argv[1],0)||!objects.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0)||!lines.Load(argv[1],0)||!packages.Load(argv[1],0))return 2;
    for(auto id:{124038u,122685u,122686u}){
        auto s=sparse.LookupEntry(id);auto item=items.LookupEntry(id);if(!s||!item||s->Stackable!=1||s->Bonding!=4||item->ClassID!=12)return 3;
        unsigned casts=0;for(auto u:uses)if(u->ItemID==id)++casts;if(casts)return 3;
        std::cout<<"item"<<id<<" "<<s->Display->Str[0]<<" max="<<s->MaxCount<<" stack="<<s->Stackable<<" bonding="<<unsigned(s->Bonding)<<" class="<<unsigned(item->ClassID)<<" use="<<casts<<'\n';
    }
    auto lock=locks.LookupEntry(1690);if(!lock||lock->Type[1]!=2||lock->Index[1]!=5||lock->Skill[1])return 4;
    bool open=false;for(auto e:effects)if(e->SpellID==3365&&e->Effect==33&&e->EffectMiscValue[0]==5&&e->ImplicitTarget[0]==23)open=true;
    if(!open)return 4;std::cout<<"native lock1690/Open3365 effect33 type5 target23\n";
    for(auto id:{25701u,25704u}){auto go=objects.LookupEntry(id);if(!go||!go->FileDataID)return 5;std::cout<<"object"<<id<<" file"<<go->FileDataID<<" minZ="<<go->GeoBoxMin.Z<<" maxZ="<<go->GeoBoxMax.Z<<'\n';}
    for(auto id:{62485u,34520u,65132u}){auto d=displays.LookupEntry(id);if(!d)return 6;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 6;std::cout<<"actor"<<id<<" model"<<m->ID<<" file"<<m->FileDataID<<'\n';}
    std::set<unsigned> found;
    for(auto q:lines)if(q->QuestID==38405||q->QuestID==38614){if(q->QuestLineID!=(q->QuestID==38405?132:133)||q->OrderIndex!=3)return 7;found.insert(q->QuestID);std::cout<<"quest"<<q->QuestID<<" line"<<q->QuestLineID<<" order"<<unsigned(q->OrderIndex)<<'\n';}if(found.size()!=2)return 7;
    unsigned rewards=0;for(auto p:packages)if(p->PackageID==647){auto item=sparse.LookupEntry(p->ItemID);if(!item||p->ItemQuantity!=1)return 8;++rewards;std::cout<<"package647 item"<<p->ItemID<<" displayType"<<unsigned(p->DisplayType)<<" quantity"<<p->ItemQuantity<<'\n';}if(!rewards)return 8;
}
