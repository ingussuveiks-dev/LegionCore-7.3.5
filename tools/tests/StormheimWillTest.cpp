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
using QuestObjectiveType=int;
using PrevQuests=std::vector<int32>; using ExclusiveQuestGroups=std::multimap<int32,uint32>;
#define ASSERT assert
#define TC_LOG_DEBUG(...) (void)0
#define TC_LOG_ERROR(...) (void)0
struct ObjectGuid {uint64 value=0; static ObjectGuid const Empty; bool IsEmpty()const{return !value;}bool operator==(ObjectGuid o)const{return value==o.value;}bool operator<(ObjectGuid other)const{return value<other.value;}};
ObjectGuid const ObjectGuid::Empty{};
struct QuestObjective {int Type=0;uint32 ObjectID=0,Amount=1,Flags=0;int8 StorageIndex=0;};
struct Quest {uint32 id=0,special=0;int32 ExclusiveGroup=0;PrevQuests prevQuests;std::vector<QuestObjective> objectives;
 uint32 GetQuestId()const{return id;}bool IsDFQuest()const{return false;}auto const& GetObjectives()const{return objectives;}bool IsDailyOrWeekly()const{return false;}bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}
 bool IsRepeatable()const{return false;}bool IsAutoComplete()const{return false;}bool HasSpecialFlag(uint32 f)const{return special&f;}bool IsAllowedInRaid(int)const{return true;}};
struct QuestStore {std::map<uint32,Quest> quests;ExclusiveQuestGroups mExclusiveQuestGroups;
 Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}} store;
auto sQuestDataStore=&store; enum {TEMPSUMMON_TIMED_DESPAWN=1,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN=2,TEMPSUMMON_MANUAL_DESPAWN=8};
struct QuestObjectScope {bool cancelled=false;void Cancel(){cancelled=true;}};
struct QuestStatusData {std::shared_ptr<QuestObjectScope> ScriptObjects;int Status=QUEST_STATUS_NONE;uint32 Timer=0;std::shared_ptr<uint8> ScriptLifetime;};

struct Unit;struct Player;struct Creature;struct GameObject;struct SmartAI;
struct Vehicle {struct Seat {struct {ObjectGuid Guid;} Passenger;};std::map<int,Seat> Seats;struct Unit* GetPassenger(int);};
struct WorldObject {ObjectGuid guid{1};std::weak_ptr<QuestObjectScope> questScope;bool bound=false,expired=false; bool HasQuestObjectScope(){return bound;} bool IsQuestObjectExpired(){auto scope=questScope.lock();return expired||(bound&&(!scope||scope->cancelled)); } uint32 map=1220,zone=7541,area=7557,entry=0;bool world=true,alive=true;float distance=0;
 bool IsWithinDistInMap(WorldObject* object,float range){return object->distance<=range;}virtual bool IsPlayer(){return ToPlayer()!=nullptr;}virtual bool IsGameObject(){return !ToUnit();}WorldObject* ToWorldObject(){return this;}virtual ~WorldObject()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}virtual Unit* ToUnit(){return nullptr;}virtual bool IsCreature(){return false;}float GetPositionX(){return 3442.69f;}float GetPositionY(){return 2103.1f;}float GetPositionZ(){return 277.243f;}float GetOrientation(){return 1.57336f;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}uint32 GetAreaId(){return area;}virtual ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}};
