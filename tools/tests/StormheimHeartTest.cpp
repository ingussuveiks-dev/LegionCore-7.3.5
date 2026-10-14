#include <algorithm>
#include <cmath>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <list>
#include <memory>
#include <mutex>
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
 UNIT_FIELD_NPC_FLAGS=11,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,UNIT_FIELD_STATE_WORLD_EFFECT_ID=12,UNIT_FIELD_STATE_SPELL_VISUAL_ID=13,UNIT_FIELD_STATE_ANIM_ID=14,UNIT_FIELD_STATE_ANIM_KIT_ID=15,UNIT_STAND_STATE_SLEEP=3,UNIT_STAND_STATE_KNEEL=8,UNIT_STAND_STATE_STAND=0,UNIT_STATE_CASTING=1,QUEST_SPECIAL_FLAGS_KILL=32,QUEST_SPECIAL_FLAGS_CAST=64,CRITERIA_TYPE_KILL_CREATURE_TYPE=1,CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD=2,CRITERIA_TIMED_TYPE_CREATURE=3,CRITERIA_TYPE_KILL_CREATURE=4,LOOT_ITEM_TYPE_ITEM=0,QUEST_SPECIAL_FLAGS_DELIVER=1,RATE_DROP_ITEM_REFERENCED_AMOUNT=0,QUEST_OBJECTIVE_CURRENCY=4,QUEST_OBJECTIVE_MONEY=8,EQUIP_ERR_ITEM_NOT_FOUND=1,TARGET_UNIT_DEST_AREA_ENTRY=8};
enum {UNIT_NPC_FLAG_GOSSIP=1,UNIT_NPC_FLAG_QUESTGIVER=2,SPELL_AURA_MOD_ROOT=26,AURA_EFFECT_HANDLE_REAL=1,AURA_REMOVE_BY_DEFAULT=0,AURA_REMOVE_BY_EXPIRE=1,AURA_REMOVE_BY_ENEMY_SPELL=2};using AuraEffectHandleModes=int;struct AuraEffect{};
using DamageEffectType=int; using QuestObjectiveType=int;
enum {UNIT_FIELD_FLAGS=20,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,REACT_PASSIVE=0,REACT_AGGRESSIVE=2};
using PrevQuests=std::vector<int32>; using ExclusiveQuestGroups=std::multimap<int32,uint32>;
#define ASSERT assert
constexpr float M_PI=3.14159265358979323846f;
#define TC_LOG_DEBUG(...) (void)0
enum class HighGuid {Player,Creature,Pet,Vehicle,GameObject};
struct ObjectGuid {template<HighGuid H>static ObjectGuid Create(uint32,uint32,uint64 n){ObjectGuid g(n);g.high=H;return g;}bool IsPlayer()const{return high==HighGuid::Player;}ObjectGuid(uint64 n=0):value(n){}HighGuid high=HighGuid::Creature;HighGuid GetHigh()const{return high;}using LowType=uint64;uint64 value=0; static ObjectGuid const Empty; bool IsEmpty()const{return !value;}bool operator==(ObjectGuid o)const{return value==o.value;}bool operator<(ObjectGuid other)const{return value<other.value;}};
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
struct Unit;struct Player;struct Creature;struct GameObject;struct SmartAI;struct SmartScript;
struct Vehicle {struct Seat {struct {ObjectGuid Guid;} Passenger;};std::map<int,Seat> Seats;struct Unit* GetPassenger(int);};
struct WorldObject {ObjectGuid guid{1};std::weak_ptr<QuestObjectScope> questScope;bool bound=false,expired=false; bool HasQuestObjectScope(){return bound;} bool IsQuestObjectExpired(){auto scope=questScope.lock();return expired||(bound&&(!scope||scope->cancelled)); } uint32 map=1220,zone=7541,area=7557,entry=0;bool world=true,alive=true;float distance=0;
 virtual ~WorldObject()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}virtual Player const* ToPlayer()const{return nullptr;}virtual Unit* ToUnit(){return nullptr;}virtual bool IsCreature(){return false;}float wx=0,wy=0,wz=0;virtual float GetPositionX(){return wx;}virtual float GetPositionY(){return wy;}virtual float GetPositionZ(){return wz;}float GetOrientation(){return 1.57336f;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}uint32 GetAreaId(){return area;}virtual ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}};
