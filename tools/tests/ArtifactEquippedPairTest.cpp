// Actual force-equip/paired preflight/movement methods; inventory services controlled.
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;
using SpellEffIndex=int;
enum {INVENTORY_SLOT_BAG_0=255,NULL_BAG=255,NULL_SLOT=255,INVTYPE_2HWEAPON=17,SPELL_EFFECT_HANDLE_HIT_TARGET=1};
enum InventoryResult {EQUIP_ERR_OK,EQUIP_ERR_BAG_FULL,EQUIP_ERR_CANT_SWAP,EQUIP_ERR_LOCKED};
struct Guid {uint32 value=0;bool IsEmpty()const{return !value;}bool operator==(Guid other)const{return value==other.value;}};
struct ItemTemplate {uint32 GetArtifactID()const{return 1;}uint32 GetInventoryType()const{return 13;}};
struct Item {
 uint32 entry,id;uint16 pos;Guid child;ItemTemplate proto;uint32 copies=0;
 bool IsEquipped()const{return (pos>>8)==255&&(pos&255)<19;}
 bool IsBag()const{return false;}uint16 GetPos()const{return pos;}uint8 GetBagSlot()const{return pos>>8;}uint8 GetSlot()const{return pos;}
 uint32 GetEntry()const{return entry;}Guid GetChildItem()const{return child;}ItemTemplate const* GetTemplate()const{return &proto;}
 void CopyArtifactDataFromParent(Item*){++copies;}
};
struct ItemChildEquipmentEntry {uint32 ChildItemID;uint8 ChildItemEquipSlot;};
struct DB2Manager {std::map<uint32,ItemChildEquipmentEntry> pairs;auto GetItemChildEquipment(uint32 id)const{auto it=pairs.find(id);return it==pairs.end()?nullptr:&it->second;}} sDB2Manager;
struct ItemPosCount {uint16 pos;uint32 count;};using ItemPosCountVec=std::vector<ItemPosCount>;
struct Player {
 std::map<uint16,Item*> items;std::map<uint32,Item*> owned;uint32 errors=0,swaps=0,moves=0;bool full=false,locked=false;InventoryResult last=EQUIP_ERR_OK;
 static bool IsInventoryPos(uint16 p){return (p>>8)==255&&(p&255)>=23&&(p&255)<39;}
 static bool IsInventoryPos(uint8 b,uint8 s){return IsInventoryPos(uint16(b)<<8|s);}
 static bool IsBankPos(uint16 p){return (p>>8)==255&&(p&255)>=39&&(p&255)<67;}
 static bool IsBankPos(uint8 b,uint8 s){return IsBankPos(uint16(b)<<8|s);}
 static bool IsEquipmentPos(uint16 p){return (p>>8)==255&&(p&255)<19;}
 static bool IsEquipmentPos(uint8 b,uint8 s){return IsEquipmentPos(uint16(b)<<8|s);}
 Item* GetItemByPos(uint16 p)const{auto it=items.find(p);return it==items.end()?nullptr:it->second;}
 Item* GetItemByPos(uint8 b,uint8 s)const{return GetItemByPos(uint16(b)<<8|s);}
 Item* GetItemByEntry(uint32 entry)const{for(auto const& p:owned)if(p.second->entry==entry)return p.second;return nullptr;}
 Item* GetChildItemByGuid(Guid guid)const{auto it=owned.find(guid.value);return it==owned.end()?nullptr:it->second;}
 InventoryResult CanUnequipItem(uint16,bool)const{return locked?EQUIP_ERR_LOCKED:EQUIP_ERR_OK;}
 InventoryResult CanStoreItem(uint8,uint8,ItemPosCountVec& destination,Item*,bool)const{
  if(full)return EQUIP_ERR_BAG_FULL;
  for(uint8 slot=23;slot<39;++slot)if(!GetItemByPos(255,slot)){destination.push_back({uint16(0xFF00|slot),1});return EQUIP_ERR_OK;}
  return EQUIP_ERR_BAG_FULL;
 }
 InventoryResult CanBankItem(uint8 b,uint8 s,ItemPosCountVec& d,Item* i,bool swap)const{return CanStoreItem(b,s,d,i,swap);}
 InventoryResult CanEquipItem(uint8 slot,uint16& destination,Item* item,bool,bool=true,bool=false)const{
  destination=slot==NULL_SLOT?uint16(0xFF00|(item->entry==128289?16:15)):uint16(0xFF00|slot);return EQUIP_ERR_OK;
 }
 InventoryResult CanEquipChildItem(Item*)const;
 void EquipChildItem(uint8,uint8,Item*);
 void SendEquipError(InventoryResult e,Item*,Item* = nullptr);
 void RemoveItem(uint8 b,uint8 s,bool){items.erase(uint16(b)<<8|s);++moves;}
 void EquipItem(uint16 p,Item* item,bool){assert(!GetItemByPos(p));items[p]=item;item->pos=p;}
 void StoreItem(ItemPosCountVec const& d,Item* item,bool){assert(d.size()==1&&!GetItemByPos(d[0].pos));items[d[0].pos]=item;item->pos=d[0].pos;}
 void BankItem(ItemPosCountVec const& d,Item* i,bool u){StoreItem(d,i,u);}
 void AutoUnequipOffhandIfNeed(){}
 void SwapItem(uint16 source,uint16 destination,bool){Item* item=GetItemByPos(source);assert(item&&!GetItemByPos(destination));items.erase(source);EquipItem(destination,item,true);++swaps;EquipChildItem(source>>8,uint8(source),item);}
};
void Player::SendEquipError(InventoryResult e,Item*,Item*){++errors;last=e;}
struct Unit {Player* player;Player* ToPlayer(){return player;}};
struct Effect {uint32 MiscValue;};struct SpellInfo {Effect effect;Effect const* GetEffect(SpellEffIndex,int)const{return &effect;}};
struct Spell {int effectHandleMode=1,m_diffMode=0;Unit* m_caster;SpellInfo* m_spellInfo;void EffectForceEquipItem(SpellEffIndex);};
#include "PairPreflight.inc"
#include "PairEquip.inc"
#include "ForceEquip.inc"
int main(int argc,char**argv){
 assert(argc==2);std::ifstream catalog(argv[1]);uint32 parentEntry,childEntry,childSlot,cases=0,pairs=0;
 while(catalog>>parentEntry>>childEntry>>childSlot){
  assert((parentEntry==128908&&childEntry==134553&&childSlot==16)||(parentEntry==128289&&childEntry==128288&&childSlot==15));++pairs;
  sDB2Manager.pairs[parentEntry]={childEntry,uint8(childSlot)};
  for(uint32 mode=0;mode<10;++mode){
   uint16 parentPos=uint16(0xFF00|(parentEntry==128289?16:15)),childPos=uint16(0xFF00|childSlot),hidden=0xFF60;
   Player p;Item parent{parentEntry,1,parentPos,{2}},child{childEntry,2,hidden,{}},previous{100,3,childPos,{}};
   if(mode==9)parent.pos=0xFF17;
   p.items[parent.pos]=&parent;p.owned[1]=&parent;p.items[child.pos]=&child;p.owned[2]=&child;
   if(mode==1||mode==2||mode==3){p.items[childPos]=&previous;p.owned[3]=&previous;}
   if(mode==2||mode==8)p.full=true;if(mode==3)p.locked=true;
   if(mode==4){p.items.erase(hidden);child.pos=childPos;p.items[childPos]=&child;}
   if(mode==5||mode==7){p.items.erase(hidden);p.owned.erase(2);}
   if(mode==6)parent.child={};
   auto before=p.items;Unit caster{&p};SpellInfo info{{parentEntry}};Spell spell{1,0,&caster,&info};
   spell.EffectForceEquipItem(0);
#ifdef BASELINE
   bool completes=mode==9||mode==4;
#else
   bool completes=mode==0||mode==1||mode==4||mode==8||mode==9;
#endif
   if(completes){assert(p.GetItemByPos(parentPos)==&parent&&p.GetItemByPos(childPos)==&child);if(mode==1){assert(previous.IsEquipped()==false&&p.GetItemByPos(previous.pos)==&previous);}}
   else {assert(p.items==before);assert(p.GetItemByPos(parent.pos)==&parent);}
#ifndef BASELINE
   if(mode==2||mode==3)assert(p.errors==1&&p.last==(mode==2?EQUIP_ERR_BAG_FULL:EQUIP_ERR_LOCKED));
   else assert(!p.errors);
   if(completes){auto stable=p.items;auto moves=p.moves;spell.EffectForceEquipItem(0);assert(p.items==stable&&p.moves==moves&&p.errors==0);}
#endif
   assert(p.owned.size()==(mode==1||mode==2||mode==3?3u:mode==5||mode==7?1u:2u));++cases;
  }
 }
 assert(pairs==2&&cases==20);
#ifdef BASELINE
 std::cout<<"COUNTERFACTUAL20 pinned native-method contexts:equipped parent blocks missing linked child;legacy unequipped path retained. Inventory services controlled\n";
#else
 std::cout<<"PASS20 actual force-equip/child preflight/move method contexts:Fury thenProtection,empty/occupied target,full bags,locked item,repeated complete,missing child/link and legacy unequipped parent;owned item identities conserved,no item creation. Inventory services controlled,not full native/client traversal\n";
#endif
}
