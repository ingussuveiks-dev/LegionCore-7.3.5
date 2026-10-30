#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
#define ASSERT(x) assert(x)
#define TC_LOG_DEBUG(...) ((void)0)
#include "EquipmentEnums.inc"
enum {INVENTORY_SLOT_BAG_0=255,NULL_SLOT=255,NULL_BAG=255,MAX_LEVEL=110,CHAR_SEL_ITEM_INSTANCE_ARTIFACT_XP=1,CHAR_INS_ITEM_BOP_TRADE=2,LANG_NOT_EQUIPPED_ITEM=1,CRITERIA_TYPE_RECEIVE_EPIC_ITEM=3,CRITERIA_TYPE_OWN_ITEM=4,CRITERIA_TYPE_OWN_HEIRLOOMS=5,UNIT_STATE_STUNNED=8,UNIT_FLAG_DISARMED=2,STATUS_IN_PROGRESS=3,CLASS_WARRIOR=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,ITEM_CHANGED=1,SPELL_EFFECT_HANDLE_HIT_TARGET=1};
struct ObjectGuid {uint64 id=0;bool go=false;explicit operator bool()const{return id!=0;}bool IsEmpty()const{return !id;}bool IsGameObject()const{return go;}bool operator<(ObjectGuid b)const{return id<b.id;}bool operator==(ObjectGuid b)const{return id==b.id&&go==b.go;}};
std::ostream&operator<<(std::ostream&s,ObjectGuid g){return s<<g.id;}
using GuidSet=std::set<ObjectGuid>;
struct ItemPosCount {uint16 pos;uint32 count;ItemPosCount(uint16 p,uint32 c):pos(p),count(c){}};using ItemPosCountVec=std::vector<ItemPosCount>;
struct ItemRandomEnchantmentId {int32 id=23;};
struct Player;struct Item;
std::vector<std::string> events;uint64 nextItem=100;uint64 savedXP=(uint64(1)<<40)+123;bool allocationFails=false;
struct Transaction {std::vector<std::string> operations;void Append(struct Statement*);};using CharacterDatabaseTransaction=std::shared_ptr<Transaction>;
struct Statement {uint32 id;std::map<uint32,uint64> values;std::string text;void setUInt64(uint32 i,uint64 v){values[i]=v;}void setUInt32(uint32 i,uint32 v){values[i]=v;}void setString(uint32,std::string v){text=v;}};
using CharacterDatabasePreparedStatement=Statement;
void Transaction::Append(Statement*s){operations.push_back("trade:"+std::to_string(s->values[0]));}
struct Query {struct Field{uint64 GetUInt64(){return savedXP;}}field;Field*Fetch(){return &field;}};using PreparedQueryResult=std::shared_ptr<Query>;
struct Database {
 std::vector<std::unique_ptr<Statement>> statements;uint32 commits=0,queries=0,executed=0;std::vector<std::string> committed;
 Statement*GetPreparedStatement(uint32 id){auto p=std::make_unique<Statement>();p->id=id;auto*r=p.get();statements.push_back(std::move(p));return r;}
 PreparedQueryResult Query(Statement*s){assert(s->id==CHAR_SEL_ITEM_INSTANCE_ARTIFACT_XP);++queries;return std::make_shared<struct Query>();}
 void Execute(Statement*s){assert(s->id==CHAR_INS_ITEM_BOP_TRADE);++executed;events.push_back("trade");}
 CharacterDatabaseTransaction BeginTransaction(){events.push_back("begin");return std::make_shared<Transaction>();}
 void CommitTransaction(CharacterDatabaseTransaction const&t){++commits;committed=t->operations;assert(!committed.empty());events.push_back("commit");}
}CharacterDatabase;
struct ItemTemplate {
 uint32 id=128910,artifact=36,type=INVTYPE_2HWEAPON,itemClass=ITEM_CLASS_WEAPON,subClass=0,flags3=0,limit=0;int32 maxCount=1;bool combat=true,legendary=false;
 uint32 GetId()const{return id;}uint32 GetArtifactID()const{return artifact;}uint32 GetInventoryType()const{return type;}uint32 GetClass()const{return itemClass;}uint32 GetSubClass()const{return subClass;}uint32 GetFlags3()const{return flags3;}uint32 GetMaxStackSize()const{return 1;}int32 GetMaxCount()const{return maxCount;}uint32 GetLimitCategory()const{return limit;}bool CanChangeEquipStateInCombat()const{return combat;}bool IsLegendaryLoot()const{return legendary;}
};
std::map<uint32,ItemTemplate> templates;
struct ObjectMgr {ItemTemplate const*GetItemTemplate(uint32 id){auto i=templates.find(id);return i==templates.end()?nullptr:&i->second;}}objectMgr;auto*sObjectMgr=&objectMgr;
struct ItemChildEquipmentEntry {uint32 ChildItemID=130001;uint8 ChildItemEquipSlot=16;};
struct DB2Manager {bool child=false,heirloom=false;ItemChildEquipmentEntry pair;uint32 GetRulesetItemUpgrade(uint32){return 51;}ItemChildEquipmentEntry const*GetItemChildEquipment(uint32 id){return child&&id!=130001?&pair:nullptr;}bool GetHeirloomByItemId(uint32){return heirloom;}}db2;auto&sDB2Manager=db2;
struct ScalingStatDistributionEntry {uint32 MaxLevel=110;};
struct ScalingStore {ScalingStatDistributionEntry entry;ScalingStatDistributionEntry const*LookupEntry(uint32 id)const{return id?&entry:nullptr;}}sScalingStatDistributionStore;
struct ItemLimitCategoryEntry {uint32 Flags=0,Quantity=1;};struct LimitStore {ItemLimitCategoryEntry entry;ItemLimitCategoryEntry const*LookupEntry(uint32 id)const{return id?&entry:nullptr;}}sItemLimitCategoryStore;
std::map<uint32,int> GlobalArtifactData{{128910,1},{130000,1}};
struct Bag;
struct Item {
 static std::set<Item*> allocated;
 uint32 entry=0,count=1,flags=0,context=0,fixed=0,upgrade=0,scaleEntry=0,dungeonEncounterID=0;uint16 pos=0;ObjectGuid guid{nextItem++},child,creator;ItemRandomEnchantmentId random;std::vector<uint32>bonuses;uint64 xp=0;bool powers=false,bound=false,trade=false,m_lootGenerated=false,foreign=false;
 explicit Item(uint32 id):entry(id){}~Item(){allocated.erase(this);}
 static Item*CreateItem(uint32 id,uint32 count,Player*){if(allocationFails)return nullptr;auto*p=new Item(id);p->count=count;allocated.insert(p);events.push_back("create");return p;}
 ItemTemplate const*GetTemplate()const{return sObjectMgr->GetItemTemplate(entry);}uint32 GetEntry()const{return entry;}uint32 GetCount()const{return count;}uint64 GetGUIDLow()const{return guid.id;}ObjectGuid GetGUID()const{return guid;}ObjectGuid GetChildItem()const{return child;}uint16 GetPos()const{return pos;}uint8 GetBagSlot()const{return uint8(pos>>8);}uint8 GetSlot()const{return uint8(pos);}uint32 GetScalingStatDistribution()const{return scaleEntry;}bool IsBindedNotWith(Player const*)const{return foreign;}
 bool IsEquipped()const{return GetBagSlot()==255&&GetSlot()<19;}bool IsInWorld()const{return true;}bool IsInUse()const{return false;}bool IsBag()const{return false;}Bag*ToBag(){return nullptr;}void SetCount(uint32 n){count=n;}void SendUpdateToPlayer(Player*){}void SetState(uint32,Player*){}ObjectGuid GetGuidValue(uint32)const{return creator;}
 void SetUInt32Value(uint32 field,uint32 value){if(field==ITEM_FIELD_CONTEXT)context=value;else assert(field==ITEM_FIELD_CREATE_PLAYED_TIME);}
 void SetUInt64Value(uint32 field,uint64 value){assert(field==ITEM_FIELD_ARTIFACT_XP);xp=value;events.push_back("xp");}
 void SetFlag(uint32 field,uint32 value){assert(field==ITEM_FIELD_DYNAMIC_FLAGS);flags|=value;}bool HasFlag(uint32 field,uint32 value)const{assert(field==ITEM_FIELD_DYNAMIC_FLAGS);return flags&value;}
 void SetItemRandomProperties(ItemRandomEnchantmentId const&r){random=r;}void SetModifier(uint32 field,uint32 value){assert(field==ITEM_MODIFIER_UPGRADE_ID);upgrade=value;}void AddBonuses(uint32 value){bonuses.push_back(value);}void SetFixedLevel(uint32 value){fixed=value;events.push_back("fixed");}
 void LoadArtifactData(Player*,int){powers=true;events.push_back("powers");}bool IsSoulBound()const{return bound;}void SetSoulboundTradeable(GuidSet const&){trade=true;}void SetGuidValue(uint32 field,ObjectGuid value){assert(field==ITEM_FIELD_CREATOR);creator=value;}void SetChildItem(ObjectGuid value){child=value;}
 void DeleteFromInventoryDB(CharacterDatabaseTransaction const&t){t->operations.push_back("delete:"+std::to_string(guid.id));}void SaveToDB(CharacterDatabaseTransaction const&t){t->operations.push_back("save:"+std::to_string(guid.id));}
};std::set<Item*>Item::allocated;
struct Bag:Item {bool IsEmpty(){assert(false);return true;}uint32 GetBagSize(){assert(false);return 0;}Item*GetItemByPos(uint32){assert(false);return nullptr;}void RemoveItem(uint32,bool){assert(false);}void StoreItem(uint32,Item*,bool){assert(false);}};
bool ItemCanGoIntoBag(ItemTemplate const*,ItemTemplate const*){assert(false);return false;}
struct Battleground {bool arena=false;uint32 status=STATUS_IN_PROGRESS;bool IsArena(){return arena;}uint32 GetStatus(){return status;}};
struct GameObject {uint32 entry=247877;bool inWorld=true,spawned=true,phase=true,visible=true,los=true;float distance=3;ObjectGuid guid{50,true};uint32 GetEntry(){return entry;}bool IsInWorld(){return inWorld;}bool isSpawned(){return spawned;}struct Map*GetMap();};
struct Map {bool dungeon=true;GameObject*object=nullptr;bool IsDungeonOrRaid(){return dungeon;}GameObject*GetGameObject(ObjectGuid g){return object&&object->guid==g?object:nullptr;}}world;
Map*GameObject::GetMap(){return &world;}
struct Scenario {uint32 id=1037,step=9;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}}scenarioState;
struct ScenarioMgr {bool present=true;Scenario*GetScenario(uint32 id){return present&&id==42?&scenarioState:nullptr;}}scenarioMgr;auto*sScenarioMgr=&scenarioMgr;
struct Collection {bool owned=false;uint32 additions=0;bool HasHeirloom(uint32){return owned;}void AddHeirloom(uint32,uint32){++additions;}};
struct QuestItem {bool is_looted=false;uint32 index=0;};
struct LootItem {struct Info {uint32 ItemID=128910;ItemRandomEnchantmentId RandomPropertiesID;struct Bonus {std::vector<uint32>BonusListIDs{100,200};uint32 Context=5;}ItemBonus;}item;uint32 count=1;bool is_blocked=false,is_looted=false,freeforall=false;GuidSet GetAllowedLooters(){return {{1},{2}};}};
 struct Loot {
 LootItem row;bool allowed=true,personal=false,quest=false,ffa=false,conditional=false;uint32 unlootedCount=1,dungeonEncounterID=7,notifies=0;ObjectGuid objGuid{50,true};QuestItem questRow,ffaRow,conditionRow;
 LootItem*LootItemInSlot(uint8,Player*,QuestItem**q,QuestItem**f,QuestItem**c,QuestItem**currency){*currency=nullptr;*q=quest?&questRow:nullptr;*f=ffa?&ffaRow:nullptr;*c=conditional?&conditionRow:nullptr;return allowed&&!row.is_looted&&!questRow.is_looted&&!ffaRow.is_looted&&!conditionRow.is_looted?&row:nullptr;}
 std::vector<int>GetPlayerQuestItems(){return {1};}void NotifyQuestItemRemoved(uint32){++notifies;}void NotifyItemRemoved(uint8){++notifies;}
 ObjectGuid GetGUID(){return objGuid;}
};
struct Player {
 std::map<uint16,Item*> inventory;Collection collection;Collection*_collectionMgr=&collection;Battleground battleground;std::vector<std::function<void()>> delayed;
 bool stunned=false,disarmed=false,combat=false,casting=false,titan=false,dual=false,worldState=true,contact=true,alive=true,teleport=false,rewarded=false;uint32 map=1539,klass=1,questStatus=3,bankCount=0,bagSpace=0,errors=0,notifications=0,questAdds=0,achievementCalls=0,tradeAdds=0,socketUnlocks=0,saves=0,lootHooks=0;InventoryResult useResult=EQUIP_ERR_OK,uniqueResult=EQUIP_ERR_OK,unequipResult=EQUIP_ERR_OK;uint8 equipSlot=EQUIPMENT_SLOT_MAINHAND;
 struct Session {std::string GetTrinityString(uint32 id){assert(id==LANG_NOT_EQUIPPED_ITEM);return "Returned equipment";}void DoLootRelease(ObjectGuid){assert(false);}}session;
 Session*m_session=&session;
 Item*GetItemByPos(uint8 bag,uint8 slot)const{auto i=inventory.find(uint16(bag)<<8|slot);return i==inventory.end()?nullptr:i->second;}Item*GetItemByGuid(ObjectGuid g){for(auto const&i:inventory)if(i.second->guid==g)return i.second;return nullptr;}
 uint32 GetItemCount(uint32 id,bool bank,Item*ignore=nullptr)const{uint32 n=bank?bankCount:0;for(auto const&i:inventory)if(i.second!=ignore&&i.second->entry==id)n+=i.second->count;return n;}uint32 GetItemCountWithLimitCategory(uint32,Item*)const{return 0;}
 InventoryResult CanTakeMoreSimilarItems(uint32,uint32,Item* = nullptr,uint32* = nullptr,bool=false)const;InventoryResult CanTakeMoreSimilarItems(Item*i)const{return CanTakeMoreSimilarItems(i->entry,i->count,i);}
 bool HasUnitState(uint32)const{return stunned;}bool HasFlag(uint32,uint32)const{return disarmed;}bool isInCombat()const{return combat;}bool IsNonMeleeSpellCast(bool)const{return casting;}Battleground*GetBattleground()const{return const_cast<Battleground*>(&battleground);}uint32 getLevel()const{return 104;}uint32 m_weaponChangeTimer=0;
 uint8 FindEquipSlot(ItemTemplate const*,uint8,bool swap)const{return !swap&&GetItemByPos(255,equipSlot)?uint8(NULL_SLOT):equipSlot;}InventoryResult CanUseItem(Item*,bool=true)const{return useResult;}InventoryResult CanEquipUniqueItem(Item*,uint8)const{return uniqueResult;}bool CanDualWield()const{return dual;}bool CanTitanGrip()const{return titan;}bool IsTwoHandUsed()const{Item*i=GetItemByPos(255,15);return i&&i->GetTemplate()->GetInventoryType()==INVTYPE_2HWEAPON;}
 InventoryResult CanUnequipItem(uint16,bool)const{return unequipResult;}
 InventoryResult CanStoreItem(uint8 bag,uint8 slot,ItemPosCountVec&dest,Item*,bool swap)const{if(bag==255&&slot!=NULL_SLOT){if(slot<23||slot>=23+bagSpace||(!swap&&GetItemByPos(bag,slot)))return EQUIP_ERR_INV_FULL;dest.emplace_back(uint16(uint16(bag)<<8|slot),1);return EQUIP_ERR_OK;}for(uint32 s=23;s<23+bagSpace;++s)if(!GetItemByPos(255,uint8(s))){dest.emplace_back(uint16(uint16(255)<<8|s),1);return EQUIP_ERR_OK;}return EQUIP_ERR_INV_FULL;}
 InventoryResult CanEquipItem(uint8,uint16&,Item*,bool,bool=true,bool=false)const;
 InventoryResult CanStoreNewItem(uint8,uint8,ItemPosCountVec&dest,uint32 id,uint32 count,uint32*,bool quest){auto result=CanTakeMoreSimilarItems(id,count,nullptr,nullptr,quest);return result==EQUIP_ERR_OK?CanStoreItem(NULL_BAG,NULL_SLOT,dest,nullptr,false):result;}
 bool IsInventoryPos(uint16 pos)const{return (pos>>8)==255&&uint8(pos)>=23&&uint8(pos)<100;}bool IsEquipmentPos(uint16 pos)const{return (pos>>8)==255&&uint8(pos)<19;}bool IsBagPos(uint16)const{return false;}bool IsBankPos(uint16)const{return false;}bool IsReagentBankPos(uint16)const{return false;}
 void TradeCancel(bool){assert(false);}void AutoUnequipChildItem(Item*){assert(false);}void EquipChildItem(uint8,uint8,Item*){assert(false);}InventoryResult CanEquipChildItem(Item*)const{assert(false);return EQUIP_ERR_CANT_SWAP;}void SendRefundInfo(Item*){assert(false);}
 InventoryResult CanBankItem(uint8,uint8,ItemPosCountVec&,Item*,bool)const{assert(false);return EQUIP_ERR_CANT_SWAP;}InventoryResult CanBankReagentItem(uint8,uint8,ItemPosCountVec&,Item*,bool)const{assert(false);return EQUIP_ERR_CANT_SWAP;}void BankItem(ItemPosCountVec const&,Item*,bool){assert(false);}void ItemRemovedQuestCheck(uint32,uint32){assert(false);}
 void SwapItem(uint16,uint16,bool=false);void AutoUnequipOffhandIfNeed(bool=false);Player*ToPlayer(){return this;}Item*GetItemByEntry(uint32 id){for(auto const&i:inventory)if(i.second->entry==id)return i.second;return nullptr;}
 void SendEquipError(InventoryResult,Item* = nullptr,Item* = nullptr,uint32=0){++errors;}void SendLootRelease(ObjectGuid,ObjectGuid){}ObjectGuid GetLootGUID(){return {50,true};}
 Item*StoreItem(ItemPosCountVec const&dest,Item*item,bool){assert(!dest.empty());item->pos=dest.front().pos;inventory[item->pos]=item;item->bound=true;events.push_back("store:"+std::to_string(item->entry));return item;}
 Item*EquipItem(uint16 pos,Item*item,bool){assert(!GetItemByPos(uint8(pos>>8),uint8(pos)));assert(item->powers&&item->xp==savedXP);item->pos=pos;inventory[pos]=item;item->bound=true;events.push_back("equip");return item;}
 void RemoveItem(uint8 bag,uint8 slot,bool){inventory.erase(uint16(bag)<<8|slot);}void MoveItemFromInventory(Item*item,bool update){RemoveItem(item->GetBagSlot(),item->GetSlot(),update);events.push_back("remove:"+std::to_string(item->entry));}
 void SaveInventoryAndGoldToDB(CharacterDatabaseTransaction const&t){++saves;t->operations.push_back("inventory");}
 Session*GetSession(){return &session;}Map*GetMap(){return &world;}uint64 GetGUIDLow(){return 1;}uint32 GetTotalPlayedTime(){return 50;}void AddTradeableItem(Item*){++tradeAdds;}void ItemAddedQuestCheck(uint32,uint32){++questAdds;events.push_back("quest");}void UpdateAchievementCriteria(uint32,uint32=0,uint32=0){++achievementCalls;}
 void CanStoreItem_InInventorySlots(uint8,uint8,ItemPosCountVec&d,ItemTemplate const*,uint32,bool,Item*,uint8,uint8){d.emplace_back(uint16(uint16(255)<<8|140),1);}
 void AddDelayedEvent(uint32 ms,std::function<void()>f){assert(ms==3000);delayed.push_back(std::move(f));}void UnLockThirdSocketIfNeed(Item*){++socketUnlocks;}
 std::set<uint32> AllLegendarys;
 Item*StoreNewItem(ItemPosCountVec const&,uint32,bool,ItemRandomEnchantmentId const& = {},GuidSet const& = {},std::vector<uint32>const& = {},uint32=0,bool=false,bool=false);
 Item*StoreNewItemBaseline(ItemPosCountVec const&,uint32,bool,ItemRandomEnchantmentId const& = {},GuidSet const& = {},std::vector<uint32>const& = {},uint32=0,bool=false);
 void StoreLootItem(uint8,Loot*);void StoreLootItemBaseline(uint8,Loot*);
 bool IsInWorld(){return worldState;}bool CanContact(){return contact;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}uint32 getClass(){return klass;}uint32 GetMapId(){return map;}uint32 GetInstanceId(){return 42;}bool GetQuestRewardStatus(uint32 q){assert(q==41105);return rewarded;}uint32 GetQuestStatus(uint32 q){assert(q==41105);return questStatus;}bool HasItemCount(uint32 id,uint32 count,bool bank){return GetItemCount(id,bank)>=count;}
 bool InSamePhase(GameObject*g){return g->phase;}bool canSeeOrDetect(GameObject*g){return g->visible;}bool IsWithinLOSInMap(GameObject*g){return g->los;}float GetExactDist(GameObject*g){return g->distance;}
 void SendNotifyLootItemRemoved(uint8,Loot*){++notifications;}
};
struct MailSender {explicit MailSender(Player*,uint32){}};
struct MailDraft {Item*item=nullptr;MailDraft(std::string,std::string){}MailDraft&AddItem(Item*i){item=i;return*this;}void SendMailTo(CharacterDatabaseTransaction const&t,Player*,MailSender,uint32){assert(item);t->operations.push_back("mail:"+std::to_string(item->guid.id));events.push_back("mail:"+std::to_string(item->entry));}};
struct PlayerScript;std::map<uint32,PlayerScript*>scripts;uint32 nextScript=0;
struct PlayerScript {uint32 key;explicit PlayerScript(char const*){key=++nextScript;scripts[key]=this;}virtual~PlayerScript(){scripts.erase(key);}virtual bool CanAutoEquipLoot(Player*,ObjectGuid const&,uint32,uint32){return false;}};
struct ScriptMgr {uint32 calls=0;bool CanAutoEquipLoot(Player*,ObjectGuid const&,uint32,uint32);void OnLootItem(Player*p,Item*,uint32){++p->lootHooks;}}scriptMgr;auto*sScriptMgr=&scriptMgr;
#define FOR_SCRIPTS(T,I,E) for(auto I=scripts.begin();I!=scripts.end();++I)
#include "LootPolicyDispatch.inc"
namespace ArtifactAcquisition {
#include "LootActive.inc"
}
#include "StromkarLootPolicy.inc"
#include "NativeSimilarItems.inc"
#include "NativeCanEquip.inc"
#include "NativeSwap.inc"
#include "NativeOffhand.inc"
using SpellEffIndex=int;
struct SpellInfo {struct Effect{uint32 MiscValue=128910;}effect;Effect const*GetEffect(int,int){return &effect;}};
struct Spell {int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;Player*m_caster;SpellInfo*m_spellInfo;void EffectForceEquipItem(SpellEffIndex);};
#include "NativeForceEquip.inc"
#include "NativeStoreNewItem.inc"
#include "BaselineStoreNewItem.inc"
#include "NativeStoreLootItem.inc"
#include "BaselineStoreLootItem.inc"

