#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <vector>
using uint8=uint8_t; using uint32=uint32_t; using uint64=uint64_t; using int32=int32_t; using int8=int8_t;
using QuestStatus=int; using SpellCastResult=int; using SpellEffIndex=int;
enum {ALLIANCE=469,HORDE=67,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_REWARDED=6,
 QUEST_OBJECTIVE_MONSTER=0,QUEST_OBJECTIVE_ITEM=1,QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS=16,QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR=64,
 QUEST_OBJECTIVE_FLAG_OPTIONAL=4,QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=2,QUEST_SPECIAL_FLAGS_TIMED=4,
 GAMEOBJECT_FIELD_FLAGS=0,GO_FLAG_IN_USE=1,GO_FLAG_NOT_SELECTABLE=16,GO_READY=1,
 SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_AURA_ENABLE_ALT_POWER=346,POWER_ALTERNATE=10,
 EFFECT_0=0,SPELL_EFFECT_KILL_CREDIT=90,INVALIDREASON_DONT_HAVE_REQ=1,MAX_QUEST_LOG_SIZE=50,
 LOOT_ITEM_TYPE_ITEM=0,RATE_DROP_ITEM_REFERENCED_AMOUNT=0};
using QuestObjectiveType=int;
using PrevQuests=std::vector<int32>; using ExclusiveQuestGroups=std::multimap<int32,uint32>;
#define ASSERT assert
#define TC_LOG_DEBUG(...) (void)0
struct ObjectGuid {uint64 value=0; static ObjectGuid const Empty; bool operator<(ObjectGuid other)const{return value<other.value;}};
ObjectGuid const ObjectGuid::Empty{};
struct QuestObjective {int Type=0;uint32 ObjectID=0,Amount=1,Flags=0;int8 StorageIndex=0;};
struct Quest {uint32 id=0,special=0;int32 ExclusiveGroup=0;PrevQuests prevQuests;std::vector<QuestObjective> objectives;
 auto const& GetObjectives()const{return objectives;}bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}
 bool IsRepeatable()const{return false;}bool IsAutoComplete()const{return false;}bool HasSpecialFlag(uint32 f)const{return special&f;}};
struct QuestStore {std::map<uint32,Quest> quests;ExclusiveQuestGroups mExclusiveQuestGroups;
 Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}} store;
auto sQuestDataStore=&store;
struct QuestStatusData {int Status=QUEST_STATUS_NONE;uint32 Timer=0;std::shared_ptr<uint8> ScriptLifetime;};
struct Player;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}};
struct GameObject {uint32 entry=240600,map=1220,area=7563,flags=0,state=GO_READY;ObjectGuid guid{100};
 bool world=true,spawned=true;float distance=0;uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}
 ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool isSpawned(){return spawned;}bool HasFlag(int,uint32 f){return flags&f;}uint32 getLootState(){return state;}};
