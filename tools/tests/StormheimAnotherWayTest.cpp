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
 UNIT_FIELD_NPC_FLAGS=11,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,UNIT_FIELD_STATE_WORLD_EFFECT_ID=12,UNIT_FIELD_STATE_SPELL_VISUAL_ID=13,UNIT_FIELD_STATE_ANIM_ID=14,UNIT_FIELD_STATE_ANIM_KIT_ID=15,UNIT_STAND_STATE_KNEEL=8,UNIT_STAND_STATE_STAND=0,UNIT_STATE_CASTING=1,QUEST_SPECIAL_FLAGS_KILL=32,QUEST_SPECIAL_FLAGS_CAST=64,CRITERIA_TYPE_KILL_CREATURE_TYPE=1,CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD=2,CRITERIA_TIMED_TYPE_CREATURE=3,CRITERIA_TYPE_KILL_CREATURE=4,LOOT_ITEM_TYPE_ITEM=0,QUEST_SPECIAL_FLAGS_DELIVER=1,RATE_DROP_ITEM_REFERENCED_AMOUNT=0,QUEST_OBJECTIVE_CURRENCY=4,QUEST_OBJECTIVE_MONEY=8,EQUIP_ERR_ITEM_NOT_FOUND=1,TARGET_UNIT_DEST_AREA_ENTRY=8};
enum {UNIT_NPC_FLAG_GOSSIP=1,UNIT_NPC_FLAG_QUESTGIVER=2,SPELL_AURA_MOD_ROOT=26,AURA_EFFECT_HANDLE_REAL=1,AURA_REMOVE_BY_DEFAULT=0,AURA_REMOVE_BY_EXPIRE=1,AURA_REMOVE_BY_ENEMY_SPELL=2};using AuraEffectHandleModes=int;struct AuraEffect{};
using DamageEffectType=int; using QuestObjectiveType=int;
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
struct Unit:WorldObject {std::map<std::pair<uint32,uint64>,Aura> roots;Aura* GetAura(uint32 id,ObjectGuid caster){auto i=roots.find({id,caster.value});return i==roots.end()?nullptr:&i->second;}void RemoveAurasDueToSpell(uint32 id,ObjectGuid caster){roots.erase({id,caster.value});}std::function<int(Unit*,uint32)> castTargetHook; Player* ownerPlayer=nullptr;Player* GetCharmerOrOwnerPlayerOrPlayerItself() const {return const_cast<Unit*>(this)->ToPlayer()?const_cast<Unit*>(this)->ToPlayer():ownerPlayer;}int castResult=0; std::vector<uint32> casts; std::function<void(uint32)> castHook; int CastSpell(Unit* target,uint32 id,bool){casts.push_back(id);if(castTargetHook)return castTargetHook(target,id);if(castHook)castHook(id);return castResult;} Creature* SummonCreature(uint32,float,float,float,float,int,uint32); GameObject* SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32);Vehicle* kit=nullptr;Unit* ToUnit()override{return this;}Vehicle* GetVehicleKit(){return kit;}bool InSamePhase(WorldObject*){return true;}virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct SpellInfo {uint32 Id=180737;};
enum {QUEST_OBJECTIVE_FLAG_SEQUENCED=2,MOVEMENTFLAG_FALLING=1,MOVEMENTFLAG_FALLING_FAR=2,MOVEMENTFLAG_FLYING=4,MAX_KILL_CREDIT=2};
struct CreatureTemplate {uint32 Entry=91240,KillCredit[2]{113889,0},ScriptID=1;uint32 StateWorldEffectID=0,SpellStateVisualID=45944,SpellStateAnimID=0,SpellStateAnimKitID=6050;};
struct HeightMap {float value=229.427f;float GetHeight(float,float,float){return value;}} heightMap;
struct FakeMotion {uint32 resets=0,follows=0;void Clear(){++resets;}void MoveFollow(Player*,float,float){++follows;}};
struct TempSummon{};
struct Creature:Unit {bool temporary=false;FakeMotion motion;TempSummon* ToTempSummon(){return temporary?reinterpret_cast<TempSummon*>(this):nullptr;}bool IsWithinDistInMap(WorldObject*,float range){return distance<=range;}void NearTeleportTo(float,float,float,float){distance=0;}FakeMotion* GetMotionMaster(){return &motion;}uint64 dbGuid=0;uint64 GetDBTableGUIDLow(){return dbGuid;}void SetHealth(uint64 n){health=n;}HeightMap* GetMap(){return &heightMap;} SmartAI* ai=nullptr;SmartAI*& i_AI=ai;SmartAI* i_disabledAI=nullptr;bool charmed=false;bool IsPlayer(){return false;}bool isCharmed(){return charmed;}bool isPossessed(){return false;}bool IsVehicle(){return kit!=nullptr;}void UpdateCharmAI();std::vector<std::function<void()>> callbacks;void AddDelayedEvent(uint64,std::function<void()> f){callbacks.push_back(std::move(f));}void Tick(){auto q=std::move(callbacks);callbacks.clear();for(auto& f:q)f();}CreatureTemplate data;std::map<int,uint32> fields;uint32 stand=8,dirty=0,talks=0;bool visible=true,combat=false;
 Creature* ToCreature()override{return this;}bool IsCreature()override{return true;}Player* GetLootRecipient(){return nullptr;}void AddPlayerInPersonnalVisibilityList(ObjectGuid p){personal=p;}ObjectGuid personal;void setActive(bool){}void DespawnOrUnsummon(){world=false;} uint64 health=1000,maximum=1000;float multiplier=1;Unit* victim=nullptr;uint32 react=0;void SetReactState(uint32 r){react=r;}void CombatStop(bool){combat=false;}uint64 GetHealth(Unit*){return health;}uint64 GetMaxHealth(Unit*){return maximum;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}SmartAI* AI(){return ai;}CreatureTemplate const* GetCreatureTemplate(){return &data;}
 bool HasFlag(int f,uint32 v){return fields[f]&v;}void SetFlag(int f,uint32 v){fields[f]|=v;}void RemoveFlag(int f,uint32 v){fields[f]&=~v;}
 void SetUInt32Value(int f,uint32 v){fields[f]=v;}void ForceValuesUpdateAtIndex(int){++dirty;}void SetVisible(bool v){visible=v;}void SetStandState(uint32 s){stand=s;}
 using Unit::RemoveAurasDueToSpell;std::set<ObjectGuid> hidden;void AddToHideList(ObjectGuid g){hidden.insert(g);}void RemoveFromHideList(ObjectGuid g){hidden.erase(g);}void DestroyForPlayer(Player*){} bool HasUnitState(int){return false;}uint32 GetCreatureType(){return 7;}};
