#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <iostream>
#include <map>
#include <list>
#include <memory>
#include <set>
#include <tuple>
#include <vector>
using uint16=uint16_t; using uint8=uint8_t; using uint32=uint32_t; using uint64=uint64_t; using int32=int32_t; using int8=int8_t;
using QuestStatus=int; using SpellCastResult=int; using SpellEffIndex=int;
enum {ALLIANCE=469,HORDE=67,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_REWARDED=6,
 QUEST_OBJECTIVE_TALKTO=3,QUEST_OBJECTIVE_GAMEOBJECT=2,QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_ITEM=1,QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS=16,QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR=64,
 QUEST_OBJECTIVE_FLAG_OPTIONAL=4,QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=2,QUEST_SPECIAL_FLAGS_TIMED=4,
 GAMEOBJECT_FIELD_FLAGS=0,GO_FLAG_IN_USE=1,GO_FLAG_NOT_SELECTABLE=16,GO_READY=1,
 SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_AURA_ENABLE_ALT_POWER=346,POWER_ALTERNATE=10,
 EFFECT_1=1,EFFECT_0=0,SPELL_EFFECT_KILL_CREDIT=90,SPELL_EFFECT_KILL_CREDIT2=134,SPELL_EFFECT_DUMMY=3,SPELL_EFFECT_ENERGIZE=30,INVALIDREASON_DONT_HAVE_REQ=1,MAX_QUEST_LOG_SIZE=50,
 UNIT_FIELD_NPC_FLAGS=11,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,UNIT_FIELD_STATE_WORLD_EFFECT_ID=12,UNIT_FIELD_STATE_SPELL_VISUAL_ID=13,UNIT_FIELD_STATE_ANIM_ID=14,UNIT_FIELD_STATE_ANIM_KIT_ID=15,UNIT_STAND_STATE_KNEEL=8,UNIT_STAND_STATE_STAND=0,UNIT_STATE_CASTING=1,QUEST_SPECIAL_FLAGS_KILL=32,QUEST_SPECIAL_FLAGS_CAST=64,CRITERIA_TYPE_KILL_CREATURE_TYPE=1,CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD=2,CRITERIA_TIMED_TYPE_CREATURE=3,CRITERIA_TYPE_KILL_CREATURE=4,LOOT_ITEM_TYPE_ITEM=0,RATE_DROP_ITEM_REFERENCED_AMOUNT=0,QUEST_OBJECTIVE_CURRENCY=4,QUEST_OBJECTIVE_MONEY=8,EQUIP_ERR_ITEM_NOT_FOUND=1};
using QuestObjectiveType=int;
using PrevQuests=std::vector<int32>; using ExclusiveQuestGroups=std::multimap<int32,uint32>;
#define ASSERT assert
#define TC_LOG_DEBUG(...) (void)0
struct ObjectGuid {uint64 value=0; static ObjectGuid const Empty; bool IsEmpty()const{return !value;}bool operator==(ObjectGuid o)const{return value==o.value;}bool operator<(ObjectGuid other)const{return value<other.value;}};
ObjectGuid const ObjectGuid::Empty{};
struct QuestObjective {int Type=0;uint32 ObjectID=0,Amount=1,Flags=0;int8 StorageIndex=0;};
struct Quest {uint32 FlagsEx=0;uint32 ItemDrop[6]{},ItemDropQuantity[6]{};uint32 id=0,special=0;int32 ExclusiveGroup=0;PrevQuests prevQuests;std::vector<QuestObjective> objectives;
 uint32 GetQuestId()const{return id;}bool IsDFQuest()const{return false;}auto const& GetObjectives()const{return objectives;}bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}
 bool IsRepeatable()const{return false;}bool IsAutoComplete()const{return false;}bool HasSpecialFlag(uint32 f)const{return special&f;}bool IsAllowedInRaid(int)const{return true;}};
struct QuestStore {std::map<uint32,Quest> quests;ExclusiveQuestGroups mExclusiveQuestGroups;
 Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}} store;
