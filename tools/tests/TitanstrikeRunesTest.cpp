#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "titanstrike_rune_wave.h"
using uint32=uint32_t;
constexpr uint32 CLASS_HUNTER=3,QUEST_STATUS_INCOMPLETE=3;
constexpr int TEMPSUMMON_MANUAL_DESPAWN=0,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT=1,REACT_DEFENSIVE=1;
constexpr int UNIT_FIELD_EMOTE_STATE=1,EMOTE_STATE_USE_STANDING=69,EMOTE_ONESHOT_NONE=0;
constexpr uint32 UNIT_FIELD_FLAGS=2,UNIT_FLAG_NOT_SELECTABLE=33554432;
enum UnitMoveType { MOVE_WALK, MOVE_RUN };
constexpr int FOLLOW_MOTION_TYPE=14, CHASE_MOTION_TYPE=5;
int failedSummonLogs=0;
#define TC_LOG_ERROR(...) (++failedSummonLogs)
struct ObjectGuid {
 uint64_t n=0;ObjectGuid()=default;ObjectGuid(uint64_t value):n(value){}bool IsEmpty()const{return !n;}void Clear(){n=0;}
 bool operator==(ObjectGuid b)const{return n==b.n;}bool operator!=(ObjectGuid b)const{return n!=b.n;}bool operator<(ObjectGuid b)const{return n<b.n;}
 static const ObjectGuid Empty;
};const ObjectGuid ObjectGuid::Empty;
struct Creature;struct Player;struct Pet;
struct Motion {int follows=0,type=0;void MoveFollow(Creature*,float,float){++follows;type=FOLLOW_MOTION_TYPE;}int GetCurrentMovementGeneratorType(){return type;}};
struct Unit {
 virtual ~Unit()=default;bool alive=true,visible=true,hostile=false,combat=false;int phase=1,level=102,faction=35;uint32 entry=0;
 float x=4890,y=315,z=-36;ObjectGuid guid;Unit* victim=nullptr;std::set<Unit*> attackers;
 float speeds[2]={1.0f,1.0f};float GetSpeedRate(UnitMoveType t){return speeds[t];}void SetSpeed(UnitMoveType t,float rate){speeds[t]=rate;}
 bool IsAlive()const{return alive;}bool isInCombat(){return combat;}Unit* getVictim(){return victim;}uint32 GetEntry(){return entry;}void CombatStop(bool){combat=false;victim=nullptr;}
 std::set<Unit*>* getAttackers(){return &attackers;}
 bool InSamePhaseId(Unit* u)const{return phase==u->phase;}
 bool IsValidAttackTarget(Unit* u){return u&&u!=this&&u->hostile&&u->alive;}
 float GetDistance(Unit* u)const{return std::hypot(x-u->x,y-u->y);}
 float GetDistance(float px,float py,float)const{return std::hypot(x-px,y-py);}
 ObjectGuid GetGUID()const{return guid;}
};
struct Pet:Unit{};
struct Player:Unit {uint32 cls=3,status=3;Pet* pet=nullptr;uint32 getClass(){return cls;}uint32 GetQuestStatus(uint32 q){assert(q==41574);return status;}Pet* GetPet(){return pet;}};
struct Ref {Player* player;Player* getSource()const{return player;}};
struct Map {std::vector<Ref> players;auto const& GetPlayers(){return players;}} worldMap;
struct Scenario {uint32 id=1068,step=2;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} scenario;
struct ScenarioMgr {Scenario* GetScenario(uint32){return &scenario;}} mgr;
ScenarioMgr* sScenarioMgr=&mgr;
struct GameObject {int resets=0;void ResetDoorOrButton(){++resets;}} fixtureDoor;
struct AI {virtual ~AI()=default;virtual void AttackStart(Unit*)=0;};
struct NativeScript {int resets=0;void OnReset(){++resets;}};
std::map<ObjectGuid,Creature*> objects;std::vector<std::unique_ptr<Creature>> allocated;
uint64_t serial=100;uint32 failEntry=0;
struct Creature:Unit {
 AI* ai=nullptr;Motion motion;uint32 map=1495,dbguid=0,flags=0;int despawns=0,shots=0,bolts=0,activationCasts=0,spawns=0,respawns=0,emote=0;bool reactivating=false;ObjectGuid summoner;
 Creature(uint32 e=104949){entry=e;guid=++serial;objects[guid]=this;}
 ~Creature(){objects.erase(guid);}
 AI* AI(){return ai;}Map* GetMap(){return &worldMap;}uint32 GetMapId()const{return map;}uint32 GetInstanceId()const{return 1;}
 Creature* SummonCreature(uint32,float,float,float,float,int,uint32=0);
 void DespawnOrUnsummon(){visible=false;++despawns;}void Respawn(){alive=true;visible=true;++respawns;}
 Creature* FindNearestCreature(uint32 e,float dist,bool live){for(auto p:objects)if(p.second->visible&&p.second->entry==e&&p.second->alive==live&&GetDistance(p.second)<dist)return p.second;return nullptr;}
 GameObject* FindNearestGameObject(uint32 e,float){assert(e==248780);return &fixtureDoor;}
 Motion* GetMotionMaster(){return &motion;}float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return 0;}
 void setFaction(int f){faction=f;}int getFaction(){return faction;}void SetLevel(int l){level=l;}int getLevel(){return level;}void SetReactState(int){}
 void CastSpell(Unit* u,uint32 s,bool){assert(u&&(s==6660||s==187207||s==210845));if(s==6660)++shots;else if(s==187207)++bolts;else ++activationCasts;}void SetFacingToObject(Unit*){}
 bool HasAura(uint32 s){assert(s==210836);return reactivating;}void SetUInt32Value(int field,int value){assert(field==UNIT_FIELD_EMOTE_STATE);emote=value;}
 uint32 GetDBTableGUIDLow(){return dbguid;}bool HasFlag(uint32 field,uint32 flag){assert(field==UNIT_FIELD_FLAGS);return (flags&flag)!=0;}
 void RemoveAurasDueToSpell(uint32 s){assert(s==210836);reactivating=false;}
 void GetCreatureListWithEntryInGrid(std::list<Creature*>& list,uint32 e,float distance){for(auto p:objects)if(p.second->visible&&p.second->entry==e&&GetDistance(p.second)<distance)list.push_back(p.second);}
};
struct SmartAI:AI {
 Creature* me;NativeScript script;bool combatMove=true;int baseUpdates=0;std::vector<std::pair<uint32,uint32>> calls;
 explicit SmartAI(Creature* c):me(c){c->ai=this;}
 virtual void JustDied(Unit*){}virtual void SetData(uint32 id,uint32 value){calls.emplace_back(id,value);}
 virtual void UpdateAI(uint32){++baseUpdates;}void SetCombatMove(bool b){combatMove=b;}
 void AttackStart(Unit* target)override{me->victim=target;me->combat=true;me->motion.type=CHASE_MOTION_TYPE;}
 NativeScript* GetScript(){return &script;}
};
Creature* Creature::SummonCreature(uint32 e,float px,float py,float pz,float,int,uint32){
 if(e==failEntry)return nullptr;
 auto p=std::make_unique<Creature>(e);p->x=px;p->y=py;p->z=pz;p->phase=phase;p->hostile=e!=106289;p->summoner=guid;
 new SmartAI(p.get()); // fixture-owned small AI objects reclaimed by process exit
 ++spawns;auto result=p.get();allocated.push_back(std::move(p));return result;
}
namespace ObjectAccessor {Creature* GetCreature(Creature&,ObjectGuid guid){auto p=objects.find(guid);return p==objects.end()||!p->second->visible?nullptr:p->second;}}
#include "TitanstrikeActors.inc"
int main(){
 Player player;Pet pet;player.pet=&pet;worldMap.players={{&player}};
 // Run the actual Prustaga callbacks, not just an independent state model.
 for(auto killer:{0,1,2,3}){ // player/pet/guardian/ally deaths have identical dead actor observations
  (void)killer;Creature base;npc_titanstrike_prustaga ai(&base);
  ai.SetData(2,2);ai.SetData(2,2);assert(ai.calls.size()==1&&ai.wave.started);
  for(unsigned wave=0;wave<3;++wave){
   auto before=ai.calls.size();ai.SetData(3,3);ai.SetData(4,4);ai.SetData(5,5);assert(ai.calls.size()==before);
   ai.SetData(7,wave+1);ai.UpdateAI(500);assert(ai.wave.Count()==(wave==0?1:wave==1?3:5));
   auto guids=ai.wave.guids;unsigned count=unsigned(ai.wave.Count());
   if(wave==1){assert(objects[guids[0]]->entry==106341&&objects[guids[1]]->entry==106335&&objects[guids[2]]->entry==106344);assert(objects[guids[2]]->summoner==guids[0]);}
   if(wave==2){assert(objects[guids[3]]->summoner==guids[0]&&objects[guids[4]]->summoner==guids[1]);assert(!dynamic_cast<SmartAI*>(objects[guids[0]]->ai)->combatMove);}
   for(unsigned i=0;i<count;++i){objects[guids[i]]->alive=false;objects[guids[i]]->x+=float(i)*100;ai.UpdateAI(500);assert(ai.wave.index==wave+(i+1==count));}
   assert(ai.calls.back()==std::make_pair(wave+3,wave+3));before=ai.calls.size();ai.UpdateAI(500);assert(ai.calls.size()==before);
   ai.SetData(7,wave+1);assert(!ai.wave.armed); // stale summon signal cannot arm the next wave
  }
  ai.Cleanup();assert(!ai.wave.started);
 }
 // Failed summons, disappearing live actors and failed wolf-after-owner-death retry.
 Creature base;npc_titanstrike_prustaga ai(&base);ai.SetData(2,2);ai.SetData(7,1);
 failEntry=106335;ai.UpdateAI(500);assert(ai.wave.index==0&&ai.wave.guids[0].IsEmpty()&&failedSummonLogs);
 failEntry=0;ai.UpdateAI(500);auto lost=ai.wave.guids[0];objects[lost]->visible=false;ai.UpdateAI(500);assert(ai.wave.guids[0]!=lost&&ai.wave.index==0);
 objects[ai.wave.guids[0]]->alive=false;ai.UpdateAI(500);ai.SetData(7,2);ai.UpdateAI(500);
 for(auto g:ai.wave.guids)if(!g.IsEmpty())objects[g]->alive=false;ai.UpdateAI(500);
 ai.SetData(7,3);failEntry=106344;ai.UpdateAI(500);auto parents=ai.wave.guids;objects[parents[0]]->alive=false;objects[parents[1]]->alive=false;ai.UpdateAI(500);
 objects[parents[0]]->visible=objects[parents[1]]->visible=false;failEntry=0;ai.UpdateAI(500);assert(!ai.wave.guids[3].IsEmpty()&&!ai.wave.guids[4].IsEmpty()&&ai.wave.index==2);
 for(auto g:ai.wave.guids)if(!g.IsEmpty()&&objects[g]->visible)objects[g]->alive=false;ai.UpdateAI(500);assert(ai.wave.index==3);
 // Context, death pause and abandonment cleanup/reset; other scenarios delegate normally.
 ai.Cleanup();scenario.id=909;ai.SetData(2,2);assert(!ai.wave.started);scenario.id=1068;
 ai.SetData(2,2);ai.SetData(7,1);ai.UpdateAI(500);auto live=ai.wave.guids[0];player.alive=false;ai.UpdateAI(500);assert(ai.wave.guids[0]==live);
 player.alive=true;player.status=0;ai.UpdateAI(500);assert(!ai.wave.started&&!objects[live]->visible&&ai.script.resets==1);player.status=3;
 // Theodore is Grif's persistent NPC summon beginning at Stage2, including
 // later stages, pet-only combat, death/resummon and quest/map cleanup.
 Creature grif(104904);npc_titanstrike_grif helper(&grif);scenario.step=0;helper.UpdateAI(500);assert(helper.bearGuid.IsEmpty());
 scenario.step=1;helper.UpdateAI(500);auto bear=helper.bearGuid;assert(!bear.IsEmpty()&&objects[bear]->summoner==grif.guid&&player.pet==&pet);
 // A follower keeps one generator instead of restarting its spline every
 // 500 ms, and follows Grif's changing rates rather than its slower template.
 grif.speeds[MOVE_RUN]=1.14286f;grif.speeds[MOVE_WALK]=1.2f;
 for(int i=0;i<10;++i)helper.UpdateAI(500);
 assert(objects[bear]->motion.follows==1);
 assert(objects[bear]->speeds[MOVE_RUN]==grif.speeds[MOVE_RUN]&&objects[bear]->speeds[MOVE_WALK]==grif.speeds[MOVE_WALK]);
 grif.speeds[MOVE_RUN]=1.5f;helper.UpdateAI(500);
 assert(objects[bear]->speeds[MOVE_RUN]==1.5f&&objects[bear]->motion.follows==1);
 Creature enemy(106335);enemy.hostile=true;pet.victim=&enemy;
 for(unsigned step=1;step<=5;++step){scenario.step=step;helper.UpdateAI(500);assert(helper.bearGuid==bear);if(step==3)assert(!objects[bear]->victim);else assert(objects[bear]->victim==&enemy);}
 scenario.step=3;Creature robot(105016);robot.x=grif.x;robot.y=grif.y;robot.reactivating=true;grif.combat=true;helper.UpdateAI(500);
 assert(!grif.combat&&grif.emote==EMOTE_STATE_USE_STANDING&&!objects[bear]->combat);
 auto follows=objects[bear]->motion.follows;helper.UpdateAI(500);assert(objects[bear]->motion.follows==follows);
 robot.reactivating=false;helper.UpdateAI(500);assert(grif.emote==0);
 Creature machine(106354);machine.hostile=true;machine.x=base.x;machine.y=base.y;pet.victim=&machine;ai.UpdateAI(500);assert(base.bolts==1);ai.UpdateAI(500);assert(base.bolts==1);
 pet.victim=&enemy;scenario.step=5;
 assert(grif.shots>0&&player.pet==&pet);objects[bear]->alive=false;helper.UpdateAI(500);assert(helper.bearGuid!=bear&&!objects[bear]->visible);
 bear=helper.bearGuid;player.status=0;helper.UpdateAI(500);assert(helper.bearGuid.IsEmpty()&&!objects[bear]->visible);
 player.status=3;scenario.id=909;helper.UpdateAI(500);assert(helper.bearGuid.IsEmpty());
 // Every possible final-wave kill order must retain both wolves and wait for five deaths.
 std::array<int,5> order{{0,1,2,3,4}};int permutations=0;
 do {TitanstrikeRunes::Wave<int> w;w.Start();w.index=2;w.Arm(3);std::map<int,TitanstrikeRunes::Life> states;int id=0;
  auto spawn=[&](auto const&,int){states[++id]=TitanstrikeRunes::Life::Alive;return id;};auto read=[&](int g){return states[g];};
  assert(!w.Update(spawn,read));auto ids=w.guids;
  for(int i=0;i<5;++i){states[ids[order[i]]]=TitanstrikeRunes::Life::Dead;assert(w.Update(spawn,read)==(i==4));}
  assert(!w.Update(spawn,read));++permutations;
 }while(std::next_permutation(order.begin(),order.end()));assert(permutations==120);
 std::cout<<"PASS: production Prustaga/Grif callbacks and wave state; 1/(2+1)/(3+2),120 final kill orders, retries/lost actors/dead owners, duplicate/stale reports, wrong scenario, death pause/abandonment, Stage2-through-final Theodore and permanent pet preserved\n";
}
