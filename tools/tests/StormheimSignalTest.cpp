#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
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
struct Quest {uint32 id=0,special=0;int32 ExclusiveGroup=0;PrevQuests prevQuests;std::vector<QuestObjective> objectives;
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
 virtual ~WorldObject()=default;uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}uint32 GetAreaId(){return area;}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}Creature* SummonCreature(uint32,Position const&,int,uint32);uint32 GetPhaseMask(){return 1;}std::set<uint32> GetPhases(){return {};}};
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
struct FakeMap {bool IsDungeon(){return false;}int GetDifficultyID(){return 0;}Creature* GetCreature(ObjectGuid g){return creatures.count(g)?creatures[g]:nullptr;}};
struct Group {bool isRaidGroup(){return false;}};
struct Guild {template<class... T>void UpdateAchievementCriteria(T...) {}};
struct GuildMgr {Guild* GetGuildById(uint32){return nullptr;}} guildMgr;auto sGuildMgr=&guildMgr;
struct Achievement {void StartTimedAchievement(int,uint32){}};
struct Player:Unit {
 ObjectGuid guid{1};uint32 team=ALLIANCE,packets=0,completions=0,power=0;bool teleport=false,
 vehicle=false,transport=false,combat=false,phase=true,visible=true,los=true,otherPower=false;
 int castResult=0;std::set<uint32> auras, m_RewardedQuests;std::map<uint32,QuestStatusData> quests;
 std::map<std::pair<uint32,uint32>,int32> data;std::vector<std::function<void()>> events;std::function<void()> castHook;
 Player* ToPlayer()override{return this;}Player* GetCharmerOrOwnerPlayerOrPlayerItself()const override{return const_cast<Player*>(this);}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}
 uint32 GetTeam(){return team;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}bool IsBeingTeleported(){return teleport;}
 bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInCombat(){return combat;}
 bool InSamePhase(WorldObject*){return phase;}bool canSeeOrDetect(WorldObject*){return visible;}bool IsWithinLOSInMap(WorldObject*){return los;}
 float GetExactDist(WorldObject* o){return o->distance;}QuestStatusData* getQuestStatus(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}
 int GetQuestStatus(uint32 q){auto s=getQuestStatus(q);return s?s->Status:QUEST_STATUS_NONE;}
 int32 GetQuestObjectiveData(uint32 q,uint32 e){return data[{q,e}];}
 int32 GetQuestObjectiveData(Quest const* q,int8 index){for(auto const& o:q->objectives)if(o.StorageIndex==index)return GetQuestObjectiveData(q->id,o.ObjectID);return 0;}
 void SetQuestObjectiveData(Quest const* q,QuestObjective const* o,int32 n){data[{q->id,o->ObjectID}]=n;}
 void SendQuestUpdateAddCredit(Quest const* q,ObjectGuid,QuestObjective const& o,uint32 n){assert(n>0);if(q->id==38058||q->id==38059||q->id==38360)assert(n==uint32(GetQuestObjectiveData(q->id,o.ObjectID)));++packets;}
 bool HasQuestObjectiveComplete(Quest const* q,QuestObjective const& o){return GetQuestObjectiveData(q->id,o.ObjectID)>=int32(o.Amount);}
 bool IsQuestWeekRewarded(uint32){return false;}bool IsQuestSeasonalRewarded(uint32){return false;}
 bool SatisfyQuestRace(Quest const*,bool){return true;}void SendCanTakeQuestResponse(int,Quest const*,char const*){}
 bool SatisfyQuestPreviousQuest(Quest const*,bool);bool CanCompleteQuest(uint32);bool CanTakeQuest(Quest const* q,bool b){return SatisfyQuestPreviousQuest(q,b);}
 void CompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_COMPLETE;++completions;}
 uint32 GetQuestSlotQuestId(uint8 index){auto i=quests.begin();while(index--&&i!=quests.end())++i;return i==quests.end()?0:i->first;}
 void QuestObjectiveSatisfy(uint32,uint32,QuestObjectiveType=QUEST_OBJECTIVE_MONSTER,ObjectGuid=ObjectGuid::Empty);
 bool HasAura(uint32 a){return auras.count(a);}bool HasAuraType(int){return otherPower||HasAura(180645);}
 void RemoveAurasDueToSpell(uint32 a){if(auras.erase(a)&&a==180645)power=0;}void SetPower(int,uint32 n){power=n;}
 int CastSpell(Unit*,uint32 a,bool){if(!castResult)auras.insert(a);return castResult;}
 void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Accept(uint32 q){quests.erase(q);quests[q].Status=QUEST_STATUS_INCOMPLETE;for(auto i=data.begin();i!=data.end();)if(i->first.first==q)i=data.erase(i);else ++i;}
 Achievement achievement;Achievement* m_achievementMgr=&achievement;Group* GetGroup(){return nullptr;}uint32 GetGuildId(){return 0;}template<class... T>void UpdateAchievementCriteria(T...){}void AddKillCreature(uint32,uint16,bool){}void AddTrackingQuestIfNeeded(ObjectGuid){}void SetQuestStatus(uint32 q,int state){quests[q].Status=state;}void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty);FakeMap* GetMap()const{static FakeMap m;return &m;}bool CanRewardQuest(Quest const*,bool);bool IsQuestDailyRewarded(uint32){return false;}bool GetQuestRewardStatus(uint32 q){return m_RewardedQuests.count(q);}void SendEquipError(int,void*,void*,uint32){}bool HasCurrency(uint32,uint32){return false;}bool HasEnoughMoney(uint64){return false;}bool CanGetItemForLoot(void const*,bool)const{return true;}void ItemAddedQuestCheck(uint32,uint32);void ItemRemovedQuestCheck(uint32,uint32);uint32 inventory=0,bankInventory=0;uint32 GetItemCount(uint32,bool bank=false){return inventory+(bank?bankInventory:0);}void IncompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_INCOMPLETE;}void UpdateForQuestWorldObjects(){}
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
struct SmartAI {Creature* me;uint32 delegated=0;explicit SmartAI(Creature* c):me(c){c->ai=this;}virtual ~SmartAI()=default;virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){++delegated;}virtual void SetGUID(ObjectGuid const&,int32){++delegated;}virtual ObjectGuid GetGUID(int32){return {};}
 virtual bool CanAIAttack(Unit const*)const{return true;}virtual void IsSummonedBy(Unit*){++delegated;}virtual void JustSummoned(Creature*){++delegated;}virtual void MovementInform(uint32,uint32){++delegated;}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){++delegated;}virtual void EnterEvadeMode(){++delegated;}virtual void UpdateAI(uint32){++delegated;}
 bool UpdateVictim(){return me->combat;}void AttackStart(Unit* u){me->victim=u;me->combat=true;}void DoCastVictim(uint32){}void DoMeleeAttackIfReady(){}virtual void Talk(uint32,ObjectGuid){++me->talks;}};
