#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
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
 UNIT_FIELD_NPC_FLAGS=11,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,UNIT_FIELD_STATE_WORLD_EFFECT_ID=12,UNIT_FIELD_STATE_SPELL_VISUAL_ID=13,UNIT_FIELD_STATE_ANIM_ID=14,UNIT_FIELD_STATE_ANIM_KIT_ID=15,UNIT_STAND_STATE_KNEEL=8,UNIT_STAND_STATE_STAND=0,UNIT_STATE_CASTING=1,QUEST_SPECIAL_FLAGS_KILL=32,QUEST_SPECIAL_FLAGS_CAST=64,CRITERIA_TYPE_KILL_CREATURE_TYPE=1,CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD=2,CRITERIA_TIMED_TYPE_CREATURE=3,CRITERIA_TYPE_KILL_CREATURE=4,LOOT_ITEM_TYPE_ITEM=0,QUEST_SPECIAL_FLAGS_DELIVER=1,RATE_DROP_ITEM_REFERENCED_AMOUNT=0,QUEST_OBJECTIVE_CURRENCY=4,QUEST_OBJECTIVE_MONEY=8,EQUIP_ERR_ITEM_NOT_FOUND=1,TARGET_UNIT_DEST_AREA_ENTRY=8};
enum {UNIT_NPC_FLAG_GOSSIP=1,UNIT_NPC_FLAG_QUESTGIVER=2,SPELL_AURA_MOD_ROOT=26,AURA_EFFECT_HANDLE_REAL=1,AURA_REMOVE_BY_DEFAULT=0,AURA_REMOVE_BY_EXPIRE=1,AURA_REMOVE_BY_ENEMY_SPELL=2};using AuraEffectHandleModes=int;struct AuraEffect{};
enum {QUEST_OBJECTIVE_COMPLETE_CRITERIA_TREE=14};using DamageEffectType=int; using QuestObjectiveType=int;
enum {UNIT_FIELD_FLAGS=20,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,REACT_PASSIVE=0,REACT_AGGRESSIVE=2};
using PrevQuests=std::vector<int32>; using ExclusiveQuestGroups=std::multimap<int32,uint32>;
#define ASSERT assert
#define TC_LOG_DEBUG(...) (void)0
enum class HighGuid {Player,Creature,Pet,Vehicle,GameObject};
struct ObjectGuid {ObjectGuid(uint64 n=0):value(n){}HighGuid high=HighGuid::Creature;HighGuid GetHigh()const{return high;}using LowType=uint64;uint64 value=0; static ObjectGuid const Empty; bool IsEmpty()const{return !value;}bool operator==(ObjectGuid o)const{return value==o.value;}bool operator<(ObjectGuid other)const{return value<other.value;}};
ObjectGuid const ObjectGuid::Empty{};
struct QuestObjective {int Type=0;uint32 ObjectID=0,Amount=1,Flags=0;int8 StorageIndex=0;};
enum {QUEST_FLAGS_AUTOCOMPLETE=1,INVALIDREASON_QUEST_FAILED_WRONG_RACE=8};
struct Quest {uint64 AllowableRaces=uint64(-1);uint32 NextQuestIdChain=0;uint32 flags=0;bool HasFlag(uint32 f)const{return flags&f;}uint32 id=0,special=0;int32 ExclusiveGroup=0;PrevQuests prevQuests;std::vector<QuestObjective> objectives;
 uint32 GetQuestId()const{return id;}bool IsDFQuest()const{return false;}auto const& GetObjectives()const{return objectives;}bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}
 bool IsRepeatable()const{return false;}bool IsAutoComplete()const{return false;}bool HasSpecialFlag(uint32 f)const{return special&f;}bool IsAllowedInRaid(int)const{return true;}};
using QuestRelations=std::multimap<uint32,uint32>;using QuestRelationBounds=std::pair<QuestRelations::const_iterator,QuestRelations::const_iterator>;
struct QuestStore {QuestRelations starters;QuestRelationBounds GetCreatureQuestRelationBounds(uint32 id){return starters.equal_range(id);}QuestRelationBounds GetGOQuestRelationBounds(uint32 id){return starters.equal_range(id);}std::map<uint32,Quest> quests;ExclusiveQuestGroups mExclusiveQuestGroups;
 Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}} store;
