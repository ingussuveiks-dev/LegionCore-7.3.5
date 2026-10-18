#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t;using int8=int8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
#include "QuestType.inc"
#include "QuestInfo.inc"
#include "QuestStatus.inc"
#include "QuestObjectiveType.inc"
#include "__QuestFlags.inc"
#include "QuestFlagsEx.inc"
#include "__QuestSpecialFlags.inc"
#include "QuestObjectiveFlags.inc"
#include "QuestObjective.inc"
#include "WorldQuestInfo.inc"
#include "InventoryResult.inc"
#include "QUEST_ITEM_DROP_COUNT.inc"
#include "QUEST_REWARD_DISPLAY_SPELL_COUNT.inc"
constexpr uint32 MAX_QUEST_LOG_SIZE=25,QUEST_STATE_COMPLETE=1,NULL_BAG=255,NULL_SLOT=255,CLASS_NONE=0,TEAM_HORDE=1;
// Identifiers on controlled services; they do not model the underlying systems.
enum {CRITERIA_TYPE_KILL_CREATURE_TYPE,CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD,CRITERIA_TYPE_KILL_CREATURE,CRITERIA_TIMED_TYPE_CREATURE,
 CRITERIA_TYPE_COMPLETE_WORLD_QUEST,CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD,CRITERIA_TYPE_COMPLETE_DAILY_QUEST,CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY,
 CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE,CRITERIA_TYPE_COMPLETE_QUEST_COUNT,CRITERIA_TYPE_COMPLETE_QUEST,CRITERIA_TYPE_COMPLETE_QUESTS_COUNT,
 CONFIG_MAX_PLAYER_LEVEL,CONFIG_DROP_DUNGEON_ONLY_X1,RATE_DROP_MONEY,WORLD_QUEST_TYPE_REWARD_ARTIFACT_POWER,
 MAIL_CHECK_MASK_HAS_BODY,QUEST_DEFAULT_SAVE_TYPE,SPELL_EFFECT_LEARN_SPELL,SPELL_EFFECT_CREATE_ITEM,SPELL_EFFECT_APPLY_AURA,SPELL_EFFECT_TRIGGER_SPELL,
 EFFECT_0,TARGET_UNIT_CASTER,QUEST_SAVE,CHAR_INS_CHARACTER_WORLDQUESTSTATUS};