struct Unit:WorldObject {using WorldObject::ToPlayer;std::map<std::pair<uint32,uint64>,Aura> roots;Aura* GetAura(uint32 id,ObjectGuid caster){auto i=roots.find({id,caster.value});return i==roots.end()?nullptr:&i->second;}void RemoveAurasDueToSpell(uint32 id,ObjectGuid caster){roots.erase({id,caster.value});}std::function<int(Unit*,uint32)> castTargetHook; Player* ownerPlayer=nullptr;ObjectGuid GetCharmerOrOwnerGUID()const;Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;std::set<uint32> auraIDs;void RemoveAurasDueToSpell(uint32 id){auraIDs.erase(id);}void RemoveAllAuras(){auraIDs.clear();}int castResult=0; std::vector<uint32> casts; std::function<void(uint32)> castHook; int CastSpell(Unit* target,uint32 id,bool){casts.push_back(id);if(castTargetHook)return castTargetHook(target,id);if(castHook)castHook(id);return castResult;} Creature* SummonCreature(uint32,float,float,float,float,int,uint32); GameObject* SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32);Vehicle* kit=nullptr;Unit* ToUnit()override{return this;}Vehicle* GetVehicleKit(){return kit;}bool InSamePhase(WorldObject*){return true;}virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct SpellInfo {uint32 Id=180737;};
enum {QUEST_OBJECTIVE_FLAG_SEQUENCED=2,MOVEMENTFLAG_FALLING=1,MOVEMENTFLAG_FALLING_FAR=2,MOVEMENTFLAG_FLYING=4,MAX_KILL_CREDIT=2};
struct CreatureTemplate {uint32 Entry=91240,KillCredit[2]{113889,0},ScriptID=1;uint32 StateWorldEffectID=0,SpellStateVisualID=45944,SpellStateAnimID=0,SpellStateAnimKitID=6050;};
struct HeightMap {float value=229.427f;float GetHeight(float,float,float){return value;}} heightMap;
struct FakeMotion {uint32 resets=0,follows=0;void Clear(){++resets;}void MoveFollow(Player*,float,float){++follows;}};
struct TempSummon{};
struct Creature:Unit {bool temporary=false;FakeMotion motion;TempSummon* ToTempSummon(){return temporary?reinterpret_cast<TempSummon*>(this):nullptr;}bool IsWithinDistInMap(WorldObject*,float range){return distance<=range;}void NearTeleportTo(float,float,float,float){distance=0;}FakeMotion* GetMotionMaster(){return &motion;}bool isInCombat(){return combat;}bool HasAura(uint32 id){return auraIDs.count(id);}uint32 instance=0;uint32 GetInstanceId(){return instance;}void AddRewardPlayer(ObjectGuid){}uint64 dbGuid=0;uint64 GetDBTableGUIDLow(){return dbGuid;}void SetHealth(uint64 n){health=n;}HeightMap* GetMap(){return &heightMap;} SmartAI* ai=nullptr;SmartAI*& i_AI=ai;SmartAI* i_disabledAI=nullptr;bool charmed=false;bool IsPlayer(){return false;}bool isCharmed(){return charmed;}bool isPossessed(){return false;}bool IsVehicle(){return kit!=nullptr;}void UpdateCharmAI();std::vector<std::function<void()>> callbacks;void AddDelayedEvent(uint64,std::function<void()> f){callbacks.push_back(std::move(f));}void Tick(){auto q=std::move(callbacks);callbacks.clear();for(auto& f:q)f();}CreatureTemplate data;std::map<int,uint32> fields;uint32 stand=8,dirty=0,talks=0;bool visible=true,combat=false;
 Creature* ToCreature()override{return this;}bool IsCreature()override{return true;}Player* GetLootRecipient(){return nullptr;}void AddPlayerInPersonnalVisibilityList(ObjectGuid p){personal=p;}ObjectGuid personal;void setActive(bool){}void DespawnOrUnsummon(){world=false;} uint64 health=1000,maximum=1000;float multiplier=1;Unit* victim=nullptr;uint32 react=0;void SetReactState(uint32 r){react=r;}void CombatStop(bool){combat=false;}uint64 GetHealth(Unit*){return health;}uint64 GetMaxHealth(Unit*){return maximum;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}SmartAI* AI(){return ai;}CreatureTemplate const* GetCreatureTemplate(){return &data;}
 bool HasFlag(int f,uint32 v){return fields[f]&v;}void SetFlag(int f,uint32 v){fields[f]|=v;}void RemoveFlag(int f,uint32 v){fields[f]&=~v;}
 void SetUInt32Value(int f,uint32 v){fields[f]=v;}void ForceValuesUpdateAtIndex(int){++dirty;}void SetVisible(bool v){visible=v;}void SetStandState(uint32 s){stand=s;}
 using Unit::RemoveAurasDueToSpell;std::set<ObjectGuid> hidden;void AddToHideList(ObjectGuid g){hidden.insert(g);}void RemoveFromHideList(ObjectGuid g){hidden.erase(g);}void DestroyForPlayer(Player*){} bool HasUnitState(int){return false;}uint32 GetCreatureType(){return 7;}};
