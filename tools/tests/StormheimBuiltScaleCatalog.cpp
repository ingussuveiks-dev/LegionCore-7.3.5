#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ItemSparseEntry> sparse("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    DB2Storage<ItemEffectEntry> uses("ItemEffect.db2",ItemEffectLoadInfo::Instance());
    DB2Storage<QuestPackageItemEntry> packages("QuestPackageItem.db2",QuestPackageItemLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> displays("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    if(!sparse.Load(argv[1],0)||!uses.Load(argv[1],0)||!packages.Load(argv[1],0)||!displays.Load(argv[1],0)||!models.Load(argv[1],0))return 2;
    for(auto id:{122609u,122610u,124037u}){
        auto s=sparse.LookupEntry(id);if(!s||s->MaxCount||s->Stackable!=(id==122609?50:1)||s->StartQuestID!=(id==122610?38337:id==124037?38616:0)||s->Bonding!=(id==122609?4:1)||s->AllowableRace!=-1||s->Flags[0]!=67584||s->Flags[1]!=8192)return 3;
        unsigned effects=0;for(auto u:uses)if(u->ItemID==id)++effects;if(effects)return 3;
        std::cout<<"item "<<id<<" "<<s->Display->Str[0]<<" quest="<<s->StartQuestID<<" max="<<s->MaxCount<<" stack="<<s->Stackable<<" bonding="<<unsigned(s->Bonding)<<" race="<<s->AllowableRace<<" flags="<<s->Flags[0]<<","<<s->Flags[1]<<","<<s->Flags[2]<<","<<s->Flags[3]<<" use="<<effects<<'\n';
    }
    unsigned rewards=0;for(auto p:packages)if(p->PackageID==651){if(!sparse.LookupEntry(p->ItemID)||p->ItemQuantity!=1)return 4;++rewards;std::cout<<"package651 "<<p->ItemID<<" quantity="<<p->ItemQuantity<<" display="<<unsigned(p->DisplayType)<<'\n';}if(rewards!=8)return 4;
    for(auto id:{70608u,70589u,70590u}){auto d=displays.LookupEntry(id);if(!d)return 5;auto m=models.LookupEntry(d->ModelID);if(!m||!m->FileDataID)return 5;std::cout<<"display "<<id<<" model="<<m->ID<<" file="<<m->FileDataID<<'\n';}
}
