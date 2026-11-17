#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using uint8 = uint8_t;
using uint32 = uint32_t;
using ObjectGuid = uint32;
enum { INVENTORY_SLOT_BAG_0 = 255, EQUIPMENT_SLOT_MAINHAND = 15, EQUIPMENT_SLOT_OFFHAND = 16,
    ITEM_FIELD_CREATOR, ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_CHILD, CLASS_HUNTER = 3,
    QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1 };
struct ArtifactEntry { uint32 ChrSpecializationID = 0; };
struct ChrSpecializationEntry { uint32 ID = 0, ClassID = 0; };
template<class T> struct Store
{
    std::map<uint32, T> records;
    T const* LookupEntry(uint32 id) const { auto p=records.find(id); return p==records.end()?nullptr:&p->second; }
};
Store<ArtifactEntry> sArtifactStore;
Store<ChrSpecializationEntry> sChrSpecializationStore;
struct ItemTemplate { uint32 artifact = 0; uint32 GetArtifactID() const { return artifact; } };
struct Item
{
    uint32 entry = 0, guid = 2, owner = 1, count = 1, creator = 0, child = 0;
    uint8 slot = 32;
    bool childFlag = false, carried = true;
    ItemTemplate proto;
    uint32 GetEntry() const { return entry; }
    uint32 GetGUID() const { return guid; }
    uint32 GetOwnerGUID() const { return owner; }
    uint32 GetCount() const { return count; }
    bool IsEquipped() const { return slot == 15 || slot == 16; }
    ItemTemplate const* GetTemplate() const { return &proto; }
    uint32 GetChildItem() const { return child; }
    uint32 GetGuidValue(int) const { return creator; }
    bool HasFlag(int, int) const { return childFlag; }
};
struct Player
{
    uint32 klass = 0, spec = 0, map = 1489, status = QUEST_STATUS_INCOMPLETE;
    bool gm = false, rewarded = false;
    std::vector<Item*> items;
    std::map<uint32, uint32> receipts;
    uint32 GetGUID() const { return 1; }
    uint32 getClass() const { return klass; }
    uint32 GetSpecializationId() const { return spec; }
    Item* GetItemByEntry(uint32 entry, bool bank = false) const
    { for (Item* i : items) if (i->entry == entry && (bank || i->carried)) return i; return nullptr; }
    Item* GetItemByPos(uint8 bag, uint8 slot) const
    { assert(bag==255); for (Item* i : items) if(i->carried && i->slot == slot) return i; return nullptr; }
    Item* GetChildItemByGuid(uint32 guid) const
    { for (Item* i : items) if (i->carried && i->guid == guid) return i; return nullptr; }
    uint32 GetMapId() const { return map; }
    bool isGameMaster() const { return gm; }
    bool GetQuestRewardStatus(uint32 q) const { assert(q==40419); return rewarded; }
    uint32 GetQuestStatus(uint32 q) const { assert(q==40419); return status; }
    uint32 GetQuestObjectiveData(uint32 q, uint32 o) const
    { assert(q==40419); auto p=receipts.find(o); return p==receipts.end()?0:p->second; }
    bool HasEarnedMarksmanshipReturn(bool requireSelection = true) const;
};
#include "Inventory.inc"
#include "EarnedReturn.inc"
#include "OutcomeGates.inc"