auto sQuestDataStore=&store;
struct QuestStatusData {int Status=QUEST_STATUS_NONE;uint32 Timer=0;std::shared_ptr<uint8> ScriptLifetime;};

struct Player;struct Creature;struct SmartAI;
using DamageEffectType=int;
enum {REACT_PASSIVE, REACT_AGGRESSIVE, POINT_MOTION_TYPE=8, TEMPSUMMON_TIMED_DESPAWN=1, UNIT_FIELD_FLAGS=20, UNIT_FLAG_NON_ATTACKABLE=2,QUEST_SPECIAL_FLAGS_DELIVER=1};
struct Position {float x=0,y=0,z=0,o=0;};
struct Motion {uint32 point=0;Position pos;void MovePoint(uint32 id,Position const& p,bool){point=id;pos=p;}void MovePoint(uint32 id,float x,float y,float z,bool){MovePoint(id,{x,y,z,0},false);}};
struct WorldObject {ObjectGuid guid{1};uint32 map=1220,zone=7541,area=7557,entry=0;bool world=true,alive=true;float distance=0;
 virtual Player* ToPlayer(){return nullptr;}virtual ~WorldObject()=default;uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}uint32 GetAreaId(){return area;}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}Creature* SummonCreature(uint32,Position const&,int,uint32);uint32 GetPhaseMask(){return 1;}std::set<uint32> GetPhases(){return {};}};
struct Unit:WorldObject {virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}virtual Player* GetCharmerOrOwnerPlayerOrPlayerItself()const{return nullptr;}};
struct SpellInfo {uint32 Id=180737;};
struct CreatureTemplate {uint32 StateWorldEffectID=0,SpellStateVisualID=45944,SpellStateAnimID=0,SpellStateAnimKitID=6050;};
struct Creature:Unit {SmartAI* ai=nullptr;CreatureTemplate data;std::map<int,uint32> fields;uint32 stand=8,dirty=0,talks=0;bool visible=true,combat=false;
 uint64 health=1000,maxHealth=1000;float multiplier=1;uint32 faction=16,despawnDelay=0,duration=0;bool active=false,fly=false,gravity=false;Motion motion;ObjectGuid personal;Unit* victim=nullptr;
 Creature* ToCreature()override{return this;}void AddPlayerInPersonnalVisibilityList(ObjectGuid p){personal=p;}void SetPhaseMask(uint32,bool){}void SetPhaseId(std::set<uint32> const&,bool){}void SetReactState(int){}void setActive(bool a){active=a;}void SetWalk(bool){}void SetCanFly(bool a){fly=a;}void SetDisableGravity(bool a){gravity=a;}Motion* GetMotionMaster(){return &motion;}void CombatStop(bool){combat=false;}void setFaction(uint32 f){faction=f;}uint64 GetHealth(Unit*){return health;}uint64 GetMaxHealth(Unit*){return maxHealth;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}void CastSpell(Unit*,uint32,bool){}void DespawnOrUnsummon(uint32 delay=0){despawnDelay=delay;if(!delay)world=false;}SmartAI* AI(){return ai;}CreatureTemplate const* GetCreatureTemplate(){return &data;}
 bool HasFlag(int f,uint32 v){return fields[f]&v;}void SetFlag(int f,uint32 v){fields[f]|=v;}void RemoveFlag(int f,uint32 v){fields[f]&=~v;}
 void SetUInt32Value(int f,uint32 v){fields[f]=v;}void ForceValuesUpdateAtIndex(int){++dirty;}void SetVisible(bool v){visible=v;}void SetStandState(uint32 s){stand=s;}
 void RemoveAurasDueToSpell(uint32){}bool HasUnitState(int){return false;}uint32 GetCreatureType(){return 7;}};
