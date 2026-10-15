// Sparse native ItemSparse records must use the server reader, not fixed offsets.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    DB2Storage<ArtifactEntry> artifacts("Artifact.db2",ArtifactLoadInfo::Instance());
    DB2Storage<ChrSpecializationEntry> specs("ChrSpecialization.db2",ChrSpecializationLoadInfo::Instance());
    if(!items.Load(argv[1],0)||!artifacts.Load(argv[1],0)||!specs.Load(argv[1],0))return 2;
    std::cout<<"ID\tName\tArtifactID\tAllowableClass\tAllowableRace\tMaxCount\tStackable\tInventoryType\tSpecID\tClassID\tSpecialization\n";
    for(auto item:items)
        if(item->ArtifactID)
        {
            auto artifact=artifacts.LookupEntry(item->ArtifactID);
            auto spec=artifact?specs.LookupEntry(artifact->ChrSpecializationID):nullptr;
            std::cout<<item->ID<<'\t'<<item->Display->Str[0]<<'\t'<<unsigned(item->ArtifactID)<<'\t'
                <<item->AllowableClass<<'\t'<<item->AllowableRace<<'\t'<<item->MaxCount<<'\t'
                <<item->Stackable<<'\t'<<unsigned(item->InventoryType)<<'\t'<<(spec?spec->ID:0)<<'\t'
                <<(spec?unsigned(spec->ClassID):0)<<'\t'<<(spec?spec->Name->Str[0]:"")<<'\n';
        }
}