void Reset(){assert(Item::allocated.empty());events.clear();nextItem=100;allocationFails=false;CharacterDatabase={};db2={};world={};scenarioState={};scenarioMgr={};templates.clear();ItemTemplate weapon;templates[128910]=weapon;weapon.id=50000;weapon.artifact=0;weapon.maxCount=0;templates[50000]=weapon;weapon.id=50001;weapon.type=INVTYPE_SHIELD;weapon.itemClass=ITEM_CLASS_ARMOR;templates[50001]=weapon;sScalingStatDistributionStore={};}
void Cleanup(){while(!Item::allocated.empty())delete *Item::allocated.begin();}
std::vector<uint64>Metadata(Item const*i){return {i->entry,i->count,i->flags,i->context,i->fixed,i->upgrade,uint64(i->random.id),i->xp,i->powers,i->bound,i->trade,i->bonuses.size()};}
int main(){
 player_stromkar_loot_equipment policy;uint32 cases=0;
 // The unchanged ordinary storage path must match the pinned implementation.
 std::vector<uint64> baseline;std::vector<std::string>baselineEvents;
 for(uint32 variant=0;variant<8;++variant)for(bool old:{true,false}){
  Reset();Player p;p.bagSpace=1;ItemPosCountVec dest{{uint16(255)<<8|23,1}};GuidSet looters{{1},{2}};
  db2.child=bool(variant&1);db2.heirloom=bool(variant&2);templates[128910].legendary=bool(variant&4);ItemTemplate child;child.id=130001;child.artifact=0;templates[130001]=child;
  Item*i=old?p.StoreNewItemBaseline(dest,128910,true,{},looters,{100,200},5,true):p.StoreNewItem(dest,128910,true,{},looters,{100,200},5,true);
  assert(i&&CharacterDatabase.commits==0&&p.questAdds==(db2.child?2u:1u)&&p.tradeAdds==1&&CharacterDatabase.executed==1);
  std::vector<uint64> snapshot;for(auto const&entry:p.inventory){snapshot.push_back(entry.first);auto metadata=Metadata(entry.second);snapshot.insert(snapshot.end(),metadata.begin(),metadata.end());snapshot.push_back(entry.second->child.id);snapshot.push_back(entry.second->creator.id);}snapshot.push_back(p.collection.additions);snapshot.push_back(p.AllLegendarys.size());snapshot.push_back(p.achievementCalls);
  if(old){baseline=snapshot;baselineEvents=events;}else{assert(snapshot==baseline&&events==baselineEvents);}
  Cleanup();++cases;
 }
 for(bool main:{false,true})for(bool off:{false,true})for(bool titan:{false,true})for(bool old:{true,false}){
  Reset();Player p;p.titan=titan;GameObject go;world.object=&go;Loot loot;Item previous(50000),shield(50001);previous.pos=uint16(255)<<8|15;shield.pos=uint16(255)<<8|16;
  if(main)p.inventory[previous.pos]=&previous;if(off)p.inventory[shield.pos]=&shield;
  if(old)p.StoreLootItemBaseline(0,&loot);else p.StoreLootItem(0,&loot);
  if(old){assert(p.errors==1&&!loot.row.is_looted&&loot.unlootedCount==1&&CharacterDatabase.commits==0&&Item::allocated.empty());}
  else{Item*i=p.GetItemByPos(255,15);assert(i&&i->entry==128910&&i->powers&&i->xp==savedXP);assert(loot.row.is_looted&&loot.unlootedCount==0&&p.lootHooks==1&&p.questAdds==1&&p.saves==1&&CharacterDatabase.commits==1);uint32 mails=0;for(auto const&s:CharacterDatabase.committed)if(s.find("mail:")==0)++mails;assert(mails==uint32(main)+uint32(off&&!titan));assert(CharacterDatabase.committed.back()=="inventory");for(Item*oldItem:{main?&previous:nullptr,off&&!titan?&shield:nullptr})if(oldItem){auto id=std::to_string(oldItem->guid.id);for(auto const&action:{"delete:","save:","mail:"})assert(std::count(CharacterDatabase.committed.begin(),CharacterDatabase.committed.end(),std::string(action)+id)==1);assert(!p.GetItemByGuid(oldItem->guid));}if(off&&titan)assert(p.GetItemByPos(255,16)==&shield);assert(i->context==5&&i->random.id==23&&i->bonuses==std::vector<uint32>({100,200})&&i->fixed==104&&i->upgrade==51&&i->trade);auto before=CharacterDatabase.commits;p.StoreLootItem(0,&loot);assert(CharacterDatabase.commits==before&&Item::allocated.size()==1);for(auto&f:p.delayed)f();assert(p.socketUnlocks==1);}
  Cleanup();++cases;
 }
 // Actual production swap/force-equip and native offhand cleanup must use a
 // source slot freed by the blade before deciding whether mail is necessary.
 for(uint32 capacity:{1u,2u,3u})for(bool main:{false,true})for(bool off:{false,true})for(bool titan:{false,true}){
  Reset();Player p;p.bagSpace=capacity;p.titan=titan;GameObject go;world.object=&go;Loot loot;Item previous(50000),shield(50001);previous.pos=0xff0f;shield.pos=0xff10;previous.bound=shield.bound=true;
  if(main)p.inventory[previous.pos]=&previous;if(off)p.inventory[shield.pos]=&shield;
  p.StoreLootItem(0,&loot);assert(loot.row.is_looted&&p.GetItemByEntry(128910)&&p.lootHooks==1);
  if(off&&!titan&&capacity==1){auto inventory=p.inventory;p.SwapItem(p.GetItemByEntry(128910)->pos,0xff0f);assert(p.errors==1&&p.inventory==inventory&&CharacterDatabase.commits==0);p.errors=0;}
  SpellInfo info;Spell spell{SPELL_EFFECT_HANDLE_HIT_TARGET,0,&p,&info};spell.EffectForceEquipItem(0);
  Item*i=p.GetItemByPos(255,15);assert(i&&i->entry==128910&&p.errors==0);
  assert(main?previous.IsEquipped()==false:true);
  uint32 expectedMail=off&&!titan&&main&&capacity==1?1u:0u;
  assert(CharacterDatabase.commits==expectedMail);
  if(expectedMail){assert(CharacterDatabase.committed==std::vector<std::string>({"delete:"+std::to_string(shield.guid.id),"save:"+std::to_string(shield.guid.id),"mail:"+std::to_string(shield.guid.id)}));assert(!p.GetItemByGuid(shield.guid));}
  else if(off)assert(p.GetItemByGuid(shield.guid)==&shield);
  if(off&&titan)assert(p.GetItemByPos(255,16)==&shield);
  auto inventory=p.inventory;spell.EffectForceEquipItem(0);assert(p.inventory==inventory&&CharacterDatabase.commits==expectedMail);Cleanup();++cases;
 }
 for(uint32 invalid=0;invalid<38;++invalid){
  Reset();Player p;GameObject go;world.object=&go;Loot loot;Item previous(50000),shield(50001);previous.pos=uint16(255)<<8|15;shield.pos=uint16(255)<<8|16;p.inventory[previous.pos]=&previous;p.inventory[shield.pos]=&shield;
  if(invalid==0)p.stunned=true;if(invalid==1)p.disarmed=true;if(invalid==2)p.casting=true;if(invalid==3)p.useResult=EQUIP_ERR_NOT_EQUIPPABLE;if(invalid==4)p.uniqueResult=EQUIP_ERR_CANT_SWAP;if(invalid==5)p.unequipResult=EQUIP_ERR_CANT_SWAP;if(invalid==6)p.bankCount=1;if(invalid==7)allocationFails=true;if(invalid==8)loot.allowed=false;if(invalid==9)loot.row.is_blocked=true;if(invalid==10)p.klass=2;if(invalid==11)p.questStatus=0;if(invalid==12)scenarioState.step=10;if(invalid==13)go.distance=5.1f;if(invalid==14)go.phase=false;if(invalid==15)world.object=nullptr;if(invalid==16)previous.child={999};if(invalid==17)shield.flags|=ITEM_FLAG_CHILD;
  if(invalid==18)p.worldState=false;if(invalid==19)p.contact=false;if(invalid==20)p.alive=false;if(invalid==21)p.teleport=true;if(invalid==22)p.rewarded=true;if(invalid==23)p.map=1;if(invalid==24)go.inWorld=false;if(invalid==25)go.spawned=false;if(invalid==26)go.visible=false;if(invalid==27)go.los=false;if(invalid==28)go.entry=1;if(invalid==29)scenarioMgr.present=false;if(invalid==30)scenarioState.id=1;if(invalid==31)loot.objGuid.go=false;if(invalid==32)loot.objGuid.id=51;if(invalid==33)loot.row.count=2;if(invalid==34)loot.row.item.ItemID=50000;if(invalid==35)p.questStatus=6;if(invalid==36)p.combat=true,p.m_weaponChangeTimer=1;if(invalid==37)previous.flags|=ITEM_FLAG_CHILD;
  p.StoreLootItem(0,&loot);assert(!loot.row.is_looted&&loot.unlootedCount==1&&CharacterDatabase.commits==0&&Item::allocated.empty()&&p.inventory[previous.pos]==&previous&&p.inventory[shield.pos]==&shield);Cleanup();++cases;
 }
 std::cout<<"PASS "<<cases<<" native shared-storage differential/full-loot/equipment/metadata/mail-transaction cases; real DB/mail, item-mod effects and client delivery remain separate services.\n";
}
