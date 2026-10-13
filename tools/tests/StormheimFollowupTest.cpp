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
 UNIT_FIELD_NPC_FLAGS=11,UNIT_NPC_FLAG_SPELLCLICK=0x1000000,UNIT_FIELD_STATE_WORLD_EFFECT_ID=12,UNIT_FIELD_STATE_SPELL_VISUAL_ID=13,UNIT_FIELD_STATE_ANIM_ID=14,UNIT_FIELD_STATE_ANIM_KIT_ID=15,UNIT_STAND_STATE_KNEEL=8,UNIT_STAND_STATE_STAND=0,UNIT_STATE_CASTING=1,QUEST_SPECIAL_FLAGS_KILL=32,QUEST_SPECIAL_FLAGS_CAST=64,CRITERIA_TYPE_KILL_CREATURE_TYPE=1,CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD=2,CRITERIA_TIMED_TYPE_CREATURE=3,CRITERIA_TYPE_KILL_CREATURE=4,LOOT_ITEM_TYPE_ITEM=0,RATE_DROP_ITEM_REFERENCED_AMOUNT=0};
using QuestObjectiveType=int;
using PrevQuests=std::vector<int32>; using ExclusiveQuestGroups=std::multimap<int32,uint32>;
#define ASSERT assert
#define TC_LOG_DEBUG(...) (void)0
struct ObjectGuid {uint64 value=0; static ObjectGuid const Empty; bool IsEmpty()const{return !value;}bool operator==(ObjectGuid o)const{return value==o.value;}bool operator<(ObjectGuid other)const{return value<other.value;}};
ObjectGuid const ObjectGuid::Empty{};
struct QuestObjective {int Type=0;uint32 ObjectID=0,Amount=1,Flags=0;int8 StorageIndex=0;};
struct Quest {uint32 id=0,special=0;int32 ExclusiveGroup=0;PrevQuests prevQuests;std::vector<QuestObjective> objectives;
 auto const& GetObjectives()const{return objectives;}bool IsWeekly()const{return false;}bool IsSeasonal()const{return false;}
 bool IsRepeatable()const{return false;}bool IsAutoComplete()const{return false;}bool HasSpecialFlag(uint32 f)const{return special&f;}bool IsAllowedInRaid(int)const{return true;}};
struct QuestStore {std::map<uint32,Quest> quests;ExclusiveQuestGroups mExclusiveQuestGroups;
 Quest const* GetQuestTemplate(uint32 id){auto i=quests.find(id);return i==quests.end()?nullptr:&i->second;}} store;
auto sQuestDataStore=&store;
struct QuestStatusData {int Status=QUEST_STATUS_NONE;uint32 Timer=0;std::shared_ptr<uint8> ScriptLifetime;};

struct Player;struct Creature;struct SmartAI;
struct WorldObject {ObjectGuid guid{1};uint32 map=1220,zone=7541,area=7557,entry=0;bool world=true,alive=true;float distance=0;
 virtual ~WorldObject()=default;uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}uint32 GetAreaId(){return area;}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}};
