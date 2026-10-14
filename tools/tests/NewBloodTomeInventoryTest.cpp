#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
using uint8=uint8_t;
using uint32=uint32_t;
#define TC_LOG_DEBUG(...) ((void)0)
#include "InventorySlots.inc"
enum {EQUIP_ERR_OK=0,ITEM_CHANGED=1};
struct Player;
struct Template {int GetGemProperties(){return 0;}int GetSocketType(int){return 0;}};
struct Item {
    uint32 entry,count=1;bool world=false,trade=false,removed=false;
    Template proto;
    uint32 GetEntry(){return entry;}uint32 GetCount(){return count;}
    bool IsInWorld(){return world;}bool IsInTrade(){return trade;}
    Template* GetTemplate(){return &proto;}
    int GetSocketColor(int){return 0;}
    uint32 GetGemCountWithID(uint32){return 0;}
    void SetCount(uint32 value){count=value;}
    void SetState(int,Player*){}void SendUpdateToPlayer(Player*){}
};
struct Bag {
    Item* m_bagslot[32]={};
    uint32 GetBagSize() const{return 32;}
    Item* GetItemByPos(uint32 slot){return m_bagslot[slot];}
    uint32 GetItemCount(uint32,Item*) const;
};
#include "BagCount.inc"
struct Player {
    std::map<uint8,Item*> slots;
    std::map<uint8,Bag*> bags;
    uint32 GetInventoryEndSlot() const{return INVENTORY_SLOT_ITEM_END;}
    Item* GetItemByPos(uint8 bag,uint8 slot) const{
        if(bag==INVENTORY_SLOT_BAG_0)return slots.count(slot)?slots.at(slot):nullptr;
        return bags.count(bag)?bags.at(bag)->GetItemByPos(slot):nullptr;
    }
    Bag* GetBagByPos(uint8 slot) const{return bags.count(slot)?bags.at(slot):nullptr;}
    uint32 GetItemCount(uint32,bool,Item* =nullptr) const;
    bool IsInWorld(){return false;}
    int CanUnequipItem(int,bool){return EQUIP_ERR_OK;}
    void ItemRemovedQuestCheck(uint32,uint32){}
    void DestroyItem(uint8 bag,uint8 slot,bool update){
        assert(!update);
        Item* item=GetItemByPos(bag,slot);assert(item);item->removed=true;
        if(bag==INVENTORY_SLOT_BAG_0)slots.erase(slot);
        else bags.at(bag)->m_bagslot[slot]=nullptr;
    }
    void DestroyItemCount(uint32,uint32,bool,bool=false);
    void restart(bool restartNewBloodAtPrison){
#include "InventoryRestore.inc"
    }
};
#include "DestroyCount.inc"
#include "PlayerCount.inc"
int main(){
    // Real Bag counter excludes not-yet-in-world items. Real DestroyItemCount
    // must still remove those entries when executing the login cleanup block.
    for(uint8 slot:{uint8(INVENTORY_SLOT_BAG_START),uint8(BANK_SLOT_BAG_START)}){
        Player p;Bag bag;Item tome{132512},other{999999};
        p.bags[slot]=&bag;bag.m_bagslot[0]=&tome;bag.m_bagslot[1]=&other;
        assert(bag.GetItemCount(132512,nullptr)==0);
        assert(p.GetItemCount(132512,true)==0); // previous login guard skips removal
        p.restart(false);assert(!tome.removed && !other.removed);
        p.restart(true);assert(tome.removed && !other.removed && !bag.m_bagslot[0]);
        p.restart(true);assert(!other.removed); // absent/repeated cleanup
        tome.removed=false;tome.world=true;bag.m_bagslot[0]=&tome;
        assert(bag.GetItemCount(132512,nullptr)==1);
        p.restart(true);assert(tome.removed && !other.removed);
    }
    for(uint8 slot:{uint8(INVENTORY_SLOT_ITEM_START),uint8(BANK_SLOT_ITEM_START)}){
        Player p;Item tome{132512};p.slots[slot]=&tome;p.restart(true);assert(tome.removed);
    }
    std::cout<<"PASS: native Bag count reproduces zero during loading; native DestroyItemCount and login cleanup remove loaded bag/bank/backpack tome, preserve other items and live scenario, and tolerate absent items\n";
}
