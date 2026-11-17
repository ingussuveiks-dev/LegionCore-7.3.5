// Read selected independent artifact/quest items through the real sparse reader.
// This does not initialize a world, actor, quest, connection or character.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
int main(int argc,char** argv)
{
    char const* path=std::getenv("LEGION_ARTIFACT_REVIEW_ITEMS");
    if(argc!=2||!path)return 1;
    std::ifstream input(path);
    if(!input)return 2;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    DB2Storage<ArtifactEntry> artifacts("Artifact.db2",ArtifactLoadInfo::Instance());
    DB2Storage<ChrSpecializationEntry> specs("ChrSpecialization.db2",ChrSpecializationLoadInfo::Instance());
    if(!items.Load(argv[1],0)||!artifacts.Load(argv[1],0)||!specs.Load(argv[1],0))return 3;
    uint32 id;
    while(input>>id)
    {
        auto item=items.LookupEntry(id);
        auto artifact=item?artifacts.LookupEntry(item->ArtifactID):nullptr;
        auto spec=artifact?specs.LookupEntry(artifact->ChrSpecializationID):nullptr;
        std::cout<<"ITEM\t"<<id<<'\t'<<bool(item)<<'\t'<<(item?unsigned(item->ArtifactID):0)<<'\t'
            <<(spec?unsigned(spec->ClassID):0)<<'\t'<<(spec?spec->ID:0)<<'\n';
    }
    return input.eof()?0:4;
}