struct SummonList {Creature* owner;std::vector<Creature*> children;explicit SummonList(Creature* c):owner(c){}void Summon(Creature* c){children.push_back(c);}void DespawnAll(){for(auto c:children)c->DespawnOrUnsummon();children.clear();}};
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}virtual void OnUpdate(Player*,uint32){}};
#include "Signal.inc"
std::vector<std::unique_ptr<Creature>> allocations;std::vector<std::unique_ptr<SmartAI>> ais;
uint32 failEntry=0;uint64 nextGuid=100;
Creature* WorldObject::SummonCreature(uint32 e,Position const&,int,uint32 duration)
{
 if(e==failEntry)return nullptr;
 auto c=std::make_unique<Creature>();c->entry=e;c->duration=duration;c->guid={++nextGuid};auto ptr=c.get();creatures[c->guid]=ptr;allocations.push_back(std::move(c));
 std::unique_ptr<SmartAI> ai;if(e==92475)ai=std::make_unique<npc_stormheim_signal_controller>(ptr);else ai=std::make_unique<npc_stormheim_signal_actor>(ptr);
 ais.push_back(std::move(ai));if(auto parent=dynamic_cast<Creature*>(this))parent->AI()->JustSummoned(ptr);ptr->AI()->IsSummonedBy(dynamic_cast<Unit*>(this));return ptr;
}
struct ItemTemplate {uint32 GetInventoryType()const{return 0;}};
struct ObjectMgr {ItemTemplate const* GetItemTemplate(uint32){static ItemTemplate p;return &p;}} objectMgr;
auto sObjectMgr=&objectMgr;
struct World {float getRate(int){return 1;}} world;auto sWorld=&world;
struct LootStoreItem {uint32 itemid=0,lootmode=0,ClassificationMask=0,type=0,reference=0,maxcount=1,groupid=0;bool allowRoll=true;
 bool Roll(bool,bool=false,bool=false)const{return allowRoll;}};
