#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
using uint8=uint8_t;using int8=int8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
#include "InventoryResult.inc"
#include "ItemBondingType.inc"
#include "ItemUpdateState.inc"
#include "ItemLimitCategoryMode.inc"
#include "WorldQuestTypeReward.inc"
#include "MailMessageType.inc"
#include "MailCheckMask.inc"
#include "MailStationery.inc"
#include "MailState.inc"
#include "MailLimit.inc"
constexpr uint32 DAY=86400,NULL_BAG=255,NULL_SLOT=255,CLASS_NONE=0,TEAM_HORDE=1;
enum {ITEM_FIELD_CONTEXT,ITEM_FIELD_DYNAMIC_FLAGS,ITEM_FLAG_NEW_ITEM,ITEM_MODIFIER_UPGRADE_ID,CONFIG_MAIL_DELIVERY_DELAY,
 CHAR_INS_MAIL,CHAR_INS_MAIL_ITEM,CHAR_DEL_ITEM_INSTANCE,CHAR_UPD_ITEM_OWNER,ITEM_SAVE,QUEST_SAVE,WORLD_SAVE,INVENTORY_SAVE};
#define TC_LOG_ERROR(...) ((void)0)
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(x) assert(x)
void Check(bool v,char const*why){if(!v)throw std::runtime_error(why);}
struct Player;struct Item;
struct ObjectGuid{uint64 value=0;using LowType=uint64;explicit operator bool()const{return value!=0;}bool operator!=(ObjectGuid g)const{return value!=g.value;}uint64 GetGUIDLow()const{return value;}static ObjectGuid const Empty;};
ObjectGuid const ObjectGuid::Empty{};using GuidSet=std::set<uint64>;
enum class HighGuid{Item};
namespace GameTime{uint32 GetGameTime(){return 1000;}}
struct Snapshot{uint32 entry=0,count=0,context=0,property=0,upgrade=0,level=0;bool bound=false;std::vector<uint32>bonuses;uint64 owner=0,guid=0;};
struct CharacterDatabasePreparedStatement{
 uint32 type;std::map<int,uint64>n;std::map<int,std::string>s;
 explicit CharacterDatabasePreparedStatement(uint32 t):type(t){}
 void setUInt64(int i,uint64 v){n[i]=v;}void setUInt32(int i,uint32 v){n[i]=v;}void setUInt16(int i,uint16 v){n[i]=v;}
 void setUInt8(int i,uint8 v){n[i]=v;}void setInt8(int i,int8 v){n[i]=uint8(v);}void setBool(int i,bool v){n[i]=v;}void setString(int i,std::string v){s[i]=v;}
};
struct Transaction{
 std::vector<std::unique_ptr<CharacterDatabasePreparedStatement>>rows;std::vector<Snapshot>items;
 void Append(CharacterDatabasePreparedStatement*p){rows.emplace_back(p);}
};
using CharacterDatabaseTransaction=std::shared_ptr<Transaction>;
struct Database{
 int standalone=0,commits=0;
 CharacterDatabasePreparedStatement*GetPreparedStatement(uint32 type){return new CharacterDatabasePreparedStatement(type);}
 void Execute(CharacterDatabasePreparedStatement*p){++standalone;delete p;}
 void CommitTransaction(CharacterDatabaseTransaction const&){++commits;}
}CharacterDatabase;
struct ItemTemplate{
 uint32 id=0,stack=5;ItemBondingType bonding=BIND_WHEN_PICKED_UP;int32 maxCount=0;uint32 category=0;
 uint32 GetMaxStackSize()const{return stack;}
 int32 GetMaxCount()const{return maxCount;}uint32 GetLimitCategory()const{return category;}uint32 GetId()const{return id;}
};
struct ItemLimitCategoryEntry{uint32 Flags=0,Quantity=1;};
struct LimitStore{ItemLimitCategoryEntry entry;ItemLimitCategoryEntry const*LookupEntry(uint32 id){return id==9?&entry:nullptr;}}sItemLimitCategoryStore;
struct Generator{uint64 next=100;uint64 Generate(){return next++;}};
struct ObjectMgr{
 std::map<uint32,ItemTemplate>templates;Generator generator;uint32 observedNeedLevel=0;
 ItemTemplate const*GetItemTemplate(uint32 id){auto i=templates.find(id);return i==templates.end()?nullptr:&i->second;}
 template<HighGuid>Generator*GetGenerator(){return &generator;}
 std::vector<uint32>GetItemBonusTree(uint32,uint32,uint32,uint32=0,uint32=0,uint32 level=0){observedNeedLevel=level;return {11,22};}
 Player*GetPlayerByLowGUID(uint64){return nullptr;}uint32 GenerateMailID(){static uint32 id=1;return id++;}
}objectMgr;auto sObjectMgr=&objectMgr;
struct DB2{uint32 GetRulesetItemUpgrade(uint32){return 7;}void*GetHeirloomByItemId(uint32){return nullptr;}}sDB2Manager;
struct World{uint32 getIntConfig(uint32){return 0;}}world;auto sWorld=&world;
struct Scripts{void OnItemCreate(Player*,ItemTemplate const*,Item*);}scripts;auto sScriptMgr=&scripts;
int living=0,createAttempts=0,failAt=0;bool badQueue=false;
struct Item{
 Snapshot data;ItemTemplate const*proto=nullptr;Player*ownerPlayer=nullptr;int uQueuePos=-1;ItemUpdateState uState=ITEM_NEW;bool m_lootGenerated=false;
 Item(){++living;}~Item(){--living;}
 bool Create(uint64 guid,uint32 entry,Player const*);
 void SetCount(uint32 n){data.count=n;}uint64 GetGUIDLow()const{return data.guid;}uint32 GetEntry()const{return data.entry;}
 ObjectGuid GetOwnerGUID()const{return {data.owner};}void SetOwnerGUID(ObjectGuid g){data.owner=g.value;}
 Player*GetOwner()const{return data.owner?ownerPlayer:nullptr;}
 void SetState(ItemUpdateState,Player* = nullptr);void AddToUpdateQueueOf(Player*);void RemoveFromUpdateQueueOf(Player*);bool IsInUpdateQueue()const;
 static Item*CreateItem(uint32,uint32,Player const*);
 static uint32 GenerateItemRandomPropertyId(uint32,uint32){return 77;}
 void SetUInt32Value(uint32 f,uint32 n){if(f==ITEM_FIELD_CONTEXT)data.context=n;}
 void SetFlag(uint32,uint32){}void SetItemRandomProperties(uint32 n){data.property=n;SetState(ITEM_CHANGED,GetOwner());}
 void SetModifier(uint32,uint32 n){data.upgrade=n;}void AddBonuses(uint32 n){data.bonuses.push_back(n);}
 void SetFixedLevel(uint32 n){Check(data.bonuses==std::vector<uint32>({11,22}),"fixed level set before bonuses");data.level=n;}
 ItemBondingType GetBonding()const{return proto->bonding;}void SetBinding(bool b){data.bound=b;}
 void SaveToDB(CharacterDatabaseTransaction&t){t->items.push_back(data);auto p=CharacterDatabase.GetPreparedStatement(ITEM_SAVE);p->setUInt64(0,data.guid);t->Append(p);SetState(ITEM_UNCHANGED);}
};
Item*NewItemOrBag(ItemTemplate const*){return new Item;}
struct Recipe{uint32 ItemID=0,ItemCount=1,NeedSpell=0,NotNeedSpell=0;};
struct RewardItem{uint32 ItemIDH=0,ItemIDA=0,ItemCount=1;};
struct RewardTemplate{uint32 MinItemLevel=0;};
struct WorldQuest{RewardTemplate*worldQuest;Recipe const*Recipe=nullptr;RewardItem ItemList[12];int typeReward=0;};
struct Garrison{bool hasLegionFall()const{return true;}};
bool proc=false;bool roll_chance_i(int){return proc;}
using ItemPosCountVec=std::vector<uint32>;
namespace ToastType{constexpr int ITEM=0;}namespace DisplayToastMethod{constexpr int DISPLAY_TOAST_SPECIAL_UNK=0;}
struct Mail{
 uint32 messageID=0,mailTemplateId=0,messageType=0,stationery=0,checked=0,state=0;uint64 money=0,COD=0,sender=0,receiver=0;
 time_t expire_time=0,deliver_time=0;std::string subject,body;std::vector<uint64>items;
 void AddItem(uint64 guid,uint32){items.push_back(guid);}
};
class MailSender{
 MailMessageType type;uint64 id;
public:MailSender(MailMessageType t,uint64 n):type(t),id(n){}
 MailMessageType GetMailMessageType()const{return type;}uint64 GetSenderId()const{return id;}MailStationery GetStationery()const{return MAIL_STATIONERY_DEFAULT;}
};
class MailReceiver{
 Player*p;
public:MailReceiver(Player* n):p(n){}Player*GetPlayer()const{return p;}uint64 GetPlayerGUIDLow()const;
};
class MailDraft{
 using MailItemMap=std::map<uint64,Item*>;
 MailItemMap m_items;uint64 m_money=0,m_COD=0;std::string subject,body;
public:MailDraft(std::string a,std::string b):subject(a),body(b){}
 uint16 GetMailTemplateId()const{return 0;}std::string const&GetSubject()const{return subject;}std::string const&GetBody()const{return body;}
 uint64 GetMoney()const{return m_money;}uint64 GetCOD()const{return m_COD;}
 void prepareItems(Player*,CharacterDatabaseTransaction&){} // Non-template retrieval mail has no generated loot.
 MailDraft&AddItem(Item*);void deleteIncludedItems(CharacterDatabaseTransaction&,bool=false);
 void SendMailTo(CharacterDatabaseTransaction&,MailReceiver const&,MailSender const&,MailCheckMask=MAIL_CHECK_MASK_NONE,uint32=0);
};
void RewardWorldQuestItem(Player*,WorldQuest const*,uint32,uint32,uint32,CharacterDatabaseTransaction&);
struct Player{
 std::set<Item*>m_itemUpdateQueue;bool m_itemUpdateQueueBlocked=false;
 ~Player(){if(!m_itemUpdateQueue.empty())badQueue=true;}
 InventoryResult admission=EQUIP_ERR_OK;bool loaded=true,horde=false,partial=false,storeFails=false;
 std::map<uint32,uint32>bank;uint32 categoryCount=0;
 uint32 storeCalls=0,pushes=0,toasts=0,level=110;std::set<uint32>spells;
 std::vector<std::unique_ptr<Item>>bags,mailItems;std::vector<std::unique_ptr<Mail>>mails;
 uint64 GetGUIDLow()const{return 12345;}ObjectGuid GetGUID()const{return {GetGUIDLow()};}void AddDeleteItem(Item*){}bool isGameMaster()const{return false;}bool IsMailsLoaded()const{return loaded;}
 void AddNewMailDeliverTime(time_t){}void AddMail(Mail*m){mails.emplace_back(m);}void AddMItem(Item*i){mailItems.emplace_back(i);}
 InventoryResult CanStoreNewItem(uint32,uint32,ItemPosCountVec&dest,uint32,uint32 count){if(partial)dest.push_back(1);else if(admission==EQUIP_ERR_OK)dest.push_back(count);return admission;}
 InventoryResult CanTakeMoreSimilarItems(uint32 id,uint32 count)const{return CanTakeMoreSimilarItems(id,count,nullptr,nullptr,false);}
 InventoryResult CanTakeMoreSimilarItems(uint32,uint32,Item*,uint32*,bool)const;
 uint32 GetItemCount(uint32 id,bool includeBank,Item*)const{Check(includeBank,"unique check excluded bank");auto i=bank.find(id);return i==bank.end()?0:i->second;}
 uint32 GetItemCountWithLimitCategory(uint32,Item*)const{return categoryCount;}
 std::tuple<uint32,uint32>GetWorldQuestBonusTreeMod(WorldQuest const*){return {25,845};}
 uint32 getLevel()const{return level;}uint32 GetLootSpecID()const{return 71;}uint32 GetTeamId()const{return horde?TEAM_HORDE:0;}uint32 getClass()const{return 3;}
 bool HasSpell(uint32 s)const{return spells.count(s)!=0;}Garrison*GetGarrisonPtr(){static Garrison g;return &g;}
 Item*StoreNewItem(ItemPosCountVec const&dest,uint32 id,bool,uint32 random,GuidSet const&,std::vector<uint32>const&bonus,uint32 context){
  ++storeCalls;if(storeFails)return nullptr;uint32 count=0;for(auto n:dest)count+=n;
  auto item=new Item;item->proto=sObjectMgr->GetItemTemplate(id);item->data={id,count,context,random,7,level,true,bonus,GetGUIDLow(),1};bags.emplace_back(item);return item;
 }
 void SendDisplayToast(uint32,int,bool,uint32,int,uint32,Item*i){Check(i!=nullptr,"null item toast");++toasts;}
 void SendNewItem(Item*i,uint32,bool,bool){Check(i!=nullptr,"null item push");++pushes;}
 void Select(WorldQuest const*wq,CharacterDatabaseTransaction&rewardTrans){uint32 quest_id=44923;
#ifdef OLD_DELIVERY
  Item*rewardItem=nullptr;
#endif
#include "Selection.inc"
 }
 void SaveInventoryAndGoldToDB(CharacterDatabaseTransaction&t){t->Append(CharacterDatabase.GetPreparedStatement(INVENTORY_SAVE));}
 void _SaveQuestStatus(CharacterDatabaseTransaction&t){t->Append(CharacterDatabase.GetPreparedStatement(QUEST_SAVE));}
 void _SaveWorldQuestStatus(CharacterDatabaseTransaction&t){t->Append(CharacterDatabase.GetPreparedStatement(WORLD_SAVE));}
 void Commit(CharacterDatabaseTransaction&rewardTrans){
#include "Commit.inc"
 }
};
uint64 MailReceiver::GetPlayerGUIDLow()const{return p->GetGUIDLow();}
bool Item::Create(uint64 guid,uint32 entry,Player const*p){++createAttempts;if(failAt&&createAttempts==failAt)return false;data.guid=guid;data.entry=entry;data.owner=p?p->GetGUIDLow():0;ownerPlayer=const_cast<Player*>(p);proto=sObjectMgr->GetItemTemplate(entry);return proto!=nullptr;}
void Scripts::OnItemCreate(Player*p,ItemTemplate const*,Item*i){i->SetState(ITEM_CHANGED,p);}
#include "CreateItem.inc"
#include "ItemState.inc"
#include "QueueAdd.inc"
#include "QueueRemove.inc"
#include "Queued.inc"
#include "Similar.inc"
#include "AddItem.inc"
#include "SendMail.inc"
#include "DeleteMailItems.inc"
#include "Delivery.inc"
size_t Rows(Transaction const&t,uint32 type){return std::count_if(t.rows.begin(),t.rows.end(),[=](auto const&p){return p->type==type;});}
void CheckItem(Snapshot const&i,uint32 count,uint32 context=25){Check(i.count==count&&i.context==context&&i.property==77&&i.upgrade==7&&i.level==110&&i.bonuses==std::vector<uint32>({11,22}),"reward metadata/count changed");}
int Run(){
 objectMgr.templates={{100,{100,5,BIND_WHEN_PICKED_UP}},{101,{101,5,BIND_WHEN_EQUIPED}},{200,{200,1,BIND_QUEST_ITEM}}};
 RewardTemplate templ;WorldQuest wq{};wq.worldQuest=&templ;wq.ItemList[0]={100,101,7};Recipe recipe{200,1};
 for(bool horde:{false,true})for(bool isRecipe:{false,true}){
  Player p;p.horde=horde;p.admission=EQUIP_ERR_INV_FULL;p.partial=true;p.loaded=false;
  auto tx=std::make_shared<Transaction>();wq.Recipe=isRecipe?&recipe:nullptr;p.Select(&wq,tx);
#ifdef OLD_DELIVERY
  Check(!p.storeCalls&&tx->items.empty()&&tx->rows.empty(),"baseline full inventory no longer reproduces lost reward");
#else
  Check(!p.storeCalls&&p.bags.empty()&&!p.pushes&&!p.toasts,"full inventory applied partial plan or inventory notification");
  Check(tx->items.size()==size_t(isRecipe?3:2),"missing mailed recipe/main stacks");
  uint32 mainCount=0,recipeCount=0;
  for(auto const&i:tx->items){Check(!i.owner,"mail ownership not saved atomically");Check(i.bound==(i.entry==200||horde),"BOP/quest/BOE binding changed");CheckItem(i,i.entry==200?1:(mainCount?2:5));if(i.entry==200)recipeCount+=i.count;else{Check(i.entry==(horde?100:101),"faction item changed");mainCount+=i.count;}}
  Check(mainCount==7&&recipeCount==(isRecipe?1u:0u),"main or recipe quantity lost");
  Check(Rows(*tx,CHAR_INS_MAIL_ITEM)==tx->items.size()&&Rows(*tx,CHAR_INS_MAIL)==size_t(isRecipe?2:1),"mail attachment rows");
  p.Commit(tx);Check(Rows(*tx,INVENTORY_SAVE)==1&&tx->rows[tx->rows.size()-3]->type==INVENTORY_SAVE&&tx->rows[tx->rows.size()-2]->type==QUEST_SAVE&&tx->rows.back()->type==WORLD_SAVE,"inventory, mail and checkpoint use different transaction");
#endif
 }
#ifndef OLD_DELIVERY
 wq.Recipe=nullptr;
 for(bool loaded:{false,true}){
  Player p;p.loaded=loaded;p.admission=EQUIP_ERR_INV_FULL;auto tx=std::make_shared<Transaction>();RewardWorldQuestItem(&p,&wq,44923,100,63,tx);
  Check(tx->items.size()==13&&Rows(*tx,CHAR_INS_MAIL)==2&&Rows(*tx,CHAR_INS_MAIL_ITEM)==13,"stack clamp/mail12 batching");
  uint32 sum=0;for(auto const&i:tx->items){sum+=i.count;Check(i.count<=5&&i.bound,"stack size or BOP binding");CheckItem(i,i.count);}
  Check(sum==63&&p.mailItems.size()==(loaded?13u:0u)&&p.mails.size()==(loaded?2u:0u),"loaded/unloaded native ownership");
 }
 for(auto error:{EQUIP_ERR_ITEM_MAX_COUNT,EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS,EQUIP_ERR_ITEM_NOT_FOUND,EQUIP_ERR_BAG_FULL}){
  Player p;p.admission=error;auto tx=std::make_shared<Transaction>();RewardWorldQuestItem(&p,&wq,44923,100,1,tx);Check(tx->rows.empty()&&!p.storeCalls,"non-INVENTORY_FULL error bypassed");
 }
 {Player p;p.admission=EQUIP_ERR_INV_FULL;objectMgr.templates[100].maxCount=1;p.bank[100]=1;auto tx=std::make_shared<Transaction>();RewardWorldQuestItem(&p,&wq,44923,100,1,tx);Check(tx->rows.empty()&&!p.storeCalls,"full inventory masks bank uniqueness rejection");objectMgr.templates[100].maxCount=0;}
 {Player p;p.admission=EQUIP_ERR_INV_FULL;objectMgr.templates[100].category=9;p.categoryCount=1;auto tx=std::make_shared<Transaction>();RewardWorldQuestItem(&p,&wq,44923,100,1,tx);Check(tx->rows.empty(),"full inventory bypassed category limit");objectMgr.templates[100].category=0;}
 {Player p;auto tx=std::make_shared<Transaction>();RewardWorldQuestItem(&p,&wq,44923,100,7,tx);Check(p.bags.size()==1&&p.storeCalls==1&&p.pushes==1&&p.toasts==1&&tx->rows.empty(),"ordinary bag path changed");CheckItem(p.bags[0]->data,7);}
 {Player p;p.storeFails=true;auto tx=std::make_shared<Transaction>();RewardWorldQuestItem(&p,&wq,44923,100,1,tx);Check(!p.pushes&&!p.toasts&&tx->rows.empty(),"null StoreNewItem pushes");}
 {Player p;p.admission=EQUIP_ERR_INV_FULL;auto tx=std::make_shared<Transaction>();int before=living;createAttempts=0;failAt=2;RewardWorldQuestItem(&p,&wq,44923,100,11,tx);failAt=0;Check(tx->rows.empty()&&living==before,"partial creation queued mail or leaked");}
 {Player p;p.admission=EQUIP_ERR_INV_FULL;auto tx=std::make_shared<Transaction>();templ.MinItemLevel=900;RewardWorldQuestItem(&p,&wq,44923,101,1,tx);Check(objectMgr.observedNeedLevel==900&&!tx->items[0].bound,"level override or BOE binding");templ.MinItemLevel=0;}
 {Player p;p.admission=EQUIP_ERR_INV_FULL;auto tx=std::make_shared<Transaction>();RewardWorldQuestItem(&p,&wq,44923,999,1,tx);RewardWorldQuestItem(&p,&wq,44923,100,0,tx);Check(tx->rows.empty(),"missing/zero item generated mail");}
 {Player p;p.admission=EQUIP_ERR_INV_FULL;wq.ItemList[0]={0,0,1};wq.ItemList[3]={100,101,4};auto tx=std::make_shared<Transaction>();p.Select(&wq,tx);Check(tx->items.size()==1&&tx->items[0].entry==101&&tx->items[0].count==4,"class fallback changed");}
 {Player p;p.admission=EQUIP_ERR_INV_FULL;recipe.NeedSpell=42;recipe.NotNeedSpell=43;wq.Recipe=&recipe;auto tx=std::make_shared<Transaction>();p.Select(&wq,tx);Check(tx->items.size()==1,"unlearned recipe admitted");p.spells={42};auto yes=std::make_shared<Transaction>();p.Select(&wq,yes);Check(yes->items.size()==2,"eligible recipe lost");p.spells.insert(43);auto no=std::make_shared<Transaction>();p.Select(&wq,no);Check(no->items.size()==1,"excluded recipe admitted");}
 {Player p;p.admission=EQUIP_ERR_INV_FULL;wq.Recipe=nullptr;wq.ItemList[0]={100,101,7};wq.typeReward=WORLD_QUEST_TYPE_REWARD_ARTIFACT_POWER;proc=true;auto tx=std::make_shared<Transaction>();p.Select(&wq,tx);uint32 sum=0;for(auto const&i:tx->items)sum+=i.count;Check(sum==14,"existing AP bonus quantity lost");proc=false;}
 Check(CharacterDatabase.standalone==0,"mail escaped completion transaction with standalone owner UPDATE");
#endif
 Check(living==0,"native mail cleanup leaked/double-owned items");
 Check(!badQueue,"mail item left a dangling native inventory queue entry");
#ifdef OLD_DELIVERY
 std::cout<<"PASS: pinned ccb40e91 native faction/recipe selection reproduces lost items on full inventory.\n";
#else
 std::cout<<"PASS: native helper/selection/CreateItem/mail dispatcher preserve main/recipe/faction/class metadata, stack counts, batching, error gates, mail ownership and shared completion transaction.\n";
#endif
 return 0;
}
int main(){try{return Run();}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
