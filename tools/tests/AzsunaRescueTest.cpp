#include <cassert>
#include <functional>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <vector>
using int32=int32_t; using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t; using int8=int8_t;
using SpellEffIndex=int; using DamageEffectType=int; using SpellCastResult=int;
enum class GossipOptionNpc {None};
enum { GOSSIP_SENDER_MAIN=1,GOSSIP_ACTION_INFO_DEF=100,DEFAULT_GOSSIP_MESSAGE=0,UNIT_FLAG_IMMUNE_TO_PC=64,PLAYER_FIELD_BYTES_5=5, PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET=1, QUEST_STATUS_INCOMPLETE=1, QUEST_STATUS_COMPLETE=2, QUEST_STATUS_REWARDED=3,
 TEMPSUMMON_MANUAL_DESPAWN=0,TEMPSUMMON_TIMED_DESPAWN=1, REACT_PASSIVE=0, REACT_AGGRESSIVE=1, FOLLOW_MOTION_TYPE=4, POINT_MOTION_TYPE=8,
 UNIT_FIELD_FLAGS=1, UNIT_FLAG_NON_ATTACKABLE=2, SPELL_CAST_OK=0, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW=1,
 SPELL_FAILED_OUT_OF_RANGE=2, EFFECT_0=0, EFFECT_1=1, SPELL_EFFECT_REMOVE_AURA=164, SPELL_EFFECT_APPLY_AURA=6,
 SPELL_EFFECT_ANY=999, SPELL_EFFECT_DUMMY=3, SPELL_EFFECT_TRIGGER_SPELL=64 };
