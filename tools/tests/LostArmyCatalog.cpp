#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "ObjectMgr.h"
#include "ItemTemplate.h"
#include <iostream>
#include <map>
#include <cassert>
#include <set>
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
int main(int argc,char** argv){if(argc!=2)return 1;
#define S(T,N,F,I) DB2Storage<T>N(F ".db2",I::Instance());if(!N.Load(argv[1],0))return 2
S(ItemEntry,basics,"Item",ItemLoadInfo);S(ItemSparseEntry,items,"ItemSparse",ItemSparseLoadInfo);S(ItemEffectEntry,effects,"ItemEffect",ItemEffectLoadInfo);S(QuestPackageItemEntry,packages,"QuestPackageItem",QuestPackageItemLoadInfo);S(ItemSpecEntry,specs,"ItemSpec",ItemSpecLoadInfo);S(ItemSpecOverrideEntry,overrides,"ItemSpecOverride",ItemSpecOverrideLoadInfo);S(ChrSpecializationEntry,specializations,"ChrSpecialization",ChrSpecializationLoadInfo);S(SpellEffectEntry,spellEffects,"SpellEffect",SpellEffectLoadInfo);S(SpellEntry,spells,"Spell",SpellLoadInfo);
S(TransmogSetEntry,sets,"TransmogSet",TransmogSetLoadInfo);S(TransmogSetItemEntry,setItems,"TransmogSetItem",TransmogSetItemLoadInfo);S(ItemModifiedAppearanceEntry,appearances,"ItemModifiedAppearance",ItemModifiedAppearanceLoadInfo);
Overrides nativeOverrides;for(auto e:overrides)nativeOverrides.rows[e->ItemID].push_back(e);
auto f=items.LookupEntry(142487);
assert(f&&f->MaxCount==0&&f->Stackable==1000&&f->Bonding==4);
std::cout<<"fragment "<<f->Display->Str[0]<<" max="<<f->MaxCount<<" stack="<<f->Stackable<<" flags="<<f->Flags[0]<<","<<f->Flags[1]<<","<<f->Flags[2]<<" bond="<<int(f->Bonding)<<" class="<<f->AllowableClass<<"\n";
std::map<uint32,uint32> expected{{143518,183},{143514,179},{143510,175},{143506,171}};
uint32 count=0,allMasks=0;std::vector<uint32> masks;
for(auto p:packages)if(p->PackageID==18559){
    auto i=items.LookupEntry(p->ItemID);auto b=basics.LookupEntry(p->ItemID);assert(i&&b);
    auto mask=NativeClassMask(b,i,nativeOverrides,specs,specializations);
    assert(expected.count(p->ItemID)&&p->DisplayType==1&&p->ItemQuantity==1&&mask==uint16_t(i->AllowableClass));
    assert(!(i->Flags[1]&(ITEM_FLAG2_FACTION_ALLIANCE|ITEM_FLAG2_FACTION_HORDE)));
    ++count;allMasks|=mask;masks.push_back(mask);
    std::cout<<"package "<<p->ID<<" item="<<p->ItemID<<" classMask="<<mask<<" "<<i->Display->Str[0]<<"\n";
    uint32 uses=0,learns=0;
    for(auto e:effects)if(e->ItemID==p->ItemID){
        auto spell=spells.LookupEntry(e->SpellID);assert(spell&&e->TriggerType==0);++uses;
        for(auto x:spellEffects)if(x->SpellID==e->SpellID){
            assert(x->Effect==255&&x->EffectMiscValue[0]==expected.at(p->ItemID)&&sets.LookupEntry(x->EffectMiscValue[0]));++learns;
            std::cout<<"use spell="<<e->SpellID<<" effect255 set="<<x->EffectMiscValue[0]<<"\n";
        }
    }
    assert(uses==1&&learns==1);
}
assert(count==4&&allMasks==4095);
for(uint32 cls=0;cls<12;++cls){uint32 n=0;for(auto mask:masks)if(mask&(1u<<cls))++n;assert(n==1);}
for(auto const& pair:expected){
    uint32 n=0;
    for(auto row:setItems)if(row->TransmogSetID==pair.second){assert(appearances.LookupEntry(row->ItemModifiedAppearanceID));++n;}
    assert(n>0);std::cout<<"transmogSet "<<pair.second<<" appearances="<<n<<"\n";
}
std::cout<<"PASS: native fragment142487, package18559 four class masks cover each of12 classes once, real use spells/effect255 and existing transmog set appearances. Collection saving and client presentation are pending.\n";
}