struct GameObject:WorldObject {std::set<ObjectGuid> hidden;void AddToHideList(ObjectGuid id){hidden.insert(id);}void RemoveFromHideList(ObjectGuid id){hidden.erase(id);}void DestroyForPlayer(Player*){}uint32 flags=0,state=GO_READY;bool spawned=true;bool isSpawned(){return spawned;}bool HasFlag(int,uint32 f){return flags&f;}};
std::map<ObjectGuid,Creature*> creatures;
std::map<ObjectGuid,Player*> players;std::map<ObjectGuid,GameObject*> objects;
namespace ObjectAccessor {Player* GetPlayer(WorldObject&,ObjectGuid);Player* FindPlayer(ObjectGuid,bool=true);GameObject* GetGameObject(WorldObject&,ObjectGuid);Creature* GetCreature(WorldObject&,ObjectGuid id){auto i=creatures.find(id);return i==creatures.end()||!i->second->world?nullptr:i->second;}}
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
struct Player:Unit {uint32 instance=0;uint32 GetInstanceId(){return instance;}float x=2348,y=2380,z=335.5f;uint32 movement=0;FakeSpline spline;FakeSpline* movespline=&spline;std::map<uint32,std::vector<ObjectGuid>> ownedGuides;auto GetSummonList(uint32 entry){return &ownedGuides[entry];}bool HasUnitMovementFlag(uint32 flags){return (movement&flags)!=0;}bool isInFlight(){return false;}float GetExactDist2d(float a,float b){return std::sqrt((x-a)*(x-a)+(y-b)*(y-b));}void KilledMonster(CreatureTemplate const*,ObjectGuid);uint64 race=1;uint64 getRaceMask(){return race;}Quest const* GetNextQuest(ObjectGuid,Quest const*);PlayerMenu menu;PlayerMenu* PlayerTalkClass=&menu;using Unit::GetAura;using Unit::RemoveAurasDueToSpell;
 ObjectGuid guid{1};uint32 team=ALLIANCE,packets=0,completions=0,power=0;bool teleport=false,
 vehicle=false,transport=false,combat=false,phase=true,visible=true,los=true,otherPower=false;
 int castResult=0;std::set<uint32> auras, m_RewardedQuests;std::map<uint32,QuestStatusData> quests;
 std::map<std::pair<uint32,uint32>,int32> data;std::vector<std::function<void()>> events;std::function<void()> castHook;
 Player* ToPlayer()override{return this;} std::function<Creature*(uint32)> summonHook; std::vector<WorldObject*> boundObjects; uint32 cancels=0; using Unit::CastSpell; using Unit::SummonCreature; void UpdateVisibilityForPlayer(){} void KillCreditGO(uint32,ObjectGuid const&); void BindQuestObject(uint32 q,WorldObject* o){auto& scope=quests[q].ScriptObjects;if(!scope)scope=std::make_shared<QuestObjectScope>();o->questScope=scope;o->bound=true;boundObjects.push_back(o);} void CancelQuestObjects(uint32 q){++cancels;quests[q].ScriptObjects.reset();for(auto o:boundObjects)o->expired=true;boundObjects.clear();}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}
 uint32 GetTeam(){return team;}float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return 1.57336f;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}bool IsBeingTeleported(){return teleport;}
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
 Achievement achievement;Achievement* m_achievementMgr=&achievement;Group* group=nullptr;Group* GetGroup(){return group;}void RewardPlayerAndGroupAtEvent(uint32,WorldObject*);bool TalkedToCreature(uint32,ObjectGuid){return false;}bool rewardRange=true;bool IsAtGroupRewardDistance(WorldObject*){return rewardRange;}bool corpse=false;bool GetCorpse(){return corpse;}uint32 GetGuildId(){return 0;}template<class... T>void UpdateAchievementCriteria(T...){}void AddKillCreature(uint32,uint16,bool){}void AddTrackingQuestIfNeeded(ObjectGuid){}void SetQuestStatus(uint32 q,int state){quests[q].Status=state;}void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty);FakeMap* GetMap()const{static FakeMap m;return &m;}bool CanGetItemForLoot(void const*,bool)const{return true;}
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
 virtual void sQuestAccept(Player*,Quest const*){++delegated;}virtual void sGossipSelect(Player*,uint32,uint32){++delegated;} virtual void JustSummoned(Creature*){++delegated;} virtual void JustDied(Unit*){++delegated;} void Talk(uint8,ObjectGuid){++me->talks;} virtual void DoAction(int32){}virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual ObjectGuid GetGUID(int32){return {};}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){++delegated;}
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