struct Unit:WorldObject {virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct SpellInfo {uint32 Id=180737;};
struct CreatureTemplate {uint32 StateWorldEffectID=0,SpellStateVisualID=45944,SpellStateAnimID=0,SpellStateAnimKitID=6050;};
struct Creature:Unit {SmartAI* ai=nullptr;CreatureTemplate data;std::map<int,uint32> fields;uint32 stand=8,dirty=0,talks=0;bool visible=true,combat=false;
 Creature* ToCreature()override{return this;}SmartAI* AI(){return ai;}CreatureTemplate const* GetCreatureTemplate(){return &data;}
 bool HasFlag(int f,uint32 v){return fields[f]&v;}void SetFlag(int f,uint32 v){fields[f]|=v;}void RemoveFlag(int f,uint32 v){fields[f]&=~v;}
 void SetUInt32Value(int f,uint32 v){fields[f]=v;}void ForceValuesUpdateAtIndex(int){++dirty;}void SetVisible(bool v){visible=v;}void SetStandState(uint32 s){stand=s;}
 void RemoveAurasDueToSpell(uint32){}bool HasUnitState(int){return false;}uint32 GetCreatureType(){return 7;}};
struct GameObject:WorldObject {uint32 flags=0,state=GO_READY;bool spawned=true;bool isSpawned(){return spawned;}bool HasFlag(int,uint32 f){return flags&f;}};
std::map<ObjectGuid,Creature*> creatures;
namespace ObjectAccessor {Creature* GetCreature(Player&,ObjectGuid id){auto i=creatures.find(id);return i==creatures.end()||!i->second->world?nullptr:i->second;}}
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
 Player* ToPlayer()override{return this;}ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}
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
 int CastSpell(Player*,uint32 a,bool){if(!castResult)auras.insert(a);return castResult;}
 void AddDelayedEvent(uint64,std::function<void()> f){events.push_back(std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void Accept(uint32 q){quests.erase(q);quests[q].Status=QUEST_STATUS_INCOMPLETE;for(auto i=data.begin();i!=data.end();)if(i->first.first==q)i=data.erase(i);else ++i;}
 Achievement achievement;Achievement* m_achievementMgr=&achievement;Group* GetGroup(){return nullptr;}uint32 GetGuildId(){return 0;}template<class... T>void UpdateAchievementCriteria(T...){}void AddKillCreature(uint32,uint16,bool){}void AddTrackingQuestIfNeeded(ObjectGuid){}void SetQuestStatus(uint32 q,int state){quests[q].Status=state;}void KilledMonsterCredit(uint32,ObjectGuid=ObjectGuid::Empty);FakeMap* GetMap()const{static FakeMap m;return &m;}bool CanGetItemForLoot(void const*,bool)const{return true;}
};
#include "QuestDelayedEvent.inc"
#include "CanCompleteQuest.inc"
#include "PreviousQuest.inc"
#include "QuestObjectiveSatisfy.inc"
#include "KilledMonsterCredit.inc"
struct Hook {std::vector<int> values;void operator+=(int value){values.push_back(value);}};
struct SpellScript {uint32 m_scriptSpellId=0;bool hasSpellContext=true;Unit* original=nullptr;Unit* target=nullptr;Unit* source=nullptr;SpellInfo info;Unit* caster=nullptr;int32 effectValue=15;bool prevented=false;Hook OnCheckCast,OnEffectHitTarget,AfterCast;
 Unit* GetCaster(){return caster;}Unit* GetOriginalCaster(){return source?source:caster;}Unit* GetOriginalTarget(){return original;}SpellInfo const* GetSpellInfo(){assert(hasSpellContext);return &info;}Player* GetHitPlayer(){return target?target->ToPlayer():nullptr;}Creature* GetHitCreature(){return target?target->ToCreature():nullptr;}int32 GetEffectValue(){return effectValue;}void PreventHitDefaultEffect(int){prevented=true;}virtual void Register(){}};
#define PrepareSpellScript(x) public:
#define SpellCheckCastFn(x) 0
#define SpellEffectFn(F,I,E) (1000+(I)*100+(E))
#define SpellCastFn(x) 0
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual bool OnGossipHello(Player*,GameObject*){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}};

struct SmartAI {Creature* me;explicit SmartAI(Creature* c):me(c){c->ai=this;}virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){}virtual void SpellHit(Unit*,SpellInfo const*){}virtual void UpdateAI(uint32){}bool UpdateVictim(){return me->combat;}void DoCastVictim(uint32){}void DoMeleeAttackIfReady(){}void Talk(uint32,ObjectGuid){++me->talks;}};
#include "Followup.inc"

