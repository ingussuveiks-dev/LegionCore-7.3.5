#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<LockEntry> locks("Lock.db2",LockLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<GameObjectDisplayInfoEntry> objects("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
    DB2Storage<BroadcastTextEntry> texts("BroadcastText.db2",BroadcastTextLoadInfo::Instance());
    if(!locks.Load(argv[1],0)||!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!objects.Load(argv[1],0)||!texts.Load(argv[1],0))return 2;
    auto lock=locks.LookupEntry(1691);if(!lock)return 3;
    for(unsigned i=0;i<8;++i){
        std::cout<<"lock1691 slot="<<i<<" type="<<unsigned(lock->Type[i])<<" index="<<lock->Index[i]<<" skill="<<lock->Skill[i]<<'\n';
        if(lock->Type[i]!=(i==1?2:0)||lock->Index[i]!=(i==1?5:0)||lock->Skill[i])return 4;
    }
    for(uint32 id:{63347u,63369u,63370u,328u}){auto row=actors.LookupEntry(id);if(!row)return 5;auto model=models.LookupEntry(row->ModelID);if(!model||!model->FileDataID)return 8;std::cout<<"display"<<id<<" model="<<row->ModelID<<" file="<<model->FileDataID<<'\n';}
    auto bowl=objects.LookupEntry(14638);if(!bowl)return 6;std::cout<<"bowl14638 file="<<bowl->FileDataID<<'\n';
    for(uint32 id:{95185u,95188u,95192u,95193u,95196u}){
        auto row=texts.LookupEntry(id);if(!row){std::cout<<"broadcast"<<id<<" native=0 (requires checked Hotfix overlay)\n";continue;}
        std::cout<<"broadcast"<<id<<" sound="<<row->SoundEntriesID[0]<<","<<row->SoundEntriesID[1]<<'\n';
    }
    std::cout<<"PASS: native ordinary Open lock1691/type5, three ruler displays, helper and bowl; missing native dialogue IDs explicitly require checked Hotfix overlays.\n";
}