std::map<ObjectGuid,GameObject*> objects;
namespace ObjectAccessor {GameObject* GetGameObject(Player&,ObjectGuid id){auto i=objects.find(id);return i==objects.end()?nullptr:i->second;}}
struct FakeMap {bool IsDungeon(){return false;}};
struct Player:Unit {
 ObjectGuid guid{1};uint32 team=HORDE,map=1220,area=7563,packets=0,completions=0,power=0;bool world=true,alive=true,teleport=false,
 vehicle=false,transport=false,combat=false,phase=true,visible=true,los=true,otherPower=false;
 int castResult=0;std::set<uint32> auras, m_RewardedQuests;std::map<uint32,QuestStatusData> quests;
 std::map<std::pair<uint32,uint32>,int32> data;std::vector<std::function<void()>> events;std::function<void()> castHook;
 Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}
 uint32 GetTeam(){return team;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}bool IsBeingTeleported(){return teleport;}
 bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInCombat(){return combat;}
 bool InSamePhase(GameObject*){return phase;}bool canSeeOrDetect(GameObject*){return visible;}bool IsWithinLOSInMap(GameObject*){return los;}
 float GetExactDist(GameObject* o){return o->distance;}QuestStatusData* getQuestStatus(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}
 int GetQuestStatus(uint32 q){auto s=getQuestStatus(q);return s?s->Status:QUEST_STATUS_NONE;}
 int32 GetQuestObjectiveData(uint32 q,uint32 e){return data[{q,e}];}
 int32 GetQuestObjectiveData(Quest const* q,int8 index){for(auto const& o:q->objectives)if(o.StorageIndex==index)return GetQuestObjectiveData(q->id,o.ObjectID);return 0;}
 void SetQuestObjectiveData(Quest const* q,QuestObjective const* o,int32 n){data[{q->id,o->ObjectID}]=n;}
 void SendQuestUpdateAddCredit(Quest const* q,ObjectGuid,QuestObjective const& o,uint32 n){assert(n>0);if(q->id==38358)assert(n==uint32(GetQuestObjectiveData(q->id,o.ObjectID)));++packets;}
 bool HasQuestObjectiveComplete(Quest const* q,QuestObjective const& o){return GetQuestObjectiveData(q->id,o.ObjectID)>=int32(o.Amount);}
 bool IsQuestWeekRewarded(uint32){return false;}bool IsQuestSeasonalRewarded(uint32){return false;}
 bool SatisfyQuestRace(Quest const*,bool){return true;}void SendCanTakeQuestResponse(int,Quest const*,char const*){}
 bool SatisfyQuestPreviousQuest(Quest const*,bool);bool CanCompleteQuest(uint32);bool CanTakeQuest(Quest const* q,bool b){return SatisfyQuestPreviousQuest(q,b);}
 void CompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_COMPLETE;++completions;}
 uint32 GetQuestSlotQuestId(uint8 index){auto i=quests.begin();while(index--&&i!=quests.end())++i;return i==quests.end()?0:i->first;}
 void QuestObjectiveSatisfy(uint32,uint32,QuestObjectiveType=QUEST_OBJECTIVE_MONSTER,ObjectGuid=ObjectGuid::Empty);
 bool HasAura(uint32 a){return auras.count(a);}bool HasAuraType(int){return otherPower||HasAura(181970);}
 void RemoveAurasDueToSpell(uint32 a){if(auras.erase(a)&&a==181970)power=0;}void SetPower(int,uint32 n){power=n;}
 int CastSpell(Player*,uint32 a,bool){if(a==181981){if(!castResult&&castHook)castHook();return castResult;}if(!castResult)auras.insert(a);return castResult;}
 void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Accept(uint32 q){quests.erase(q);quests[q].Status=QUEST_STATUS_INCOMPLETE;for(auto i=data.begin();i!=data.end();)if(i->first.first==q)i=data.erase(i);else ++i;}
 FakeMap* GetMap()const{static FakeMap m;return &m;}bool CanGetItemForLoot(void const*,bool)const{return true;}
};
#include "QuestDelayedEvent.inc"
#include "CanCompleteQuest.inc"
#include "PreviousQuest.inc"
#include "QuestObjectiveSatisfy.inc"
struct Hook {void operator+=(int){}};
struct SpellScript {Unit* caster=nullptr;int32 effectValue=15;bool prevented=false;Hook OnCheckCast,OnEffectHitTarget,AfterCast;
 Unit* GetCaster(){return caster;}int32 GetEffectValue(){return effectValue;}void PreventHitDefaultEffect(int){prevented=true;}virtual void Register(){}};
#define PrepareSpellScript(x) public:
#define SpellCheckCastFn(x) 0
#define SpellEffectFn(...) 0
#define SpellCastFn(x) 0
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}};
#include "Shore.inc"

// The real ordinary loot processor establishes whether zero mode excludes
// Malwick's non-grouped quest item. RNG and inventory services are stubbed.
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