struct GameObject:WorldObject {uint32 flags=0,state=GO_READY;bool spawned=true;bool isSpawned(){return spawned;}bool HasFlag(int,uint32 f){return flags&f;}uint32 GetGoAnimProgress(){return 255;}void SendCustomAnim(uint32 value){assert(value==255);}};
std::map<ObjectGuid,Creature*> creatures;
std::map<ObjectGuid,Player*> players;
namespace ObjectAccessor {Creature* GetCreature(WorldObject&,ObjectGuid id){auto i=creatures.find(id);return i==creatures.end()||!i->second->world?nullptr:i->second;}Player* GetPlayer(WorldObject&,ObjectGuid);}

void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,WorldObject*,uint32 entry,float){for(auto const& pair:creatures)if(pair.second->entry==entry)out.push_back(pair.second);}
struct FakeMap {bool IsDungeon()const{return false;}int GetDifficultyID()const{return 0;}Creature* GetCreature(ObjectGuid g){return creatures.count(g)?creatures[g]:nullptr;}};
struct Group {bool isRaidGroup(){return false;}void UpdateLooterGuid(struct GameObject*,bool=true){}};
struct Guild {template<class... T>void UpdateAchievementCriteria(T...) {}};
struct GuildMgr {Guild* GetGuildById(uint32){return nullptr;}} guildMgr;auto sGuildMgr=&guildMgr;
struct Achievement {void StartTimedAchievement(int,uint32){}};
struct Player:Unit {void OpenChestForTest(struct Chest*,struct Loot*);
 ObjectGuid guid{1};uint32 team=ALLIANCE,packets=0,completions=0,power=0;bool teleport=false,
 vehicle=false,transport=false,combat=false,phase=true,visible=true,los=true,otherPower=false;
 int castResult=0;std::set<uint32> auras, m_RewardedQuests;std::map<uint32,QuestStatusData> quests;
 std::map<std::pair<uint32,uint32>,int32> data;std::vector<std::function<void()>> events;std::function<void()> castHook;
 Player* ToPlayer()override{return this;}Player* GetCharmerOrOwnerPlayerOrPlayerItself()const override{return const_cast<Player*>(this);}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}
 uint32 GetTeam(){return team;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}bool IsBeingTeleported(){return teleport;}
 bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInCombat(){return combat;}
 bool InSamePhase(WorldObject*){return phase;}bool canSeeOrDetect(WorldObject*){return visible;}bool IsWithinLOSInMap(WorldObject*){return los;}
 float GetExactDist(WorldObject* o){return o->distance;}QuestStatusData* getQuestStatus(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}
 int GetQuestStatus(uint32 q){if(m_RewardedQuests.count(q))return QUEST_STATUS_REWARDED;auto s=getQuestStatus(q);return s?s->Status:QUEST_STATUS_NONE;}
 int32 GetQuestObjectiveData(uint32 q,uint32 e)const{auto i=data.find({q,e});return i==data.end()?0:i->second;}
 int32 GetQuestObjectiveData(Quest const* q,int8 index)const{for(auto const& o:q->objectives)if(o.StorageIndex==index)return GetQuestObjectiveData(q->id,o.ObjectID);return 0;}
 void SetQuestObjectiveData(Quest const* q,QuestObjective const* o,int32 n){data[{q->id,o->ObjectID}]=n;}
 void SendQuestUpdateAddCredit(Quest const* q,ObjectGuid,QuestObjective const& o,uint32 n){assert(n>0);if(q->id==38058||q->id==38059||q->id==38360)assert(n==uint32(GetQuestObjectiveData(q->id,o.ObjectID)));++packets;}
 bool HasQuestObjectiveComplete(Quest const* q,QuestObjective const& o){return GetQuestObjectiveData(q->id,o.ObjectID)>=int32(o.Amount);}
 bool IsQuestWeekRewarded(uint32){return false;}bool IsQuestSeasonalRewarded(uint32){return false;}
 bool SatisfyQuestRace(Quest const*,bool){return true;}void SendCanTakeQuestResponse(int,Quest const*,char const*){}
 bool SatisfyQuestPreviousQuest(Quest const*,bool);bool CanCompleteQuest(uint32);bool CanTakeQuest(Quest const* q,bool b){return SatisfyQuestPreviousQuest(q,b);}
 void CompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_COMPLETE;++completions;}
 uint32 GetQuestSlotQuestId(uint8 index)const{auto i=quests.begin();while(index--&&i!=quests.end())++i;return i==quests.end()?0:i->first;}
 void QuestObjectiveSatisfy(uint32,uint32,QuestObjectiveType=QUEST_OBJECTIVE_MONSTER,ObjectGuid=ObjectGuid::Empty);
 bool HasAura(uint32 a){return auras.count(a);}bool HasAuraType(int){return otherPower||HasAura(180645);}
 void RemoveAurasDueToSpell(uint32 a){if(auras.erase(a)&&a==180645)power=0;}void SetPower(int,uint32 n){power=n;}
 int CastSpell(Unit*,uint32 a,bool){if(!castResult)auras.insert(a);return castResult;}
 void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Accept(uint32 q){quests.erase(q);quests[q].Status=QUEST_STATUS_INCOMPLETE;for(auto i=data.begin();i!=data.end();)if(i->first.first==q)i=data.erase(i);else ++i;}
 Achievement achievement;Achievement* m_achievementMgr=&achievement;Group* GetGroup()const{return nullptr;}uint32 GetGuildId(){return 0;}template<class... T>void UpdateAchievementCriteria(T...){}void AddKillCreature(uint32,uint16,bool){}void AddTrackingQuestIfNeeded(ObjectGuid){}void SetQuestStatus(uint32 q,int state){quests[q].Status=state;}void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty);FakeMap* GetMap()const{static FakeMap m;return &m;}bool CanRewardQuest(Quest const*,bool);bool IsQuestDailyRewarded(uint32){return false;}bool GetQuestRewardStatus(uint32 q){return m_RewardedQuests.count(q);}void SendEquipError(int,void*,void*,uint32){}bool HasCurrency(uint32,uint32){return false;}bool HasEnoughMoney(uint64){return false;}bool CanGetItemForLoot(void const*,bool)const{return true;}void ItemAddedQuestCheck(uint32,uint32);void ItemRemovedQuestCheck(uint32,uint32);std::map<uint32,uint32> bags,bank;uint32 GetItemCount(uint32 id,bool includeBank=false)const{auto b=bags.find(id),k=bank.find(id);return(b==bags.end()?0:b->second)+(includeBank&&k!=bank.end()?k->second:0);}bool HasQuestForItem(uint32)const;bool InBattleground()const{return false;}void AdjustQuestReqItemCount(Quest const*);void RewardForTest(Quest const*);void DestroyItemCount(uint32 id,uint32 count,bool){assert(bags[id]>=count);ItemRemovedQuestCheck(id,count);bags[id]-=count;}uint32 visibilityRefresh=0;std::set<uint32> phases;bool NeedPhaseRecalculate=false,NeedPhaseUpdate=false;uint32 GetCurrentZoneID(){return 7541;}auto const& GetPhases(){return phases;}void SetPhaseId(std::set<uint32> ids,bool refresh){phases=ids;if(refresh)++visibilityRefresh;}struct PhaseMgr& GetPhaseMgr();void IncompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_INCOMPLETE;}void UpdateForQuestWorldObjects(){}
};
#include "QuestDelayedEvent.inc"
#include "CanCompleteQuest.inc"
#include "PreviousQuest.inc"
#include "QuestObjectiveSatisfy.inc"
#include "KilledMonsterCredit.inc"

