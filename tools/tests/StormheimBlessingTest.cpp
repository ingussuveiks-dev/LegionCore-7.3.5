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
struct ObjectGuid {using LowType=uint64;uint64 value=0; static ObjectGuid const Empty; bool IsEmpty()const{return !value;}bool operator==(ObjectGuid o)const{return value==o.value;}bool operator<(ObjectGuid other)const{return value<other.value;}};
ObjectGuid const ObjectGuid::Empty{};
struct QuestObjective {int Type=0;uint32 ObjectID=0,Amount=1,Flags=0;int8 StorageIndex=0;};
struct Quest {uint32 id=0,special=0;int32 ExclusiveGroup=0;PrevQuests prevQuests;std::vector<QuestObjective> objectives;
 uint32 GetQuestId()const{return id;}bool IsDFQuest()const{return false;}auto const& GetObjectives()const{return objectives;}bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}
 bool IsRepeatable()const{return false;}bool IsAutoComplete()const{return false;}bool HasSpecialFlag(uint32 f)const{return special&f;}bool IsAllowedInRaid(int)const{return true;}};
struct QuestStore {std::map<uint32,Quest> quests;ExclusiveQuestGroups mExclusiveQuestGroups;
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
struct FakeMap {bool IsDungeon(){return false;}int GetDifficultyID(){return 0;}Creature* GetCreature(ObjectGuid g){return creatures.count(g)?creatures[g]:nullptr;}};
struct GroupReference {Player* player=nullptr;GroupReference* following=nullptr;Player* getSource(){return player;}GroupReference* next(){return following;}};
struct Group {GroupReference* first=nullptr;GroupReference* GetFirstMember(){return first;}bool isRaidGroup(){return false;}};
struct Guild {template<class... T>void UpdateAchievementCriteria(T...) {}};
struct GuildMgr {Guild* GetGuildById(uint32){return nullptr;}} guildMgr;auto sGuildMgr=&guildMgr;
struct Achievement {void StartTimedAchievement(int,uint32){}};
struct GossipMenu {uint32 menu=18672;bool item=true;uint32 GetMenuId(){return menu;}void* GetItem(uint32 id){return id==0&&item?this:nullptr;}};
struct PlayerMenu {GossipMenu menu;uint32 closed=0;GossipMenu& GetGossipMenu(){return menu;}void ClearMenus(){menu.item=false;}void SendCloseGossip(){++closed;}};
struct Player:Unit {PlayerMenu menu;PlayerMenu* PlayerTalkClass=&menu;using Unit::GetAura;using Unit::RemoveAurasDueToSpell;
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
 bool SatisfyQuestRace(Quest const*,bool){return true;}void SendCanTakeQuestResponse(int,Quest const*,char const*){}
 bool SatisfyQuestPreviousQuest(Quest const*,bool);bool CanCompleteQuest(uint32);bool CanTakeQuest(Quest const* q,bool b){return SatisfyQuestPreviousQuest(q,b);}
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
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}virtual GameObjectAI* GetAI(GameObject*)const{return nullptr;}};