void Click(Player& p, Creature& c, uint32 spellId)
{
 spell_stormheim_followup_click spell;spell.caster=&p;spell.original=&c;spell.info.Id=spellId;
 // 182313's explicit target is cleared by the production target initializer;
 // only its original clicked target remains, while its effect hits the player.
 spell.target=spellId==180737?static_cast<Unit*>(&c):static_cast<Unit*>(&p);
 assert(spell.Check()==SPELL_CAST_OK);spell.Action(0);spell.Suppress(0);assert(spell.prevented);spell.Complete();
 auto packets=p.packets;spell.Complete();assert(p.packets==packets);
}
int main()
{
 // Startup registration has a script ID but no active Spell instance.
 for(uint32 id:{180737u,182313u}){spell_stormheim_followup_click script;script.m_scriptSpellId=id;script.hasSpellContext=false;script.Register();assert(script.OnEffectHitTarget.values==std::vector<int>(id==180737?std::initializer_list<int>{1003,1130}:std::initializer_list<int>{1134,1134}));}
 for(uint32 id:{180677u,183386u,183454u}){spell_stormheim_followup_lightning script;script.m_scriptSpellId=id;script.hasSpellContext=false;script.Register();assert(script.OnEffectHitTarget.values==std::vector<int>{id==183386?1130:1190});}
 for(uint32 id:{38058u,38059u,38360u,38060u,90000u})store.quests[id].id=id;
 store.quests[38058].objectives={{0,90866,1,0,0},{0,90903,100,12,1}};
 store.quests[38059].objectives={{0,90948,6,0,0}};
 store.quests[38360].objectives={{2,241031,1,0,0},{0,91904,1,2,1},{2,241032,1,2,2}};
 store.quests[90000].special=QUEST_SPECIAL_FLAGS_KILL;store.quests[90000].objectives={{0,90903,2,0,0}};
 Player p;p.Accept(38058);p.Accept(38059);StormheimFollowup::SyncCharge(&p);assert(p.HasAura(180645)&&p.power==0);
 Creature drake;drake.entry=90903;drake.guid={20};creatures[drake.guid]=&drake;
 npc_stormheim_followup_actor drakeAI(&drake);drakeAI.Reset();drake.combat=true;drakeAI.UpdateAI(3000);
 // Actual generic kill dispatch must not grant electricity, including calls
 // used for a pet's attributed kill. An unrelated ordinary kill quest works.
 p.Accept(90000);p.KilledMonsterCredit(90903);assert(!StormheimFollowup::Value(&p,38058,90903)&&StormheimFollowup::Value(&p,90000,90903)==1);
 p.KilledMonsterCredit(90903,drake.guid);assert(!StormheimFollowup::Value(&p,38058,90903)&&p.GetQuestStatus(90000)==QUEST_STATUS_COMPLETE);
 for(int i=0;i<24;++i){spell_stormheim_followup_lightning hit;hit.caster=&drake;hit.target=&p;hit.effectValue=4;hit.Charge(1);assert(hit.prevented);}
 assert(p.power==96&&p.GetQuestStatus(38058)==QUEST_STATUS_INCOMPLETE);
 spell_stormheim_followup_lightning last;last.caster=&drake;last.target=&p;last.effectValue=4;last.Charge(1);
 assert(p.GetQuestStatus(38058)==QUEST_STATUS_COMPLETE&&p.HasAura(180860)&&!p.HasAura(180645));
 auto packets=p.packets;last.Charge(1);assert(p.packets==packets);
 p={};p.Accept(38058);p.data[{38058,90903}]=40;player_stormheim_followup watcher;watcher.OnLogin(&p);assert(p.power==40);
 p.map=0;watcher.OnUpdate(&p,500);assert(!p.HasAura(180645));p.map=1220;watcher.OnUpdate(&p,500);assert(p.power==40);
 p.quests[38058].Status=QUEST_STATUS_NONE;p.quests[38058].ScriptLifetime.reset();watcher.OnUpdate(&p,500);assert(!p.HasAura(180645));p.Accept(38058);watcher.OnUpdate(&p,500);assert(p.power==0);
 for(int reason=0;reason<5;++reason){Player wrong;wrong.Accept(38058);StormheimFollowup::SyncCharge(&wrong);Creature bad;bad.entry=90903;
  if(reason==0)wrong.team=HORDE;if(reason==1)bad.entry=1;if(reason==2)bad.map=0;if(reason==3)wrong.phase=false;if(reason==4)wrong.auras.clear();
  spell_stormheim_followup_lightning hit;hit.caster=&bad;hit.target=&wrong;hit.effectValue=4;hit.Charge(1);assert(!wrong.packets);
 }
 p={};p.Accept(38058);p.Accept(38059);StormheimFollowup::SyncCharge(&p);
 std::vector<std::unique_ptr<Creature>> rescued;std::vector<std::unique_ptr<npc_stormheim_followup_actor>> ais;
 for(int i=0;i<6;++i){auto c=std::make_unique<Creature>();c->entry=90948;c->guid={uint64(100+i)};creatures[c->guid]=c.get();auto ai=std::make_unique<npc_stormheim_followup_actor>(c.get());ai->Reset();
  assert(c->fields[UNIT_FIELD_STATE_SPELL_VISUAL_ID]==45944&&c->fields[UNIT_FIELD_STATE_ANIM_KIT_ID]==6050&&!c->fields[UNIT_FIELD_STATE_WORLD_EFFECT_ID]);
  Click(p,*c,180737);assert(!ai->GetData(38059)&&c->stand==0&&!c->fields[UNIT_FIELD_STATE_SPELL_VISUAL_ID]&&c->talks==1);
  spell_stormheim_followup_click repeat;repeat.caster=&p;repeat.original=c.get();assert(repeat.Check()!=0);
  rescued.push_back(std::move(c));ais.push_back(std::move(ai));}
 assert(p.GetQuestStatus(38059)==QUEST_STATUS_COMPLETE&&p.power==6);
 ais[0]->UpdateAI(5000);assert(!rescued[0]->visible);ais[0]->UpdateAI(15000);assert(rescued[0]->visible&&ais[0]->GetData(38059));
 // Interrupted and abandoned casts leave the actor/credit untouched.
 for(int reason=0;reason<5;++reason){Player owner;owner.Accept(38059);auto c=rescued[0].get();spell_stormheim_followup_click spell;spell.caster=&owner;spell.original=c;spell.target=c;assert(spell.Check()==0);
  if(reason==0)owner.Accept(38059);if(reason==1)owner.phase=false;if(reason==2)c->world=false;if(reason==3)spell.target=&owner;
  if(reason!=4)spell.Action(0);spell.Complete();assert(!owner.packets);c->world=true;assert(ais[0]->GetData(38059));
 }
 p={};p.team=HORDE;p.Accept(38360);Creature captain;captain.entry=91904;captain.guid={500};captain.fields[UNIT_FIELD_NPC_FLAGS]=UNIT_NPC_FLAG_SPELLCLICK;
 creatures[captain.guid]=&captain;npc_stormheim_followup_actor captainAI(&captain);captainAI.Reset();
 GameObject saber,arrow;saber.entry=241031;arrow.entry=241032;go_stormheim_followup_clue clue;
 assert(clue.OnGossipHello(&p,&arrow));spell_stormheim_followup_click early;early.caster=&p;early.original=&captain;early.info.Id=182313;assert(early.Check()!=0);
 assert(!clue.OnGossipHello(&p,&saber));assert(captain.dirty);packets=p.packets;assert(clue.OnGossipHello(&p,&saber)&&p.packets==packets);
 Click(p,captain,182313);assert(captain.talks==1&&p.GetQuestStatus(38360)==QUEST_STATUS_INCOMPLETE);assert(!clue.OnGossipHello(&p,&arrow));
 assert(p.GetQuestStatus(38360)==QUEST_STATUS_COMPLETE&&p.completions==1);
 // Abandon/re-entry retries use the saved ordered objectives; NPC field
 // updates are forced when that eligibility changes, including logout/login.
 p={};p.team=HORDE;p.Accept(38360);clue.OnGossipHello(&p,&saber);auto dirty=captain.dirty;watcher.OnLogin(&p);assert(captain.dirty>dirty);
 p.quests[38360].Status=QUEST_STATUS_NONE;p.quests[38360].ScriptLifetime.reset();watcher.OnUpdate(&p,500);p.Accept(38360);assert(early.Check()!=0);clue.OnGossipHello(&p,&saber);Click(p,captain,182313);clue.OnGossipHello(&p,&arrow);assert(p.completions==1);
 // Shared entries on later/other maps retain their original SmartAI path.
 Creature elsewhere;elsewhere.entry=90948;elsewhere.map=1;npc_stormheim_followup_actor otherAI(&elsewhere);otherAI.Reset();assert(!elsewhere.fields[UNIT_FIELD_STATE_SPELL_VISUAL_ID]);
 Player visitor;visitor.map=1;spell_stormheim_followup_click delegated;delegated.caster=&visitor;delegated.original=&elsewhere;assert(delegated.Check()==0);delegated.Suppress(1);assert(!delegated.prevented);
 p={};p.Accept(38060);p.auras.insert(180860);StormheimFollowup::SyncCharge(&p);assert(p.HasAura(180860));
 p={};p.Accept(38058);p.data[{38058,90903}]=100;watcher.OnLogin(&p);assert(p.completions==1);watcher.OnLogin(&p);assert(p.completions==1);
 std::cout<<"PASS: extracted followup/quest/kill handlers; native electricity only, no player/pet corpse charge, capped durable charge, six rescue completions, native actor state fields, interrupted/stale casts, retained original click target, ordered Horde clues, explicit NPC flag updates, retry/re-entry and later-map/Signal Boost isolation.\n";
}