#include "CanRewardQuest.inc"
#include "ItemAddedQuestCheck.inc"
#include "ItemRemovedQuestCheck.inc"
namespace ObjectAccessor {Player* GetPlayer(WorldObject&,ObjectGuid id){auto i=players.find(id);return i==players.end()||!i->second->world?nullptr:i->second;}}

using Map=FakeMap;enum {QUEST_ITEM_COUNT=6,QUEST_FLAGS_EX_KEEP_ADDITIONAL_ITEMS=1,GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_FISHINGHOLE=25,GAMEOBJECT_TYPE_GATHERING_NODE=50,GAMEOBJECT_TYPE_CHALLENGE_MODE_REWARD=51};
struct ItemTemplate {int32 GetMaxCount()const{return 1;}uint32 GetMaxStackSize()const{return 1;}uint32 GetInventoryType()const{return 0;}};
struct ObjectMgr {ItemTemplate const* GetItemTemplate(uint32){static ItemTemplate p;return &p;}} objectMgr;
auto sObjectMgr=&objectMgr;
struct World {float getRate(int){return 1;}} world;auto sWorld=&world;
struct LootStoreItem {uint32 itemid=0,lootmode=0,ClassificationMask=0,type=0,reference=0,maxcount=1,groupid=0;bool allowRoll=true;
 bool Roll(bool,bool=false,bool=false)const{return allowRoll;}};