using LootStoreItemList=std::vector<LootStoreItem>;
struct LootItem {struct {uint32 ItemID=0;} item;};using LootItemList=std::vector<LootItem>;
struct Loot {Player* owner=nullptr;uint32 _DifficultyMask=1,_ClassificationMask=1;bool _specCheck=false;LootItemList items;
 Player const* GetLootOwner(){return owner;}void AddItem(LootStoreItem const& i){items.push_back({{i.itemid}});}};
struct LootTemplate;
struct LootGroup {void Process(Loot&,LootTemplate const*)const{}void ProcessAutoGroup(Loot&,LootTemplate const*)const{}};
using LootGroups=std::vector<LootGroup>;
struct LootTemplate {LootStoreItemList Entries;LootGroups Groups,AutoGroups;bool _isZoneLoot=false,condition=true;
 bool CheckItemCondition(Player const*,uint32,uint32)const{return condition;}void Process(Loot&,bool,uint8=0)const;};
struct References {LootTemplate const* GetLootFor(uint32)const{return nullptr;}} LootTemplates_Reference;
#include "LootProcess.inc"

struct Pet:Unit {Player* owner=nullptr;Player* GetCharmerOrOwnerPlayerOrPlayerItself()const override{return owner;}};
void Setup(Player& p,uint64 guid=1){p={};p.guid={guid};p.Accept(38060);p.auras.insert(180860);players[p.guid]=&p;}
npc_stormheim_signal_controller* Start(Player& p)
{
 GameObject go;go.entry=240235;go_stormheim_signal use;assert(use.OnGossipHello(&p,&go));auto i=StormheimSignal::encounters.find(p.guid);assert(i!=StormheimSignal::encounters.end());return dynamic_cast<npc_stormheim_signal_controller*>(creatures[i->second]->AI());
}
Creature* Bird(npc_stormheim_signal_controller* ai)
{
 ai->UpdateAI(3000);for(uint32 time:{3000u,5000u,5000u,5000u})ai->UpdateAI(time);
 assert(ai->stage==10&&ai->summons.children.size()==4);for(auto wave:ai->summons.children)assert(wave->duration==120000);assert(!StormheimSignal::Objective(ai->PlayerOwner(),90907));
 ai->UpdateAI(60000);assert(ai->stage==5&&creatures[ai->carrier]->duration==180000);return creatures[ai->carrier];
}
Creature* Battle(npc_stormheim_signal_controller* ai)
{
 Creature* bird=Bird(ai);auto a=dynamic_cast<npc_stormheim_signal_actor*>(bird->AI());
 a->MovementInform(0,1);assert(a->point==1);a->MovementInform(8,99);assert(a->point==1);
 for(uint32 i=1;i<=15;++i)a->MovementInform(8,i);assert(ai->stage==5);
 a->MovementInform(8,16);assert(ai->stage==6&&ai->rogers.value&&ai->overspark.value);
 a->MovementInform(8,16);assert(ai->stage==6);ai->UpdateAI(4000);assert(ai->stage==7);ai->UpdateAI(4000);if(ai->stopped)return nullptr;assert(ai->stage==8);
 return creatures[ai->boss];
}
int main()
{
 for(uint32 id:{38060u,38362u,38360u,38058u,38210u,38308u,38459u})store.quests[id].id=id;
 store.quests[38060].objectives={{0,92475,1,0,1},{0,90907,1,3,0}};
 store.quests[38362].objectives={{1,122746,1,0,0}};store.quests[38362].special=QUEST_SPECIAL_FLAGS_DELIVER;
 store.quests[38362].prevQuests={38360};store.quests[38210].prevQuests={38060};store.quests[38308].prevQuests={38362};store.quests[38459].prevQuests={38362};
 for(int reason=0;reason<14;++reason){Player p;Setup(p);GameObject go;go.entry=240235;
  if(reason==0)p.team=HORDE;if(reason==1)p.map=1;if(reason==2)p.alive=false;if(reason==3)p.teleport=true;if(reason==4)p.vehicle=true;if(reason==5)p.transport=true;
  if(reason==6)p.combat=true;if(reason==7)p.phase=false;if(reason==8)p.visible=false;if(reason==9)p.los=false;if(reason==10)go.distance=7;if(reason==11)go.spawned=false;if(reason==12)go.flags=GO_FLAG_NOT_SELECTABLE;if(reason==13)p.auras.clear();
  go_stormheim_signal use;use.OnGossipHello(&p,&go);assert(!p.packets&&StormheimSignal::encounters.empty());}
 Player p;Setup(p);failEntry=92475;go_stormheim_signal use;GameObject go;go.entry=240235;use.OnGossipHello(&p,&go);assert(!p.packets);failEntry=0;
 auto ai=Start(p);assert(p.packets==1&&!p.CanCompleteQuest(38060));auto count=allocations.size();use.OnGossipHello(&p,&go);assert(allocations.size()==count);
 Creature* boss=Battle(ai);assert(!p.CanCompleteQuest(38060));assert(creatures[ai->rogers]->personal==p.guid);
 Player other;Setup(other,2);uint32 damage=4000000000u;boss->AI()->DamageTaken(&other,damage,0);assert(!damage&&!p.completions&&!other.completions);
 damage=100;boss->multiplier=2;boss->AI()->DamageTaken(&p,damage,0);assert(damage==100&&!p.completions);
 Pet pet;pet.owner=&p;assert(boss->AI()->CanAIAttack(&p)&&boss->AI()->CanAIAttack(&pet)&&!boss->AI()->CanAIAttack(&other));boss->multiplier=0.001f;damage=1000000;boss->AI()->DamageTaken(&pet,damage,0);
 assert(!boss->AI()->CanAIAttack(&p));assert(!damage&&p.completions==1&&p.packets==2&&boss->health==1000&&boss->faction==35&&ai->stage==9);
 damage=999999999;boss->AI()->DamageTaken(&p,damage,0);assert(!damage&&p.completions==1&&p.packets==2);
 ai->SetGUID(boss->guid,3);assert(p.completions==1);ai->UpdateAI(1000);assert(creatures[ai->rogers]->world);
 use.OnGossipHello(&p,&go);assert(allocations.size()==count+8);p.quests[38060].Status=QUEST_STATUS_REWARDED;ai->UpdateAI(1000);assert(ai->stopped&&!creatures[ai->rogers]->world);
 // Independent players have independent private waves and completion proofs.
 Setup(p);Setup(other,2);auto first=Start(p);auto second=Start(other);auto firstBoss=Battle(first);auto secondBoss=Battle(second);
 assert(firstBoss->personal==p.guid&&secondBoss->personal==other.guid);first->SetGUID(secondBoss->guid,3);assert(!p.completions);
 damage=500;firstBoss->AI()->DamageTaken(&p,damage,0);assert(!damage&&p.completions==1&&!other.completions);first->Stop();second->Stop();
 // Every aborted acceptance can restart, including abandonment/reacceptance
 // between updates and logout/re-entry with the same player GUID.
 for(int reason=0;reason<7;++reason){Setup(p);auto interrupted=Start(p);auto child=Bird(interrupted);auto originalChildren=interrupted->summons.children;
  if(reason==0)p.Accept(38060);if(reason==1)p.world=false;if(reason==2)p.alive=false;if(reason==3)p.map=1;if(reason==4)p.phase=false;if(reason==5)interrupted->me->distance=101;if(reason==6)child->world=false;
  interrupted->UpdateAI(1);assert(interrupted->stopped&&StormheimSignal::encounters.empty()&&!p.completions);for(auto c:originalChildren)assert(!c->world);
  Setup(p);auto restarted=Start(p);assert(!p.CanCompleteQuest(38060));restarted->Stop();}
 Setup(p);auto timeout=Start(p);Bird(timeout);timeout->UpdateAI(180000);assert(timeout->stopped&&!p.completions);
 for(uint32 missing:{90899u,92547u,92557u,92556u,90907u}){Setup(p);auto failed=Start(p);failEntry=missing;
  if(missing==90899){failed->UpdateAI(3000);failed->UpdateAI(3000);}else if(missing==92547){failed->UpdateAI(3000);for(int i=0;i<4;++i)failed->UpdateAI(5000);failed->UpdateAI(60000);}else if(missing==90907){Battle(failed);}else{
   auto b=Bird(failed);for(uint32 i=1;i<=16;++i)b->AI()->MovementInform(8,i);}
  assert(failed->stopped&&!p.completions);failEntry=0;}
 Creature later;later.entry=90907;later.map=1522;npc_stormheim_signal_actor unchanged(&later);damage=8000;unchanged.DamageTaken(&p,damage,0);unchanged.UpdateAI(1);assert(damage==8000&&unchanged.delegated==2);
 Setup(p);p.auras.clear();p.quests[38058].Status=QUEST_STATUS_REWARDED;player_stormheim_signal sync;sync.OnLogin(&p);assert(p.HasAura(180860));p.team=HORDE;p.auras.clear();sync.OnUpdate(&p,1);assert(!p.HasAura(180860));
 // Native trophy loot has a valid ungrouped zero mask. Collecting the item,
 // rather than killing the entry, completes the actual delivery objective.
 for(uint32 mode:{0u,1u,2u}){Loot loot;loot.owner=&p;LootTemplate tab;tab.Entries={{122746,mode}};tab.Process(loot,false);assert(loot.items.size()==(mode==2?0:1));}
 p={};p.team=HORDE;p.Accept(38362);assert(!p.CanCompleteQuest(38362));p.ItemAddedQuestCheck(122745,1);assert(!p.completions);
 p.KilledMonsterCredit(91571);assert(!p.completions);p.inventory=1;p.ItemAddedQuestCheck(122746,1);assert(p.GetQuestStatus(38362)==QUEST_STATUS_COMPLETE&&p.completions==1);
 assert(p.CanRewardQuest(&store.quests[38362],false));p.inventory=0;p.bankInventory=1;assert(!p.CanRewardQuest(&store.quests[38362],false));p.bankInventory=0;p.inventory=1;
 p.ItemAddedQuestCheck(122746,1);assert(p.completions==1);p.inventory=0;p.ItemRemovedQuestCheck(122746,1);assert(p.GetQuestStatus(38362)==QUEST_STATUS_INCOMPLETE);
 p.inventory=1;p.ItemAddedQuestCheck(122746,1);assert(p.completions==2);assert(!p.CanTakeQuest(&store.quests[38308],false));p.m_RewardedQuests.insert(38362);assert(p.CanTakeQuest(&store.quests[38308],false)&&p.CanTakeQuest(&store.quests[38459],false));
 std::cout<<"PASS: extracted production signal controller/actors/GO/player and quest/item/loot handlers; validated private start, four waves, real flight arrival, helper/boss progression, scaled and pet surrender once, owner isolation, missing summons, timeout/abandon/logout/retry, later-map delegation, charged-pack recovery, real trophy collect/remove/recollect and reward-gated next quests.\n";
}
