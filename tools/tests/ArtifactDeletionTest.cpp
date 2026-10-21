#include <cassert>
#include <cstdint>
#include <iostream>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;
enum {ITEM_FLAG_NO_USER_DESTROY=0x20,EQUIP_ERR_OK=0,EQUIP_ERR_DROP_BOUND_ITEM,EQUIP_ERR_ITEM_NOT_FOUND,EQUIP_ERR_OBJECT_IS_BUSY,TRADE_SLOT_INVALID=255};
using InventoryResult=int;
struct Template {uint32 flags=0,artifact=0;uint32 GetFlags(){return flags;}uint32 GetArtifactID(){return artifact;}};
struct Item {Template data;bool inWorld=true;Template* GetTemplate(){return &data;}bool IsInWorld(){return inWorld;}int GetGUID(){return 7;}};
struct TradeData {int GetTradeSlotForItem(int){return 0;}};
struct Player {
 Item item;bool present=true,equipped=false,unequip=true,trading=false;
 int destroyed=0,partial=0,error=-1;TradeData trade;
 bool IsEquipmentPos(uint16){return equipped;}bool IsBagPos(uint16){return false;}
 int CanUnequipItem(uint16,bool){return unequip?EQUIP_ERR_OK:EQUIP_ERR_OBJECT_IS_BUSY;}
 Item* GetItemByPos(uint16){return present?&item:nullptr;}Item* GetItemByPos(uint8,uint8){return GetItemByPos(0);}
 void SendEquipError(int e,Item* =nullptr){error=e;}TradeData* GetTradeData(){return trading?&trade:nullptr;}
 void DestroyItemCount(Item*,uint32&,bool){++partial;}void DestroyItem(uint8,uint8,bool){++destroyed;}
};
namespace WorldPackets{namespace Item{struct DestroyItem {uint8 ContainerId=0,SlotNum=0;uint32 Count=0;};}}
struct WorldSession {Player* _player;void HandleDestroyItemOpcode(WorldPackets::Item::DestroyItem&);};
#include "Destroy.inc"
int main(){
 for(uint32 artifact:{0u,1u,16u,26u,35u})for(uint32 flags:{0u,32u})for(uint32 count:{0u,1u,9u}){
  Player p;p.item.data={flags,artifact};WorldSession s{&p};WorldPackets::Item::DestroyItem packet;packet.Count=count;
  s.HandleDestroyItemOpcode(packet);bool blocked=artifact||flags;
  assert(p.destroyed+p.partial==int(!blocked));assert(!blocked||p.error==EQUIP_ERR_DROP_BOUND_ITEM);
 }
 for(int state=0;state<4;++state){Player p;p.item.data.artifact=26;
  if(state==0)p.present=false;if(state==1)p.item.inWorld=false;if(state==2){p.equipped=true;p.unequip=false;}if(state==3)p.trading=true;
  WorldSession s{&p};WorldPackets::Item::DestroyItem packet;s.HandleDestroyItemOpcode(packet);assert(!p.destroyed&&!p.partial);
 }
 std::cout<<"PASS: native destroy opcode rejects artifacts with/without metadata flag, whole/partial stacks, equipped/missing/traded items; ordinary item destruction retained\n";
}