using LootStoreItemList=std::vector<LootStoreItem>;
struct LootItem {struct {uint32 ItemID=0;} item;};using LootItemList=std::vector<LootItem>;
struct Loot {uint32 objType=0;void clear(){items.clear();}void FillLoot(uint32,struct LootStore&,Player*,bool,bool,struct GameObject*);Player* owner=nullptr;uint32 _DifficultyMask=1,_ClassificationMask=1;bool _specCheck=false;LootItemList items;
 Player const* GetLootOwner(){return owner;}void AddItem(LootStoreItem const& i){items.push_back({{i.itemid}});}};
struct LootTemplate;
struct LootGroup {void Process(Loot&,LootTemplate const*)const{}void ProcessAutoGroup(Loot&,LootTemplate const*)const{}};
using LootGroups=std::vector<LootGroup>;
struct LootTemplate {LootStoreItemList Entries;LootGroups Groups,AutoGroups;bool _isZoneLoot=false,condition=true;
 bool CheckItemCondition(Player const*,uint32,uint32)const{return condition;}void Process(Loot&,bool,uint8=0)const;};
struct References {LootTemplate const* GetLootFor(uint32)const{return nullptr;}} LootTemplates_Reference;
#include "LootProcess.inc"


#include "HasQuestForItem.inc"
#include "AdjustQuestReqItemCount.inc"
struct GameObjectTemplate {uint32 type=GAMEOBJECT_TYPE_CHEST;struct {uint32 open=1691,chestLoot=0,chestRestockTime=1,consumable=0,usegrouplootrules=0;} chest;struct {uint32 chestLoot=0;} fishingHole,gatheringNode,challengeModeReward;uint32 GetLootId()const;};
#include "GetLootId.inc"
// The fixture models DB-backed chest templates and regular (non-personal) loot
// services. These IDs/data come from the SQL audit; no custom quest script runs.
struct Chest:GameObject {GameObjectTemplate info;GameObjectTemplate const* GetGOInfo(){return &info;}};
struct LootStore {std::map<uint32,LootTemplate> rows;} LootTemplates_Gameobject;
void Loot::FillLoot(uint32 id,LootStore& store,Player* player,bool,bool,GameObject*){owner=player;auto i=store.rows.find(id);if(i!=store.rows.end())i->second.Process(*this,false);}
void Player::OpenChestForTest(Chest* go,Loot* loot){uint32 lootid=go->GetGOInfo()->GetLootId();if(lootid){
#include "ChestFill.inc"
}}
struct NativeOpen {Player* player;Chest* go;Loot storage;void Execute(){player->OpenChestForTest(go,&storage);}};
enum {CONDITION_QUESTREWARDED=8,CONDITION_QUESTTAKEN=9,CONDITION_QUEST_COMPLETE=28,CONDITION_QUEST_NONE=14,CONDITION_SCENARION_STEP=40,PHASE_UPDATE_FLAG_CLIENTSIDE_CHANGED=1,PHASE_UPDATE_FLAG_SERVERSIDE_CHANGED=2};
struct Condition {uint32 ConditionType=8,ConditionValue1=39593,ConditionValue2=0;bool Meet(WorldObject* object){bool condMeets=false;switch(ConditionType){
#include "RewardCondition.inc"
}return condMeets;}};
using ConditionList=std::vector<Condition*>;
struct Conditions {Condition restored,night;Conditions(){night.ConditionValue1=39592;}ConditionList GetConditionsForPhaseDefinition(uint32,uint32 entry){return{entry==650?&restored:&night};}bool IsObjectMeetToConditions(Player* p,ConditionList const& c){for(auto v:c)if(!v->Meet(p))return false;return true;}} conditions;
auto sConditionMgr=&conditions;
struct PhaseDefinition {uint32 zoneId=7541,entry=650,phasemask=0,terrainswapmap=0,wmAreaId=0;std::set<uint32> phaseId{6665};bool IsLastDefinition()const{return false;}};
using PhaseDefinitionContainer=std::vector<PhaseDefinition>;using PhaseDefinitionStore=std::map<uint32,PhaseDefinitionContainer>;
struct PhaseInfo {uint32 phaseId=0;};using PhaseInfoContainer=std::map<uint32,PhaseInfo>;
struct PhaseUpdateData {uint64 _conditionTypeFlags=0;uint32 _questId=0;void AddConditionType(uint32 type){_conditionTypeFlags|=uint64(1)<<type;}void AddQuestUpdate(uint32 const);bool IsConditionRelated(Condition const*)const;};
#include "QuestPhaseData.inc"
#include "PhaseRelated.inc"
struct PhaseData {uint32 sent=0;std::list<PhaseDefinition const*> activePhaseDefinitions;PhaseInfoContainer spellPhaseInfo;bool HasActiveDefinitions(){return !activePhaseDefinitions.empty();}void ResetDefinitions(){activePhaseDefinitions.clear();}void AddPhaseDefinition(PhaseDefinition const* d){activePhaseDefinitions.push_back(d);}void SendPhaseshiftToPlayer(){++sent;}void SendPhaseMaskToPlayer(){}};
struct PhaseMgr {Player* player;PhaseData phaseData;PhaseDefinitionStore* _PhaseDefinitionStore;std::mutex _updateLock;uint32 _UpdateFlags=0;bool IsUpdateInProgress(){return false;}bool CheckDefinition(PhaseDefinition const*);bool NeedsPhaseUpdateWithData(PhaseUpdateData const)const;void NotifyConditionChanged(PhaseUpdateData const&);void Recalculate();void Update();};
#include "PhaseCheck.inc"
#include "PhaseNeeds.inc"
#include "PhaseNotify.inc"
#include "PhaseRecalculate.inc"
#include "PhaseUpdate.inc"
PhaseMgr* activePhase=nullptr;PhaseMgr& Player::GetPhaseMgr(){return *activePhase;}
void Player::RewardForTest(Quest const* quest){assert(CanRewardQuest(quest,false));for(auto const& obj:quest->GetObjectives()){switch(obj.Type){
#include "RewardItemCase.inc"
default:break;}}uint32 quest_id=quest->id;m_RewardedQuests.insert(quest_id);quests.erase(quest_id);
#include "RewardPhaseNotify.inc"
}
void Flush(Player& p){auto events=std::move(p.events);p.events.clear();for(auto& f:events)f();}
int main(){
 uint32 const objects[]={243819,243820,243822,243823},items[]={128493,128495,128496,128497};
 for(uint32 id:{38331u,39590u,39592u,39593u,39594u})store.quests[id].id=id;
 auto& q=store.quests[39593];q.special=QUEST_SPECIAL_FLAGS_DELIVER;q.prevQuests={39590,39592};q.objectives={{1,128493,1,0,3},{1,128495,1,0,1},{1,128496,1,0,2},{1,128497,1,0,4}};store.quests[39594].prevQuests={39593};
 for(uint32 i=0;i<4;++i)LootTemplates_Gameobject.rows[objects[i]].Entries={{items[i],1}};
 for(uint32 team:{ALLIANCE,HORDE})for(uint32 predecessor:{39590u,39592u}){
  Player p;p.team=team;p.Accept(39593);assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(predecessor);assert(p.CanTakeQuest(&q,false));
  p.KilledMonsterCredit(96257);p.KilledMonsterCredit(243819);assert(!p.CanCompleteQuest(39593));PhaseDefinitionStore definitions;definitions[7541]={{7541,650,0,0,0,{6665}},{7541,16,0,0,0,{6754}}};PhaseMgr phase{&p,{},&definitions};activePhase=&phase;phase.Recalculate();assert(!p.phases.count(6665));
  for(uint32 i=0;i<4;++i){Chest go;go.entry=objects[i];NativeOpen old{&p,&go};old.Execute();assert(old.storage.items.empty());go.info.chest.chestLoot=objects[i];NativeOpen repaired{&p,&go};repaired.Execute();assert(repaired.storage.items.size()==1&&repaired.storage.items[0].item.ItemID==items[i]);assert(p.HasQuestForItem(items[i]));assert(!p.CanRewardQuest(&q,false));p.bags[items[i]]=1;p.ItemAddedQuestCheck(items[i],1);assert(!p.HasQuestForItem(items[i]));assert(p.completions==(i==3?1:0));}
  assert(p.CanRewardQuest(&q,false)&&p.GetQuestStatus(39593)==QUEST_STATUS_COMPLETE);assert(!p.CanTakeQuest(&store.quests[39594],false));phase.Recalculate();assert(!p.phases.count(6665));
  p.ItemAddedQuestCheck(items[0],1);assert(p.completions==1);p.ItemRemovedQuestCheck(items[0],1);p.bags[items[0]]=0;assert(p.GetQuestStatus(39593)==QUEST_STATUS_INCOMPLETE&&p.HasQuestForItem(items[0]));p.bags[items[0]]=1;p.ItemAddedQuestCheck(items[0],1);assert(p.CanRewardQuest(&q,false));
  // Bank-inclusive acceptance progress cannot satisfy a physical hand-in.
  p.bags[items[1]]=0;p.bank[items[1]]=1;p.AdjustQuestReqItemCount(&q);assert(!p.CanRewardQuest(&q,false));p.bank[items[1]]=0;p.bags[items[1]]=1;
  p.RewardForTest(&q);for(auto item:items)assert(!p.bags[item]);assert(p.GetQuestStatus(39593)==QUEST_STATUS_REWARDED&&p.CanTakeQuest(&store.quests[39594],false));Flush(p);assert(p.NeedPhaseRecalculate&&p.NeedPhaseUpdate);phase.Recalculate();phase.Update();assert(p.phases.count(6665)&&p.visibilityRefresh&&phase.phaseData.sent);assert(p.phases.count(6754)==(predecessor==39592));
 }
 {Player p;p.Accept(39593);p.bags[128493]=1;p.AdjustQuestReqItemCount(&q);assert((p.data[{39593,128493}]==1));p.quests.erase(39593);assert(!p.HasQuestForItem(128495));p.Accept(39593);p.AdjustQuestReqItemCount(&q);assert((p.data[{39593,128493}]==1&&p.data[{39593,128495}]==0));p.ItemAddedQuestCheck(77777,1);assert(!p.CanCompleteQuest(39593));}
 {Player a,b;a.Accept(39593);b.Accept(39593);a.bags[128493]=1;a.ItemAddedQuestCheck(128493,1);assert(!a.HasQuestForItem(128493)&&b.HasQuestForItem(128493));}
 std::cout<<"PASS: original chest GetLootId/regular open branch and loot processing reproduce empty Data1=0 versus all four fragments; real item eligibility/add/remove/reaccept/bank/reward handlers and original slots; native reward phase notifier/condition/recalculate/client-update dispatch exposes restored Yotnar and next quest for both factions/predecessor alternatives, preserving Nighteyes.\n";
}