void NativeCast(Player& p,int32 value=15)
{
 spell_stormheim_shore_pump spell;spell.caster=&p;spell.effectValue=value;
 assert(spell.Check()==SPELL_CAST_OK);spell.Pump(0);assert(spell.prevented);spell.Complete();
 auto packets=p.packets;spell.Complete();assert(p.packets==packets);
}
void Collect(Player& p,GameObject& go,int32 value=15)
{p.castHook=[&]{NativeCast(p,value);};go_stormheim_volatile_barrel use;assert(!use.OnGossipHello(&p,&go));assert(StormheimShore::attempts.empty());}
int main()
{
 for(uint32 id:{39800u,39801u,38036u,38052u,38053u,38057u,38332u,38357u,38358u})store.quests[id].id=id;
 for(uint32 id:{38036u,38052u,38053u,38057u})store.quests[id].prevQuests={39800};
 for(uint32 id:{38332u,38357u,38358u})store.quests[id].prevQuests={39801};
 store.quests[38036].objectives={{QUEST_OBJECTIVE_ITEM,123876,5,0,0}};
 store.quests[38052].objectives={{QUEST_OBJECTIVE_MONSTER,90899,8,0,1}};
 store.quests[38053].objectives={{QUEST_OBJECTIVE_ITEM,122295,1,1,0}};
 store.quests[38357].objectives={{QUEST_OBJECTIVE_MONSTER,91569,10,0,0}};
 store.quests[38358].objectives={{0,91590,1,0,0},{0,91693,100,12,1}};
 for(uint32 id:{38036u,38052u,38053u,38057u,38332u,38357u,38358u}){
  Player p;Quest const* q=&store.quests[id];uint32 previous=q->prevQuests[0];
  assert(!p.SatisfyQuestPreviousQuest(q,false));p.quests[previous].Status=QUEST_STATUS_COMPLETE;assert(!p.SatisfyQuestPreviousQuest(q,false));
  p.m_RewardedQuests.insert(previous);assert(p.SatisfyQuestPreviousQuest(q,false));p.Accept(id);
  if(q->objectives.empty())assert(p.CanCompleteQuest(id));else assert(!p.CanCompleteQuest(id));
 }
 // Both ordinary combat totals and inventory totals cap and complete using
 // the real quest objective/completion handlers. Pet attribution is an engine
 // boundary, not represented as an actual pet kill in this harness.
 for(auto [q,entry,type,total]:{std::tuple{38036u,123876u,1,5},std::tuple{38052u,90899u,0,8},std::tuple{38053u,122295u,1,1},std::tuple{38357u,91569u,0,10}}){
  Player p;p.Accept(q);for(int n=0;n<total;++n)p.QuestObjectiveSatisfy(entry,1,type);
  assert(p.GetQuestStatus(q)==QUEST_STATUS_COMPLETE&&p.completions==1);p.QuestObjectiveSatisfy(entry,1,type);assert(p.completions==1);
 }
 Player p;p.Accept(38358);GameObject go;objects[go.guid]=&go;player_stormheim_shore player;
 player.OnLogin(&p);assert(p.HasAura(181970)&&p.power==0);
 for(int i=0;i<6;++i)Collect(p,go);assert(p.power==90&&p.GetQuestStatus(38358)==QUEST_STATUS_INCOMPLETE);
 Collect(p,go,20);assert(StormheimShore::Progress(&p)==100&&p.GetQuestStatus(38358)==QUEST_STATUS_COMPLETE&&p.completions==1&&p.HasAura(182151)&&!p.HasAura(181970));
 auto packets=p.packets;StormheimShore::Sync(&p);assert(p.packets==packets);
 p.quests[38358].Status=QUEST_STATUS_REWARDED;player.OnUpdate(&p,1000);assert(!p.HasAura(182151));
 p={};p.Accept(38358);Collect(p,go,20);p.auras.clear();player.OnLogin(&p);assert(p.power==20);
 p.area=1;player.OnUpdate(&p,1000);assert(!p.HasAura(181970));p.area=7563;player.OnUpdate(&p,1000);assert(p.power==20);
 p.quests[38358].Status=QUEST_STATUS_NONE;p.quests[38358].ScriptLifetime.reset();player.OnUpdate(&p,1000);assert(!p.HasAura(181970));p.Accept(38358);player.OnUpdate(&p,1000);assert(p.power==0);
 p.castResult=1;go_stormheim_volatile_barrel use;auto beforeFailure=p.packets;assert(use.OnGossipHello(&p,&go)&&StormheimShore::attempts.empty()&&p.packets==beforeFailure);p.castResult=0;
 for(int reason=0;reason<18;++reason){Player bad;bad.Accept(38358);GameObject object;object.guid={uint64(200+reason)};objects[object.guid]=&object;
  if(reason==0)bad.team=ALLIANCE;if(reason==1)bad.map=0;if(reason==2)bad.area=1;if(reason==3)bad.alive=false;if(reason==4)bad.teleport=true;
  if(reason==5)bad.vehicle=true;if(reason==6)bad.transport=true;if(reason==7)bad.combat=true;if(reason==8)bad.phase=false;if(reason==9)bad.visible=false;
  if(reason==10)bad.los=false;if(reason==11)object.distance=6;if(reason==12)object.world=false;if(reason==13)object.spawned=false;
  if(reason==14)object.flags=GO_FLAG_IN_USE;if(reason==15)object.flags=GO_FLAG_NOT_SELECTABLE;if(reason==16)object.state=2;if(reason==17)bad.world=false;
  use.OnGossipHello(&bad,&object);assert(!bad.packets&&StormheimShore::attempts.empty());
 }
 // Successful Check alone cannot grant credit; interrupted/old acceptance,
 // changed targets and another player's pending proof cannot complete it.
 for(int reason=0;reason<6;++reason){p={};p.Accept(38358);p.AddQuestDelayedEvent(38358,0,[]{});
  StormheimShore::attempts[p.guid]={go.guid,p.getQuestStatus(38358)->ScriptLifetime,++StormheimShore::nextToken};
  spell_stormheim_shore_pump spell;spell.caster=&p;assert(spell.Check()==0);
  if(reason==0)p.Accept(38358);if(reason==1)go.world=false;if(reason==2)p.phase=false;if(reason==3)StormheimShore::attempts[p.guid].token++;
  if(reason==4)spell.effectValue=1;if(reason!=5){spell.Pump(0);spell.Complete();}assert(!p.packets);go.world=true;StormheimShore::attempts.clear();
 }
 p={};p.Accept(38358);p.data[{38358,91693}]=100;player.OnLogin(&p);assert(p.completions==1);player.OnLogin(&p);assert(p.completions==1);
 p={};p.Accept(38358);p.otherPower=true;StormheimShore::Sync(&p);assert(!p.HasAura(181970));
 Player first,second;first.Accept(38358);second.Accept(38358);second.guid={2};go.entry=249829;
 Collect(first,go,16);assert(StormheimShore::Progress(&first)==16&&!StormheimShore::Progress(&second));
 Collect(second,go,19);assert(StormheimShore::Progress(&first)==16&&StormheimShore::Progress(&second)==19);
 spell_stormheim_shore_pump unarmed;unarmed.caster=&second;assert(unarmed.Check()==SPELL_FAILED_BAD_TARGETS);unarmed.Pump(0);unarmed.Complete();assert(unarmed.prevented&&StormheimShore::Progress(&second)==19);
 for(uint32 mode:{0u,1u,2u}){Player owner;Loot loot;loot.owner=&owner;LootTemplate tab;tab.Entries={{122295,mode}};tab.Process(loot,false);assert(loot.items.size()==(mode==2?0:1));
  loot.items.clear();tab.Entries[0].allowRoll=false;tab.Process(loot,false);assert(loot.items.empty());}
 std::cout<<"PASS: extracted shore handlers, real prerequisite/completion/objective/loot handlers; seven quest gates, native combat/item totals, successful Pump quantities, full-container credit once, cast failure, interrupted/stale acceptance, invalid interactions, alternate-power isolation and login/abandon/re-entry recovery.\n";
}