auto sQuestDataStore=&store; enum {TEMPSUMMON_TIMED_DESPAWN=1,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN=2,TEMPSUMMON_MANUAL_DESPAWN=8};
struct QuestObjectScope {bool cancelled=false;void Cancel(){cancelled=true;}};
struct QuestStatusData {std::shared_ptr<QuestObjectScope> ScriptObjects;int Status=QUEST_STATUS_NONE;uint32 Timer=0;std::shared_ptr<uint8> ScriptLifetime;};

struct Aura {int mode=0;int GetRemoveMode(){return mode;}};
struct Unit;struct Player;struct Creature;struct GameObject;struct SmartAI;
struct Vehicle {struct Seat {struct {ObjectGuid Guid;} Passenger;};std::map<int,Seat> Seats;struct Unit* GetPassenger(int);};
struct WorldObject {ObjectGuid guid{1};std::weak_ptr<QuestObjectScope> questScope;bool bound=false,expired=false; bool HasQuestObjectScope(){return bound;} bool IsQuestObjectExpired(){auto scope=questScope.lock();return expired||(bound&&(!scope||scope->cancelled)); } uint32 map=1220,zone=7541,area=7557,entry=0;bool world=true,alive=true;float distance=0;
 virtual ~WorldObject()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}virtual Unit* ToUnit(){return nullptr;}virtual bool IsCreature(){return false;}float GetPositionX(){return 3442.69f;}float GetPositionY(){return 2103.1f;}float GetPositionZ(){return 277.243f;}float GetOrientation(){return 1.57336f;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}uint32 GetAreaId(){return area;}virtual ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}};
struct Unit:WorldObject {std::map<std::pair<uint32,uint64>,Aura> roots;Aura* GetAura(uint32 id,ObjectGuid caster){auto i=roots.find({id,caster.value});return i==roots.end()?nullptr:&i->second;}void RemoveAurasDueToSpell(uint32 id,ObjectGuid caster){roots.erase({id,caster.value});}std::function<int(Unit*,uint32)> castTargetHook; Player* ownerPlayer=nullptr;Player* GetCharmerOrOwnerPlayerOrPlayerItself() const {return const_cast<Unit*>(this)->ToPlayer()?const_cast<Unit*>(this)->ToPlayer():ownerPlayer;}int castResult=0; std::vector<uint32> casts; std::function<void(uint32)> castHook; int CastSpell(Unit* target,uint32 id,bool){casts.push_back(id);if(castTargetHook)return castTargetHook(target,id);if(castHook)castHook(id);return castResult;} Creature* SummonCreature(uint32,float,float,float,float,int,uint32); GameObject* SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32);Vehicle* kit=nullptr;virtual int GetTypeId(){return 4;}Unit* ToUnit()override{return this;}Vehicle* GetVehicleKit(){return kit;}bool InSamePhase(WorldObject*){return true;}virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct SpellInfo {uint32 Id=180737;};
struct CreatureTemplate {uint32 StateWorldEffectID=0,SpellStateVisualID=45944,SpellStateAnimID=0,SpellStateAnimKitID=6050;};
struct HeightMap {float value=229.427f;float GetHeight(float,float,float){return value;}} heightMap;
struct Creature:Unit {uint64 dbGuid=0;uint64 GetDBTableGUIDLow(){return dbGuid;}void SetHealth(uint64 n){health=n;}HeightMap* GetMap(){return &heightMap;} SmartAI* ai=nullptr;SmartAI*& i_AI=ai;SmartAI* i_disabledAI=nullptr;bool charmed=false;bool IsPlayer(){return false;}bool isCharmed(){return charmed;}bool isPossessed(){return false;}bool IsVehicle(){return kit!=nullptr;}void UpdateCharmAI();std::vector<std::function<void()>> callbacks;void AddDelayedEvent(uint64,std::function<void()> f){callbacks.push_back(std::move(f));}void Tick(){auto q=std::move(callbacks);callbacks.clear();for(auto& f:q)f();}CreatureTemplate data;std::map<int,uint32> fields;uint32 stand=8,dirty=0,talks=0;bool visible=true,combat=false;
 Creature* ToCreature()override{return this;}bool IsCreature()override{return true;}Player* GetLootRecipient(){return nullptr;}void AddPlayerInPersonnalVisibilityList(ObjectGuid p){personal=p;}ObjectGuid personal;void setActive(bool){}void DespawnOrUnsummon(){world=false;} uint64 health=1000,maximum=1000;float multiplier=1;Unit* victim=nullptr;uint32 react=0;void SetReactState(uint32 r){react=r;}void CombatStop(bool){combat=false;}uint64 GetHealth(Unit*){return health;}uint64 GetMaxHealth(Unit*){return maximum;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}SmartAI* AI(){return ai;}CreatureTemplate const* GetCreatureTemplate(){return &data;}
 bool HasFlag(int f,uint32 v){return fields[f]&v;}void SetFlag(int f,uint32 v){fields[f]|=v;}void RemoveFlag(int f,uint32 v){fields[f]&=~v;}
 void SetUInt32Value(int f,uint32 v){fields[f]=v;}void ForceValuesUpdateAtIndex(int){++dirty;}void SetVisible(bool v){visible=v;}void SetStandState(uint32 s){stand=s;}
 using Unit::RemoveAurasDueToSpell;std::set<ObjectGuid> hidden;void AddToHideList(ObjectGuid g){hidden.insert(g);}void RemoveFromHideList(ObjectGuid g){hidden.erase(g);}void DestroyForPlayer(Player*){} bool HasUnitState(int){return false;}uint32 GetCreatureType(){return 7;}};
