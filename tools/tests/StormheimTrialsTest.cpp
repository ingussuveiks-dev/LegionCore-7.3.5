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

struct Unit;struct Player;struct Creature;struct GameObject;struct SmartAI;
struct Vehicle {struct Seat {struct {ObjectGuid Guid;} Passenger;};std::map<int,Seat> Seats;struct Unit* GetPassenger(int);};
struct WorldObject {ObjectGuid guid{1};uint32 map=1220,zone=7541,area=7557,entry=0;bool world=true,alive=true;float distance=0;
 virtual ~WorldObject()=default;virtual Creature* ToCreature(){return nullptr;}virtual Player* ToPlayer(){return nullptr;}virtual Unit* ToUnit(){return nullptr;}virtual bool IsCreature(){return false;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}uint32 GetAreaId(){return area;}virtual ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}};
struct Unit:WorldObject {Vehicle* kit=nullptr;Unit* ToUnit()override{return this;}Vehicle* GetVehicleKit(){return kit;}bool InSamePhase(WorldObject*){return true;}virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct SpellInfo {uint32 Id=180737;};
struct CreatureTemplate {uint32 StateWorldEffectID=0,SpellStateVisualID=45944,SpellStateAnimID=0,SpellStateAnimKitID=6050;};
struct Creature:Unit {SmartAI* ai=nullptr;SmartAI*& i_AI=ai;SmartAI* i_disabledAI=nullptr;bool charmed=false;bool IsPlayer(){return false;}bool isCharmed(){return charmed;}bool isPossessed(){return false;}bool IsVehicle(){return kit!=nullptr;}void UpdateCharmAI();std::vector<std::function<void()>> callbacks;void AddDelayedEvent(uint64,std::function<void()> f){callbacks.push_back(std::move(f));}void Tick(){auto q=std::move(callbacks);callbacks.clear();for(auto& f:q)f();}CreatureTemplate data;std::map<int,uint32> fields;uint32 stand=8,dirty=0,talks=0;bool visible=true,combat=false;
 Creature* ToCreature()override{return this;}bool IsCreature()override{return true;}Player* GetLootRecipient(){return nullptr;}void AddPlayerInPersonnalVisibilityList(ObjectGuid p){personal=p;}ObjectGuid personal;void setActive(bool){}void DespawnOrUnsummon(){world=false;}SmartAI* AI(){return ai;}CreatureTemplate const* GetCreatureTemplate(){return &data;}
 bool HasFlag(int f,uint32 v){return fields[f]&v;}void SetFlag(int f,uint32 v){fields[f]|=v;}void RemoveFlag(int f,uint32 v){fields[f]&=~v;}
 void SetUInt32Value(int f,uint32 v){fields[f]=v;}void ForceValuesUpdateAtIndex(int){++dirty;}void SetVisible(bool v){visible=v;}void SetStandState(uint32 s){stand=s;}
 void RemoveAurasDueToSpell(uint32){}bool HasUnitState(int){return false;}uint32 GetCreatureType(){return 7;}};
struct GameObject:WorldObject {uint32 flags=0,state=GO_READY;bool spawned=true;bool isSpawned(){return spawned;}bool HasFlag(int,uint32 f){return flags&f;}};
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
 Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}
 uint32 GetTeam(){return team;}uint32 GetMapId(){return map;}uint32 GetAreaId(){return area;}bool IsBeingTeleported(){return teleport;}
 bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInCombat(){return combat;}
 Unit* vehicleBase=nullptr;Unit* GetVehicleBase(){return vehicleBase;}void ExitVehicle(){if(vehicleBase&&vehicleBase->kit)vehicleBase->kit->Seats.clear();vehicleBase=nullptr;vehicle=false;}bool InSamePhase(WorldObject*){return phase;}bool canSeeOrDetect(WorldObject*){return visible;}bool IsWithinLOSInMap(WorldObject*){return los;}
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
 bool CanRewardQuest(Quest const*,bool);bool IsQuestDailyRewarded(uint32){return false;}bool GetQuestRewardStatus(uint32 q){return m_RewardedQuests.count(q);}uint32 GetItemCount(uint32){return 0;}void SendEquipError(int,void*,void*,uint32){}bool HasCurrency(uint32,uint32){return false;}bool HasEnoughMoney(uint64){return false;}void CompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_COMPLETE;++completions;}
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
namespace ObjectAccessor {
Player* GetPlayer(WorldObject&,ObjectGuid id){auto i=players.find(id);return i==players.end()||!i->second->world?nullptr:i->second;}
Player* FindPlayer(ObjectGuid id){Creature ignored;return GetPlayer(ignored,id);}
GameObject* GetGameObject(WorldObject&,ObjectGuid id){auto i=objects.find(id);return i==objects.end()||!i->second->world?nullptr:i->second;}}
Unit* Vehicle::GetPassenger(int index){auto i=Seats.find(index);return i==Seats.end()?nullptr:ObjectAccessor::FindPlayer(i->second.Passenger.Guid);}
using ObjectList=std::list<WorldObject*>;
bool IsUnit(WorldObject* u){return u&&u->ToUnit();}bool IsPlayer(WorldObject* u){return u&&u->ToPlayer();}
enum {SMART_ACTION_CALL_KILLEDMONSTER=33};
struct NativeSmartCredit {
 Creature* me;bool trigger=false;WorldObject* GetBaseObject(){return me;}template<class E>ObjectList* GetTargets(E const&,Unit* u){return new ObjectList{u};}
 void Apply(Unit* unit,uint32 id){struct Event {struct {struct {uint32 creature;} killedMonster;} action;} e{{{id}}};
 switch(33){
#include "SmartCredit.inc"
 }}
};
struct SmartAI {Creature* me;uint32 delegated=0;explicit SmartAI(Creature* c):me(c){c->ai=this;}virtual ~SmartAI()=default;
 virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual ObjectGuid GetGUID(int32){return {};}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){++delegated;}
 virtual void PassengerBoarded(Unit*,int8,bool){++delegated;}virtual void IsSummonedBy(Unit*){++delegated;}virtual void UpdateAI(uint32){++delegated;}
 virtual void SpellHit(Unit* caster,SpellInfo const* spell){++delegated;if(me->entry==110604){uint32 id=spell->Id==219924?110610:spell->Id==219936?110611:spell->Id==219947?110612:0;if(id){NativeSmartCredit native{me};native.Apply(caster,id);}}}
};
struct PossessedAI:SmartAI {explicit PossessedAI(Creature* c):SmartAI(c){}};
struct PetAI:SmartAI {explicit PetAI(Creature* c):SmartAI(c){}};
// Compile the unchanged production body against the fixture creature, which
// carries the same current/disabled AI slots as its real Unit base.
#define Unit Creature
#include "UpdateCharmAI.inc"
#undef Unit
struct Hook {std::vector<int> values;void operator+=(int i){values.push_back(i);}};
struct SpellScript {uint32 m_scriptSpellId=219924;Unit* caster=nullptr;Unit* original=nullptr;Hook OnObjectAreaTargetSelect;Unit* GetCaster(){return caster;}Unit* GetOriginalCaster(){return original?original:caster;}virtual void Register(){}};
#define PrepareSpellScript(x) public:
#define SpellObjectAreaTargetSelectFn(F,I,T) (1000+(I)*100+(T))
struct GameObjectAI {virtual ~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual bool GossipUse(Player*){return false;}};
struct SmartGameObjectAI:GameObjectAI {uint32 delegated=0;explicit SmartGameObjectAI(GameObject*){}bool GossipHello(Player*,bool)override{++delegated;return false;}bool GossipUse(Player*)override{++delegated;return false;}};
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}virtual GameObjectAI* GetAI(GameObject*)const{return nullptr;}};
#include "Trials.inc"
struct Session {Creature me;Vehicle kit;npc_stormheim_havi_actor ai;Session(Player& p,uint32 entry,uint64 id):ai(&me){me.entry=entry;me.guid={id};me.area=7808;me.kit=&kit;me.fields[UNIT_FIELD_NPC_FLAGS]=UNIT_NPC_FLAG_SPELLCLICK;creatures[me.guid]=&me;ai.IsSummonedBy(&p);}~Session(){me.callbacks.clear();if(me.i_disabledAI){me.charmed=false;me.UpdateCharmAI();}}void Board(Player& p){kit.Seats[0].Passenger.Guid=p.guid;p.vehicleBase=&me;p.vehicle=true;ai.PassengerBoarded(&p,0,true);me.charmed=true;me.UpdateCharmAI();assert(dynamic_cast<PossessedAI*>(me.AI())&&me.i_disabledAI==&ai);}};
void Setup(Player& p,uint64 id=1){p={};p.guid={id};p.area=7808;p.Accept(38331);players[p.guid]=&p;}
std::unique_ptr<Session> Grab(Player& p,GameObject& object,bool fail=false,int route=0)
{
 static uint64 next=200;std::unique_ptr<Session> session;go_stormheim_havi_ingredient go;
 objects[object.guid]=&object;
 p.castHook=[&]{auto kind=StormheimTrials::Kind(object.entry);session=std::make_unique<Session>(p,kind->vehicle,++next);};
 p.castResult=fail?1:0;if(route==0)go.OnGossipHello(&p,&object);else{std::unique_ptr<GameObjectAI> ai(go.GetAI(&object));if(route==1)ai->GossipHello(&p,false);else ai->GossipUse(&p);}p.castHook={};p.castResult=0;return session;
}
void Toss(Player& p,Session& session,Creature& pot,uint32 impact)
{
 spell_stormheim_havi_impact spell;spell.m_scriptSpellId=impact;spell.caster=&session.me;
 std::list<WorldObject*> targets{&pot};spell.Targets(targets);assert(targets.size()==1);
 SpellInfo info{impact};pot.ai->SpellHit(&session.me,&info);
 auto packets=p.packets;pot.ai->SpellHit(&session.me,&info);assert(p.packets==packets);
}
int main()
{
 for(uint32 id:{38060u,38362u,38210u,38459u,38331u,39590u})store.quests[id].id=id;
 store.quests[38210].prevQuests={38060};store.quests[38459].prevQuests={38362};store.quests[38210].ExclusiveGroup=store.quests[38459].ExclusiveGroup=38459;
 store.quests[38331].prevQuests={38210,38210,38459};store.quests[38331].special=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST;
 store.quests[38331].objectives={{0,110610,1,0,0},{0,110611,1,0,1},{0,110612,1,0,2}};store.quests[39590].prevQuests={38331};
 for(uint32 team:{ALLIANCE,HORDE}){Player p;Setup(p);p.team=team;uint32 trial=team==ALLIANCE?38210:38459,prev=team==ALLIANCE?38060:38362;
  assert(!p.CanTakeQuest(&store.quests[trial],false));p.m_RewardedQuests.insert(prev);assert(p.CanTakeQuest(&store.quests[trial],false));p.Accept(trial);assert(p.CanCompleteQuest(trial));p.CompleteQuest(trial);
  assert(p.CanRewardQuest(&store.quests[trial],false));assert(!p.CanTakeQuest(&store.quests[38331],false));p.m_RewardedQuests.insert(trial);assert(p.CanTakeQuest(&store.quests[38331],false));
  p.Accept(38331);assert(!p.CanCompleteQuest(38331));Creature pot;pot.entry=110604;pot.area=7808;npc_stormheim_havi_actor potAI(&pot);
  for(auto const& kind:StormheimTrials::Ingredients){GameObject go;go.entry=kind.object;go.area=7808;go.guid={kind.object};auto session=Grab(p,go,false,int(kind.credit-110610));assert(session&&session->ai.owner==p.guid&&!p.GetQuestObjectiveData(38331,kind.credit));assert(!session->me.HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK));
   SpellInfo impact{kind.impact};potAI.SpellHit(&session->me,&impact);assert(!p.GetQuestObjectiveData(38331,kind.credit));session->Board(p);Toss(p,*session,pot,kind.impact);assert(p.GetQuestObjectiveData(38331,kind.credit)==1);p.ExitVehicle();session->me.Tick();assert(!session->me.world);}
  assert(p.GetQuestStatus(38331)==QUEST_STATUS_COMPLETE&&p.CanRewardQuest(&store.quests[38331],false));assert(!p.CanTakeQuest(&store.quests[39590],false));p.m_RewardedQuests.insert(38331);assert(p.CanTakeQuest(&store.quests[39590],false));}
 for(uint32 id:{219924u,219936u,219947u}){spell_stormheim_havi_impact script;script.m_scriptSpellId=id;script.Register();assert(script.OnObjectAreaTargetSelect.values==std::vector<int>{1008});}
 for(int reason=0;reason<16;++reason){Player p;Setup(p);GameObject object;object.entry=252074;object.area=7808;object.guid={252074};
  if(reason==0)p.map=1;if(reason==1)p.area=1;if(reason==2)p.alive=false;if(reason==3)p.teleport=true;if(reason==4)p.vehicle=true;if(reason==5)p.transport=true;
  if(reason==6)p.combat=true;if(reason==7)p.phase=false;if(reason==8)p.visible=false;if(reason==9)p.los=false;if(reason==10)object.distance=7;if(reason==11)object.spawned=false;if(reason==12)object.flags=GO_FLAG_NOT_SELECTABLE;if(reason==13)object.flags=GO_FLAG_IN_USE;if(reason==14)p.quests[38331].Status=QUEST_STATUS_NONE;if(reason==15)p.data[{38331,110610}]=1;
  auto s=Grab(p,object,false,reason%3);assert(!s&&StormheimTrials::pending.empty()&&StormheimTrials::vehicles.empty());}
 Player p;Setup(p);GameObject go;go.entry=252074;go.area=7808;go.guid={252074};assert(!Grab(p,go,true));assert(!p.packets&&StormheimTrials::pending.empty());
 auto first=Grab(p,go);assert(first);assert(!Grab(p,go));first->Board(p);Creature pot;pot.entry=110604;pot.area=7808;npc_stormheim_havi_actor potAI(&pot);
 Creature unrelated;unrelated.area=7808;unrelated.entry=96254;spell_stormheim_havi_impact filter;filter.caster=&first->me;
 std::list<WorldObject*> targets{&pot,&unrelated,&go};filter.Targets(targets);assert(targets==std::list<WorldObject*>{&pot});
 auto before=p.packets;SpellInfo wrong{219936};potAI.SpellHit(&first->me,&wrong);assert(p.packets==before);
 // Preserve native passenger/group credit, including its normal range checks.
 Player member,far;Setup(member,2);Setup(far,3);far.rewardRange=false;GroupReference c{&far},b{&member,&c},a{&p,&b};Group group{&a};p.group=&group;
 Toss(p,*first,pot,219924);assert(member.GetQuestObjectiveData(38331,110610)==1&&!far.GetQuestObjectiveData(38331,110610));p.ExitVehicle();first->ai.Stop();p.group=nullptr;
 // Old casts and accepted sessions cannot survive abandon/reaccept, phase,
 // death, logout, transfer, or a seat occupied by someone else.
 for(int reason=0;reason<8;++reason){Setup(p);auto stale=Grab(p,go);stale->Board(p);before=p.packets;
  if(reason==0)p.Accept(38331);if(reason==1)p.phase=false;if(reason==2)p.alive=false;if(reason==3){p.ExitVehicle();p.world=false;}if(reason==4)p.map=1;if(reason==5)p.teleport=true;if(reason==6)p.ExitVehicle();if(reason==7)stale->kit.Seats[0].Passenger.Guid=member.guid;
  SpellInfo info{219924};potAI.SpellHit(&stale->me,&info);assert(p.packets==before);stale->me.Tick();assert(!stale->me.world&&!p.vehicleBase);stale->ai.Stop();}
 Setup(p);auto neverBoarded=Grab(p,go);for(int tick=0;tick<20;++tick)neverBoarded->me.Tick();assert(!neverBoarded->me.world&&StormheimTrials::vehicles.empty()&&!p.packets);
 Setup(p);Player independent;Setup(independent,9);auto one=Grab(p,go);auto two=Grab(independent,go);one->Board(p);two->Board(independent);Toss(p,*one,pot,219924);assert(!independent.packets);Toss(independent,*two,pot,219924);assert(independent.packets==1&&two->me.personal==independent.guid);p.ExitVehicle();independent.ExitVehicle();one->ai.Stop();two->ai.Stop();
 Setup(p);auto old=Grab(p,go);p.Accept(38331);auto fresh=Grab(p,go);assert(fresh&&!old->me.world);old.reset();assert(StormheimTrials::vehicles[p.guid]==fresh->me.guid);fresh.reset();assert(StormheimTrials::vehicles.empty());
 Creature later;later.map=1522;later.area=7808;later.entry=110604;npc_stormheim_havi_actor legacy(&later);SpellInfo info{219924};legacy.SpellHit(&p,&info);assert(legacy.delegated==1);
 Setup(p);auto current=Grab(p,go);current->Board(p);auto saved=p.packets;SpellInfo forged{219924};potAI.SpellHit(&p,&forged);assert(p.packets==saved);p.ExitVehicle();current->ai.Stop();
 GameObject laterGO;laterGO.map=1522;go_stormheim_havi_ingredient ingredient;std::unique_ptr<GameObjectAI> ai(ingredient.GetAI(&laterGO));ai->GossipHello(&p,false);assert(dynamic_cast<go_stormheim_havi_ingredient::IngredientAI*>(ai.get())->delegated==1);
 Creature unarmed;unarmed.entry=110605;unarmed.area=7808;npc_stormheim_havi_actor ordinary(&unarmed);ordinary.IsSummonedBy(&p);assert(ordinary.owner.IsEmpty()&&ordinary.delegated==1);
 std::cout<<"PASS: extracted native quest/completion/hand-in/previous/group and SmartAI passenger-credit handlers plus complete trial GO/actor/impact handlers; both faction routes, native empty travel objectives, real three pot hits, private once-only vehicle ownership, failed/duplicate grabs, target/seat guards, native party range behavior, stale-acceptance cleanup/retry and later-map/unarmed delegation.\n";
}