struct Unit:WorldObject {Player* ownerPlayer=nullptr;Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():ownerPlayer;}int castResult=0; std::vector<uint32> casts; std::function<void(uint32)> castHook; int CastSpell(Unit*,uint32 id,bool){casts.push_back(id);if(!castResult&&castHook)castHook(id);return castResult;} Creature* SummonCreature(uint32,float,float,float,float,int,uint32); GameObject* SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32);Vehicle* kit=nullptr;Unit* ToUnit()override{return this;}Vehicle* GetVehicleKit(){return kit;}bool InSamePhase(WorldObject*){return true;}virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct SpellInfo {uint32 Id=180737;};
struct CreatureTemplate {uint32 StateWorldEffectID=0,SpellStateVisualID=45944,SpellStateAnimID=0,SpellStateAnimKitID=6050;};
struct Creature:Unit {SmartAI* ai=nullptr;SmartAI*& i_AI=ai;SmartAI* i_disabledAI=nullptr;bool charmed=false;bool IsPlayer(){return false;}bool isCharmed(){return charmed;}bool isPossessed(){return false;}bool IsVehicle(){return kit!=nullptr;}void UpdateCharmAI();std::vector<std::function<void()>> callbacks;void AddDelayedEvent(uint64,std::function<void()> f){callbacks.push_back(std::move(f));}void Tick(){auto q=std::move(callbacks);callbacks.clear();for(auto& f:q)f();}CreatureTemplate data;std::map<int,uint32> fields;uint32 stand=8,dirty=0,talks=0;bool visible=true,combat=false;
 Creature* ToCreature()override{return this;}bool IsCreature()override{return true;}Player* GetLootRecipient(){return nullptr;}void AddPlayerInPersonnalVisibilityList(ObjectGuid p){personal=p;}ObjectGuid personal;void setActive(bool){}void DespawnOrUnsummon(){world=false;}SmartAI* AI(){return ai;}CreatureTemplate const* GetCreatureTemplate(){return &data;}
 bool HasFlag(int f,uint32 v){return fields[f]&v;}void SetFlag(int f,uint32 v){fields[f]|=v;}void RemoveFlag(int f,uint32 v){fields[f]&=~v;}
 void SetUInt32Value(int f,uint32 v){fields[f]=v;}void ForceValuesUpdateAtIndex(int){++dirty;}void SetVisible(bool v){visible=v;}void SetStandState(uint32 s){stand=s;}
 void RemoveAurasDueToSpell(uint32){}bool HasUnitState(int){return false;}uint32 GetCreatureType(){return 7;}};
struct GameObject:WorldObject {std::set<ObjectGuid> hidden;void AddToHideList(ObjectGuid id){hidden.insert(id);}void RemoveFromHideList(ObjectGuid id){hidden.erase(id);}void DestroyForPlayer(Player*){}uint32 flags=0,state=GO_READY;bool spawned=true;bool isSpawned(){return spawned;}bool HasFlag(int,uint32 f){return flags&f;}};
std::map<ObjectGuid,Creature*> creatures;
std::map<ObjectGuid,Player*> players;std::map<ObjectGuid,GameObject*> objects;
namespace ObjectAccessor {Player* GetPlayer(WorldObject&,ObjectGuid);Player* FindPlayer(ObjectGuid);GameObject* GetGameObject(WorldObject&,ObjectGuid);Creature* GetCreature(WorldObject&,ObjectGuid id){auto i=creatures.find(id);return i==creatures.end()||!i->second->world?nullptr:i->second;}}
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,WorldObject*,uint32 entry,float){for(auto const& pair:creatures)if(pair.second->entry==entry)out.push_back(pair.second);}
struct FakeMap {bool IsDungeon(){return false;}int GetDifficultyID(){return 0;}Creature* GetCreature(ObjectGuid g){return creatures.count(g)?creatures[g]:nullptr;}};
struct GroupReference {Player* player=nullptr;GroupReference* following=nullptr;Player* getSource(){return player;}GroupReference* next(){return following;}};
struct Group {GroupReference* first=nullptr;GroupReference* GetFirstMember(){return first;}bool isRaidGroup(){return false;}};
struct Guild {template<class... T>void UpdateAchievementCriteria(T...) {}};
struct GuildMgr {Guild* GetGuildById(uint32){return nullptr;}} guildMgr;auto sGuildMgr=&guildMgr;
struct Achievement {void StartTimedAchievement(int,uint32){}};
enum {QUEST_DEFAULT_SAVE_TYPE=0,QUEST_STATE_COMPLETE=1,SCRIPT_COMMAND_QUEST_EXPLORED=7};
namespace WorldPackets {namespace Quest {struct QuestUpdateAddCreditSimple {uint32 QuestID,ObjectID;int ObjectiveType;int Write(){return 0;}};}}
struct TalkClass {template<class... T> void SendQuestGiverRequestItems(T...) {}} talkClass;
struct Player:Unit {std::map<uint32,uint32> m_QuestStatusSave;TalkClass* PlayerTalkClass=&talkClass;void AreaExploredOrEventHappens(uint32);uint16 FindQuestSlot(uint32 q){return getQuestStatus(q)?0:MAX_QUEST_LOG_SIZE;}void SetQuestSlotState(uint16,uint32){}void SendQuestComplete(Quest const*){}void SetSpecialCriteriaComplete(uint16,int8){}void SendDirectMessage(int){}bool CanCompleteRepeatableQuest(Quest const*){return false;}void FailQuest(uint32 q){SetQuestStatus(q,QUEST_STATUS_NONE);}
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
Player* FindPlayer(ObjectGuid id){Creature ignored;return GetPlayer(ignored,id);}
GameObject* GetGameObject(WorldObject&,ObjectGuid id){auto i=objects.find(id);return i==objects.end()||!i->second->world?nullptr:i->second;}}
Unit* Vehicle::GetPassenger(int index){auto i=Seats.find(index);return i==Seats.end()?nullptr:ObjectAccessor::FindPlayer(i->second.Passenger.Guid);}
struct SmartAI {ObjectGuid attacked;void AttackStart(Unit* target){attacked=target->GetGUID();}Creature* me;uint32 delegated=0;explicit SmartAI(Creature* c):me(c){c->ai=this;}virtual ~SmartAI()=default;
 virtual void JustSummoned(Creature*){++delegated;} virtual void JustDied(Unit*){++delegated;} void Talk(uint8,ObjectGuid){++me->talks;} virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual ObjectGuid GetGUID(int32){return {};}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){++delegated;}
 virtual void PassengerBoarded(Unit*,int8,bool){++delegated;}virtual void IsSummonedBy(Unit*){++delegated;}virtual void UpdateAI(uint32){++delegated;}
 virtual void SpellHit(Unit*,SpellInfo const*){++delegated;}
};
struct Hook {std::vector<int> values;void operator+=(int i){values.push_back(i);}};
struct SpellScript {uint32 m_scriptSpellId=219924;Unit* caster=nullptr;Unit* original=nullptr;Hook OnObjectAreaTargetSelect;Unit* GetCaster(){return caster;}Unit* GetOriginalCaster(){return original?original:caster;}virtual void Register(){}};
#define PrepareSpellScript(x) public:
#define SpellObjectAreaTargetSelectFn(F,I,T) (1000+(I)*100+(T))
struct GameObjectAI {virtual ~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual bool GossipUse(Player*){return false;}};
struct SmartGameObjectAI:GameObjectAI {uint32 delegated=0;explicit SmartGameObjectAI(GameObject*){}bool GossipHello(Player*,bool)override{++delegated;return false;}bool GossipUse(Player*)override{++delegated;return false;}};
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}virtual GameObjectAI* GetAI(GameObject*)const{return nullptr;}};