struct GameObject:WorldObject {std::set<ObjectGuid> hidden;void AddToHideList(ObjectGuid id){hidden.insert(id);}void RemoveFromHideList(ObjectGuid id){hidden.erase(id);}void DestroyForPlayer(Player*){}uint32 flags=0,state=GO_READY;bool spawned=true;bool isSpawned(){return spawned;}bool HasFlag(int,uint32 f){return flags&f;}};
std::map<ObjectGuid,Creature*> creatures;
std::map<ObjectGuid,Player*> players;std::map<ObjectGuid,GameObject*> objects;
namespace ObjectAccessor {Player* GetPlayer(WorldObject&,ObjectGuid);Player* FindPlayer(ObjectGuid,bool=true);GameObject* GetGameObject(WorldObject&,ObjectGuid);Creature* GetCreature(WorldObject&,ObjectGuid id){auto i=creatures.find(id);return i==creatures.end()||!i->second->world?nullptr:i->second;}}
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,WorldObject*,uint32 entry,float){for(auto const& pair:creatures)if(pair.second->entry==entry)out.push_back(pair.second);}
struct FakeMap {GameObject* GetGameObject(ObjectGuid g){auto i=objects.find(g);return i==objects.end()?nullptr:i->second;} bool IsDungeon(){return false;}int GetDifficultyID(){return 0;}Creature* GetCreature(ObjectGuid g){return creatures.count(g)?creatures[g]:nullptr;}};
struct GroupReference {Player* player=nullptr;GroupReference* following=nullptr;Player* getSource(){return player;}GroupReference* next(){return following;}};
struct Group {GroupReference* first=nullptr;GroupReference* GetFirstMember(){return first;}bool isRaidGroup(){return false;}};
struct Guild {template<class... T>void UpdateAchievementCriteria(T...) {}};
struct GuildMgr {Guild* GetGuildById(uint32){return nullptr;}} guildMgr;auto sGuildMgr=&guildMgr;
struct Achievement {void StartTimedAchievement(int,uint32){}};
struct GossipMenu {uint32 menu=18672;bool item=true;uint32 GetMenuId(){return menu;}void* GetItem(uint32 id){return id==0&&item?this:nullptr;}};
struct PlayerMenu {GossipMenu menu;uint32 closed=0;GossipMenu& GetGossipMenu(){return menu;}void ClearMenus(){menu.item=false;}void SendCloseGossip(){++closed;}};
enum {INVENTORY_SLOT_BAG_0=0,INVENTORY_SLOT_ITEM_START=23,INVENTORY_SLOT_BAG_START=19,INVENTORY_SLOT_BAG_END=23,EQUIPMENT_SLOT_START=0,CHILD_EQUIPMENT_SLOT_START=80,CHILD_EQUIPMENT_SLOT_END=90,BANK_SLOT_ITEM_START=39,BANK_SLOT_BAG_START=67,BANK_SLOT_BAG_END=74,REAGENT_SLOT_START=91,REAGENT_SLOT_END=100,QUEST_DEFAULT_SAVE_TYPE=1,CRITERIA_TYPE_COMPLETE_QUEST=27,QUEST_FLAGS_TRACKING=1024,TYPEID_PLAYER=4,SPELL_EFFECT_HANDLE_HIT_TARGET=1};
struct Item {uint32 entry=138111;bool world=false;virtual ~Item()=default;uint32 GetEntry(){return entry;}};
struct Bag:Item {std::vector<Item*> items;uint32 GetBagSize(){return uint32(items.size());}Item* GetItemByPos(uint32 n){return items[n];}};
struct Player:Unit {std::map<uint32,int> m_RewardedQuestsSave;void SetRewardedQuest(uint32);uint16 FindQuestSlot(uint32 q){return ActiveSlot(q)?0:MAX_QUEST_LOG_SIZE;}bool ActiveSlot(uint32 q){return GetQuestStatus(q)==QUEST_STATUS_INCOMPLETE||GetQuestStatus(q)==QUEST_STATUS_COMPLETE;}void AreaExploredOrEventHappens(uint32){}std::map<int,Item*> slots;std::map<int,Bag*> bagslots;std::vector<std::unique_ptr<Item>> granted;bool full=false,suppressEffect=false;uint32 grants=0,criteriaEvents=0;Item* GetItemByEntry(uint32,bool=false)const;Item* GetItemByPos(int,int slot)const{auto i=slots.find(slot);return i==slots.end()?nullptr:i->second;}Bag* GetBagByPos(int slot)const{auto i=bagslots.find(slot);return i==bagslots.end()?nullptr:i->second;}int GetInventoryEndSlot()const{return 39;}bool AddItem(uint32 id,uint32 count){if(full)return false;assert(count==1);auto item=std::make_unique<Item>();item->entry=id;slots[23]=item.get();granted.push_back(std::move(item));++grants;return true;}void UpdateAchievementCriteria(int type,uint32 asset);void AchieveCriteriaCredit(uint32);uint64 race=1;uint64 getRaceMask(){return race;}uint32 GetInstanceId(){return 0;}Quest const* GetNextQuest(ObjectGuid,Quest const*);PlayerMenu menu;PlayerMenu* PlayerTalkClass=&menu;using Unit::GetAura;using Unit::RemoveAurasDueToSpell;
 ObjectGuid guid{1};uint32 team=ALLIANCE,packets=0,completions=0,power=0;bool teleport=false,
 vehicle=false,transport=false,combat=false,phase=true,visible=true,los=true,otherPower=false;
 int castResult=0;std::set<uint32> auras, m_RewardedQuests;std::map<uint32,QuestStatusData> quests;
 std::map<std::pair<uint32,uint32>,int32> data;std::vector<std::function<void()>> events;std::function<void()> castHook;
 Player* ToPlayer()override{return this;} std::function<Creature*(uint32)> summonHook; std::vector<WorldObject*> boundObjects; uint32 cancels=0; using Unit::CastSpell; using Unit::SummonCreature; void UpdateVisibilityForPlayer(){} void KillCreditGO(uint32,ObjectGuid const&); void BindQuestObject(uint32 q,WorldObject* o){auto& scope=quests[q].ScriptObjects;if(!scope)scope=std::make_shared<QuestObjectScope>();o->questScope=scope;o->bound=true;boundObjects.push_back(o);} void CancelQuestObjects(uint32 q){++cancels;quests[q].ScriptObjects.reset();for(auto o:boundObjects)o->expired=true;boundObjects.clear();}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}
 uint32 GetTeam(){return team;}float GetPositionX(){return 3442.69f;}float GetPositionY(){return 2103.1f;}float GetPositionZ(){return 277.243f;}float GetOrientation(){return 1.57336f;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}bool IsBeingTeleported(){return teleport;}
 bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInCombat(){return combat;}
 Unit* vehicleBase=nullptr;Unit* GetVehicleBase(){return vehicleBase;}void ExitVehicle(){if(vehicleBase&&vehicleBase->kit)vehicleBase->kit->Seats.clear();vehicleBase=nullptr;vehicle=false;}bool InSamePhase(WorldObject*){return phase;}bool canSeeOrDetect(WorldObject*){return visible;}bool IsWithinLOSInMap(WorldObject*){return los;}
 bool IsWithinDistInMap(WorldObject* o,float range){return o->distance<=range;} float GetExactDist(WorldObject* o){return o->distance;}QuestStatusData* getQuestStatus(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}
 int GetQuestStatus(uint32 q){auto s=getQuestStatus(q);return s?s->Status:QUEST_STATUS_NONE;}
 int32 GetQuestObjectiveData(uint32 q,uint32 e){return data[{q,e}];}
 int32 GetQuestObjectiveData(Quest const* q,int8 index){for(auto const& o:q->objectives)if(o.StorageIndex==index)return GetQuestObjectiveData(q->id,o.ObjectID);return 0;}
 void SetQuestObjectiveData(Quest const* q,QuestObjective const* o,int32 n){data[{q->id,o->ObjectID}]=n;}
 void SendQuestUpdateAddCredit(Quest const* q,ObjectGuid,QuestObjective const& o,uint32 n){assert(n>0);if(q->id==38058||q->id==38059||q->id==38360)assert(n==uint32(GetQuestObjectiveData(q->id,o.ObjectID)));++packets;}
 bool HasQuestObjectiveComplete(Quest const* q,QuestObjective const& o){return GetQuestObjectiveData(q->id,o.ObjectID)>=int32(o.Amount);}
 bool IsQuestWeekRewarded(uint32){return false;}bool IsQuestSeasonalRewarded(uint32){return false;}
 bool SatisfyQuestRace(Quest const*,bool);void SendCanTakeQuestResponse(int,Quest const*,char const* = nullptr){}
 bool SatisfyQuestPreviousQuest(Quest const*,bool);bool CanCompleteQuest(uint32);bool CanTakeQuest(Quest const* q,bool b){return SatisfyQuestRace(q,b)&&SatisfyQuestPreviousQuest(q,b);}
 bool CanRewardQuest(Quest const*,bool);bool IsQuestDailyRewarded(uint32){return false;}bool GetQuestRewardStatus(uint32 q){return m_RewardedQuests.count(q);}std::map<uint32,uint32> bags; uint32 GetItemCount(uint32 id){return bags[id];}void SendEquipError(int,void*,void*,uint32){}bool HasCurrency(uint32,uint32){return false;}bool HasEnoughMoney(uint64){return false;}void CompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_COMPLETE;++completions;}
 uint32 GetQuestSlotQuestId(uint8 index){auto i=quests.begin();while(index--&&i!=quests.end())++i;return i==quests.end()?0:i->first;}
 void QuestObjectiveSatisfy(uint32,uint32,QuestObjectiveType=QUEST_OBJECTIVE_MONSTER,ObjectGuid=ObjectGuid::Empty);
 bool HasAura(uint32 a){return auras.count(a);}bool HasAuraType(int){return otherPower||HasAura(180645);}
 void RemoveAurasDueToSpell(uint32 a){if(auras.erase(a)&&a==180645)power=0;}void SetPower(int,uint32 n){power=n;}
 int CastSpell(Player*,uint32,bool);
 void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Accept(uint32 q){quests.erase(q);quests[q].Status=QUEST_STATUS_INCOMPLETE;for(auto i=data.begin();i!=data.end();)if(i->first.first==q)i=data.erase(i);else ++i;}
 Achievement achievement;Achievement* m_achievementMgr=&achievement;Group* group=nullptr;Group* GetGroup(){return group;}void RewardPlayerAndGroupAtEvent(uint32,WorldObject*);bool TalkedToCreature(uint32,ObjectGuid){return false;}bool rewardRange=true;bool IsAtGroupRewardDistance(WorldObject*){return rewardRange;}bool corpse=false;bool GetCorpse(){return corpse;}uint32 GetGuildId(){return 0;}template<class A,class B,class C,class... T>void UpdateAchievementCriteria(A,B,C,T...){}void AddKillCreature(uint32,uint16,bool){}void AddTrackingQuestIfNeeded(ObjectGuid){}void SetQuestStatus(uint32 q,int state){quests[q].Status=state;}void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty);FakeMap* GetMap()const{static FakeMap m;return &m;}bool CanGetItemForLoot(void const*,bool)const{return true;}
};
#include "QuestDelayedEvent.inc"
#include "CanCompleteQuest.inc"
#include "PreviousQuest.inc"
#include "QuestObjectiveSatisfy.inc"
#include "KilledMonsterCredit.inc"