struct ObjectGuid { unsigned value=0; bool operator!=(ObjectGuid b)const{return value!=b.value;}bool operator<(ObjectGuid b)const{return value<b.value;}bool operator==(ObjectGuid b)const{return value==b.value;} };
using GuidList=std::vector<ObjectGuid>;
struct Position { float x=0,y=0,z=0,o=0;float GetOrientation()const{return o;}float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;} };
struct Player;struct Creature;struct TempSummon;struct SpellInfo;
struct Unit {
 ObjectGuid guid;Position pos;unsigned map=1220;bool alive=true;
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}
 virtual Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer();}
 bool IsCreature(){return ToCreature()!=nullptr;}ObjectGuid GetGUID()const{return guid;}bool IsAlive()const{return alive;}
 float GetDistance(Position p){return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z));}
 bool IsWithinDistInMap(Unit* u,float r){return u && map==u->map && GetDistance(u->pos)<=r;}
 Position GetPosition(){return pos;}float GetPositionX(){return pos.x;}float GetPositionY(){return pos.y;}float GetPositionZ(){return pos.z;}
};
struct Motion { unsigned type=0,moves=0,id=0;Position destination;
 void MovePoint(unsigned p,float x,float y,float z,bool=true){type=POINT_MOTION_TYPE;id=p;destination={x,y,z,0};++moves;}
 void MovePoint(unsigned p,Position const& where){MovePoint(p,where.x,where.y,where.z);}
 void MoveFollow(Player*,float,float){type=FOLLOW_MOTION_TYPE;++moves;}unsigned GetCurrentMovementGeneratorType(){return type;}
};
struct CreatureAI { virtual ~CreatureAI()=default;virtual void SetGUID(ObjectGuid const&,int32){}virtual void DoAction(int32){}virtual void JustDied(Unit*){}virtual void JustSummoned(Creature*){}virtual void SummonedCreatureDies(Creature*,Unit*){}virtual void SummonedCreatureDespawn(Creature*){}virtual void SpellHit(Unit*,SpellInfo const*){}virtual void AttackStart(Unit*){}virtual void IsSummonedBy(Unit*){}virtual void Reset(){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}virtual void UpdateAI(uint32){}virtual void MovementInform(uint32,uint32){}virtual void PassengerBoarded(Unit*,int8,bool){} };
struct Creature:Unit {
 CreatureAI* ai=nullptr;bool visible=true;unsigned display=0;std::vector<unsigned> casts;CreatureAI* AI(){return ai;}void SetVisible(bool v){visible=v;}void setFaction(unsigned){}void SetDisplayId(unsigned v){display=v;}TempSummon* SummonCreature(unsigned,Position,int,unsigned,int,ObjectGuid);void CastSpell(float,float,float,unsigned id,bool){casts.push_back(id);}unsigned entry=0,expiry=0,flags=0,react=0;uint64 spawn=0,health=100,maxHealth=100;float multiplier=1;ObjectGuid owner,viewer;
 bool removed=false;Motion motion;Creature* ToCreature()override{return this;}virtual TempSummon* ToTempSummon(){return nullptr;}
 unsigned GetEntry(){return entry;}uint64 GetDBTableGUIDLow(){return spawn;}
 void DespawnOrUnsummon(unsigned delay=0){if(delay)expiry=delay;else removed=true;}
 void AddPlayerInPersonnalVisibilityList(ObjectGuid g){viewer=g;}void SetReactState(int state){react=state;}void SetDisableGravity(bool){}void SetCanFly(bool){}void CombatStop(bool){}
 void SetFlag(unsigned,unsigned value){flags|=value;}void RemoveFlag(unsigned,unsigned value){flags&=~value;}
 uint64 GetHealth(Unit*){return health;}uint64 GetMaxHealth(Unit*){return maxHealth;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}
 Motion* GetMotionMaster(){return &motion;}
};
struct TempSummon:Creature {TempSummon* ToTempSummon()override{return this;}ObjectGuid GetSummonerGUID(){return owner;}Unit* GetSummoner();};
static std::map<ObjectGuid,TempSummon> creatures;static std::map<ObjectGuid,Player*> players;static unsigned serial=100;
struct Menu { unsigned id=0;unsigned GetMenuId(){return id;} };
struct TalkClass { Menu menu;Menu& GetGossipMenu(){return menu;}void SendCloseGossip(){} };
struct QuestObjective { unsigned ID=284778,ObjectID=107995; };
struct Quest {unsigned id=0;unsigned GetQuestId()const{return id;} std::vector<QuestObjective> objectives{{}};auto const& GetObjectives()const{return objectives;} };
struct QuestStore { Quest quest;Quest const* GetQuestTemplate(unsigned id){assert(id==37862);return &quest;} } questStore;
auto sQuestDataStore=&questStore;
struct Player:Unit {
 Player(){guid={++serial};players[guid]=this;}~Player(){players.erase(guid);}Player* ToPlayer()override{return this;}
 bool teleporting=false;std::vector<std::function<void()>> pending;bool IsBeingTeleported(){return teleporting;}void AddDelayedEvent(unsigned,std::function<void()> f){pending.push_back(f);}uint16 bar=0;std::set<unsigned> completable;std::set<unsigned> temporarySpells,auras; unsigned zone=7334;bool los=true,failedSpawn=false;Creature* vehicle=nullptr;TalkClass talk;TalkClass* PlayerTalkClass=&talk;
 std::map<unsigned,int> quests;std::map<std::pair<unsigned,unsigned>,unsigned> objectives;std::map<unsigned,unsigned> items;
 std::map<unsigned,GuidList> summons;std::vector<unsigned> credits,casts;
 unsigned GetGUIDLow(){return guid.value;}unsigned GetMapId(){return map;}unsigned GetZoneId(){return zone;}
 bool CanCompleteQuest(unsigned q){return completable.count(q);}void CompleteQuest(unsigned q){quests[q]=QUEST_STATUS_COMPLETE;}int GetQuestStatus(unsigned q){return quests[q];}unsigned GetQuestObjectiveData(unsigned q,unsigned id){return objectives[{q,id}];}
 uint16 GetUInt16Value(unsigned,unsigned){return bar;}void SetUInt16Value(unsigned,unsigned,unsigned value){bar=static_cast<uint16>(value);}void AddTemporarySpell(unsigned id){temporarySpells.insert(id);}void RemoveTemporarySpell(unsigned id){temporarySpells.erase(id);}bool HasSpell(unsigned id){return temporarySpells.count(id);}
 bool HasAura(unsigned id){return auras.count(id);}void RemoveAurasDueToSpell(unsigned id){auras.erase(id);}
 void PrepareQuestMenu(ObjectGuid){}void ADD_GOSSIP_ITEM(GossipOptionNpc,char const*,unsigned,unsigned){}void SEND_GOSSIP_MENU(unsigned,ObjectGuid){}
 bool HasItemCount(unsigned id,unsigned n){return items[id]>=n;}void DestroyItemCount(unsigned id,unsigned n,bool){assert(items[id]>=n);items[id]-=n;}
 bool IsWithinLOSInMap(Unit*){return los;}GuidList* GetSummonList(unsigned id){return &summons[id];}
 bool isInCombat(){return false;}unsigned getFaction(){return 35;}unsigned GetNativeDisplayId(){return 123;}unsigned GetGossipTextId(Creature*){return 0;}void NearTeleportTo(float x,float y,float z,float o){pos={x,y,z,o};}Creature* GetVehicleCreatureBase(){return vehicle;}Creature* GetVehicle(){return vehicle;}Creature* GetVehicleBase(){return vehicle;}void ExitVehicle(Position const* p=nullptr){auto old=vehicle;vehicle=nullptr;if(p)pos=*p;if(old && old->ai)old->ai->PassengerBoarded(this,0,false);}
 void KilledMonsterCredit(unsigned id,ObjectGuid={}){credits.push_back(id);++objectives[{37530,id}];}