enum class HighGuid {Conversation};
struct Generator {uint64 n=0;uint64 Generate(){return ++n;}};
struct ObjectMgr {Generator g;template<HighGuid>Generator* GetGenerator(){return &g;}} objectMgr;
auto sObjectMgr=&objectMgr;
struct SpellMgr {SpellInfo info;SpellInfo const* GetSpellInfo(uint32 id){info.Id=id;return &info;}} spellMgr;
auto sSpellMgr=&spellMgr;
struct Conversation {static inline uint32 fail=0;static inline std::vector<uint32> made;uint32 entry=0;
 bool CreateConversation(uint64,uint32 id,Unit* caster,SpellInfo const* info,WorldObject const&){assert(caster->bound&&info);if(fail==id)return false;entry=id;made.push_back(id);return true;}
 int32 GetDuration(){return entry==773?8150:entry==774?8050:7200;}};
#include "AreaExplored.inc"
struct LegacyStartAction {
 struct ScriptInfo {struct {uint32 QuestID=39592,Distance=0;} QuestExplored;} info;
 struct {ScriptInfo* script;} step{&info};
 void Execute(WorldObject* source,WorldObject* target){switch(SCRIPT_COMMAND_QUEST_EXPLORED){
 #include "QuestStartAction.inc"
 }}
};
#include "Will.inc"

struct Session {Creature me;npc_stormheim_will_actor ai;Session(Unit& parent,uint32 entry,uint64 id):ai(&me){me.entry=entry;me.guid={id};me.bound=parent.bound;me.questScope=parent.questScope;creatures[me.guid]=&me;ai.IsSummonedBy(&parent);}~Session(){creatures.erase(me.guid);}};
std::function<Creature*(Unit&,uint32)> spawn;
std::vector<std::unique_ptr<GameObject>> spawnedObjects;
bool failGo=false;uint32 failCreature=0;
Creature* Unit::SummonCreature(uint32 id,float,float,float,float,int,uint32){if(id==failCreature)return nullptr;if(auto p=ToPlayer())return p->summonHook?p->summonHook(id):nullptr;return spawn?spawn(*this,id):nullptr;}
GameObject* Unit::SummonGameObject(uint32 id,float,float,float,float,float,float,float,float,uint32){if(failGo)return nullptr;auto go=std::make_unique<GameObject>();go->entry=id;go->bound=bound;go->questScope=questScope;go->guid={uint64(50000+spawnedObjects.size())};objects[go->guid]=go.get();auto result=go.get();spawnedObjects.push_back(std::move(go));return result;}

