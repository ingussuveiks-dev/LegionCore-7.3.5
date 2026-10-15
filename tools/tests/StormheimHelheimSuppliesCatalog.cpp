#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    if(!items.Load(argv[1],0)||!locks.Load(argv[1],0)||!objects.Load(argv[1],0)||!actors.Load(argv[1],0)||!models.Load(argv[1],0))return 2;
    for(uint32 id:{122611u,122619u}){
        auto item=items.LookupEntry(id);uint32 required=id==122611?8:20;
        if(!item||(item->MaxCount&&item->MaxCount<required)||item->Stackable<1||item->Bonding!=4)return 3;
        std::cout<<"item"<<id<<" name="<<item->Display->Str[0]<<" max="<<item->MaxCount<<" stack="<<item->Stackable<<" bonding="<<unsigned(item->Bonding)<<" races="<<item->AllowableRace<<" flags="<<item->Flags[0]<<","<<item->Flags[1]<<","<<item->Flags[2]<<'\n';
    }
    auto lock=locks.LookupEntry(2160);if(!lock)return 4;
    for(unsigned i=0;i<8;++i){std::cout<<"lock2160 case"<<i<<" type="<<unsigned(lock->Type[i])<<" index="<<lock->Index[i]<<" skill="<<lock->Skill[i]<<'\n';if(lock->Type[i]!=(i==1?2:0)||lock->Index[i]!=(i==1?13:0)||lock->Skill[i])return 5;}
    auto pile=objects.LookupEntry(6328);if(!pile||!pile->FileDataID)return 6;
    std::cout<<"bones6328 file="<<pile->FileDataID<<'\n';
    for(uint32 id:{25633u,70917u,63385u}){
        auto actor=actors.LookupEntry(id);if(!actor)return 7;auto model=models.LookupEntry(actor->ModelID);if(!model||!model->FileDataID)return 8;
        std::cout<<"actor"<<id<<" model="<<actor->ModelID<<" file="<<model->FileDataID<<'\n';
    }
    std::cout<<"PASS: native sparse bones/seaweed limits, Gathering lock2160/type13, pile model and both Kvaldir/Colborn display/model chains. No rendered/looting-client verification.\n";
}
