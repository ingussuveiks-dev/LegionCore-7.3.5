#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <map>

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2", ItemSparseLoadInfo::Instance());
    DB2Storage<ArtifactEntry> artifacts("Artifact.db2", ArtifactLoadInfo::Instance());
    DB2Storage<ChrSpecializationEntry> specs("ChrSpecialization.db2", ChrSpecializationLoadInfo::Instance());
    DB2Storage<ItemChildEquipmentEntry> children("ItemChildEquipment.db2", ItemChildEquipmentLoadInfo::Instance());
    if (!items.Load(argv[1], 0) || !artifacts.Load(argv[1], 0) || !specs.Load(argv[1], 0) || !children.Load(argv[1], 0)) return 2;
    std::map<uint32, ItemChildEquipmentEntry const*> pairs;
    for (auto child : children) pairs[child->ItemID] = child;
    for (auto spec : specs)
        if (spec->ClassID)
            std::cout << "spec\t" << spec->ID << '\t' << unsigned(spec->ClassID) << '\n';
    for (auto item : items)
        if (item->ArtifactID)
        {
            auto artifact = artifacts.LookupEntry(item->ArtifactID);
            auto spec = artifact ? specs.LookupEntry(artifact->ChrSpecializationID) : nullptr;
            if (!artifact || !spec) return 3;
            auto found = pairs.find(item->ID);
            auto child = found != pairs.end() ? found->second : nullptr;
            std::cout << "item\t" << item->ID << '\t' << unsigned(item->ArtifactID) << '\t'
                << spec->ID << '\t' << unsigned(spec->ClassID) << '\t' << unsigned(item->InventoryType) << '\t'
                << (child ? child->ChildItemID : 0) << '\t' << (child ? int(child->ChildItemEquipSlot) : 0) << '\n';
        }
}