struct ObjectMgr {CreatureTemplate data;CreatureTemplate const* GetCreatureTemplate(uint32){return &data;}std::string GetScriptName(uint32 id){return id==1?"npc_stormheim_another_way_guide":"custom";}} objectMgr;auto sObjectMgr=&objectMgr;
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogin(Player*){}virtual void OnLogout(Player*){}virtual void OnUpdate(Player*,uint32){}};
#include "KilledMonster.inc"
#include "AnotherWay.inc"
int Player::CastSpell(Player*,uint32 id,bool){assert(id==205485||id==205487);if(castResult)return castResult;auto entry=id==205485?91558u:92569u;auto* guide=new Creature;static uint32 next=1000;guide->guid={++next};guide->entry=entry;guide->temporary=true;guide->fields[UNIT_FIELD_NPC_FLAGS]=2;guide->distance=20;creatures[guide->guid]=guide;ownedGuides[entry].push_back(guide->guid);auto* ai=new npc_stormheim_another_way_guide(guide);ai->IsSummonedBy(this);return 0;}
struct Run {Player p;player_stormheim_another_way script;uint32 quest;Run(uint32 q=38342):quest(q){static uint32 nextPlayer=10;p.guid={++nextPlayer};p.team=q==38342?ALLIANCE:HORDE;p.race=q==38342?1:2;p.Accept(q);players[p.guid]=&p;StormheimAnotherWay::timers.erase(p.guid);}void Tick(){script.OnUpdate(&p,1000);}Creature* Guide(){auto entry=quest==38342?91558u:92569u;return p.ownedGuides[entry].empty()?nullptr:creatures[p.ownedGuides[entry].back()];}void Kill(){CreatureTemplate boss;Creature enemy;enemy.entry=91240;enemy.guid={100};creatures[enemy.guid]=&enemy;p.KilledMonster(&boss,enemy.guid);creatures.erase(enemy.guid);} };
int main(){
 for(auto q:{38342u,38617u}){auto& definition=store.quests[q];definition.id=q;definition.special=QUEST_SPECIAL_FLAGS_KILL;definition.objectives={{0,91240,1,1,0},{0,98255,1,2,1}};}
 for(auto q:{38342u,38617u}){Run r(q);r.Tick();assert(!r.p.GetQuestObjectiveData(q,98255)&&r.Guide());r.Kill();r.Tick();assert(r.p.GetQuestObjectiveData(q,91240)==1&&r.p.GetQuestObjectiveData(q,98255)==1&&r.p.GetQuestStatus(q)==QUEST_STATUS_COMPLETE);auto packets=r.p.packets;r.Tick();assert(r.p.packets==packets&&r.p.ownedGuides[q==38342?91558:92569].size()==1);}
 for(int f=0;f<10;++f){Run r;r.Kill();if(f==0)r.p.z=300.555f;if(f==1)r.p.z=337.4f;if(f==2)r.p.x=2360;if(f==3)r.p.movement=MOVEMENTFLAG_FALLING;if(f==4)r.p.movement=MOVEMENTFLAG_FLYING;if(f==5)r.p.spline.done=false;if(f==6)r.p.vehicle=true;if(f==7)r.p.transport=true;if(f==8)r.p.alive=false;if(f==9)r.p.GetMap()->floor=300;r.Tick();assert(!r.p.GetQuestObjectiveData(38342,98255));r.p.GetMap()->floor=335.429f;}
 {Run r;r.p.castResult=1;r.Kill();r.Tick();assert(r.p.GetQuestStatus(38342)==QUEST_STATUS_COMPLETE&&!r.Guide());r.p.castResult=0;r.Tick();assert(r.Guide());r.Tick();assert(r.Guide()->motion.follows==1);}
 {Run r;r.Tick();auto* old=r.Guide();r.p.Accept(38342);assert(!old->ai->GetData(StormheimAnotherWay::ValidKey));r.Tick();assert(r.Guide()!=old);r.p.quests.erase(38342);r.Guide()->ai->UpdateAI(1);assert(!r.Guide()->world);}
 {Run r;r.Tick();auto* guide=r.Guide();r.p.m_RewardedQuests.insert(38342);r.p.quests.erase(38342);assert(guide->ai->GetData(StormheimAnotherWay::ValidKey));r.p.m_RewardedQuests.insert(38412);guide->ai->UpdateAI(1);assert(!guide->world);}
 {Run r;r.Tick();auto* guide=r.Guide();r.p.world=false;guide->ai->UpdateAI(1);assert(!guide->world);r.script.OnLogout(&r.p);assert(!StormheimAnotherWay::timers.count(r.p.guid));}
 {Run r;Unit pet;pet.ownerPlayer=&r.p;CreatureTemplate boss;pet.GetCharmerOrOwnerPlayerOrPlayerItself()->KilledMonster(&boss,ObjectGuid::Empty);assert(r.p.GetQuestObjectiveData(38342,91240)==1);r.Tick();assert(r.p.GetQuestStatus(38342)==QUEST_STATUS_COMPLETE);}
 {Run r;r.p.castResult=1;for(int i=0;i<10;++i)r.script.OnUpdate(&r.p,10);assert(StormheimAnotherWay::timers[r.p.guid]==910);}
 {Run r;r.p.quests[38342].Status=QUEST_STATUS_COMPLETE;r.script.OnLogin(&r.p);assert(r.p.GetQuestStatus(38342)==QUEST_STATUS_INCOMPLETE);}
 {Run r;r.p.map=1;r.Kill();r.Tick();assert(!r.Guide()&&!r.p.GetQuestObjectiveData(38342,98255));}
 {Run r;r.p.instance=1;r.Kill();r.Tick();assert(!r.Guide()&&!r.p.GetQuestObjectiveData(38342,98255));}
 {Run owner,other;owner.Tick();auto* guide=owner.Guide();other.p.ownedGuides[91558].push_back(guide->guid);other.Tick();assert(other.Guide()!=guide);assert(guide->ai->GetGUID(StormheimAnotherWay::OwnerKey)==owner.p.guid);other.p.quests.erase(38342);other.Guide()->ai->UpdateAI(1);assert(!other.Guide()->world&&guide->world);}
 {Run r;objectMgr.data.ScriptID=2;r.Kill();r.Tick();assert(r.p.GetQuestStatus(38342)==QUEST_STATUS_COMPLETE&&!r.Guide());objectMgr.data.ScriptID=1;}
 {Run r;store.quests[38342].objectives[1].Flags=0;r.Kill();r.Tick();assert(!r.Guide()&&!r.p.GetQuestObjectiveData(38342,98255));store.quests[38342].objectives[1].Flags=2;}
 for(auto q:{38342u,38617u}){Run r(q);r.Kill();r.Tick();auto packets=r.p.packets;r.script.OnLogin(&r.p);assert(r.p.GetQuestStatus(q)==QUEST_STATUS_COMPLETE&&r.p.packets==packets);}
 {Creature staticGuide;staticGuide.entry=91558;npc_stormheim_another_way_guide ai(&staticGuide);Run r;ai.IsSummonedBy(&r.p);ai.UpdateAI(1);assert(!ai.scoped&&ai.delegated==2&&staticGuide.world);}
 std::cout<<"PASS: production tower/guide/repair handlers and native kill/completion; both factions, boss-first roof/grounded/spline gates, failed summon retry, pets, once-only credit, acceptance lifetime, handoff, logout and throttle.\n";
}