#include "CanRewardQuest.inc"
#include "GroupCredit.inc"
#include "KillCreditGO.inc"
namespace ObjectAccessor {
Player* GetPlayer(WorldObject& source,ObjectGuid id){auto i=players.find(id);return i==players.end()||!i->second->world||i->second->map!=source.map?nullptr:i->second;}
Player* FindPlayer(ObjectGuid id,bool world){auto i=players.find(id);return i==players.end()||(world&&!i->second->world)?nullptr:i->second;}
GameObject* GetGameObject(WorldObject&,ObjectGuid id){auto i=objects.find(id);return i==objects.end()||!i->second->world?nullptr:i->second;}}
Unit* Vehicle::GetPassenger(int index){auto i=Seats.find(index);return i==Seats.end()?nullptr:ObjectAccessor::FindPlayer(i->second.Passenger.Guid);}
struct SmartAI {Creature* me;uint32 delegated=0;explicit SmartAI(Creature* c):me(c){c->ai=this;}virtual ~SmartAI()=default;
 virtual void sQuestAccept(Player*,Quest const*){++delegated;}virtual void sGossipSelect(Player*,uint32,uint32){++delegated;} virtual void JustSummoned(Creature*){++delegated;} virtual void JustDied(Unit*){++delegated;} void Talk(uint8,ObjectGuid){++me->talks;} virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual ObjectGuid GetGUID(int32){return {};}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){++delegated;}
 virtual void PassengerBoarded(Unit*,int8,bool){++delegated;}virtual void IsSummonedBy(Unit*){++delegated;}virtual void UpdateAI(uint32){++delegated;}
 virtual void SpellHit(Unit*,SpellInfo const*){++delegated;} virtual bool CanAIAttack(Unit const*)const{return true;}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){++delegated;}virtual void EnterEvadeMode(){++delegated;}bool UpdateVictim(){return me->combat;}void AttackStart(Unit* p){me->victim=p;me->combat=true;}void DoMeleeAttackIfReady(){}
};
struct Hook {std::vector<int> values;void operator+=(int i){values.push_back(i);}};
struct SpellScript {uint32 m_scriptSpellId=219924;Unit* caster=nullptr;Unit* original=nullptr;Hook OnObjectAreaTargetSelect;Unit* GetCaster(){return caster;}Unit* GetOriginalCaster(){return original?original:caster;}virtual void Register(){}};
#define PrepareSpellScript(x) public:
#define SpellObjectAreaTargetSelectFn(F,I,T) (1000+(I)*100+(T))
struct GameObjectAI {virtual ~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual bool GossipUse(Player*){return false;}};
struct SmartGameObjectAI:GameObjectAI {uint32 delegated=0;explicit SmartGameObjectAI(GameObject*){}bool GossipHello(Player*,bool)override{++delegated;return false;}bool GossipUse(Player*)override{++delegated;return false;}};
struct GameObjectScript {explicit GameObjectScript(char const* = nullptr){}virtual bool OnGossipHello(Player*,GameObject*){return false;}virtual GameObjectAI* GetAI(GameObject*)const{return nullptr;}};






