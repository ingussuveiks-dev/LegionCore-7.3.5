#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "ObjectMgr.h"
#include "ItemTemplate.h"
#include <iostream>
#include <map>
#undef sDB2Manager

struct Overrides
{
    std::map<uint32,std::vector<ItemSpecOverrideEntry const*>> rows;
    auto GetItemSpecOverrides(uint32 id)const -> std::vector<ItemSpecOverrideEntry const*> const*
    {auto it=rows.find(id);return it==rows.end()?nullptr:&it->second;}
};
uint32 NativeClassMask(ItemEntry const* db2Data,ItemSparseEntry const* sparse,Overrides const& overrides,
    DB2Storage<ItemSpecEntry> const& specs,DB2Storage<ChrSpecializationEntry> const& specializations)
{
    ItemTemplate itemTemplate;itemTemplate.ItemSpecClassMask=0;
    auto& sDB2Manager=overrides;auto& sItemSpecStore=specs;auto& sChrSpecializationStore=specializations;
#include "NativeItemClassMask.inc"
    return itemTemplate.ItemSpecClassMask;
}
int main(int argc,char** argv)
{
    if(argc!=2)return 1;
    DB2Storage<ItemEntry> basics("Item.db2",ItemLoadInfo::Instance());
    DB2Storage<ItemSparseEntry> items("ItemSparse.db2",ItemSparseLoadInfo::Instance());
    DB2Storage<ItemEffectEntry> effects("ItemEffect.db2",ItemEffectLoadInfo::Instance());
    DB2Storage<QuestPackageItemEntry> packages("QuestPackageItem.db2",QuestPackageItemLoadInfo::Instance());
    DB2Storage<ItemSpecEntry> specs("ItemSpec.db2",ItemSpecLoadInfo::Instance());
    DB2Storage<ItemSpecOverrideEntry> overrides("ItemSpecOverride.db2",ItemSpecOverrideLoadInfo::Instance());
    DB2Storage<ChrSpecializationEntry> specializations("ChrSpecialization.db2",ChrSpecializationLoadInfo::Instance());
    DB2Storage<CreatureDisplayInfoEntry> actors("CreatureDisplayInfo.db2",CreatureDisplayInfoLoadInfo::Instance());
    DB2Storage<CreatureModelDataEntry> models("CreatureModelData.db2",CreatureModelDataLoadInfo::Instance());
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SoundKitEntry> sounds("SoundKit.db2",SoundKitLoadInfo::Instance());
    if(!basics.Load(argv[1],0)||!items.Load(argv[1],0)||!effects.Load(argv[1],0)||!packages.Load(argv[1],0)||!specs.Load(argv[1],0)||!overrides.Load(argv[1],0)||!specializations.Load(argv[1],0)||!actors.Load(argv[1],0)||!models.Load(argv[1],0)||!spells.Load(argv[1],0)||!sounds.Load(argv[1],0))return 2;
    auto item=items.LookupEntry(128854);
    if(!item||item->MaxCount||item->Stackable!=1||item->Bonding!=4||item->AllowableClass!=-1||item->AllowableRace!=-1)return 3;
    std::cout<<"item128854 name="<<item->Display->Str[0]<<" max=0 stack=1 bonding=4 flags="<<item->Flags[0]<<","<<item->Flags[1]<<","<<item->Flags[2]<<" classes=-1 races=-1\n";
    for(auto e:effects)if(e->ItemID==128854)return 4;
    std::cout<<"item128854 useEffects=0\n";
    Overrides nativeOverrides;for(auto e:overrides)nativeOverrides.rows[e->ItemID].push_back(e);
    uint32 count=0,classes=0;
    for(auto p:packages)if(p->PackageID==746){
        auto sparse=items.LookupEntry(p->ItemID);auto basic=basics.LookupEntry(p->ItemID);
        if(!sparse||!basic||p->ItemQuantity!=1||p->DisplayType!=QUEST_PACKAGE_FILTER_CLASS||(sparse->Flags[1]&(ITEM_FLAG2_FACTION_HORDE|ITEM_FLAG2_FACTION_ALLIANCE)))return 5;
        auto mask=NativeClassMask(basic,sparse,nativeOverrides,specs,specializations);classes|=mask?mask:0xFFF;
        std::cout<<"package746 row="<<p->ID<<" item="<<p->ItemID<<" name="<<sparse->Display->Str[0]<<" display=class quantity=1 nativeClassMask="<<mask<<" flags2="<<sparse->Flags[1]<<'\n';++count;
    }
    if(count!=2||(classes&0xFFF)!=0xFFF)return 6;
    auto actor=actors.LookupEntry(62703);if(!actor)return 7;auto model=models.LookupEntry(actor->ModelID);if(!model||!model->FileDataID)return 8;
    std::cout<<"Bloodbeard62703 model="<<actor->ModelID<<" file="<<model->FileDataID<<'\n';
    for(uint32 id:{186529u,188271u,186409u}){auto spell=spells.LookupEntry(id);if(!spell)return 9;std::cout<<"spell"<<id<<" name="<<spell->Name->Str[0]<<'\n';}
    if(!sounds.LookupEntry(53001))return 10;
    std::cout<<"PASS: native Waylight delivery item/no use effects, two package746 class-filtered rewards cover all12 classes/both factions using the production mask builder, Bloodbeard model/three combat spells and Ashildir voice53001. Client combat/loot/reward UI remain unverified.\n";
}