 void SetQuestObjectiveData(Quest const*,QuestObjective const* o,unsigned n){objectives[{37862,o->ObjectID}]=n;}
 TempSummon* SummonCreature(unsigned id,Position where,int,unsigned duration,int,ObjectGuid viewer){if(failedSpawn)return nullptr;ObjectGuid next{++serial};auto& c=creatures[next];c.guid=next;c.entry=id;c.pos=where;c.map=map;c.owner=guid;c.viewer=viewer;c.expiry=duration;summons[id].push_back(next);return &c;}
 void CastSpell(float,float,float,unsigned id,bool){casts.push_back(id);}void CastSpell(Unit*,unsigned id,bool){casts.push_back(id);if(id==197936)auras.insert(id);}
 Creature* FindNearestCreature(unsigned id,float r){for(auto& pair:creatures)if(!pair.second.removed && pair.second.entry==id && IsWithinDistInMap(&pair.second,r))return &pair.second;return nullptr;}
};
Unit* TempSummon::GetSummoner(){auto it=players.find(owner);if(it!=players.end())return it->second;auto c=creatures.find(owner);return c==creatures.end()?nullptr:&c->second;}
struct ObjectAccessor {static Creature* GetCreature(Unit& p,ObjectGuid id){auto it=creatures.find(id);return it==creatures.end()||it->second.removed||it->second.map!=p.map?nullptr:&it->second;}};
struct ScriptedAI:CreatureAI {Creature* me;ScriptedAI(Creature* c):me(c){c->ai=this;}void DoCastVictim(unsigned){}void Talk(unsigned){}bool UpdateVictim(){return true;}void DoMeleeAttackIfReady(){} };
struct PlayerScript {PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnQuestReward(Player*,Quest const*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,unsigned){} };
struct CreatureScript {CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual bool OnQuestAccept(Player*,Creature*,Quest const*){return false;}virtual bool OnGossipHello(Player*,Creature*){return false;}virtual bool OnGossipSelect(Player*,Creature*,unsigned,unsigned){return false;}virtual CreatureAI* GetAI(Creature*)const{return nullptr;} };
struct SpellInfo {unsigned Id=0;};struct Hook {unsigned count=0;void operator+=(int){++count;} };
struct SpellScript {Unit* caster=nullptr;Unit* target=nullptr;Creature* hit=nullptr;SpellInfo info;std::set<int> prevented;
 Position dest;Position const* GetExplTargetDest(){return &dest;}virtual ~SpellScript()=default;virtual void Register(){}Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return nullptr;}Unit* GetOriginalTarget(){return target;}Creature* GetHitCreature(){return hit;}SpellInfo const* GetSpellInfo(){return &info;}
 void PreventHitDefaultEffect(int i){prevented.insert(i);}Hook OnCheckCast,OnEffectHitTarget,OnEffectHit,OnEffectLaunch,OnEffectLaunchTarget;
};
#define PrepareSpellScript(x) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#define RegisterSpellScript(x) ((void)0)
#define RegisterCreatureAI(x) ((void)0)