#include "Race.inc"
#include "CriteriaCredit.inc"
#include "ItemEntry.inc"
#include "SetRewarded.inc"
using CreatureAI=SmartAI;
struct CreatureScript {explicit CreatureScript(char const*){}virtual bool OnGossipHello(Player*,Creature*){return false;}virtual CreatureAI* GetAI(Creature*)const{return nullptr;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}};
struct Effect {uint32 MiscValue=39775;};struct Info {Effect effect;Effect* GetEffect(int,int){return &effect;}};
struct Spell {int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;Unit* unitTarget=nullptr;Info info;Info* m_spellInfo=&info;void EffectQuestComplete(SpellEffIndex);};
#include "QuestCompleteEffect.inc"
int Player::CastSpell(Player*,uint32 id,bool){assert(id==191831);if(!castResult&&!suppressEffect){Spell spell;spell.unitTarget=this;spell.EffectQuestComplete(0);}return castResult;}
struct CriteriaEntry {uint32 Asset=39775;};struct CriteriaTree {};
struct AchievementCriteriaDataSet {bool Meets(void*)const{return true;}};
struct AchievementMgr {AchievementCriteriaDataSet const* GetCriteriaDataSet(CriteriaTree*){return nullptr;}} achievementMgr;auto sAchievementMgr=&achievementMgr;
bool CriteriaMatches(Player* referencePlayer,uint32 miscValue1){CriteriaEntry c;auto criteria=&c;CriteriaTree t;auto tree=&t;void* cachePtr=nullptr;switch(CRITERIA_TYPE_COMPLETE_QUEST){
#include "CompleteQuestRequirement.inc"
}return true;}
void Player::UpdateAchievementCriteria(int type,uint32 asset){assert(type==27);++criteriaEvents;if(!CriteriaMatches(this,asset))return;for(auto q:{38312u,38612u})if(ActiveSlot(q)){auto quest=store.GetQuestTemplate(q);auto const& obj=quest->objectives[0];if(obj.Type==14){AchieveCriteriaCredit(obj.ObjectID);}}}
#include "Grapple.inc"
void Flush(Player& p){auto events=std::move(p.events);p.events.clear();for(auto& event:events)event();}
struct Run {Player p;Creature npc;npc_stormheim_grapple_supply_ai ai;npc_stormheim_grapple_supply gossip;uint32 quest;
 Run(uint32 q):ai(&npc),quest(q){p.team=q==38312?ALLIANCE:HORDE;p.race=q==38312?1:2;p.Accept(q);npc.entry=q==38312?91222:92561;npc.fields[UNIT_FIELD_NPC_FLAGS]=2;}
 void Accept(){ai.sQuestAccept(&p,store.GetQuestTemplate(quest));}void Hello(){assert(!gossip.OnGossipHello(&p,&npc));}
};
int main(){
 for(auto pair:std::vector<std::pair<uint32,uint32>>{{38312,55241},{38612,55243}}){auto& q=store.quests[pair.first];q.id=pair.first;q.objectives={{14,pair.second,1,0,0}};q.AllowableRaces=pair.first==38312?824181837:234881970;}
 for(auto id:{38318u,38613u,39775u})store.quests[id].id=id;store.quests[39775].flags=37748744;
 for(uint32 id:{38312u,38612u}){Run r(id);assert(!r.p.CanCompleteQuest(id));r.Accept();assert(r.p.grants==1&&r.p.GetItemByEntry(138111)&&r.p.GetQuestRewardStatus(39775)&&r.p.GetQuestStatus(id)==QUEST_STATUS_INCOMPLETE);auto events=r.p.events.size();r.Hello();r.Accept();assert(r.p.grants==1&&r.p.events.size()==events);Flush(r.p);assert(r.p.completions==1&&r.p.GetQuestStatus(id)==QUEST_STATUS_COMPLETE&&r.p.CanRewardQuest(store.GetQuestTemplate(id),false));r.p.Accept(id);r.Accept();Flush(r.p);assert(r.p.grants==1&&r.p.completions==2);}
 for(uint32 id:{38312u,38612u}){Run r(id);r.p.full=true;r.Accept();assert(!r.p.grants&&!r.p.GetQuestRewardStatus(39775)&&!r.p.CanCompleteQuest(id));r.p.full=false;r.Hello();Flush(r.p);assert(r.p.completions==1);}
 // The original native out-of-log quest-complete effect returns without
 // setting39775: its real template does not have TRACKING. Pickup must not
 // depend on an accepted but ineffective cast.
 {Player p;p.CastSpell(&p,191831,true);assert(!p.GetQuestRewardStatus(39775));Run r(38312);r.p.castResult=SPELL_FAILED_BAD_TARGETS;r.Accept();Flush(r.p);assert(r.p.completions==1&&r.p.GetQuestRewardStatus(39775));}

 // Native entry lookup sees bag/bank items with world=false; no duplicate
 // grant or false full-bag failure at login. Counter-based surrogate data is
 // discarded until actual pickup evidence exists.
 for(int mode=0;mode<4;++mode){Run r(38312);Item item;Bag bag;bag.items={&item};if(mode==0)r.p.slots[23]=&item;if(mode==1)r.p.bagslots[19]=&bag;if(mode==2)r.p.slots[39]=&item;if(mode==3)r.p.bagslots[67]=&bag;r.p.full=true;assert(r.p.GetItemByEntry(138111,true)==&item);r.Hello();Flush(r.p);assert(!r.p.grants&&r.p.GetQuestRewardStatus(39775)&&r.p.completions==1);}
 {Run r(38312);r.p.data[{38312,55241}]=1;r.p.quests[38312].Status=QUEST_STATUS_COMPLETE;player_stormheim_grapple_pickup login;login.OnLogin(&r.p);assert(r.p.GetQuestStatus(38312)==QUEST_STATUS_INCOMPLETE&&r.p.GetQuestObjectiveData(38312,55241)==0&&!r.p.CanRewardQuest(store.GetQuestTemplate(38312),false));r.Hello();Flush(r.p);assert(r.p.completions==1);}
 {Run r(38612);r.p.SetRewardedQuest(39775);player_stormheim_grapple_pickup login;login.OnLogin(&r.p);Flush(r.p);assert(!r.p.grants&&r.p.completions==1);r.p.granted.clear();r.p.slots.clear();r.Hello();assert(r.p.grants==1);}
 for(int mode=0;mode<8;++mode){Run r(38312);if(mode==0)r.p.phase=false;if(mode==1)r.p.los=false;if(mode==2)r.npc.distance=6;if(mode==3)r.p.visible=false;if(mode==4)r.npc.world=false;if(mode==5)r.p.alive=false;if(mode==6)r.p.teleport=true;if(mode==7)r.npc.fields[UNIT_FIELD_NPC_FLAGS]=0;r.Hello();assert(!r.p.grants&&!r.p.GetQuestRewardStatus(39775));}
 {Run r(38312);r.Accept();r.p.quests[38312].Status=QUEST_STATUS_NONE;Flush(r.p);assert(!r.p.completions);r.p.Accept(38312);r.Accept();Flush(r.p);assert(r.p.completions==1&&r.p.grants==1);}
 {Run r(38312);r.Accept();r.p.Accept(38312);r.Accept();Flush(r.p);assert(r.p.completions==1);}
 {Run a(38312),b(38612);a.Accept();Flush(a.p);assert(a.p.completions==1&&!b.p.completions&&!b.p.GetQuestRewardStatus(39775));b.Accept();Flush(b.p);assert(b.p.completions==1);}
 {Run r(38612);r.npc.entry=92566;r.Accept();Flush(r.p);assert(r.p.completions==1);Run a(38312);a.npc.entry=91481;a.Hello();Flush(a.p);assert(a.p.completions==1);}
 {Run r(38312);r.npc.map=1522;r.Accept();assert(r.ai.delegated==1&&!r.p.grants);r.npc.map=1220;r.ai.sQuestAccept(&r.p,store.GetQuestTemplate(38318));assert(r.ai.delegated==2);r.p.team=HORDE;r.Hello();assert(!r.p.grants);}
 {Run r(38612);r.p.quests[38612].Status=QUEST_STATUS_REWARDED;r.p.Accept(38613);r.npc.entry=92566;r.Hello();assert(r.p.grants==1&&r.p.GetQuestRewardStatus(39775));}
 {Run r(38312);auto& objective=store.quests[38312].objectives[0];objective.ObjectID=999;r.Accept();assert(r.ai.delegated==1&&!r.p.grants);objective.ObjectID=55241;}
 std::cout<<"PASS: production supply/retry/login sync -> native entry scan, native acquisition flag and ineffective legacy spell proof and quest-completion criteria requirements -> original criteria objective/lifetime completion; both factions, loaded bags/bank, no duplicates, grant failures, legacy optional-counter repair, reaccept/re-entry and unrelated actor delegation.\n";
}