struct Run {
 Player p;GameObject original;std::vector<std::unique_ptr<Session>> actors;std::vector<std::unique_ptr<Creature>> helpers;std::vector<std::unique_ptr<SmartAI>> helperAis;go_stormheim_will_console go;uint64 n;int spawned=0;
 Run(uint64 id=1):n(id*1000){p.guid={id};p.Accept(39592);players[p.guid]=&p;original.entry=243814;original.guid={id*100000};objects[original.guid]=&original;Activate();p.summonHook=[this](uint32 entry){return New(p,entry);};}
 void Activate(){spawn=[this](Unit& parent,uint32 entry)->Creature*{return New(parent,entry);};}
 Creature* New(Unit& parent,uint32 entry){++spawned;if(entry==34527||entry==96067){auto c=std::make_unique<Creature>();c->entry=entry;c->bound=parent.bound;c->guid={++n};auto result=c.get();creatures[result->guid]=result;helpers.push_back(std::move(c));helperAis.push_back(std::make_unique<SmartAI>(result));p.BindQuestObject(39592,result);if(auto source=parent.ToCreature())source->AI()->JustSummoned(result);return result;}
  auto session=std::make_unique<Session>(parent,entry,++n);auto result=&session->me;if(auto source=parent.ToCreature())source->AI()->JustSummoned(result);session->me.castHook=[this,result](uint32 spell){if(spell==190707){New(*result,96144);if(failCreature!=96067)New(*result,96067);}};actors.push_back(std::move(session));return result;}
 npc_stormheim_will_actor* Find(uint32 entry){for(auto& a:actors)if(a->me.entry==entry)return &a->ai;return nullptr;}
 void Click(int route=0,GameObject* object=nullptr){Activate();if(!object)object=&original;if(!route)go.OnGossipHello(&p,object);else{std::unique_ptr<GameObjectAI> ai(go.GetAI(object));if(route==1)ai->GossipHello(&p,false);else ai->GossipUse(&p);}}
 void Charge(){Activate();auto c=Find(96139);assert(c);c->UpdateAI(1499);assert(!Find(96129));c->UpdateAI(1);c->UpdateAI(2499);assert(!Find(96129));c->UpdateAI(1);assert(c->defenders.size()==2&&!Find(108263));c->UpdateAI(2000);c->UpdateAI(30000);c->UpdateAI(5000);assert(c->defenders.size()==5&&!Find(108263));c->UpdateAI(2000);c->UpdateAI(45000);c->UpdateAI(4999);assert(!Find(108263));c->UpdateAI(1);assert(c->defenders.size()==6&&Find(108263)&&c->next==8&&!Powered());for(auto& a:actors)if(a->me.entry==96129||a->me.entry==108263)assert(a->ai.attacked==p.guid);}
 void Kill(npc_stormheim_will_actor* enemy,Unit* killer){enemy->me->alive=false;enemy->JustDied(killer);}
 void Defeat(Unit* killer=nullptr){Charge();if(!killer)killer=&p;auto boss=Find(108263);Kill(boss,killer);if(Find(96139)->stopped)return;assert(!Powered());for(auto& a:actors)if(a->me.entry==96129&&a->me.alive)Kill(&a->ai,killer);}
 GameObject* Powered(){return ObjectAccessor::GetGameObject(*Find(96139)->me,Find(96139)->accessObject);}
 ~Run(){spawn={};actors.clear();players.erase(p.guid);objects.erase(original.guid);for(auto& c:helpers)creatures.erase(c->guid);}
};
int main(){
 for(uint32 id:{39590u,39591u,39592u,39593u})store.quests[id].id=id;
 store.quests[39592].special=QUEST_SPECIAL_FLAGS_CAST;store.quests[39592].objectives={{2,243802,1,0,3}};store.quests[39592].prevQuests={39590,39591};
 for(uint32 team:{ALLIANCE,HORDE}){
  Run r;r.p.team=team;assert(!r.p.CanTakeQuest(&store.quests[39592],false));r.p.m_RewardedQuests.insert(39590);assert(r.p.CanTakeQuest(&store.quests[39592],false));
  assert(!r.Find(96139)&&!r.Find(96129)&&!r.Find(108263)&&!r.Find(96144));r.Click();auto c=r.Find(96139);assert(c&&c->scoped);auto count=r.spawned;r.Click();assert(r.spawned==count);
  Unit pet;pet.ownerPlayer=&r.p;r.Defeat(&pet);assert(r.Powered()&&r.original.hidden.count(r.p.guid)&&!r.Find(96144));assert(r.p.GetQuestStatus(39592)==QUEST_STATUS_INCOMPLETE);
  r.Click(0,r.Powered());auto record=r.Find(96144);assert(record&&record->ready&&!record->dragon.IsEmpty());count=r.spawned;r.Click(2,r.Powered());assert(r.spawned==count);
  Conversation::made.clear();record->UpdateAI(1999);assert(Conversation::made.empty());record->UpdateAI(1);assert(Conversation::made==std::vector<uint32>{773});
  record->UpdateAI(5000);record->UpdateAI(6000);assert(Conversation::made==std::vector<uint32>({773,774,775}));record->UpdateAI(7199);assert(r.p.GetQuestStatus(39592)==QUEST_STATUS_INCOMPLETE);
  record->UpdateAI(1);c->UpdateAI(1);assert(r.p.GetQuestStatus(39592)==QUEST_STATUS_COMPLETE&&r.original.hidden.empty());auto packets=r.p.packets;record->UpdateAI(50000);assert(r.p.packets==packets);
 }
 {Run r;LegacyStartAction legacy;assert(r.p.GetQuestStatus(39592)==QUEST_STATUS_INCOMPLETE);legacy.Execute(&r.original,&r.p);assert(r.p.GetQuestStatus(39592)==QUEST_STATUS_COMPLETE);assert((r.p.data[{39592,243802}]==0));assert(r.p.CanRewardQuest(&store.quests[39592],false));}
 // The old manual exploration flag blocks the unchanged production completion
 // handler at 1/1. The migration clears only the known flag-2 addon state.
 {Run r;r.p.data[{39592,243802}]=1;store.quests[39592].special|=QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT;assert(!r.p.CanCompleteQuest(39592));store.quests[39592].special=QUEST_SPECIAL_FLAGS_CAST;assert(r.p.CanCompleteQuest(39592));}
 for(uint32 failure:{773u,774u,775u}){Run r;r.Click();r.Defeat();r.Click(1,r.Powered());auto record=r.Find(96144);Conversation::fail=failure;for(uint32 diff:{2000u,5000u,6000u,20000u})record->UpdateAI(diff);r.Find(96139)->UpdateAI(1);assert(record->stopped&&r.p.GetQuestStatus(39592)==QUEST_STATUS_INCOMPLETE&&r.original.hidden.empty());}Conversation::fail=0;
 {Run r;r.Click();r.Defeat();r.Click(2,r.Powered());auto record=r.Find(96144);r.p.Accept(39592);r.p.AddQuestDelayedEvent(39592,0,[]{});auto cancels=r.p.cancels;record->UpdateAI(24000);assert((record->stopped&&r.p.cancels==cancels&&r.p.data[{39592,243802}]==0));}
 for(int mode=0;mode<6;++mode){Run r;r.Click();r.Defeat();r.Click(0,r.Powered());auto record=r.Find(96144);if(mode==0)r.p.quests[39592].Status=QUEST_STATUS_NONE;if(mode==1)r.p.alive=false;if(mode==2)r.p.map=1;if(mode==3)r.p.phase=false;if(mode==4)r.p.world=false;if(mode==5)record->me->distance=81;record->UpdateAI(24000);r.Find(96139)->UpdateAI(1);assert((record->stopped&&r.p.data[{39592,243802}]==0&&r.original.hidden.empty()));}
 for(int route=0;route<3;++route){Run r;r.Click(route);auto count=r.spawned;r.Click(route);assert(r.spawned==count);r.Defeat();static_cast<Unit&>(*r.Find(96139)->me).castResult=1;r.Click(route,r.Powered());assert(!r.Find(96144)&&r.Find(96139)->next==9);static_cast<Unit&>(*r.Find(96139)->me).castResult=0;r.Click(route,r.Powered());assert(r.Find(96144));}
 for(uint32 failure:{96129u,108263u}){Run r;r.Click();failCreature=failure;auto c=r.Find(96139);for(uint32 diff:{1500u,2500u,2000u,30000u,5000u,2000u,45000u,5000u}){c->UpdateAI(diff);if(c->stopped)break;}assert(c->stopped&&!r.Find(96144));failCreature=0;}
 {Run r;r.Click();failGo=true;r.Defeat();r.Find(96139)->UpdateAI(1);assert(r.Find(96139)->stopped&&!r.Find(96144)&&!r.Powered());failGo=false;}
 {Run r;failCreature=96139;r.Click();assert(!r.Find(96139));failCreature=0;r.Click();assert(r.Find(96139));}
 {Run r;r.Click();r.Defeat();static_cast<Unit&>(r.p).castResult=1;r.Click(0,r.Powered());assert(r.Find(96144)->stopped&&r.p.GetQuestStatus(39592)==QUEST_STATUS_INCOMPLETE);}
 {Run r;r.Click();r.Defeat();failCreature=96067;r.Click(0,r.Powered());assert(r.Find(96139)->stopped&&r.p.GetQuestStatus(39592)==QUEST_STATUS_INCOMPLETE);failCreature=0;}
 {Run r;Session otherConsole(r.p,96139,899);assert(!otherConsole.ai.scoped);Session arbitrary(r.p,96144,900);assert(!arbitrary.ai.scoped);arbitrary.ai.UpdateAI(50000);assert(arbitrary.ai.delegated>0);GameObject fake;fake.entry=243802;fake.guid={999};r.Click(0,&fake);assert(!r.Find(96144));}
 for(int mode=0;mode<8;++mode){Run r;if(mode==0)r.p.phase=false;if(mode==1)r.original.distance=6;if(mode==2)r.p.alive=false;if(mode==3)r.p.quests[39592].Status=QUEST_STATUS_NONE;if(mode==4)r.original.spawned=false;if(mode==5)r.original.flags=GO_FLAG_NOT_SELECTABLE;if(mode==6)r.p.los=false;if(mode==7)r.p.visible=false;r.Click();assert(!r.Find(96139));}
 {Run r;r.Click();Unit other;Player stranger;other.ownerPlayer=&stranger;r.Defeat(&other);r.Find(96139)->UpdateAI(1);assert(r.Find(96139)->stopped&&!r.Powered());}
 {Run a(1);a.Click();Run b(2);b.Click();a.Defeat();b.Defeat();assert(a.Powered()&&b.Powered());b.Click(0,a.Powered());assert(!b.Find(96144));a.p.alive=false;a.Find(96139)->UpdateAI(1);assert(a.original.hidden.empty()&&b.original.hidden.count(b.p.guid)&&!b.Find(96139)->stopped);b.Click(0,b.Powered());assert(b.Find(96144));}
 {Run r;r.Click();r.Find(96139)->UpdateAI(100000);assert(!r.Find(96129)&&!r.Find(108263));r.Find(96139)->UpdateAI(2499);assert(!r.Find(96129));r.Find(96139)->UpdateAI(1);assert(r.Find(96139)->defenders.size()==2);}
 {Run r;r.Click();auto c=r.Find(96139);c->UpdateAI(1500);c->UpdateAI(2500);auto e=r.Find(96129);e->me->world=false;c->UpdateAI(1);assert(c->stopped&&!r.Powered());}
 {Run r;r.Click();r.Defeat();r.Click(0,r.Powered());auto record=r.Find(96144);record->UpdateAI(2000);record->UpdateAI(5000);record->UpdateAI(6000);auto dragon=ObjectAccessor::GetCreature(*record->me,record->dragon);assert(dragon&&dragon->AI()->delegated==2);dragon->world=false;record->UpdateAI(7200);assert(record->stopped&&r.p.GetQuestStatus(39592)==QUEST_STATUS_INCOMPLETE);}
 std::cout<<"PASS: full production timed two/three-raider defense -> actual owned kills including pet/boss-first -> powered-console click -> exact dragon and three conversations -> real GO credit; no timer/foreign/de-spawn credit, both factions, native slot, exploration flag regression, all packet routes, failure/retry, stale acceptance and independent players.\n";
}
