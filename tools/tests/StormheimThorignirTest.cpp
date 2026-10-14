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
struct Player:Unit {uint64 race=1;uint64 getRaceMask(){return race;}uint32 GetInstanceId(){return 0;}Quest const* GetNextQuest(ObjectGuid,Quest const*);PlayerMenu menu;PlayerMenu* PlayerTalkClass=&menu;using Unit::GetAura;using Unit::RemoveAurasDueToSpell;
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
 int CastSpell(Player*,uint32 a,bool){if(!castResult){auras.insert(a);if(castHook)castHook();}return castResult;}
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




using Map=FakeMap;
struct MapMgr {Map* FindMap(uint32,uint32){static Map m;return &m;}} mapMgr;auto sMapMgr=&mapMgr;
namespace ObjectAccessor {Creature* GetCreatureOrPetOrVehicle(WorldObject& source,ObjectGuid g){return GetCreature(source,g);}}
#include "NextQuest.inc"
#include "Race.inc"
int main(){
 for(uint32 id:{38459u,39597u,38473u,38611u,38312u,38612u,38331u,39590u})store.quests[id].id=id;
 auto& blessing=store.quests[39597];blessing.objectives={{0,96285,1,0,5}};blessing.NextQuestIdChain=38473;
 auto& alliance=store.quests[38473];alliance.AllowableRaces=824181837;alliance.prevQuests={39597};alliance.NextQuestIdChain=38312;
 auto& horde=store.quests[38611];horde.AllowableRaces=234881970;horde.NextQuestIdChain=38612;
 auto& grappleA=store.quests[38312];grappleA.AllowableRaces=alliance.AllowableRaces;grappleA.prevQuests={38473};grappleA.objectives={{1,138111,1,4,0}};
 auto& grappleH=store.quests[38612];grappleH.AllowableRaces=horde.AllowableRaces;grappleH.prevQuests={38611};grappleH.objectives={{1,138111,1,4,0}};
 for(auto relation:std::vector<std::pair<uint32,uint32>>{{96254,38473},{96254,38611},{96254,39590},{91222,38312},{92561,38612}})store.starters.insert(relation);
 Creature havi;havi.entry=96254;havi.guid={100};creatures[havi.guid]=&havi;Creature lorna;lorna.entry=91222;lorna.guid={101};creatures[lorna.guid]=&lorna;Creature cullen;cullen.entry=92561;cullen.guid={102};creatures[cullen.guid]=&cullen;
 // Reproduce the existing early Horde offer through the real prerequisite
 // handler: rewarding38459 alone exposes38611. The repaired gate rejects it.
 Player early;early.team=HORDE;early.race=2;early.m_RewardedQuests.insert(38459);horde.prevQuests={38459};assert(early.CanTakeQuest(&horde,false));horde.prevQuests={39597};assert(!early.CanTakeQuest(&horde,false));
 // Before this scoped selection the configured next38473 is rejected by the
 // real Horde race check; fallback paths still demonstrate that exact failure.
 Player old;old.team=HORDE;old.race=2;old.m_RewardedQuests.insert(39597);old.zone=1;auto wrong=old.GetNextQuest(havi.guid,&blessing);assert(wrong==&alliance&&!old.CanTakeQuest(wrong,false));old.zone=7541;auto right=old.GetNextQuest(havi.guid,&blessing);assert(right==&horde&&old.CanTakeQuest(right,false));
 for(auto faction:{ALLIANCE,HORDE}){Player p;p.team=faction;p.race=faction==ALLIANCE?1:2;auto next=faction==ALLIANCE?&alliance:&horde;auto other=faction==ALLIANCE?&horde:&alliance;auto receiver=faction==ALLIANCE?&lorna:&cullen;auto grapple=faction==ALLIANCE?&grappleA:&grappleH;
  assert(p.GetNextQuest(havi.guid,&blessing)==next&&!p.CanTakeQuest(next,false)&&!p.CanTakeQuest(other,false));p.Accept(39597);assert(!p.CanCompleteQuest(39597));p.data[{39597,96285}]=1;p.CompleteQuest(39597);assert(!p.CanTakeQuest(next,false));p.m_RewardedQuests.insert(39597);assert(p.CanTakeQuest(next,false)&&!p.CanTakeQuest(other,false));
  // The native empty-objective breadcrumb completes on acceptance and still
  // requires physical hand-in/reward before its next quest becomes eligible.
  assert(next->objectives.empty());p.Accept(next->id);assert(p.CanCompleteQuest(next->id));p.CompleteQuest(next->id);assert(p.CanRewardQuest(next,false));assert(!p.CanTakeQuest(grapple,false));assert(p.GetNextQuest(receiver->guid,next)==grapple);assert(!p.GetNextQuest(havi.guid,next));p.m_RewardedQuests.insert(next->id);assert(p.CanTakeQuest(grapple,false));
  // Abandon/re-entry does not remove the shared blessing reward gate.
  p.quests[next->id].Status=QUEST_STATUS_NONE;p.m_RewardedQuests.erase(next->id);assert(p.CanTakeQuest(next,false));p.Accept(next->id);assert(p.CanCompleteQuest(next->id));p.CompleteQuest(next->id);assert(p.CanRewardQuest(next,false));
 }
 // Preserve the live allied-race extensions. Every admitted mask bit passes
 // the native race handler for its own branch and fails the opposite branch.
 for(uint32 bit=0;bit<64;++bit){Player p;p.race=uint64(1)<<bit;assert(p.SatisfyQuestRace(&alliance,false)==bool(alliance.AllowableRaces&p.race));assert(p.SatisfyQuestRace(&horde,false)==bool(horde.AllowableRaces&p.race));}
 // Other quest IDs, sources, configured links and later-zone uses continue
 // through the normal relation loop. Missing actors/relations fail closed.
 {Player p;p.team=HORDE;p.race=2;auto& other=store.quests[38331];other.NextQuestIdChain=39590;assert(p.GetNextQuest(havi.guid,&other)==&store.quests[39590]);auto& gift=store.quests[39597];gift.NextQuestIdChain=39590;assert(p.GetNextQuest(havi.guid,&gift)==&store.quests[39590]);gift.NextQuestIdChain=38473;assert(!p.GetNextQuest(lorna.guid,&gift));havi.entry=1;assert(!p.GetNextQuest(havi.guid,&gift));havi.entry=96254;p.map=1522;assert(p.GetNextQuest(havi.guid,&gift)==&alliance);p.map=1220;havi.world=false;assert(!p.GetNextQuest(havi.guid,&gift));havi.world=true;auto range=store.starters.equal_range(96254);for(auto i=range.first;i!=range.second;){if(i->second==38611)i=store.starters.erase(i);else ++i;}assert(!p.GetNextQuest(havi.guid,&gift));}
 // Auto-complete player and gameobject query routes retain the configured
 // default. The faction override is limited to the exact Havi creature path.
 {Player p;p.team=HORDE;p.race=2;ObjectGuid actor{500};actor.high=HighGuid::Player;blessing.flags=QUEST_FLAGS_AUTOCOMPLETE;assert(p.GetNextQuest(actor,&blessing)==&alliance);blessing.flags=0;GameObject object;object.guid={501};object.guid.high=HighGuid::GameObject;object.entry=96254;objects[object.guid]=&object;assert(p.GetNextQuest(object.guid,&blessing)==&alliance);objects.erase(object.guid);}
 std::cout<<"PASS: full production GetNextQuest, prerequisite/race/empty-objective completion/reward handlers reproduce early Horde gate and wrong Alliance next; both faction handoffs, allied-race masks, hand-in/next eligibility, abandon/re-entry and scoped/unrelated/missing-source behavior.\n";
}
