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

struct Unit;struct Player;struct Creature;struct GameObject;struct SmartAI;
struct Vehicle {struct Seat {struct {ObjectGuid Guid;} Passenger;};std::map<int,Seat> Seats;struct Unit* GetPassenger(int);};
struct WorldObject {ObjectGuid guid{1};std::weak_ptr<QuestObjectScope> questScope;bool bound=false,expired=false; bool HasQuestObjectScope(){return bound;} bool IsQuestObjectExpired(){auto scope=questScope.lock();return expired||(bound&&(!scope||scope->cancelled)); } uint32 map=1220,zone=7541,area=7557,entry=0;bool world=true,alive=true;float distance=0;
 virtual ~WorldObject()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}virtual Unit* ToUnit(){return nullptr;}virtual bool IsCreature(){return false;}float GetPositionX(){return 3442.69f;}float GetPositionY(){return 2103.1f;}float GetPositionZ(){return 277.243f;}float GetOrientation(){return 1.57336f;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}uint32 GetAreaId(){return area;}virtual ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}};
struct Unit:WorldObject {Player* ownerPlayer=nullptr;Player* GetCharmerOrOwnerPlayerOrPlayerItself() const {return const_cast<Unit*>(this)->ToPlayer()?const_cast<Unit*>(this)->ToPlayer():ownerPlayer;}int castResult=0; std::vector<uint32> casts; std::function<void(uint32)> castHook; int CastSpell(Unit*,uint32 id,bool){casts.push_back(id);if(castHook)castHook(id);return castResult;} Creature* SummonCreature(uint32,float,float,float,float,int,uint32); GameObject* SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32);Vehicle* kit=nullptr;Unit* ToUnit()override{return this;}Vehicle* GetVehicleKit(){return kit;}bool InSamePhase(WorldObject*){return true;}virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct SpellInfo {uint32 Id=180737;};
struct CreatureTemplate {uint32 StateWorldEffectID=0,SpellStateVisualID=45944,SpellStateAnimID=0,SpellStateAnimKitID=6050;};
struct HeightMap {float value=229.427f;float GetHeight(float,float,float){return value;}} heightMap;
struct Creature:Unit {uint64 dbGuid=0;uint64 GetDBTableGUIDLow(){return dbGuid;}void SetHealth(uint64 n){health=n;}HeightMap* GetMap(){return &heightMap;} SmartAI* ai=nullptr;SmartAI*& i_AI=ai;SmartAI* i_disabledAI=nullptr;bool charmed=false;bool IsPlayer(){return false;}bool isCharmed(){return charmed;}bool isPossessed(){return false;}bool IsVehicle(){return kit!=nullptr;}void UpdateCharmAI();std::vector<std::function<void()>> callbacks;void AddDelayedEvent(uint64,std::function<void()> f){callbacks.push_back(std::move(f));}void Tick(){auto q=std::move(callbacks);callbacks.clear();for(auto& f:q)f();}CreatureTemplate data;std::map<int,uint32> fields;uint32 stand=8,dirty=0,talks=0;bool visible=true,combat=false;
 Creature* ToCreature()override{return this;}bool IsCreature()override{return true;}Player* GetLootRecipient(){return nullptr;}void AddPlayerInPersonnalVisibilityList(ObjectGuid p){personal=p;}ObjectGuid personal;void setActive(bool){}void DespawnOrUnsummon(){world=false;} uint64 health=1000,maximum=1000;float multiplier=1;Unit* victim=nullptr;uint32 react=0;void SetReactState(uint32 r){react=r;}void CombatStop(bool){combat=false;}uint64 GetHealth(Unit*){return health;}uint64 GetMaxHealth(Unit*){return maximum;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}SmartAI* AI(){return ai;}CreatureTemplate const* GetCreatureTemplate(){return &data;}
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
struct Player:Unit {
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
struct SmartAI {Creature* me;uint32 delegated=0;explicit SmartAI(Creature* c):me(c){c->ai=this;}virtual ~SmartAI()=default;
 virtual void JustSummoned(Creature*){++delegated;} virtual void JustDied(Unit*){++delegated;} void Talk(uint8,ObjectGuid){++me->talks;} virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual ObjectGuid GetGUID(int32){return {};}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){++delegated;}
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


#include "Might.inc"
enum {SMART_EVENT_DAMAGED=1};
struct LegacyScript {void ProcessEventsFor(int,Unit*,uint32){}};
struct LegacySmartAI {Creature* me;uint32 mInvincibilityHpLevel=1;LegacyScript script;LegacyScript* GetScript(){return &script;}void DamageTaken(Unit*,uint32&,DamageEffectType);};
#include "LegacyDamage.inc"
namespace Trinity {struct CreatureWithDbGUIDCheck {uint64 i_lowguid;explicit CreatureWithDbGUIDCheck(uint64 const&);bool operator()(Creature*);};
#include "DbGuidCheck.inc"
}
struct Session {Creature me;npc_stormheim_might_actor ai;Session(Unit& parent,uint32 entry,uint64 id):ai(&me){me.entry=entry;me.guid={id};me.bound=parent.bound;me.questScope=parent.questScope;creatures[me.guid]=&me;ai.IsSummonedBy(&parent);}~Session(){creatures.erase(me.guid);}};
std::function<Creature*(Unit&,uint32)> spawn;uint32 failCreature=0,failSpell=0;
Creature* Unit::SummonCreature(uint32 id,float,float,float z,float,int,uint32){if(id==35845||id==96175)assert(z==heightMap.value);if(id==failCreature)return nullptr;if(auto p=ToPlayer())return p->summonHook?p->summonHook(id):nullptr;return spawn?spawn(*this,id):nullptr;}
GameObject* Unit::SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32){return nullptr;}
struct Run {
 Player p;GameObject original;std::vector<std::unique_ptr<Session>> actors;go_stormheim_might_console go;uint64 n;int spawned=0;
 Run(uint64 id=1):n(id*1000){p.guid={id};p.Accept(39594);players[p.guid]=&p;original.entry=243817;original.guid={id*100000};objects[original.guid]=&original;Activate();p.summonHook=[this](uint32 entry){return New(p,entry);};}
 void Activate(){spawn=[this](Unit& parent,uint32 entry)->Creature*{return New(parent,entry);};}
 Creature* New(Unit& parent,uint32 entry){++spawned;auto session=std::make_unique<Session>(parent,entry,++n);auto result=&session->me;if(entry==35845)result->castHook=[result](uint32){if(failSpell)result->castResult=1;};actors.push_back(std::move(session));return result;}
 npc_stormheim_might_actor* Find(uint32 entry){for(auto i=actors.rbegin();i!=actors.rend();++i)if((*i)->me.entry==entry)return &(*i)->ai;return nullptr;}
 void Click(int route=0){Activate();if(!route)go.OnGossipHello(&p,&original);else{std::unique_ptr<GameObjectAI> ai(go.GetAI(&original));if(route==1)ai->GossipHello(&p,false);else ai->GossipUse(&p);}}
 void Challenge(){Activate();auto c=Find(96176);assert(c);c->UpdateAI(1499);assert(!Find(35845));c->UpdateAI(1);assert(c->me->talks==1&&!Find(35845));c->UpdateAI(1499);assert(!Find(35845));c->UpdateAI(1);auto h=Find(35845);assert(h&&h->scoped&&h->me->casts==std::vector<uint32>{190763});c->UpdateAI(2999);assert(!Find(96175));c->UpdateAI(1);auto b=Find(96175);assert(b&&b->scoped&&h->me->casts==std::vector<uint32>({190763,190773}));assert(!b->me->combat&&!p.packets);b->UpdateAI(7999);assert(!p.packets);b->UpdateAI(1);assert(b->me->talks==2&&!p.packets);b->UpdateAI(7999);assert(!p.packets);b->UpdateAI(1);assert(p.packets==1&&b->me->combat&&b->me->victim==&p&&b->stage==2);assert(p.GetQuestStatus(39594)==QUEST_STATUS_INCOMPLETE&&!p.CanCompleteQuest(39594));}
 void Finish(Unit* attacker=nullptr){auto b=Find(96175);uint32 hit=4000000000u;b->DamageTaken(attacker?attacker:&p,hit,0);assert(!hit&&b->stage==3&&b->me->health==1000);auto packets=p.packets;b->UpdateAI(4999);assert(p.packets==packets);b->UpdateAI(1);assert(p.GetQuestStatus(39594)==QUEST_STATUS_COMPLETE&&p.packets==packets+1&&b->stopped);}
 ~Run(){spawn={};actors.clear();players.erase(p.guid);objects.erase(original.guid);}
};
int main(){
 // Reproduce both installed defects using unchanged production handlers:
 // GUID zero selects any summon, and raw invincibility misses scaled damage.
 {Creature unrelated, intended;unrelated.entry=999;intended.entry=35845;Trinity::CreatureWithDbGUIDCheck check(0);assert(check(&unrelated)&&check(&intended));intended.dbGuid=123;assert(!check(&intended));
  Player attacker;LegacySmartAI old{&unrelated};unrelated.multiplier=.001f;uint32 hit=100;old.DamageTaken(&attacker,hit,0);assert(hit==100&&std::floor(float(hit)/unrelated.multiplier)>=unrelated.GetHealth(&attacker));}
 for(uint32 id:{39593u,39594u,39597u})store.quests[id].id=id;
 auto& q=store.quests[39594];q.special=QUEST_SPECIAL_FLAGS_KILL;q.prevQuests={39593};q.objectives={{0,96290,1,0,0},{0,96175,1,2,5}};store.quests[39597].prevQuests={39594};
 for(auto faction:{ALLIANCE,HORDE})for(int route=0;route<3;++route){Run r;r.p.team=faction;assert(!r.p.CanTakeQuest(&q,false));r.p.m_RewardedQuests.insert(39593);assert(r.p.CanTakeQuest(&q,false));r.Click(route);auto count=r.spawned;r.Click((route+1)%3);assert(r.spawned==count);r.Challenge();auto b=r.Find(96175);assert((r.p.data[{39594,96290}]==1&&r.p.data[{39594,96175}]==0));Unit pet;pet.ownerPlayer=&r.p;r.Finish(route==1?&pet:nullptr);assert(r.p.CanRewardQuest(&q,false)&&!r.p.CanTakeQuest(&store.quests[39597],false));r.p.m_RewardedQuests.insert(39594);assert(r.p.CanTakeQuest(&store.quests[39597],false));auto packets=r.p.packets;uint32 hit=99;b->DamageTaken(&r.p,hit,0);b->UpdateAI(9000);assert(!hit&&r.p.packets==packets);r.Find(96176)->UpdateAI(1);assert(r.Find(96176)->stopped&&r.Find(35845)->me->IsQuestObjectExpired());}
 // Target-relative damage: below threshold remains damage; a scaled fresh
 // one-shot surrenders before damage, including an owner's pet hit.
 {Run r;r.Click();r.Challenge();auto b=r.Find(96175);Player stranger;uint32 hit=999999;b->DamageTaken(&stranger,hit,0);assert(!hit&&b->stage==2);hit=100;b->me->multiplier=2;b->DamageTaken(&r.p,hit,0);assert(hit==100&&b->stage==2);b->me->multiplier=.001f;Unit pet;pet.ownerPlayer=&r.p;hit=1;b->DamageTaken(&pet,hit,0);assert(!hit&&b->stage==3&&b->me->health==1000);b->UpdateAI(5000);assert(r.p.completions==1);}
 for(float invalid:{0.f,-1.f,INFINITY,NAN}){Run r;r.Click();r.Challenge();auto b=r.Find(96175);b->me->multiplier=invalid;uint32 hit=1000;b->DamageTaken(&r.p,hit,0);assert(!hit&&b->stage==2&&!r.p.completions);}
 {Run r;r.Click();r.Challenge();auto b=r.Find(96175);b->me->health=b->me->maximum=2;b->me->multiplier=.0000001f;uint32 hit=1;b->DamageTaken(&r.p,hit,0);assert(!hit&&b->stage==3&&b->me->health==2);}
 // The opponent also validates the exact projector when its own update or
 // a damage packet arrives before the controller has noticed its disappearance.
 {Run r;r.Click();r.Challenge();r.Find(35845)->me->world=false;auto boss=r.Find(96175);uint32 hit=999999;boss->DamageTaken(&r.p,hit,0);assert(!hit&&boss->stage==2);boss->UpdateAI(5000);assert(boss->stopped&&!r.p.completions);}
 // Progress survives a failed attempt, but replaying the introduction never
 // sends another first-stage credit and still requires a fresh surrender.
 {Run r;r.Click();r.Challenge();r.Find(96175)->EnterEvadeMode();r.Click();auto c=r.Find(96176);r.Activate();c->UpdateAI(1500);c->UpdateAI(1500);c->UpdateAI(3000);auto b=r.Find(96175);b->UpdateAI(8000);b->UpdateAI(8000);assert(r.p.packets==1&&b->stage==2);r.Finish();assert(r.p.packets==2);}
 // A frozen tick must not collapse the native pauses or award timed victory.
 {Run r;r.Click();auto c=r.Find(96176);c->UpdateAI(60000);assert(!r.Find(35845));c->UpdateAI(60000);assert(r.Find(35845)&&!r.Find(96175));c->UpdateAI(60000);auto b=r.Find(96175);assert(b);b->UpdateAI(60000);assert(!r.p.packets);b->UpdateAI(60000);assert(r.p.packets==1);b->UpdateAI(60000);assert(b->stopped&&!r.p.completions);}
 for(int mode=0;mode<8;++mode){Run r;if(mode==0)r.p.phase=false;if(mode==1)r.original.distance=6;if(mode==2)r.p.alive=false;if(mode==3)r.p.quests[39594].Status=QUEST_STATUS_NONE;if(mode==4)r.original.spawned=false;if(mode==5)r.original.flags=GO_FLAG_NOT_SELECTABLE;if(mode==6)r.p.los=false;if(mode==7)r.p.visible=false;r.Click();assert(!r.Find(96176));}
 for(uint32 entry:{96176u,35845u,96175u}){Run r;failCreature=entry;r.Click();if(auto c=r.Find(96176)){c->UpdateAI(1500);c->UpdateAI(1500);c->UpdateAI(3000);assert(c->stopped);}assert(!r.p.packets);failCreature=0;r.Click();assert(r.Find(96176)&&!r.Find(96176)->stopped);}
 for(float invalid:{-100000.f,300.f,NAN}){Run r;r.Click();auto c=r.Find(96176);c->UpdateAI(1500);heightMap.value=invalid;c->UpdateAI(1500);assert(c->stopped&&!r.Find(35845)&&!r.p.packets);}heightMap.value=229.427f;
 // Both native visual casts are checked independently. Never run the generic
 // GUID-zero helper SmartAI or grant progress on a failed activation.
 for(int cast=0;cast<2;++cast){Run r;r.Click();auto c=r.Find(96176);c->UpdateAI(1500);if(cast==0)failSpell=190763;c->UpdateAI(1500);if(cast==1)r.Find(35845)->me->castResult=1;c->UpdateAI(3000);assert(c->stopped&&!r.Find(96175)&&!r.p.packets);failSpell=0;}
 {Run r;r.Click();r.Challenge();auto b=r.Find(96175);b->me->castResult=1;uint32 hit=999999;b->DamageTaken(&r.p,hit,0);assert(!hit&&b->stopped&&!r.p.completions);}
 for(int mode=0;mode<9;++mode){Run r;r.Click();r.Challenge();auto b=r.Find(96175);auto old=r.p.quests[39594].ScriptObjects;if(mode==0)r.p.quests[39594].Status=QUEST_STATUS_NONE;if(mode==1)r.p.alive=false;if(mode==2)r.p.map=1;if(mode==3)r.p.phase=false;if(mode==4)r.p.world=false;if(mode==5)b->me->distance=81;if(mode==6)r.p.teleport=true;if(mode==7)r.Find(35845)->me->world=false;if(mode==8)r.Find(96176)->me->world=false;r.Find(96176)->UpdateAI(1);b->UpdateAI(5000);assert(b->stopped&&old->cancelled&&!r.p.completions);}
 {Run r;r.Click();r.Challenge();auto b=r.Find(96175);auto old=r.p.quests[39594].ScriptObjects;r.p.Accept(39594);r.p.AddQuestDelayedEvent(39594,0,[]{});auto cancels=r.p.cancels;r.Click();auto fresh=r.Find(96176);b->UpdateAI(5000);assert(b->stopped&&old->cancelled&&!fresh->stopped&&r.p.cancels==cancels+1);fresh->UpdateAI(1500);assert(!fresh->stopped);}
 {Run r;r.Click();r.Challenge();r.Find(96175)->JustDied(&r.p);assert(!r.p.completions);r.Find(96176)->UpdateAI(1);assert(r.Find(96176)->stopped);r.Click();assert(!r.Find(96176)->stopped);}
 {Run r;r.Click();r.Challenge();r.Find(96175)->EnterEvadeMode();assert(!r.p.completions);r.Find(96176)->UpdateAI(1);r.Click();assert(!r.Find(96176)->stopped);}
 {Run a(1);a.Click();a.Challenge();Run b(2);b.Click();b.Challenge();auto first=a.Find(96175),second=b.Find(96175);uint32 hit=4000000000u;first->DamageTaken(&b.p,hit,0);assert(!hit&&first->stage==2&&!a.p.completions&&!b.p.completions);a.Finish();assert(!second->stopped&&b.p.GetQuestStatus(39594)==QUEST_STATUS_INCOMPLETE);b.Finish();}
 {Run r;for(uint32 entry:{96176u,96175u,35845u}){Session arbitrary(r.p,entry,999+entry);assert(!arbitrary.ai.scoped);uint32 hit=1000;arbitrary.ai.DamageTaken(&r.p,hit,0);arbitrary.ai.UpdateAI(1);assert(hit==1000&&arbitrary.ai.delegated==3);}Creature later;later.entry=35845;later.map=1522;npc_stormheim_might_actor ai(&later);ai.IsSummonedBy(&r.p);ai.UpdateAI(1);assert(!ai.scoped&&ai.delegated==2);}
 std::cout<<"PASS: extracted production archive/narration/owner or pet scaled surrender/5-second finale and real quest completion/reward gates, both factions; three packet routes, pauses, cast/summon failures, death/evade, stale acceptance, logout/map/phase/distance cleanup, independent players and unrelated SmartAI delegation.\n";
}