struct GameObject:WorldObject {std::set<ObjectGuid> hidden;void AddToHideList(ObjectGuid id){hidden.insert(id);}void RemoveFromHideList(ObjectGuid id){hidden.erase(id);}void DestroyForPlayer(Player*){}uint32 flags=0,state=GO_READY;bool spawned=true;bool isSpawned(){return spawned;}bool HasFlag(int,uint32 f){return flags&f;}};
std::map<ObjectGuid,Creature*> creatures;
std::map<ObjectGuid,Player*> players;std::map<ObjectGuid,GameObject*> objects;
namespace ObjectAccessor {Player* GetPlayer(WorldObject const&,ObjectGuid);Player* FindPlayer(ObjectGuid,bool=true);GameObject* GetGameObject(WorldObject&,ObjectGuid);Creature* GetCreature(WorldObject&,ObjectGuid id){auto i=creatures.find(id);return i==creatures.end()||!i->second->world?nullptr:i->second;}}
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,WorldObject*,uint32 entry,float){for(auto const& pair:creatures)if(pair.second->entry==entry)out.push_back(pair.second);}
struct FakeMap {float floor=335.429f;float GetHeight(float,float,float,bool,float){return floor;}GameObject* GetGameObject(ObjectGuid g){auto i=objects.find(g);return i==objects.end()?nullptr:i->second;} bool IsDungeon(){return false;}int GetDifficultyID(){return 0;}Creature* GetCreature(ObjectGuid g){return creatures.count(g)?creatures[g]:nullptr;}};
struct GroupReference {Player* player=nullptr;GroupReference* following=nullptr;Player* getSource(){return player;}GroupReference* next(){return following;}};
struct Group {GroupReference* first=nullptr;GroupReference* GetFirstMember(){return first;}bool isRaidGroup(){return false;}};
struct Guild {template<class... T>void UpdateAchievementCriteria(T...) {}};
struct GuildMgr {Guild* GetGuildById(uint32){return nullptr;}} guildMgr;auto sGuildMgr=&guildMgr;
struct Achievement {void StartTimedAchievement(int,uint32){}};
struct GossipMenu {uint32 menu=18672;bool item=true;uint32 GetMenuId(){return menu;}void* GetItem(uint32 id){return id==0&&item?this:nullptr;}};
struct PlayerMenu {GossipMenu menu;uint32 closed=0;GossipMenu& GetGossipMenu(){return menu;}void ClearMenus(){menu.item=false;}void SendCloseGossip(){++closed;}};
struct FakeSpline {bool done=true;bool Finalized(){return done;}};
struct Player:Unit {uint32 instance=0;uint32 GetInstanceId(){return instance;}float x=2348,y=2380,z=335.5f;uint32 movement=0;FakeSpline spline;FakeSpline* movespline=&spline;std::map<uint32,std::vector<ObjectGuid>> ownedGuides;auto GetSummonList(uint32 entry){return &ownedGuides[entry];}bool HasUnitMovementFlag(uint32 flags){return (movement&flags)!=0;}bool flight=false;bool isInFlight(){return flight;}float GetExactDist2d(float a,float b){return std::sqrt((x-a)*(x-a)+(y-b)*(y-b));}void KilledMonster(CreatureTemplate const*,ObjectGuid);uint64 race=1;uint64 getRaceMask(){return race;}Quest const* GetNextQuest(ObjectGuid,Quest const*);PlayerMenu menu;PlayerMenu* PlayerTalkClass=&menu;using Unit::GetAura;using Unit::RemoveAurasDueToSpell;
 ObjectGuid guid{1};uint32 team=ALLIANCE,packets=0,completions=0,power=0;bool teleport=false,
 vehicle=false,transport=false,combat=false,phase=true,visible=true,los=true,otherPower=false;
 int castResult=0;std::set<uint32> auras, m_RewardedQuests;std::map<uint32,QuestStatusData> quests;
 std::map<std::pair<uint32,uint32>,int32> data;std::vector<std::function<void()>> events;std::function<void()> castHook;
 Player const* ToPlayer()const override{return this;}Player* ToPlayer()override{return this;} std::function<Creature*(uint32)> summonHook; std::vector<WorldObject*> boundObjects; uint32 cancels=0; using Unit::CastSpell; using Unit::SummonCreature; void UpdateVisibilityForPlayer(){} void KillCreditGO(uint32,ObjectGuid const&); void BindQuestObject(uint32 q,WorldObject* o){auto& scope=quests[q].ScriptObjects;if(!scope)scope=std::make_shared<QuestObjectScope>();o->questScope=scope;o->bound=true;boundObjects.push_back(o);} void CancelQuestObjects(uint32 q){++cancels;quests[q].ScriptObjects.reset();for(auto o:boundObjects)o->expired=true;boundObjects.clear();}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}
 uint32 GetTeam(){return team;}float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return 1.57336f;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}bool IsBeingTeleported(){return teleport;}
 bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInCombat(){return combat;}
 Unit* vehicleBase=nullptr;Unit* GetVehicleBase(){return vehicleBase;}void ExitVehicle(){if(vehicleBase&&vehicleBase->kit)vehicleBase->kit->Seats.clear();vehicleBase=nullptr;vehicle=false;}bool InSamePhase(WorldObject*){return phase;}bool canSeeOrDetect(WorldObject*){return visible;}bool IsWithinLOSInMap(WorldObject*){return los;}
 bool IsWithinDistInMap(WorldObject* o,float range){return std::sqrt((x-o->GetPositionX())*(x-o->GetPositionX())+(y-o->GetPositionY())*(y-o->GetPositionY())+(z-o->GetPositionZ())*(z-o->GetPositionZ()))<=range;} float GetExactDist(WorldObject* o){return o->distance;}float GetDistance(WorldObject* o){return std::sqrt((x-o->GetPositionX())*(x-o->GetPositionX())+(y-o->GetPositionY())*(y-o->GetPositionY())+(z-o->GetPositionZ())*(z-o->GetPositionZ()));}QuestStatusData* getQuestStatus(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}
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
 uint32 conversations=0,created=0;bool conversationFails=false;void CreateConversation(uint32 id){assert(id==2549);++conversations;if(!conversationFails)++created;}void TeleportTo(uint32,float,float,float,float){} Achievement achievement;Achievement* m_achievementMgr=&achievement;Group* group=nullptr;Group* GetGroup(){return group;}void RewardPlayerAndGroupAtEvent(uint32,WorldObject*);bool TalkedToCreature(uint32,ObjectGuid){return false;}bool rewardRange=true;bool IsAtGroupRewardDistance(WorldObject*){return rewardRange;}bool corpse=false;bool GetCorpse(){return corpse;}uint32 GetGuildId(){return 0;}template<class... T>void UpdateAchievementCriteria(T...){}void AddKillCreature(uint32,uint16,bool){}void AddTrackingQuestIfNeeded(ObjectGuid){}void SetQuestStatus(uint32 q,int state){quests[q].Status=state;}void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty);FakeMap* GetMap()const{static FakeMap m;return &m;}bool CanGetItemForLoot(void const*,bool)const{return true;}
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
Player* GetPlayer(WorldObject const& source,ObjectGuid id){auto i=players.find(id);return i==players.end()||!i->second->world||i->second->map!=source.map?nullptr:i->second;}
Player* FindPlayer(ObjectGuid id,bool world){auto i=players.find(id);return i==players.end()||(world&&!i->second->world)?nullptr:i->second;}
GameObject* GetGameObject(WorldObject&,ObjectGuid id){auto i=objects.find(id);return i==objects.end()||!i->second->world?nullptr:i->second;}}
Unit* Vehicle::GetPassenger(int index){auto i=Seats.find(index);return i==Seats.end()?nullptr:ObjectAccessor::FindPlayer(i->second.Passenger.Guid);}
struct SmartAI {Creature* me;uint32 delegated=0;explicit SmartAI(Creature* c):me(c){c->ai=this;}virtual ~SmartAI()=default;
 virtual void sQuestAccept(Player*,Quest const*){++delegated;}virtual void sGossipSelect(Player*,uint32,uint32){++delegated;} virtual void JustSummoned(Creature*){++delegated;} virtual void JustDied(Unit*){++delegated;} void Talk(uint8,ObjectGuid){++me->talks;} virtual void DoAction(int32){}virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual ObjectGuid GetGUID(int32){return {};}virtual uint32 GetData(uint32)const{return 0;}SmartScript* GetScript();virtual void SetData(uint32,uint32);
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




using Map=FakeMap;
struct MapMgr {Map* FindMap(uint32,uint32){static Map m;return &m;}} mapMgr;auto sMapMgr=&mapMgr;
namespace ObjectAccessor {Creature* GetCreatureOrPetOrVehicle(WorldObject& source,ObjectGuid g){return GetCreature(source,g);}}
#include "NextQuest.inc"
#include "Race.inc"

struct EventObjectTemplate {uint32 SpellID=0,WorldSafeLocID=0,Flags=0;float radius=5;};
using GuidSet=std::set<ObjectGuid>;
enum {EO_FLAG_ONE_ACTION=1,SMART_EVENT_EVENTOBJECT_ONTRIGGER=89,SMART_EVENT_EVENTOBJECT_OFFTRIGGER=90};
struct EventObject:WorldObject {
 uint32 instance=0;uint64 spawn=410363;bool m_isUpdate=false;EventObjectTemplate data;EventObjectTemplate* eventTemplate=&data;
 GuidSet affectedPlayers,affectedPermamentPlayers;uint32 GetInstanceId(){return instance;}uint64 GetDBTableGUIDLow(){return spawn;}
 float GetDistance(Player* p){return std::sqrt((wx-p->x)*(wx-p->x)+(wy-p->y)*(wy-p->y)+(wz-p->z)*(wz-p->z));}
 void GetPlayerListInGrid(std::list<Player*>& out,float radius){for(auto const& p:players)if(p.second->world&&p.second->map==map&&p.second->instance==instance&&GetDistance(p.second)<=radius)out.push_back(p.second);}
 void Update(uint32);bool ActivatedForPlayer(Player*)const;
};
using ObjectList=std::vector<WorldObject*>;
enum {SMART_EVENT_DATA_SET=38};
struct Holder {struct {struct {uint32 textGroupID=3,duration=6000;}talk;struct {uint32 spell=182091;}removeAura;}action;};
struct TextMgr {uint32 count=0;void SendChat(Creature*,uint8 group,ObjectGuid){assert(group==3);++count;}} textMgr;auto sCreatureTextMgr=&textMgr;
struct SmartScript {
 static uint32 delegated;Creature* me=nullptr;uint32 mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false;ObjectGuid mTextGUID;
 void OnInitialize(WorldObject* o,void*){me=o->ToCreature();}ObjectList* GetTargets(Holder&,Unit*){return nullptr;}
 bool IsCreature(WorldObject* o){return o&&o->IsCreature();}bool IsPlayer(WorldObject* o){return o&&o->ToPlayer();}Unit* GetLastInvoker(){return nullptr;}
 void ProcessTalk(Holder& e,Unit* unit){switch(1){case 1:
#include "TalkAction.inc"
 }}
 void ProcessEventsFor(int event,Unit* unit,uint32 id,uint32 value=0){++delegated;if(event==SMART_EVENT_DATA_SET){assert(!unit);if(id==1&&value==1){Holder e;ProcessTalk(e,unit);}}}
};uint32 SmartScript::delegated=0;
SmartScript* SmartAI::GetScript(){static SmartScript script;script.me=me;return &script;}
ObjectGuid Unit::GetCharmerOrOwnerGUID()const{return ownerPlayer?ownerPlayer->GetGUID():ObjectGuid::Empty;}
#include "OwnerPlayer.inc"
#include "SmartSetData.inc"
#include "KilledMonster.inc"
struct KillRewarder {Unit* _victim;Group* _group=nullptr;void _RewardKillCredit(Player*);};
#include "RewardKillCredit.inc"
struct EventObjectScript {explicit EventObjectScript(char const*){}virtual ~EventObjectScript()=default;virtual bool IsTriggerMeets(Player*,EventObject*){return true;}virtual bool OnTrigger(Player*,EventObject*,bool){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogin(Player*){}};
#include "Heart.inc"
struct ScriptMgr {eventobject_stormheim_heart handler;bool IsEOMeetConditions(Player* p,EventObject* e){return handler.IsTriggerMeets(p,e);}bool OnEventObject(Player* p,EventObject* e,bool enter){return handler.OnTrigger(p,e,enter);}} scriptMgr;auto sScriptMgr=&scriptMgr;
struct WorldSafeLocsEntry {uint32 MapID=0;struct {float X=0,Y=0,Z=0,O=0;}Loc;};
struct SafeStore {WorldSafeLocsEntry* LookupEntry(uint32){return nullptr;}} safeStore;auto& sWorldSafeLocsStore=safeStore;
int Player::CastSpell(Player*,uint32,bool){return 0;}
#include "EventUpdate.inc"
#include "Activated.inc"
using SmartAIEventList=std::vector<int32>;
struct SelectorMgr {std::map<int32,SmartAIEventList> lists{{91570,{0}},{-294116,{182091}}};SmartAIEventList GetScript(int32 entry,int){return lists[entry];}} selectorMgr;auto sSmartScriptMgr=&selectorMgr;
struct Selector {Creature* me;int mScriptType=0;SmartAIEventList selected;void FillScript(SmartAIEventList e,Creature*,void*){selected=e;}void GetScript(){
#include "CreatureScriptSelection.inc"
}};
struct Run {
 Player p;EventObject approach;Creature boss,mother;npc_stormheim_heart_mother ai{&mother};
 explicit Run(uint32 team=ALLIANCE){players.clear();creatures.clear();static uint32 guid=50;p.guid={++guid};p.guid.high=HighGuid::Player;p.team=team;p.Accept(38414);players[p.guid]=&p;
  approach.entry=613;approach.spawn=410365;approach.data.radius=10;approach.wx=2009.2f;approach.wy=2578.84f;approach.wz=486.069f;
  boss.entry=91737;boss.dbGuid=294111;boss.guid={294111};boss.data.Entry=91737;boss.data.KillCredit[0]=0;
  mother.entry=91570;mother.dbGuid=294116;mother.guid={294116};mother.stand=3;mother.auraIDs={182091,123};creatures[boss.guid]=&boss;creatures[mother.guid]=&mother;
 }
 void Place(){p.x=approach.wx;p.y=approach.wy;p.z=approach.wz;p.GetMap()->floor=approach.wz;}
 void Find(){Place();approach.Update(1);}
 void Drain(){auto pending=std::move(p.events);p.events.clear();for(auto& event:pending)event();}
 void Kill(Unit* killer=nullptr){boss.alive=false;Player* owner=killer?killer->GetCharmerOrOwnerPlayerOrPlayerItself():&p;assert(owner);KillRewarder reward{&boss};reward._RewardKillCredit(owner);if(creatures.count(mother.guid))ai.SetData(1,1);assert(!p.GetQuestObjectiveData(38414,91737));Drain();}
};
int main(){
 {Creature main,other;main.entry=other.entry=91570;main.dbGuid=294116;other.dbGuid=123;Selector scoped{&main},generic{&other};scoped.GetScript();generic.GetScript();assert(scoped.selected==SmartAIEventList{182091}&&generic.selected==SmartAIEventList{0});}
 auto& definition=store.quests[38414];definition.id=38414;definition.special=QUEST_SPECIAL_FLAGS_KILL;definition.objectives={{0,91734,1,0,0},{0,91737,1,3,1}};
 for(auto team:{ALLIANCE,HORDE}){Run r(team);r.Find();assert(r.p.GetQuestObjectiveData(38414,91734)==1&&r.p.GetQuestStatus(38414)==QUEST_STATUS_INCOMPLETE);Unit pet;pet.ownerPlayer=&r.p;r.Kill(&pet);assert(r.p.GetQuestObjectiveData(38414,91737)==1&&r.p.GetQuestStatus(38414)==QUEST_STATUS_COMPLETE);assert(r.mother.stand==0&&!r.mother.auraIDs.count(182091)&&r.mother.auraIDs.count(123));auto calls=textMgr.count;r.ai.SetData(1,1);r.ai.UpdateAI(1000);assert(textMgr.count==calls);r.p.KilledMonster(&r.boss.data,r.boss.guid);assert(r.p.packets==2);}
 for(int failure=0;failure<12;++failure){Run r;r.Place();if(failure==0)r.p.spline.done=false;if(failure==1)r.p.movement=MOVEMENTFLAG_FALLING;if(failure==2)r.p.movement=MOVEMENTFLAG_FLYING;if(failure==3)r.p.vehicle=true;if(failure==4)r.p.transport=true;if(failure==5)r.p.flight=true;if(failure==6)r.p.teleport=true;if(failure==7)r.p.alive=false;if(failure==8)r.p.phase=false;if(failure==9)r.p.GetMap()->floor-=3;if(failure==10)r.p.instance=1;if(failure==11)r.p.quests.erase(38414);r.approach.Update(1);assert(!r.p.GetQuestObjectiveData(38414,91734)&&!r.approach.ActivatedForPlayer(&r.p));}
 {Run r;r.Place();r.p.spline.done=false;r.approach.Update(1);r.p.spline.done=true;r.approach.Update(1);assert(r.p.GetQuestObjectiveData(38414,91734)==1);}
 {Run r;r.ai.SetData(1,1);assert(!r.ai.released&&r.mother.stand==3);r.boss.alive=false;r.ai.UpdateAI(1000);assert(r.ai.released&&r.mother.stand==0);r.boss.alive=true;r.ai.UpdateAI(1000);assert(!r.ai.released&&r.mother.stand==3);r.mother.auraIDs.insert(182091);r.boss.alive=false;r.ai.SetData(1,1);assert(r.ai.released&&!r.mother.auraIDs.count(182091));}
 {Run r;creatures.erase(r.boss.guid);r.ai.SetData(1,1);r.ai.UpdateAI(1000);assert(!r.ai.released&&r.mother.stand==3);creatures[r.boss.guid]=&r.boss;r.boss.alive=false;r.ai.UpdateAI(1000);assert(r.ai.released);}
 {Run r;r.Find();creatures.erase(r.mother.guid);r.Kill();assert(r.p.GetQuestStatus(38414)==QUEST_STATUS_COMPLETE);}
 {Run r;r.Kill();assert(r.p.GetQuestObjectiveData(38414,91737)==1&&r.p.GetQuestStatus(38414)==QUEST_STATUS_INCOMPLETE);r.Find();assert(r.p.GetQuestStatus(38414)==QUEST_STATUS_COMPLETE);}
 {Run r;r.Find();r.p.Accept(38414);r.approach.Update(1);assert(!r.p.GetQuestObjectiveData(38414,91734));r.p.x+=30;r.approach.Update(1);r.Find();assert(r.p.GetQuestObjectiveData(38414,91734)==1);}
 {Run r;r.Find();r.p.world=false;r.approach.Update(1);r.p.world=true;r.p.Accept(38414);r.Find();assert(r.p.GetQuestObjectiveData(38414,91734)==1);}
 {Run r;Unit ownerless;assert(!ownerless.GetCharmerOrOwnerPlayerOrPlayerItself());assert(r.p.GetCharmerOrOwnerPlayerOrPlayerItself()==&r.p);r.p.world=false;Unit pet;pet.ownerPlayer=&r.p;assert(!pet.GetCharmerOrOwnerPlayerOrPlayerItself());}
 {Run r;Group group;KillRewarder reward{&r.boss,&group};r.p.alive=false;r.p.corpse=true;reward._RewardKillCredit(&r.p);assert(r.p.events.empty());r.p.corpse=false;reward._RewardKillCredit(&r.p);assert(r.p.events.size()==1);}
 {Run r;Creature unrelated;unrelated.entry=91570;unrelated.dbGuid=123;unrelated.stand=8;npc_stormheim_heart_mother other(&unrelated);other.SetData(1,1);assert(unrelated.stand==8);other.Reset();other.UpdateAI(1000);assert(unrelated.stand==8);}
 {Run r;r.mother.instance=1;r.boss.alive=false;r.ai.SetData(1,1);r.ai.UpdateAI(1000);assert(!r.ai.released&&r.mother.stand==3);}
 {Run r;r.p.quests[38414].Status=QUEST_STATUS_COMPLETE;player_stormheim_heart script;script.OnLogin(&r.p);assert(r.p.GetQuestStatus(38414)==QUEST_STATUS_INCOMPLETE);r.Find();r.Kill();script.OnLogin(&r.p);assert(r.p.GetQuestStatus(38414)==QUEST_STATUS_COMPLETE);}
 {Run r;r.mother.auraIDs.erase(182091);r.boss.combat=true;r.ai.UpdateAI(1000);assert(r.boss.casts.empty());r.boss.combat=false;r.boss.castResult=1;r.ai.UpdateAI(1000);assert(r.boss.casts.size()==1&&!r.mother.auraIDs.count(182091));r.boss.castTargetHook=[&](Unit* target,uint32 spell){assert(target==&r.boss&&spell==182091);r.mother.auraIDs.insert(spell);return 0;};r.ai.UpdateAI(1000);assert(r.mother.auraIDs.count(182091));auto count=r.boss.casts.size();r.ai.UpdateAI(1000);assert(r.boss.casts.size()==count);}
 {Run r;Player second;second.guid={888};second.guid.high=HighGuid::Player;second.team=HORDE;second.Accept(38414);players[second.guid]=&second;r.Place();second.x=r.p.x;second.y=r.p.y;second.z=r.p.z;r.approach.Update(1);assert(second.GetQuestObjectiveData(38414,91734)==1&&r.p.GetQuestObjectiveData(38414,91734)==1);r.boss.alive=false;Group group;KillRewarder reward{&r.boss,&group};reward._RewardKillCredit(&r.p);reward._RewardKillCredit(&second);r.Drain();auto pending=std::move(second.events);for(auto& e:pending)e();assert(r.p.GetQuestStatus(38414)==QUEST_STATUS_COMPLETE&&second.GetQuestStatus(38414)==QUEST_STATUS_COMPLETE);}
 {Run r;r.Find();r.boss.alive=false;KillRewarder reward{&r.boss};reward._RewardKillCredit(&r.p);r.p.quests.erase(38414);r.Drain();assert(!r.p.GetQuestObjectiveData(38414,91737));}
 std::cout<<"PASS: actual approach/mother lifecycle plus native EventObject update, pet owner resolution, delayed KillRewarder/kill/completion and null-invoker SmartAI talk; both factions, failures, corpse/re-entry/respawn/retry and unrelated actors.\n";
}