struct GameObject:Unit {};
struct GameObjectScript {GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual bool OnGossipHello(Player*,GameObject*){return false;} };
struct SpellScene {int MiscValue=0;};
struct SceneTriggerScript {SceneTriggerScript(char const*){}virtual ~SceneTriggerScript()=default;virtual bool OnTrigger(Player*,SpellScene const*,std::string){return false;} };
struct AreaTrigger:Unit {
 Player* caster=nullptr;std::list<Creature*> targets;std::set<ObjectGuid> inside;
 Unit* GetCaster(){return caster;}bool IsInArea(Creature* c){return inside.count(c->guid);}
 void GetCreatureListWithEntryInGrid(std::list<Creature*>& list,unsigned id,float){for(auto c:targets)if(c->entry==id)list.push_back(c);}
 void GetCreatureListWithEntryInGridAppend(std::list<Creature*>& list,unsigned id,float r){GetCreatureListWithEntryInGrid(list,id,r);}
};
struct AreaTriggerAI {AreaTrigger* at;AreaTriggerAI(AreaTrigger* a):at(a){}virtual ~AreaTriggerAI()=default;virtual void OnUpdate(unsigned){} };
#define RegisterAreaTriggerAI(x) ((void)0)

TempSummon* Creature::SummonCreature(unsigned entry,Position pos,int,unsigned duration,int,ObjectGuid viewer){
 ObjectGuid next{++serial};auto& c=creatures[next];c.guid=next;c.entry=entry;c.pos=pos;c.map=map;c.owner=guid;c.viewer=viewer;c.expiry=duration;
 static CreatureAI fallback;c.ai=&fallback;if(ai)ai->JustSummoned(&c);return &c;
}
struct SummonList:std::vector<ObjectGuid>{Creature* me;SummonList(Creature* c):me(c){}void Summon(Creature* c){push_back(c->guid);}void Despawn(Creature*){}void DespawnAll(){for(auto id:*this)creatures[id].removed=true;clear();}};
#include "../../src/server/scripts/Legion/azsuna_rescue.cpp"
int main(){
 Player waiting;waiting.quests[37530]=1;waiting.pos=AzsunaRescue::Prisoner;waiting.teleporting=true;
 AzsunaRescue::BoardAfterTeleport(&waiting,2);assert(waiting.summons[89089].empty() && waiting.pending.size()==1);
 waiting.teleporting=false;waiting.pos=AzsunaRescue::Start;waiting.pending.front()();assert(waiting.summons[89089].size()==1 && waiting.casts.back()==178284);
 AzsunaRescue::BoardAfterTeleport(&waiting,2);assert(waiting.summons[89089].size()==1);
 Player p,other;p.quests[37530]=1;other.quests[37530]=1;
 auto ride=p.SummonCreature(89089,AzsunaRescue::Start,0,0,3972,p.guid);npc_azsuna_rescue_farondis ai(ride);
 ai.UpdateAI(1000);assert(p.credits.empty()); // spawning is not boarding
 ai.PassengerBoarded(&other,0,true);assert(p.credits.empty());
 p.vehicle=ride;ai.PassengerBoarded(&p,0,true);assert(AzsunaRescue::Next(&p)==1);
 ai.PassengerBoarded(&p,0,true);assert(p.credits.size()==1);
 ride->pos=AzsunaRescue::Cave;ai.UpdateAI(500);assert(AzsunaRescue::Next(&p)==1); // out of order
 ride->pos=AzsunaRescue::Academy;ride->pos.z+=30;ai.UpdateAI(500);assert(AzsunaRescue::Next(&p)==1);
 ride->pos=AzsunaRescue::Academy;ai.UpdateAI(500);assert(AzsunaRescue::Next(&p)==2);
 ride->pos=AzsunaRescue::Pursuit;ai.UpdateAI(500);ai.UpdateAI(500);assert(AzsunaRescue::Next(&p)==3);
 assert(ai.athissa!=ai.parjesh && creatures[ai.athissa].viewer==p.guid);
 auto unrelated=ride->SummonCreature(123,ride->pos,0,0,0,p.guid);ai.SummonedCreatureDies(unrelated,ride);assert(AzsunaRescue::Next(&p)==3);
 auto a=ObjectAccessor::GetCreature(p,ai.athissa),b=ObjectAccessor::GetCreature(p,ai.parjesh);
 a->alive=false;ai.SummonedCreatureDies(a,ride);ai.SummonedCreatureDies(a,ride);assert(AzsunaRescue::Next(&p)==3);
 b->alive=false;ai.SummonedCreatureDies(b,ride);assert(AzsunaRescue::Next(&p)==4);
 ride->pos=AzsunaRescue::Cave;ai.UpdateAI(500);ai.UpdateAI(500);assert(AzsunaRescue::Next(&p)==5);
 auto queen=ObjectAccessor::GetCreature(p,ai.queen);npc_azsuna_rescue_azshara queenAI(queen);SpellInfo fire{178784},wrong{123};
 queenAI.SpellHit(&other,&fire);queenAI.SpellHit(ride,&wrong);assert(!ai.finaleRunning);
 uint32 lethal=1000000;queenAI.DamageTaken(ride,lethal,0);assert(lethal==0);
 queenAI.SpellHit(ride,&fire);assert(ai.finaleRunning && !queen->visible);
 scene_azsuna_rescue scene;SpellScene native{1148};ai.UpdateAI(1000);scene.OnTrigger(&p,&native,"complete");
 ai.UpdateAI(22000);assert(AzsunaRescue::Next(&p)==5 && queen->visible); // cancel != success
 queenAI.SpellHit(ride,&fire);ride->pos=AzsunaRescue::Start;ai.UpdateAI(22000);assert(AzsunaRescue::Next(&p)==5 && !ai.finaleRunning);
 ride->pos=AzsunaRescue::Cave;queenAI.SpellHit(ride,&fire);ai.UpdateAI(20000);scene.OnTrigger(&p,&native,"complete");
 ai.UpdateAI(2000);assert(AzsunaRescue::Next(&p)==6 && queen->removed);
 assert(ai.prisonerSpawned);ride->pos=AzsunaRescue::Prisoner;ai.UpdateAI(500);assert(AzsunaRescue::Next(&p)==7 && p.credits.size()==7);
 spell_azsuna_rescue_meteor meteor;meteor.caster=ride;meteor.Launch(0);assert(ride->casts.back()==179217);
 meteor.caster=&other;meteor.Launch(0);assert(ride->casts.size()==1);
 p.quests[37530]=QUEST_STATUS_COMPLETE;player_azsuna_rescue cleanup;Quest quest{37530};cleanup.OnQuestReward(&p,&quest);
 assert(!p.vehicle && ride->removed && !p.HasAura(197936));for(auto id:ai.summons)assert(creatures[id].removed);
 // A resumed attempt preserves completed objectives but must earn remaining ones.
 p.quests[37530]=1;p.objectives[{37530,89323}]=0;
 auto retry=p.SummonCreature(89089,AzsunaRescue::Start,0,0,3972,p.guid);npc_azsuna_rescue_farondis retryAI(retry);p.vehicle=retry;
 retryAI.PassengerBoarded(&p,0,true);retryAI.UpdateAI(500);assert(AzsunaRescue::Next(&p)==6);
 p.quests[37530]=0;retryAI.UpdateAI(500);assert(retry->removed && retryAI.summons.empty());
 auto death=p.SummonCreature(89089,AzsunaRescue::Start,0,0,3972,p.guid);npc_azsuna_rescue_farondis deathAI(death);p.quests[37530]=1;p.alive=false;
 deathAI.UpdateAI(500);assert(death->removed);
 std::cout<<"PASS: production rescue handlers; all seven ordered steps, boarding, wrong floor/order/player, both unique enemies, scene cancellation/leaving/retry, native meteor dispatch, reward/abandon/death cleanup.\n";
}