namespace ToastType {enum {ITEM,CURRENCY,MONEY,HONOR};}
namespace DisplayToastMethod {constexpr int DISPLAY_TOAST_SPECIAL_UNK=0;}
#define TC_LOG_DEBUG(...) ((void)0)
static void Check(bool ok,char const* why){if(!ok)throw std::runtime_error(why);}
namespace GameTime {uint32 now=1000;uint32 GetGameTime(){return now;}}
struct ObjectGuid {
 using LowType=uint64;uint64 value=0;bool IsEmpty()const{return !value;}
 bool IsGameObject()const{return false;}bool IsCreature()const{return true;}bool IsItem()const{return false;}
 static ObjectGuid const Empty;
};
ObjectGuid const ObjectGuid::Empty{};using GuidSet=std::set<uint64>;
struct Player;struct Creature;struct Unit;
struct Object {virtual ~Object()=default;virtual bool IsUnit()const{return true;}virtual Unit*ToUnit(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}};
struct Unit:Object {uint32 casts=0;void CastSpell(Player*,uint32 id,bool){Check(id==230020,"wrong reward spell");++casts;}Unit*ToUnit()override{return this;}};
struct Creature:Unit {
 std::list<Player*> players;bool visible=true;
 uint32 GetEntry()const{return 115133;}uint32 GetCreatureType()const{return 7;}ObjectGuid GetGUID()const{return {100};}
 Creature*ToCreature()override{return this;}bool IsAlive()const{return true;}void DespawnOrUnsummon(uint32){}
 bool InSamePhase(Player*)const;bool IsWithinDistInMap(Player*,float,bool,bool)const;
 void GetPlayerListInGrid(std::list<Player*>&p,float){p=players;}void SetVisible(bool v){visible=v;}
};
struct Map {int GetDifficultyID()const{return 0;}uint32 GetDifficultyLootItemContext()const{return 0;}bool IsDungeon()const{return false;}Creature*GetCreature(ObjectGuid){return nullptr;}};
struct Group {bool raid=false;bool isRaidGroup()const{return raid;}};
struct Quest {
 uint32 Id=44746,Type=3,QuestInfoID=135,QuestSortID=7503,flags=0,FlagsEx=0,RewardSpell=230020;
 uint32 special=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST;
 uint32 m_rewChoiceItemsCount=0,m_rewItemsCount=0,m_rewCurrencyCount=0,PackageID=0;
 uint32 ItemDrop[4]={},ItemDropQuantity[4]={},RewardChoiceItemId[6]={},RewardChoiceItemCount[6]={},RewardItemId[4]={},RewardItemCount[4]={},RewardCurrencyId[4]={},RewardCurrencyCount[4]={};
 uint32 RewardSkillId=0,RewardSkillPoints=0,RewardHonor=0,RewardArtifactCategoryID=0,RewardArtifactXP=0,RewardArtifactXPMultiplier=0;
 uint32 RewardTitleId=0,RewardMailTemplateId=0,RewardMailDelay=0,RewardDisplaySpell[3]={};std::string RewardMailTitle;
 std::vector<QuestObjective> objectives;
 uint32 GetQuestId()const{return Id;}auto const&GetObjectives()const{return objectives;}
 bool HasFlag(uint32 f)const{return (flags&f)!=0;}bool HasSpecialFlag(uint32 f)const{return (special&f)!=0;}
 bool IsWorld()const{return Type==QUEST_TYPE_TASK&&QuestInfoID==QUEST_INFO_RARE_WORLD_QUEST;}bool IsEmissary()const{return false;}
 bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}bool IsRepeatable()const{return false;}
 bool IsAutoComplete()const{return Type==QUEST_TYPE_AUTOCOMPLETE;}bool IsAllowedInRaid(int)const{return false;}
 bool IsRated()const{return false;}bool IsDaily()const{return false;}bool IsDFQuest()const{return false;}
 int32 GetRewMoneyMaxLevel()const{return 0;}
};
struct RewardTemplate {uint32 Currency=0,CurrencyCount=0;};
struct Recipe {uint32 ItemID=0,ItemCount=1,NeedSpell=0,NotNeedSpell=0;};
struct RewardItem {uint32 ItemIDH=0,ItemIDA=0,ItemCount=1;};
struct WorldQuest {
 RewardTemplate*worldQuest=nullptr;Recipe const*Recipe=nullptr;RewardItem ItemList[12];
 uint32 StartTime=1000,Timer=1000,ResetTime=2000,Gold=12300,CurrencyID=1220,CurrencyCount=650;int typeReward=-1;
};
struct Store {Quest quest;RewardTemplate templ;WorldQuest rotation;bool active=true;
 Store(){rotation.worldQuest=&templ;rotation.ItemList[0]={100,101,1};}
 Quest const*GetQuestTemplate(uint32 id){return id==44746?&quest:nullptr;}
 WorldQuest const*GetWorldQuest(Quest const*){return active?&rotation:nullptr;}
}store;auto sQuestDataStore=&store;
struct Statement {uint32 type;std::map<int,uint64>values;void setUInt32(int i,uint32 n){values[i]=n;}void setUInt64(int i,uint64 n){values[i]=n;}};
using CharacterDatabasePreparedStatement=Statement;
struct Transaction {std::vector<std::unique_ptr<Statement>>rows;void Append(Statement*p){rows.emplace_back(p);}};
using CharacterDatabaseTransaction=std::shared_ptr<Transaction>;
struct Database {std::vector<CharacterDatabaseTransaction>committed;
 CharacterDatabaseTransaction BeginTransaction(){return std::make_shared<Transaction>();}
 Statement*GetPreparedStatement(uint32 type){auto p=new Statement;p->type=type;return p;}
 void CommitTransaction(CharacterDatabaseTransaction const&t){committed.push_back(t);}
}CharacterDatabase;
struct Field {uint32 n;uint32 GetUInt32()const{return n;}};
struct Result {std::vector<std::array<Field,3>>rows;size_t index=0;Field*Fetch(){return rows.at(index).data();}bool NextRow(){return ++index<rows.size();}};
using PreparedQueryResult=std::shared_ptr<Result>;
struct Session {uint32 GetAccountId()const{return 77;}uint32 GetSessionDbLocaleIndex()const{return 0;}};
struct ItemTemplate{};
struct Item {static uint32 GenerateItemRandomPropertyId(uint32,uint32=0){return 0;}void GiveArtifactXp(uint32,void*,uint32){}};
struct ObjectMgr {ItemTemplate const*GetItemTemplate(uint32){return nullptr;}template<class...T>std::vector<uint32>GetItemBonusTree(T...){return {};}} objectMgr;auto sObjectMgr=&objectMgr;
struct DB2 {uint32 GetCurrencyPrecision(uint32){return 1;}uint32 GetQuestUniqueBitFlag(uint32 id){return id;}}sDB2Manager;
struct World {uint32 getIntConfig(int){return 110;}bool getBoolConfig(int){return false;}float getRate(int){return 1;}}world;auto sWorld=&world;
struct Log {template<class...T>void outWarden(T...){}}logSink;auto sLog=&logSink;
struct CriteriaTree {std::vector<CriteriaTree const*>Children;};
struct AchievementMgr {CriteriaTree const*GetCriteriaTree(uint32){return nullptr;}}achievementMgr;auto sAchievementMgr=&achievementMgr;
struct Achievements {template<class...T>void StartTimedAchievement(T...){}void RemoveCriteriaProgress(CriteriaTree const*){}};
struct Guild {template<class...T>void UpdateAchievementCriteria(T...){}};
struct GuildMgr {Guild*GetGuildById(uint32){return nullptr;}}guildMgr;auto sGuildMgr=&guildMgr;
struct Reputation {int32 GetReputation(int32)const{return 0;}};
struct Scripts {uint32 completions=0;void OnQuestComplete(Player*,Quest const*){++completions;}}scripts;auto sScriptMgr=&scripts;
struct Garrison {bool hasLegionFall()const{return false;}};
bool roll_chance_i(int){return false;}
struct CharTitlesEntry{};struct CurrencyTypesEntry{};
struct Localized {char const*Str[1]={""};};struct MailTemplateEntry {Localized*Body=nullptr;};
template<class T>struct EmptyStore {T const*LookupEntry(uint32){return nullptr;}};
EmptyStore<CharTitlesEntry>sCharTitlesStore;EmptyStore<MailTemplateEntry>sMailTemplateStore;
struct CurrencyStore {CurrencyTypesEntry entry;CurrencyTypesEntry const*LookupEntry(uint32){return &entry;}}sCurrencyTypesStore;
struct MailDraft {explicit MailDraft(uint32){}void AddSubject(std::string const&){}void AddBody(char const*){}template<class...T>void SendMailTo(T...){throw std::runtime_error("Unexpected static reward mail");}};
struct Target {uint32 GetTarget()const{return 18;}};struct SpellEffect {Target TargetA;};
struct SpellInfo {SpellEffect effect;SpellEffect*Effects[1]={&effect};bool HasEffect(uint32)const{return false;}};
struct SpellMgr {SpellInfo spell;SpellInfo const*GetSpellInfo(uint32 id){return id==230020?&spell:nullptr;}}spellMgr;auto sSpellMgr=&spellMgr;
struct PhaseUpdateData {void AddQuestUpdate(uint32){}};struct PhaseMgr {void NotifyConditionChanged(PhaseUpdateData const&){}};
struct QuestStatusData {QuestStatus Status=QUEST_STATUS_INCOMPLETE;uint32 Timer=1;};
using ItemPosCountVec=std::vector<uint32>;
struct LootStore{};LootStore LootTemplates_Spell;
struct LootItem {struct {uint32 ItemID=142487,CurrencyID=0,RandomPropertiesID=0;struct {std::vector<uint32>BonusListIDs;uint32 Context=0;}ItemBonus;}item;
 uint32 count=10,type=0;bool needs_quest=true,currency=false;
};
struct Loot {bool _isItemLoot=false;uint32 _itemContext=0,objEntry=0,loot_type=0;ObjectGuid objGuid;LootItem item;bool present=false;
 void FillLoot(uint32,LootStore const&,Player*,bool);uint32 GetMaxSlotInLootFor(Player*)const{return present?1:0;}
 LootItem*LootItemInSlot(uint32,Player*){return &item;}bool AllowedForPlayer(Player*,uint32,uint32,uint32,bool,LootItem*){return true;}
};
uint32 rolledFragments=10;
struct Player:Unit {
 using WorldQuestStatusMap=std::map<uint32,WorldQuestInfo>;
 WorldQuestStatusMap m_worldquests;std::set<uint32>m_RewardedQuests,m_accuntQuests,bits;std::map<uint32,uint32>m_RewardedQuestsSave;
 QuestStatusData status;bool accepted=true,alive=true,phase=true,inRange=true,contributor=true,lostArmy=true,horde=false;
 InventoryResult admission=EQUIP_ERR_OK;bool partial=false,storeFails=false;
 Group*group=nullptr;Map map;Session session;PhaseMgr phaseMgr;Achievements achievements;Achievements*m_achievementMgr=&achievements;
 int32 credit=0;uint32 creditPackets=0,rewardPackets=0,fragmentCount=0,lootRolls=0,lootStores=0,errors=0,inventoryPushes=0,dynamicCount=0,mailCount=0;
 uint32 sourceRemovals=0,reputationCalls=0,areaUpdates=0,statusWrites=0;int64_t money=0;std::map<uint32,uint32>currencies;
 bool delayTeleport=false;std::vector<std::string>order;std::vector<std::function<void()>>pending;
 ObjectGuid GetGUID()const{return {4045620583ULL};}uint64 GetGUIDLow()const{return GetGUID().value;}
 char const*GetName()const{return "fixture";}Session*GetSession(){return &session;}Map*GetMap(){return &map;}Group*GetGroup(){return group;}
 bool IsAlive()const{return alive;}uint32 GetGuildId()const{return 0;}
 uint32 getLevel()const{return 110;}uint32 GetLootSpecID()const{return 0;}uint32 GetTeamId()const{return horde?TEAM_HORDE:0;}uint32 getClass()const{return 3;}
 bool HasSpell(uint32)const{return false;}bool HasCurrency(uint32,uint32)const{return true;}bool HasEnoughMoney(uint64)const{return true;}
 bool HasQuestForItem(uint32 id)const{return id==142487&&lostArmy&&fragmentCount<1000;}
 Reputation GetReputationMgr()const{return {};}
 template<class...T>void UpdateAchievementCriteria(T...){}void AddKillCreature(uint32,uint16,bool){}void AddTrackingQuestIfNeeded(ObjectGuid){}
 uint32 GetQuestSlotQuestId(uint8 slot)const{return slot==0&&accepted?44746:0;}
 QuestStatus GetQuestStatus(uint32)const{return accepted?status.Status:QUEST_STATUS_NONE;}
 QuestStatusData*getQuestStatus(uint32){return accepted?&status:nullptr;}
 uint16 FindQuestSlot(uint32)const{return accepted?0:MAX_QUEST_LOG_SIZE;}
 int32 GetQuestObjectiveData(Quest const*,int8 index){Check(index==0,"wrong objective index");return credit;}
 void SetQuestObjectiveData(Quest const*,QuestObjective const*,int32 n){credit=n;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32){++creditPackets;}
 void SetQuestStatus(uint32,QuestStatus s){++statusWrites;status.Status=s;}
 void SetQuestSlotState(uint16,int){}void SetQuestSlot(uint16,uint32){}void RemoveActiveQuest(uint32){accepted=false;order.push_back("remove");}
 bool IsQuestWeekRewarded(uint32)const{return false;}bool IsQuestSeasonalRewarded(uint32)const{return false;}bool CanTakeQuest(Quest const*,bool)const{return accepted;}
 void SetCanDelayTeleport(bool b){delayTeleport=b;}
 void DestroyItemCount(uint32,uint32,bool){}void TakeQuestSourceItem(uint32,bool){++sourceRemovals;}void RemoveTimedQuest(uint32){}
 InventoryResult CanStoreNewItem(uint8,uint8,ItemPosCountVec&dest,uint32,uint32 n){if(admission==EQUIP_ERR_OK)dest.push_back(n);else if(partial)dest.push_back(1);return admission;}
 template<class...T>Item*StoreNewItem(ItemPosCountVec const&dest,uint32 id,T...){Check(id==142487,"unexpected static item");++lootStores;if(storeFails)return nullptr;for(auto n:dest)fragmentCount+=n;static Item item;return &item;}
 void SendNewItem(Item*i,uint32,bool,bool,bool broadcast=true){Check(i&&broadcast,"native fragment item push missing");++inventoryPushes;}
 template<class...T>void SendDisplayToast(T...){}
 void SendEquipError(InventoryResult,Item* =nullptr,Item* =nullptr,uint32=0){++errors;}
 uint32 GetCurrency(uint32 id){return currencies[id];}uint32 GetTotalCurrencyCap(uint32){return 0;}
 void ModifyCurrency(uint32 id,uint32 n,bool=false){currencies[id]+=n;}
 void RewardQuestPackage(uint32,uint32){}void UpdateSkillPro(uint32,uint32,uint32){}void RewardReputation(Quest const*){++reputationCalls;}void RewardGuildReputation(Quest const*){}
 uint32 GetQuestXPReward(Quest const*)const{return 0;}int32 GetQuestMoneyReward(Quest const*)const{return 0;}void GiveXP(uint32,void*){}
 void ModifyMoney(int32 n){money+=n;}void RewardHonor(void*,uint32,uint32){}Item*GetArtifactWeapon(){return nullptr;}void SetTitle(CharTitlesEntry const*){}
 void PvpRatedQuestReward(uint32){}Garrison*GetGarrisonPtr(){return nullptr;}
 void SetDailyQuestStatus(uint32){}void SetWeeklyQuestStatus(uint32){}void SetSeasonalQuestStatus(uint32){}
 void SendQuestReward(Quest const*,uint32,Creature*,int32,bool){++rewardPackets;}
 void AddDelayedEvent(uint32 n,std::function<void()>f){Check(n==100,"wrong reward callback delay");pending.push_back(f);}
 PhaseMgr&GetPhaseMgr(){return phaseMgr;}void GetZoneAndAreaId(uint32&z,uint32&a){z=7503;a=7833;}void PrepareAreaQuest(uint32){}
 uint32 GetAreaId()const{return 7833;}void UpdateAreaQuestTasks(uint32,uint32){++areaUpdates;Check(WorldQuestCompleted(44746),"task refresh before completion record");}
 bool IsQuestRewardDisplayOnly(SpellInfo const*)const{return false;}void SetQuestCompletedBit(uint32 id,bool b){if(b)bits.insert(id);else bits.erase(id);}void SendQuestGiverStatusMultiple(){}
 void _SaveQuestStatus(CharacterDatabaseTransaction&t){t->Append(CharacterDatabase.GetPreparedStatement(QUEST_SAVE));order.push_back("save");}
 bool CanRewardWorldQuest(Quest const*)const;bool WorldQuestCompleted(uint32)const;void _SaveWorldQuestStatus(CharacterDatabaseTransaction&);void _LoadWorldQuestStatus(PreparedQueryResult);
 void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty);bool HasQuestObjectiveComplete(Quest const*,QuestObjective const&);bool CanCompleteQuest(uint32);void CompleteQuest(uint32);
 void RewardQuest(Quest const*,uint32,Object*,bool);
 bool AutoStoreLoot(uint8,uint8,uint32,LootStore const&,bool,uint32);
 bool AutoStoreLoot(uint32 id,LootStore const&s){return AutoStoreLoot(NULL_BAG,NULL_SLOT,id,s,true,0);}
};
bool Creature::InSamePhase(Player*p)const{return p->phase;}bool Creature::IsWithinDistInMap(Player*p,float,bool,bool)const{return p->inRange;}
void Loot::FillLoot(uint32 id,LootStore const&,Player*p,bool){Check(id==229113,"wrong fragment loot");++p->lootRolls;present=p->HasQuestForItem(142487);item.count=rolledFragments;}
// Production item helper has a separate native CreateItem/mail regression.
// This controlled leaf records selection and checks the transaction contract.
void RewardWorldQuestItem(Player*p,WorldQuest const*,uint32 quest,uint32 id,uint32 n,CharacterDatabaseTransaction&tx){
 Check(quest==44746&&id==(p->horde?100u:101u),"dynamic faction item selection changed");
 Check(tx!=nullptr,"item reward outside completion transaction");if(p->admission==EQUIP_ERR_INV_FULL)p->mailCount+=n;else p->dynamicCount+=n;
 p->order.push_back("item");
}
#include "AutoStoreLoot.inc"
#include "Credit.inc"
#include "HasObjective.inc"
#include "CanComplete.inc"
#include "Admission.inc"
#include "Completed.inc"
#include "Complete.inc"
#include "Reward.inc"
#include "SaveWorld.inc"
#include "LoadWorld.inc"
namespace ObjectAccessor {Creature*GetCreature(Creature&,ObjectGuid){return nullptr;}}
namespace Rockaway {
constexpr uint32 QuestId=44746,Credit=115168,Fragment=142487,CreditLoot=229113;constexpr float Radius=150;
#include "Active.inc"
#ifdef OLD_ELIGIBILITY
#include "EligibleBaseline.inc"
#else
#include "Eligible.inc"
#endif
}
struct Finale {
 Creature*me;bool enabled=true,outro=true,dead[4]={};uint32 finishTime=6000,cooldown=0;ObjectGuid slots[4];
 struct {void Despawn(Creature*){}}summons;
 bool Contributor(Player*p){return p->contributor;}void ClearRun(){outro=false;}
#include "Finish.inc"
};
static void Load(char const*path){
 std::ifstream file(path);Check(bool(file),"missing SQL snapshot");std::string line;uint32 quests=0,objectives=0,loots=0,marker=0,staticFields=0,addonFields=0;
 while(std::getline(file,line)){std::istringstream in(line);std::string tag;in>>tag;
  if(tag=="quest"){auto&q=store.quest;in>>q.Id>>q.Type>>q.QuestInfoID>>q.QuestSortID>>q.flags>>q.FlagsEx>>q.RewardSpell;++quests;}
  else if(tag=="objective"){QuestObjective o;int type,index;in>>o.ID>>o.QuestID>>type>>index>>o.ObjectID>>o.Amount>>o.Flags>>o.Flags2;o.Type=uint8(type);o.StorageIndex=int8(index);store.quest.objectives.push_back(o);++objectives;}
  else if(tag=="loot"){uint32 id,item,min,max,required;in>>id>>item>>min>>max>>required;Check(id==229113&&item==142487&&min==10&&max==15&&required==1,"scoped fragment loot changed");++loots;}
  else if(tag=="marker")in>>marker;
  else if(tag=="static"||tag=="addon"){uint32 value;while(in>>value){Check(!value,"Live reward gained a branch outside this fixture's configured policy");if(tag=="static")++staticFields;else ++addonFields;}}
 }
 Check(quests==1&&objectives==1&&loots==1&&marker==1&&staticFields==15&&addonFields==3,"incomplete live input");auto&q=store.quest;auto&o=q.objectives[0];
 Check(q.Id==44746&&q.Type==QUEST_TYPE_TASK&&q.QuestInfoID==135&&q.QuestSortID==7503&&!q.FlagsEx&&q.RewardSpell==230020,"task/reward configuration");
 Check(o.ID==286897&&o.QuestID==44746&&o.Type==QUEST_OBJECTIVE_MONSTER&&o.StorageIndex==0&&o.ObjectID==115168&&o.Amount==1&&!o.Flags&&!o.Flags2,"native Rockaway credit objective");
}
static void Reset(){store.active=true;store.rotation.worldQuest=&store.templ;store.rotation.ResetTime=2000;GameTime::now=1000;CharacterDatabase.committed.clear();}
static void Finish(Player&p){Creature actor;actor.players={&p};Finale ai{&actor};ai.Finish();ai.Finish();}
int Run(char const*path){
 Load(path);
#ifdef OLD_ELIGIBILITY
 for(bool missingTemplate:{false,true}){Reset();rolledFragments=10;Player p;
  if(missingTemplate)store.rotation.worldQuest=nullptr;else p.m_worldquests[44746]={44746,2000,false};
  Finish(p);Finish(p);
  Check(p.fragmentCount==20&&p.lootRolls==2&&p.credit==1&&!p.rewardPackets&&CharacterDatabase.committed.empty(),"pinned stale-checkpoint/template-null fragment leak no longer reproduced");
 }
 std::cout<<"PASS: pinned65c7ac0a eligibility repeats fragment grants for an already rewarded stale task or null reward template, while real completion admission rejects the WQ reward.\n";
 return 0;
#else
 for(bool horde:{false,true})for(bool full:{false,true})for(uint32 count=10;count<=15;++count){
  Reset();rolledFragments=count;Player p;p.horde=horde;p.admission=full?EQUIP_ERR_INV_FULL:EQUIP_ERR_OK;p.partial=full;
  Finish(p);
  Check(p.credit==1&&p.creditPackets==1&&p.rewardPackets==1&&!p.accepted,"one native task completion/reward");
  Check(p.money==12300&&p.currencies[1220]==650&&p.reputationCalls==1,"dynamic currency/money or reputation dispatch lost");
  Check(p.casts==1&&p.m_worldquests.at(44746).resetTime==2000&&!p.m_worldquests.at(44746).needSave,"outro/checkpoint missing");
  Check(p.fragmentCount==(full?0:count)&&p.errors==(full?1u:0u)&&p.lootRolls==1&&p.lootStores==(full?0u:1u),"native fragment admission changed");
  Check(p.dynamicCount==(full?0u:1u)&&p.mailCount==(full?1u:0u),"dynamic item reward leaf dispatch");
  Check(CharacterDatabase.committed.size()==1&&CharacterDatabase.committed[0]->rows.size()==2,"completion transaction missing");
  auto const&row=*CharacterDatabase.committed[0]->rows[1];Check(row.type==CHAR_INS_CHARACTER_WORLDQUESTSTATUS&&row.values.at(1)==44746&&row.values.at(3)==2000,"native checkpoint statement");
  auto saved=std::make_shared<Result>();saved->rows.push_back({Field{44746},Field{uint32(p.GetGUIDLow())},Field{2000}});
  Player login;login._LoadWorldQuestStatus(saved);Check(login.WorldQuestCompleted(44746)&&!login.CanRewardWorldQuest(&store.quest),"login permits duplicate reward");
  login.CompleteQuest(44746);login.RewardQuest(&store.quest,0,&login,true);Check(!login.rewardPackets&&!login.casts&&CharacterDatabase.committed.size()==1,"login/reward bypass duplicated task");
  p.KilledMonsterCredit(115168);p.CompleteQuest(44746);p.RewardQuest(&store.quest,0,&p,true);Finish(p);
  Check(p.rewardPackets==1&&p.casts==1&&p.lootRolls==1&&CharacterDatabase.committed.size()==1,"repeat callback/direct reward duplicated completion");
 }
 for(int kind=0;kind<5;++kind){Reset();Player p;if(kind==0)store.active=false;if(kind==1)store.rotation.ResetTime=999;if(kind==2)store.rotation.ResetTime=1000;if(kind==3)p.m_worldquests[44746]={44746,2000,false};if(kind==4)store.rotation.worldQuest=nullptr;
  Finish(p);Finish(p);Check(!p.credit&&!p.lootRolls&&!p.fragmentCount,"unrewardable finale leaked fragment credit");
  p.KilledMonsterCredit(115168);p.CompleteQuest(44746);p.RewardQuest(&store.quest,0,&p,true);
  Check(p.accepted&&!p.rewardPackets&&!p.casts&&!p.money&&p.m_worldquests.size()==(kind==3?1u:0u)&&CharacterDatabase.committed.empty(),"unavailable rotation mutated reward");
 }
 Reset();{Player p;p.KilledMonsterCredit(99999);Check(!p.credit&&!p.rewardPackets,"unrelated credit completes task");}
 for(int kind=0;kind<6;++kind){Reset();Player p;if(kind==0)p.accepted=false;if(kind==1)p.alive=false;if(kind==2)p.phase=false;if(kind==3)p.inRange=false;if(kind==4)p.contributor=false;Group raid{true};if(kind==5)p.group=&raid;Finish(p);Check(!p.credit&&!p.lootRolls&&!p.rewardPackets,"ineligible finale recipient");}
 Reset();{Player p;p.lostArmy=false;Finish(p);Check(p.credit==1&&!p.lootRolls,"fragment quest wrongly required for WQ completion");}
 Reset();{Player p;p.fragmentCount=1000;Finish(p);Check(p.credit==1&&p.fragmentCount==1000&&!p.lootRolls,"completed fragment objective rerolled");}
 Reset();{Player p;p.m_worldquests[44746]={44746,1000,false};Finish(p);Check(p.rewardPackets==1,"expired previous personal rotation blocked next");}
 Reset();{Player p;p.admission=EQUIP_ERR_INV_FULL;p.partial=true;
  Check(p.AutoStoreLoot(229113,LootTemplates_Spell)&&p.errors==1&&!p.fragmentCount&&!p.lootStores,"Native AutoStoreLoot result must not be mistaken for successful item storage");}
 Reset();{Player p;p.lostArmy=false;Check(!p.AutoStoreLoot(229113,LootTemplates_Spell)&&!p.errors&&!p.fragmentCount,"empty loot should return false");}
 std::cout<<"PASS: production Finish -> AutoStoreLoot -> credit -> full RewardQuest -> checkpoint save/load; both factions and all six fragment quantities, full bags/partial plans, one credit/reward/outro, native currency/money dispatch, repeat/direct/login guards, exact expiry and recipient boundaries. Item delivery, storage, loot generation, DB commit and client playback remain fixture leaves.\n";
 return 0;
#endif
}
int main(int argc,char**argv){try{Check(argc==2,"expected snapshot path");return Run(argv[1]);}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