struct NativeItem { uint32 entry, artifact, spec, klass, type, child, childSlot; };
int main(int argc, char** argv)
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    assert(argc == 3);
    std::ifstream file(argv[1]);
    assert(file);
    std::map<uint32, NativeItem> native;
    std::string line;
    while (std::getline(file,line))
    {
        // Native exports may carry a PowerShell UTF-8 BOM.
        if(line.compare(0,3,"\xEF\xBB\xBF")==0) line.erase(0,3);
        std::istringstream row(line); std::string tag; row>>tag;
        if(tag=="spec") { uint32 id,klass; row>>id>>klass; sChrSpecializationStore.records[id]={id,klass}; }
        else if(tag=="item")
        { NativeItem i{}; row>>i.entry>>i.artifact>>i.spec>>i.klass>>i.type>>i.child>>i.childSlot;
          assert(row); native[i.entry]=i; sArtifactStore.records[i.artifact]={i.spec}; }
        else assert(false);
    }
    std::ifstream routes(argv[2]); assert(routes);
    std::set<uint32> tested, specs;
    uint32 entry, alternatives=0, pairs=0;
    while(routes>>entry)
    {
        assert(tested.insert(entry).second && native.count(entry));
        auto n=native.at(entry); specs.insert(n.spec);
        Player p; p.klass=n.klass; p.spec=n.spec;
        Item parent; parent.entry=entry; parent.proto.artifact=n.artifact;
        Item child; child.entry=n.child; child.guid=3; child.creator=parent.guid; child.childFlag=true; child.slot=80;
        if(n.child) parent.child=child.guid;
        p.items={&parent}; if(n.child) p.items.push_back(&child);
        uint8 slot = (n.type==14 || n.type==23) ? 16 : 15;
        auto ready=[&]{return ArtifactAcquisitionInventory::Ready(&p,entry,slot,n.child,uint8(n.childSlot));};
        assert(!ready()); // A matching-spec item merely in bags is no equip acknowledgement.
        assert(!ArtifactAcquisitionInventory::OtherSpecialization(&p,&parent.proto));
        parent.slot=slot; if(n.child) child.slot=uint8(n.childSlot);
        assert(ready());
        if(n.child) { ++pairs; auto old=parent.child; parent.child=999; assert(!ready()); parent.child=old; }
        parent.slot=32; child.slot=80;
        for(auto const& alternate:sChrSpecializationStore.records)
            if(alternate.second.ClassID==n.klass && alternate.first!=n.spec)
            {
                ++alternatives; p.spec=alternate.first;
                assert(ArtifactAcquisitionInventory::OtherSpecialization(&p,&parent.proto));
                assert(ready() && parent.slot==32 && child.slot==80);
                assert(ArtifactAcquisitionInventory::StoredForOtherSpecialization(&p,&parent,n.child));
                parent.carried=false; assert(!ready()); parent.carried=true;
                parent.owner=77; assert(!ready()); parent.owner=1;
                parent.count=0; assert(!ready()); parent.count=1;
                p.items.clear(); assert(!ready()); p.items={&parent}; if(n.child) p.items.push_back(&child);
                if(n.child)
                {
                    child.owner=77; assert(!ready()); child.owner=1;
                    child.creator=0; assert(!ready()); child.creator=parent.guid;
                    child.childFlag=false; assert(!ready()); child.childFlag=true;
                    child.carried=false; assert(!ready()); child.carried=true;
                    child.count=0; assert(!ready()); child.count=1;
                    ++child.entry; assert(!ready()); --child.entry;
                }
                // Execute actual scoped gate bodies after their independent
                // actor/quest/presentation admission; none can require equipping.
                if(entry==128872) assert(DreadbladesAcquisition::Equipped(&p));
                if(entry==128476) assert(DevourerAcquisition::Equipped(&p));
                if(entry==128870) assert(KingslayersAcquisition::Equipped(&p));
                if(entry==128825) assert(TuureAcquisition::Equipped(&p));
                if(entry==128868 || entry==127857) assert(LightsWrathPickup::Equipped(&p,entry));
                if(entry==128911) { assert(SharasdalAcquisition::Equipped(&p,parent.guid)); assert(!SharasdalAcquisition::Equipped(&p,999)); }
                uint32 real=parent.proto.artifact; parent.proto.artifact=99999; assert(!ready()); parent.proto.artifact=real;
                uint32 klass=p.klass; p.klass=99; assert(!ready()); p.klass=klass;
            }
        p.spec=0; assert(!ready());
    }
    assert(routes.eof() && tested.size()==36 && specs.size()==36 && pairs>10 && alternatives>=72);
    // Real bow receipt recovery: no item-only, early-stage or unselected auto transfer.
    Player hunter; hunter.klass=CLASS_HUNTER; hunter.spec=253;
    Item bow; bow.entry=128826; hunter.items={&bow};
    assert(!hunter.HasEarnedMarksmanshipReturn(false));
    for(uint32 receipt:{102128u,102129u,102131u,103490u}) hunter.receipts[receipt]=1;
    assert(hunter.HasEarnedMarksmanshipReturn(false) && !hunter.HasEarnedMarksmanshipReturn());
    hunter.receipts[103481]=1; assert(hunter.HasEarnedMarksmanshipReturn());
    for(uint32 receipt:{102128u,102129u,102131u,103490u})
    { hunter.receipts[receipt]=0; assert(!hunter.HasEarnedMarksmanshipReturn(false)); hunter.receipts[receipt]=1; }
    bow.owner=77; assert(!hunter.HasEarnedMarksmanshipReturn(false)); bow.owner=1;
    hunter.items.clear(); assert(!hunter.HasEarnedMarksmanshipReturn(false)); hunter.items={&bow};
    hunter.rewarded=true; assert(!hunter.HasEarnedMarksmanshipReturn(false)); hunter.rewarded=false;
    hunter.gm=true; assert(!hunter.HasEarnedMarksmanshipReturn(false)); hunter.gm=false;
    hunter.map=1; assert(!hunter.HasEarnedMarksmanshipReturn(false));
    std::cout<<"PASS: actual production inventory outcomes for36 native artifacts, "<<alternatives
        <<" off-spec combinations, "<<pairs<<" native pairs; owner/bank/missing/full-bag/metadata guards, seven scoped gates and real bow receipt/selection separation\n";
}
