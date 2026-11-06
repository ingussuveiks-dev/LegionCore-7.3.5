// Real Player/Item inventory and native249 spell dispatch. Partial state is controlled.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "Item.h"
struct PairPlayer : NativePlayer {
 using NativePlayer::NativePlayer;
 ~PairPlayer(){for(uint8 slot=PLAYER_SLOT_START;slot<PLAYER_SLOT_END;++slot)if(Item* item=GetItemByPos(INVENTORY_SLOT_BAG_0,slot))if(item->IsInWorld())item->RemoveFromWorld();}
};
static Item* MakeItem(Player& player,uint32 entry){auto item=Item::CreateItem(entry,1,&player);Require(item,"Native item creation failed");return item;}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE pair_empty_artifact_schema LIKE item_instance_artifact");
 CharacterDatabase.DirectExecute("CREATE TEMPORARY TABLE item_instance_artifact LIKE pair_empty_artifact_schema");
 auto create=CharacterDatabase.Query("SHOW CREATE TABLE item_instance_artifact");Require(create&&create->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Artifact XP shadow is not TEMP");
 CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 auto ro=CharacterDatabase.Query("SELECT @@session.transaction_read_only");Require(ro&&ro->Fetch()[0].GetBool(),"Native character connection not readonly");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 uint32 cases=0;
 for(uint32 entry:{128908u,128289u})for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)})for(uint32 mode=0;mode<3;++mode){
  MapInstanced parent(1511,300);InstanceMap map(1511,300,9981000+cases,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();{
   PairPlayer player(session,map,301,{3515.24f,1800.51f,0.538696f,0});player.SetInventorySlotCount(16);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);player.AddWeaponProficiency(0xFFFFFFFF);player.AddArmorProficiency(0xFFFFFFFF);player.SetCanDualWield(true);player.SetCanTitanGrip(true);
   auto native=sDB2Manager.GetItemChildEquipment(entry);Require(native&&native->ChildItemID==(entry==128908?134553u:128288u),"Native pair changed");
   uint8 parentSlot=entry==128908?EQUIPMENT_SLOT_MAINHAND:EQUIPMENT_SLOT_OFFHAND;uint16 parentPos=(INVENTORY_SLOT_BAG_0<<8)|parentSlot;uint16 childPos=(INVENTORY_SLOT_BAG_0<<8)|native->ChildItemEquipSlot;
   Item* item=MakeItem(player,entry);Item* child=MakeItem(player,native->ChildItemID);child->SetGuidValue(ITEM_FIELD_CREATOR,item->GetGUID());child->SetFlag(ITEM_FIELD_DYNAMIC_FLAGS,ITEM_FLAG_CHILD);item->SetChildItem(child->GetGUID());
   ItemPosCountVec hidden;hidden.emplace_back((INVENTORY_SLOT_BAG_0<<8)|CHILD_EQUIPMENT_SLOT_START,1);Require(player.StoreItem(hidden,child,true)==child,"Native hidden child storage failed");player.QuickEquipItem(parentPos,item);
   Item* previous=nullptr;if(mode){previous=MakeItem(player,15241);player.QuickEquipItem(childPos,previous);}
   if(mode==2){for(uint8 slot=INVENTORY_SLOT_ITEM_START;slot<player.GetInventoryEndSlot();++slot)if(!player.GetItemByPos(INVENTORY_SLOT_BAG_0,slot)){ItemPosCountVec where;Require(player.CanStoreNewItem(INVENTORY_SLOT_BAG_0,slot,where,2589,200)==EQUIP_ERR_OK&&player.StoreNewItem(where,2589,true),"Native full bags failed");}}
   ObjectGuid parentGuid=item->GetGUID(),childGuid=child->GetGUID();uint16 childBefore=child->GetPos();auto expected=mode==2?EQUIP_ERR_INV_FULL:EQUIP_ERR_OK;
   Require(player.CanEquipChildItem(item)==expected,"Native paired preflight mismatch");
   Require(player.CastSpell(&player,entry==128908?194201:208562,true)==SPELL_CAST_OK,"Native presentation equip spell rejected");player.Unit::Update(250);
   Require(player.GetItemByPos(parentPos)==item&&item->GetGUID()==parentGuid&&child->GetGUID()==childGuid&&item->GetChildItem()==childGuid,"Native equip changed/lost parent/child identity");
   if(mode==2){Require(child->GetPos()==childBefore&&player.GetItemByPos(childPos)==previous,"Full bags moved/lost native items");player.DestroyItem(INVENTORY_SLOT_BAG_0,INVENTORY_SLOT_ITEM_START,true);Require(player.CastSpell(&player,entry==128908?194201:208562,true)==SPELL_CAST_OK,"Native free-space retry rejected");player.Unit::Update(250);}
   Require(player.GetItemByPos(childPos)==child,"Native249 did not equip existing linked child");if(previous)Require(!previous->IsEquipped()&&player.GetItemByGuid(previous->GetGUID())==previous,"Native displaced weapon not retained in inventory");
   auto stable=child->GetPos();Require(player.CastSpell(&player,entry==128908?194201:208562,true)==SPELL_CAST_OK,"Native repeated equip rejected");player.Unit::Update(250);Require(child->GetPos()==stable&&player.GetItemByPos(parentPos)==item,"Native repeated complete pair changed slots");
   std::cout<<"CASE native-pair parent="<<entry<<" race="<<race<<" mode="<<mode<<" native-spell/linked-child/displaced-identity verified\n";CleanStories(map);
  }session->SetPlayer(nullptr);delete session;++cases;
 }
 std::cout<<"PASS "<<cases<<" actual native249/inventory contexts,Fury thenProtection;controlled already-equipped parent,actual hidden child,empty/occupied/full+free-space retry/repeat;no character save or permanent DB write\n";CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
