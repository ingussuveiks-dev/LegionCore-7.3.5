#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "titanstrike_rune_wave.h"
using uint32=uint32_t;
constexpr uint32 CLASS_HUNTER=3,QUEST_STATUS_INCOMPLETE=3;
constexpr int TEMPSUMMON_MANUAL_DESPAWN=0,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT=1,REACT_DEFENSIVE=1;
int failedSummonLogs=0;
#define TC_LOG_ERROR(...) (++failedSummonLogs)
struct ObjectGuid {
 uint64_t n=0;ObjectGuid()=default;ObjectGuid(uint64_t value):n(value){}bool IsEmpty()const{return !n;}void Clear(){n=0;}
 bool operator==(ObjectGuid b)const{return n==b.n;}bool operator!=(ObjectGuid b)const{return n!=b.n;}bool operator<(ObjectGuid b)const{return n<b.n;}
 static const ObjectGuid Empty;
};const ObjectGuid ObjectGuid::Empty;
struct Creature;struct Player;struct Pet;
struct Motion {int follows=0;void MoveFollow(Creature*,float,float){++follows;}};
struct Unit {
 virtual ~Unit()=default;bool alive=true,visible=true,hostile=false,combat=false;int phase=1,level=102,faction=35;uint32 entry=0;
 float x=4890,y=315,z=-36;ObjectGuid guid;Unit* victim=nullptr;std::set<Unit*> attackers;
 bool IsAlive()const{return alive;}bool isInCombat(){return combat;}Unit* getVictim(){return victim;}
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
 AI* ai=nullptr;Motion motion;uint32 map=1495;int despawns=0,shots=0,spawns=0,respawns=0;ObjectGuid summoner;
 Creature(uint32 e=104949){entry=e;guid=++serial;objects[guid]=this;}
 ~Creature(){objects.erase(guid);}
 AI* AI(){return ai;}Map* GetMap(){return &worldMap;}uint32 GetMapId()const{return map;}uint32 GetInstanceId()const{return 1;}
 Creature* SummonCreature(uint32,float,float,float,float,int,uint32=0);
 void DespawnOrUnsummon(){visible=false;++despawns;}void Respawn(){alive=true;visible=true;++respawns;}
 Creature* FindNearestCreature(uint32 e,float dist,bool live){for(auto p:objects)if(p.second->visible&&p.second->entry==e&&p.second->alive==live&&GetDistance(p.second)<dist)return p.second;return nullptr;}
 GameObject* FindNearestGameObject(uint32 e,float){assert(e==248780);return &fixtureDoor;}
 Motion* GetMotionMaster(){return &motion;}float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return 0;}
 void setFaction(int f){faction=f;}int getFaction(){return faction;}void SetLevel(int l){level=l;}int getLevel(){return level;}void SetReactState(int){}
 void CastSpell(Unit* u,uint32 s,bool){assert(u&&s==6660);++shots;}void SetFacingToObject(Unit*){}
};
struct SmartAI:AI {
 Creature* me;NativeScript script;bool combatMove=true;int baseUpdates=0;std::vector<std::pair<uint32,uint32>> calls;
 explicit SmartAI(Creature* c):me(c){c->ai=this;}
 virtual void JustDied(Unit*){}virtual void SetData(uint32 id,uint32 value){calls.emplace_back(id,value);}
 virtual void UpdateAI(uint32){++baseUpdates;}void SetCombatMove(bool b){combatMove=b;}
 void AttackStart(Unit* target)override{me->victim=target;me->combat=true;}
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
   ai.SetData(7,wave+1);ai.UpdateAI(500);assert(ai.wave.Count()==(wave==0?1:wave==1?2:5));
   auto guids=ai.wave.guids;unsigned count=unsigned(ai.wave.Count());
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
 Creature enemy(106335);enemy.hostile=true;pet.victim=&enemy;
 for(unsigned step=1;step<=5;++step){scenario.step=step;helper.UpdateAI(500);assert(helper.bearGuid==bear&&objects[bear]->victim==&enemy);}
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
 std::cout<<"PASS: production Prustaga/Grif callbacks and wave state; 1/2/(3+2),120 final kill orders, retries/lost actors/dead owners, duplicate/stale reports, wrong scenario, death pause/abandonment, Stage2-through-final Theodore and permanent pet preserved\n";
}