using CreatureAI=SmartAI;
struct CreatureScript {explicit CreatureScript(char const*){}virtual bool OnGossipHello(Player*,Creature*){return false;}virtual CreatureAI* GetAI(Creature*)const{return nullptr;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogout(Player*){}};
struct AuraScript {Unit* caster=nullptr;Unit* target=nullptr;Aura app;Hook AfterEffectApply,AfterEffectRemove;Unit* GetCaster(){return caster;}Unit* GetTarget(){return target;}Aura* GetTargetApplication(){return &app;}virtual void Register(){}};
#define PrepareAuraScript(x) public:
#define AuraEffectApplyFn(F,I,A,M) (10000+(I)*1000+(A)*10+(M))
#define AuraEffectRemoveFn(F,I,A,M) (20000+(I)*1000+(A)*10+(M))
#include "Blessing.inc"
std::function<Creature*(Unit&,uint32)> spawn;uint32 failCreature=0,failSpell=0;int missingAura=0;
Creature* Unit::SummonCreature(uint32 id,float,float,float,float,int,uint32){if(id==failCreature)return nullptr;if(auto p=ToPlayer())return p->summonHook?p->summonHook(id):nullptr;return spawn?spawn(*this,id):nullptr;}
GameObject* Unit::SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32){return nullptr;}
struct Session {Creature me;npc_stormheim_blessing_actor ai;Session(Player& p,uint64 id):ai(&me){me.entry=96257;me.guid={id};creatures[me.guid]=&me;me.fields[UNIT_FIELD_NPC_FLAGS]=3;ai.IsSummonedBy(&p);}~Session(){creatures.erase(me.guid);}};
struct Run {
 Player p;Creature source;npc_stormheim_blessing_actor publicAI;npc_stormheim_blessing gossip;std::vector<std::unique_ptr<Session>> actors;uint64 n;uint32 spawned=0;
 Run(uint64 id=1):publicAI(&source),n(id*1000){p.guid={id};p.Accept(39597);players[p.guid]=&p;source.guid={id*100000};source.entry=96257;source.fields[UNIT_FIELD_NPC_FLAGS]=3;creatures[source.guid]=&source;p.summonHook=[this](uint32 entry){assert(entry==96257);++spawned;auto session=std::make_unique<Session>(p,++n);auto me=&session->me;me->castTargetHook=[this,me](Unit* target,uint32 spell){assert(target==&p);if(spell==failSpell)return SPELL_FAILED_BAD_TARGETS;if(spell==190904){if(missingAura!=1)me->roots[{spell,me->guid.value}]={};if(missingAura!=2)p.roots[{spell,me->guid.value}]={};}if(spell==190905&&missingAura!=3){p.roots[{spell,me->guid.value}]={};aura_stormheim_blessing_finish hook;hook.caster=me;hook.target=&p;hook.Applied(nullptr,1);}return SPELL_CAST_OK;};actors.push_back(std::move(session));return me;};}
 npc_stormheim_blessing_actor* Actor(){return actors.empty()?nullptr:&actors.back()->ai;}
 void Offer(){p.menu.menu.item=true;p.menu.menu.menu=18672;assert(!gossip.OnGossipHello(&p,&source));}
 void Select(uint32 menu=18672,uint32 slot=0){publicAI.sGossipSelect(&p,menu,slot);}
 void Start(){Offer();Select();auto a=Actor();assert(a&&a->scoped&&a->stage==0&&a->me->talks==1&&source.hidden.count(p.guid)&&!a->me->HasFlag(UNIT_FIELD_NPC_FLAGS,3));}
 void Begin(){auto a=Actor();a->UpdateAI(5999);assert(a->me->casts.empty()&&!p.packets);a->UpdateAI(1);assert(a->stage==1&&a->me->casts==std::vector<uint32>{190904}&&p.GetAura(190904,a->me->guid));a->UpdateAI(5999);assert(a->stage==1&&!p.packets);a->UpdateAI(1);assert(a->stage==2&&a->applied&&!a->expired&&p.GetAura(190905,a->me->guid)&&!p.GetAura(190904,a->me->guid)&&!a->me->GetAura(190904,a->me->guid));}
 void Expire(int mode=AURA_REMOVE_BY_EXPIRE){auto a=Actor();aura_stormheim_blessing_finish hook;hook.caster=a->me;hook.target=&p;hook.app.mode=mode;hook.Removed(nullptr,1);p.RemoveAurasDueToSpell(190905,a->me->guid);}
 void Finish(){auto a=Actor();Expire();a->UpdateAI(5999);assert(!p.packets);a->UpdateAI(1);assert(a->stopped&&p.GetQuestStatus(39597)==QUEST_STATUS_COMPLETE&&p.packets==1&&p.completions==1&&source.hidden.empty()&&p.roots.empty());}
 ~Run(){actors.clear();players.erase(p.guid);creatures.erase(source.guid);StormheimBlessing::offers.erase(p.guid);}
};
int main(){
 for(uint32 id:{39594u,39597u,38473u})store.quests[id].id=id;
 auto& q=store.quests[39597];q.special=QUEST_SPECIAL_FLAGS_KILL;q.prevQuests={39594};q.objectives={{0,96285,1,0,5}};store.quests[38473].prevQuests={39597};
 for(auto faction:{ALLIANCE,HORDE})for(uint32 source:{96257u,96258u}){Run r;r.p.team=faction;r.source.entry=source;assert(!r.p.CanTakeQuest(&q,false));r.p.m_RewardedQuests.insert(39594);assert(r.p.CanTakeQuest(&q,false));r.publicAI.sQuestAccept(&r.p,&q);r.publicAI.UpdateAI(18000);assert(!r.Actor()&&!r.p.packets&&r.publicAI.delegated==1);r.Select();assert(!r.Actor());r.Start();auto count=r.spawned;r.Select();r.Offer();r.Select();assert(r.spawned==count);r.Begin();r.Finish();assert(r.p.CanRewardQuest(&q,false)&&!r.p.CanTakeQuest(&store.quests[38473],false));r.p.m_RewardedQuests.insert(39597);assert(r.p.CanTakeQuest(&store.quests[38473],false));auto packets=r.p.packets;r.Actor()->UpdateAI(50000);assert(r.p.packets==packets);}
 {Run r;r.Offer();r.Select(18672,1);assert(!r.Actor());r.p.menu.menu.menu=123;r.Select();assert(!r.Actor());r.p.menu.menu.menu=18672;r.p.menu.menu.item=false;r.Select();assert(!r.Actor());}
 for(int mode=0;mode<9;++mode){Run r;r.Offer();if(mode==0)r.p.phase=false;if(mode==1)r.source.distance=6;if(mode==2)r.p.alive=false;if(mode==3)r.p.quests[39597].Status=QUEST_STATUS_NONE;if(mode==4)r.source.world=false;if(mode==5)r.source.fields[UNIT_FIELD_NPC_FLAGS]=2;if(mode==6)r.p.los=false;if(mode==7)r.p.visible=false;if(mode==8)r.p.teleport=true;r.Select();assert(!r.Actor());}
 {Run r;r.Offer();r.p.Accept(39597);r.Select();assert(!r.Actor());r.Offer();r.Select();assert(r.Actor());}
 for(uint32 spell:{190904u,190905u}){Run r;r.Start();failSpell=spell;r.Actor()->UpdateAI(6000);r.Actor()->UpdateAI(6000);assert(r.Actor()->stopped&&!r.p.packets&&r.p.roots.empty()&&r.source.hidden.empty());failSpell=0;r.Start();assert(!r.Actor()->stopped);}
 for(int fail=1;fail<4;++fail){Run r;r.Start();missingAura=fail;r.Actor()->UpdateAI(6000);r.Actor()->UpdateAI(6000);assert(r.Actor()->stopped&&!r.p.packets&&r.p.roots.empty());missingAura=0;}
 {Run r;failCreature=96257;r.Offer();r.Select();assert(!r.Actor()&&r.source.hidden.empty());failCreature=0;r.Start();assert(r.Actor());}
 for(int mode:{AURA_REMOVE_BY_DEFAULT,AURA_REMOVE_BY_ENEMY_SPELL}){Run r;r.Start();r.Begin();r.Expire(mode);r.Actor()->UpdateAI(6000);assert(r.Actor()->stopped&&!r.p.packets&&r.p.roots.empty());}
 {Run r;r.Start();r.Begin();r.Actor()->UpdateAI(6000);assert(!r.p.packets&&!r.Actor()->stopped);r.Expire();r.Actor()->UpdateAI(100);assert(r.p.packets==1);}
 for(int mode=0;mode<8;++mode){Run r;r.Start();r.Actor()->UpdateAI(6000);auto old=r.p.quests[39597].ScriptObjects;if(mode==0)r.p.quests[39597].Status=QUEST_STATUS_NONE;if(mode==1)r.p.alive=false;if(mode==2)r.p.map=1;if(mode==3)r.p.phase=false;if(mode==4)r.p.world=false;if(mode==5)r.Actor()->me->distance=16;if(mode==6)r.p.teleport=true;if(mode==7)r.Actor()->me->alive=false;r.Actor()->UpdateAI(6000);assert(r.Actor()->stopped&&old->cancelled&&!r.p.packets&&r.p.roots.empty()&&r.source.hidden.empty());}
 {Run r;r.Start();r.Actor()->UpdateAI(6000);r.p.roots[{190904,999}]={};player_stormheim_blessing cleanup;cleanup.OnLogout(&r.p);assert(r.Actor()->stopped&&r.source.hidden.empty()&&r.p.roots.size()==1&&r.p.GetAura(190904,{999}));r.p.roots.clear();r.Start();r.Begin();r.Finish();}
 {Run r;r.Start();auto old=r.Actor();old->UpdateAI(6000);r.p.Accept(39597);r.Start();auto fresh=r.Actor();fresh->UpdateAI(6000);auto cancelled=r.p.cancels;old->UpdateAI(6000);assert(old->stopped&&!fresh->stopped&&r.p.cancels==cancelled&&r.p.GetAura(190904,fresh->me->guid)&&r.source.hidden.count(r.p.guid));r.actors.erase(r.actors.begin());assert(r.source.hidden.count(r.p.guid));fresh->UpdateAI(6000);r.Finish();}
 {Run r;r.Start();Unit pet;pet.ownerPlayer=&r.p;r.Actor()->JustDied(&pet);assert(r.p.roots.empty()&&!r.p.packets);r.Start();r.Actor()->UpdateAI(60000);assert(r.Actor()->stopped&&!r.p.packets);}
 {Run a(1);a.Start();Run b(2);b.Start();a.Begin();b.Begin();a.Finish();assert(!b.Actor()->stopped&&b.p.GetAura(190905,b.Actor()->me->guid));b.Finish();}
 {Run r;r.Start();r.Begin();Player stranger;stranger.guid={999};aura_stormheim_blessing_finish hook;hook.caster=r.Actor()->me;hook.target=&stranger;hook.app.mode=AURA_REMOVE_BY_EXPIRE;hook.Removed(nullptr,1);assert(!r.Actor()->expired);r.Actor()->UpdateAI(60000);assert(!r.p.packets);}
 {Run r;r.source.map=1522;r.publicAI.sQuestAccept(&r.p,&q);r.publicAI.sGossipSelect(&r.p,18672,0);r.publicAI.UpdateAI(1);assert(r.publicAI.delegated==3);Session arbitrary(r.p,999);assert(!arbitrary.ai.scoped);arbitrary.ai.UpdateAI(60000);assert(arbitrary.ai.delegated==2);}
 {aura_stormheim_blessing_finish hook;hook.Register();assert(hook.AfterEffectApply.values==std::vector<int>{10261}&&hook.AfterEffectRemove.values==std::vector<int>{20261});}
 std::cout<<"PASS: production readiness/owner-scoped ceremony, both native root casts and real aura expiry before exact slot5 completion; both factions, duplicate/stale/forged menus, failed casts/applications/summons, cancellation versus expiry, logout/map/phase/death/timeout, new acceptance isolation, exact-caster root cleanup and unrelated SmartAI.\n";
}
