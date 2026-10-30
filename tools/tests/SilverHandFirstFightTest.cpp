#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
using FunctionList=std::multimap<uint64,std::function<void()>>;
#include "FunctionClass.inc"
#include "FunctionMethods.inc"
enum {CLASS_PALADIN=2,QUEST_STATUS_INCOMPLETE=3,UNIT_STATE_STUNNED=8,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
static uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct ObjectGuid {
 uint64 _low,_high;
 ObjectGuid();ObjectGuid(uint64 low,uint64 high):_low(low),_high(high){}
 void Clear();bool IsEmpty() const;
 bool operator==(ObjectGuid const& b)const{return _low==b._low&&_high==b._high;}
 bool operator!=(ObjectGuid const& b)const{return !(*this==b);}
 bool operator<(ObjectGuid const& b)const{return _high<b._high||(_high==b._high&&_low<b._low);}
};
#include "Guid.inc"
struct Creature;struct TempSummon;struct Map;struct InstanceScript;
struct Unit {virtual ~Unit()=default;bool IsAIEnabled=true;virtual Creature* ToCreature(){return nullptr;}bool IsCreature(){return ToCreature()!=nullptr;}void DispatchDeath(Creature*);};
struct CreatureAI {
 virtual ~CreatureAI()=default;virtual void Reset(){}virtual void JustSummoned(Creature*){}virtual void SummonedCreatureDies(Creature*,Unit*){}
 virtual void JustDied(Unit*){}virtual void IsSummonedBy(Unit*){}virtual void EnterEvadeMode(){}
 virtual void SetGUID(ObjectGuid const&,int32){++receipts;}
 virtual void UpdateLifecycle(uint32){}virtual void AttackStart(Unit*){++attacks;}
 uint32 receipts=0,attacks=0;
};
using UnitAI=CreatureAI;
struct Motion {uint32 moves=0;void MovePoint(uint32 id,float x,float y,float z){assert(id==2&&x==1962.02f&&y==2336.25f&&z==80.52f);++moves;}};
struct Player {
 Map* map=nullptr;ObjectGuid guid{1,2};
 bool world=true,predelete=false,contact=true,gm=false,teleport=false,phase=true,rewarded=false,alive=true;
 uint32 cls=CLASS_PALADIN,status=QUEST_STATUS_INCOMPLETE,credits=0;float distance=10;
 std::function<void()> onCredit;
 bool IsInWorld(){return world;}bool IsPreDelete(){return predelete;}bool CanContact(){return contact;}
 bool isGameMaster(){return gm;}bool IsBeingTeleported(){return teleport;}uint32 getClass(){return cls;}
 Map* GetMap(){return map;}uint32 GetInstanceId();bool InSamePhase(Creature*){return phase;}
 bool IsWithinDistInMap(Creature*,float r){assert(r==100);return distance<=r;}
 bool GetQuestRewardStatus(uint32 q){assert(q==42120);return rewarded;}uint32 GetQuestStatus(uint32 q){assert(q==42120);return status;}
 void UpdateAchievementCriteria(uint32 t,uint32 a){assert(t==92&&a==50850);++credits;if(onCredit)onCredit();}
};
struct Reference {Player* value;Player* getSource() const{return value;}};
struct Map {
 uint32 id=1539,instance=1;std::vector<Reference> players;std::map<ObjectGuid,Creature*> actors;
 Creature* GetCreature(ObjectGuid guid){auto i=actors.find(guid);return i==actors.end()?nullptr:i->second;}
 auto const& GetPlayers(){return players;}
};
uint32 Player::GetInstanceId(){return map?map->instance:0;}
struct Creature : Unit {
 Map* map=nullptr;uint32 entry=106429;ObjectGuid guid{10,3};
 bool world=true,predelete=false,alive=true,m_AI_locked=false;uint32 states=0,spellAttempts=0;
 CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;InstanceScript* instance=nullptr;
 Player* nearest=nullptr;bool failSpawn=false,dieOnCreate=false;uint32 spawnCalls=0;
 std::vector<std::unique_ptr<TempSummon>> summons;Motion motion;
 Creature();~Creature();Creature* ToCreature() override{return this;}virtual TempSummon* ToTempSummon(){return nullptr;}
 bool IsInWorld(){return world;}bool IsPreDelete(){return predelete;}bool IsAlive(){return alive;}
 uint32 GetEntry(){return entry;}uint32 GetMapId(){return map?map->id:0;}Map* GetMap(){return map;}
 uint32 GetInstanceId(){return map?map->instance:0;}ObjectGuid GetGUID(){return guid;}
 bool HasUnitState(uint32 bit){return (states&bit)!=0;}CreatureAI* AI(){return primary;}
#include "DisabledAI.inc"
 InstanceScript* GetInstanceScript(){return instance;}void KillAllDelayedCombats(){}
 void CastSpell(Creature* target,uint32 spell){assert(target==this&&spell==208257);++spellAttempts;}
 Creature* SummonCreature(uint32,float,float,float,float);
 Motion* GetMotionMaster(){return &motion;}Player* FindNearestPlayer(float r){assert(r==100);return nearest;}
 void Tick(uint32 diff){CreatureAI* i_AI=primary;
#include "Lifecycle.inc"
 }
};
struct Scenario {uint32 id=1092,step=2;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct ScenarioManager {Scenario* scenario=nullptr;Scenario* GetScenario(uint32){return scenario;}} manager;
ScenarioManager* sScenarioMgr=&manager;
struct TempSummon : Creature {
 CreatureAI idle;
 ObjectGuid owner;
 TempSummon* ToTempSummon()override{return this;}ObjectGuid GetSummonerGUID(){return owner;}
 Unit* GetSummoner(){return map?map->GetCreature(owner):nullptr;}
 void InitSummon();
};
Creature::Creature()=default;Creature::~Creature()=default;
#ifndef BASELINE
#include "silver_hand_first_fight.h"
#endif
struct InstanceBase {virtual ~InstanceBase()=default;virtual void CreatureDies(Creature*,Unit*){}};
struct InstanceScript : InstanceBase {
 virtual ~InstanceScript()=default;
#ifndef BASELINE
#include "InstanceDeath.inc"
#else
 void CreatureDies(Creature*,Unit*) override {}
#endif
 void CreatureDiesForScript(Creature*,Unit*){}
};
#include "InitSummon.inc"
void Unit::DispatchDeath(Creature* creature){
#include "DeathBridge.inc"
}
struct ScriptedAI : CreatureAI {
 explicit ScriptedAI(Creature* c):me(c){}Creature* me;std::vector<uint32> speech;
 void Talk(uint32 id){assert(id==17||id==18);speech.push_back(id);}
};
namespace SilverHandCompanion {void Recover(Creature*,bool&,bool&) {}}
struct Fight : ScriptedAI {
 explicit Fight(Creature* c):ScriptedAI(c){}
 FunctionProcessor _functions,_entryFunctions;bool firstfight=false,wait_heal=false,_pendingHealRecovery=false,_entryPending=false,_introDone=false;
 uint64 _entryGeneration=0;struct Events {void Reset(){}}events;
#ifndef BASELINE
 SilverHandFirstFight::State _firstFight;
#include "Methods.inc"
#else
#include "Evade.inc"
 void Start(){
#include "Start.inc"
 }
#endif
 void Ordinary(uint32 diff){if(!me->HasUnitState(UNIT_STATE_STUNNED))_functions.Update(diff);}
};
struct ImmediateDeath : CreatureAI {Creature* boss;explicit ImmediateDeath(Creature* b):boss(b){}void IsSummonedBy(Unit* owner)override{boss->alive=false;owner->DispatchDeath(boss);}};
Creature* Creature::SummonCreature(uint32 e,float x,float y,float z,float o){
 assert(e==106624&&x==1919.57f&&y==2334.55f&&z==75.41f&&o==0);++spawnCalls;if(failSpawn)return nullptr;
 auto boss=std::make_unique<TempSummon>();boss->map=map;boss->entry=e;boss->owner=guid;boss->guid={100+spawnCalls,4};boss->instance=instance;
 ImmediateDeath immediate(boss.get());CreatureAI ordinary;boss->primary=dieOnCreate?static_cast<CreatureAI*>(&immediate):&ordinary;
 auto raw=boss.get();summons.push_back(std::move(boss));map->actors[raw->guid]=raw;raw->InitSummon();raw->primary=&raw->idle;return raw;
}
struct Test {
 Map map;Scenario scenario;InstanceScript instance;Creature actor;Player eligible,invalid;CreatureAI temporary;Fight fight;
 Test():fight(&actor){manager.scenario=&scenario;actor.map=&map;actor.instance=&instance;actor.primary=&fight;map.actors[actor.guid]=&actor;
 eligible.map=invalid.map=&map;invalid.cls=5;map.players={{&invalid},{&eligible}};actor.nearest=&invalid;}
 void Tick(uint32 diff){actor.Tick(diff);fight.Ordinary(diff);}
 TempSummon* Boss(){return actor.summons.empty()?nullptr:actor.summons.back().get();}
 void Kill(){Boss()->alive=false;Unit killer;killer.DispatchDeath(Boss());}
};
int main(){uint32 cases=0;
#ifdef BASELINE
 {Test t;t.fight.Start();t.Kill();t.fight.EnterEvadeMode();t.Tick(1000);assert(!t.fight.firstfight&&t.invalid.credits==0);
  t.Tick(1000);assert(t.fight.firstfight);t.Tick(10000);assert(t.eligible.credits==0&&t.invalid.credits==0);++cases;}
 {Test t;t.fight.Start();t.Tick(2000);assert(t.Boss()->alive);t.fight.EnterEvadeMode();t.Tick(1000);
  assert(t.invalid.credits==1&&t.eligible.credits==0&&t.actor.motion.moves==1);++cases;}
 {Test t;t.actor.failSpawn=true;t.fight.Start();t.Tick(2000);t.fight.EnterEvadeMode();t.Tick(1000);
  assert(!t.Boss()&&t.invalid.credits==1);++cases;}
 std::cout<<"PASS "<<cases<<" pinned broken counterfactuals: early death missed; living/absent opponent evade publishes motion and invalid-player credit\n";
#else
 for(uint32 mode:{0u,1u,2u,3u})for(bool immediate:{false,true}){
  Test t;if(mode==1){t.actor.primary=&t.temporary;t.actor.i_disabledAI=&t.fight;}
  if(mode==2)t.actor.i_disabledAI=&t.fight;if(mode==3){t.actor.primary=nullptr;t.actor.i_disabledAI=&t.fight;t.actor.IsAIEnabled=false;}
  t.actor.dieOnCreate=immediate;t.fight.Start();if(!immediate)t.Kill();
  t.Tick(999);assert(t.eligible.credits==0);t.Tick(1001);assert(t.eligible.credits==0);t.Tick(0);
  assert(t.eligible.credits==1&&t.invalid.credits==0&&t.actor.motion.moves==1&&t.actor.spawnCalls==1);
  t.Kill();t.fight.EnterEvadeMode();t.Tick(10000);t.fight.Start();assert(t.eligible.credits==1&&t.actor.spawnCalls==1);++cases;
 }
 {Test t;t.fight.Start();t.Tick(2000);t.fight.EnterEvadeMode();t.Tick(10000);assert(t.eligible.credits==0&&t.Boss()->alive);++cases;}
 {Test t;t.actor.failSpawn=true;t.fight.Start();t.Tick(999);assert(t.actor.spawnCalls==1);t.Tick(1);assert(t.actor.spawnCalls==2);
  t.actor.failSpawn=false;t.Tick(1000);assert(t.Boss());t.Kill();t.Tick(1999);assert(t.eligible.credits==0);t.Tick(1);t.Tick(0);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.Boss()->world=false;t.map.actors.erase(t.Boss()->guid);t.Tick(1);assert(t.eligible.credits==0);t.Tick(999);assert(t.actor.spawnCalls==1);
  t.Tick(1);assert(t.actor.spawnCalls==2);t.Kill();t.Tick(2000);t.Tick(0);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.Kill();t.map.players.clear();t.Tick(2000);t.Tick(0);assert(t.eligible.credits==0);
  t.map.players={{&t.eligible}};t.Tick(0);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.Kill();t.actor.states=UNIT_STATE_STUNNED;t.Tick(2000);t.Tick(0);assert(t.eligible.credits==0);
  t.actor.states=0;t.Tick(0);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.Kill();t.scenario.step=3;t.Tick(2000);t.scenario.step=2;t.Tick(1000);assert(t.eligible.credits==0&&!t.fight.firstfight);
  t.fight.Start();t.Kill();t.Tick(2000);t.Tick(0);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.Kill();t.eligible.onCredit=[&]{t.fight.CompleteFirstFight(10000);};t.Tick(2000);t.Tick(0);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.Tick(2000);t.Kill();t.Tick(999);assert(t.eligible.credits==0);t.Tick(1);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.Boss()->alive=false;t.Tick(2000);assert(t.eligible.credits==0);t.Tick(999);assert(t.eligible.credits==0);
  t.Tick(1);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.fight.SetGUID(t.Boss()->guid,106624);t.Tick(3000);assert(t.eligible.credits==0);t.Kill();t.Tick(1000);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.Kill();t.fight.Reset();t.Tick(1999);assert(t.eligible.credits==0);t.Tick(1);t.Tick(0);assert(t.eligible.credits==1);++cases;}
 {Test t;auto prior=warnings;t.actor.failSpawn=true;t.fight.Start();for(uint32 i=1;i<30;++i)t.Tick(1000);
  t.Tick(100000);assert(t.actor.spawnCalls==30&&t.eligible.credits==0&&warnings==prior+2);++cases;}
 for(uint32 invalid=0;invalid<14;++invalid){Test t;t.map.players={{&t.eligible}};Map other;
  t.fight.Start();t.Kill();
  switch(invalid){case 0:t.eligible.world=false;break;case 1:t.eligible.predelete=true;break;case 2:t.eligible.contact=false;break;
   case 3:t.eligible.gm=true;break;case 4:t.eligible.teleport=true;break;case 5:t.eligible.cls=5;break;case 6:t.eligible.map=&other;break;
   case 7:t.eligible.phase=false;break;case 8:t.eligible.distance=100.01f;break;case 9:t.eligible.rewarded=true;break;
   case 10:t.eligible.status=0;break;case 11:t.eligible.status=6;break;case 12:t.eligible.status=1;break;case 13:t.eligible.status=5;break;}
  t.Tick(2000);t.Tick(0);assert(t.eligible.credits==0&&t.actor.motion.moves==0);++cases;
 }
 {Test t;t.eligible.alive=false;t.eligible.distance=100;t.fight.Start();t.Kill();t.Tick(2000);t.Tick(0);assert(t.eligible.credits==1);++cases;}
 for(uint32 invalid=0;invalid<9;++invalid){Test t;Map other;Scenario foreign;
  switch(invalid){case 0:t.actor.world=false;break;case 1:t.actor.predelete=true;break;case 2:t.actor.alive=false;break;
   case 3:t.actor.entry=106639;break;case 4:t.map.id=1220;break;case 5:t.actor.map=nullptr;break;
   case 6:manager.scenario=nullptr;break;case 7:t.scenario.id=991;break;case 8:t.scenario.step=3;break;}
  t.fight.Start();t.Tick(10000);assert(t.actor.spawnCalls==0&&t.eligible.credits==0);++cases;
 }
 for(uint32 invalid=0;invalid<7;++invalid){Test t;t.fight.Start();auto b=t.Boss();Map other;
  switch(invalid){case 0:b->world=false;break;case 1:b->predelete=true;break;case 2:b->entry=106669;break;
   case 3:b->map=&other;break;case 4:b->owner={999,3};break;case 5:b->guid={999,9};break;case 6:b->owner={t.actor.guid._low,t.actor.guid._high+1};break;}
  b->alive=false;t.instance.CreatureDies(b,nullptr);t.Tick(2000);t.Tick(0);assert(t.eligible.credits==0);++cases;
 }
 {Test t;t.fight.Start();t.fight.Start();t.Tick(2000);assert(t.actor.spawnCalls==1);t.Kill();t.Tick(1000);assert(t.eligible.credits==1);++cases;}
 {Test t;t.fight.Start();t.Kill();t.scenario.step=3;t.Tick(1);t.scenario.step=2;t.fight.Start();t.Kill();
  t.Tick(1999);assert(!t.fight.firstfight&&t.eligible.credits==0);t.Tick(1);t.Tick(0);assert(t.eligible.credits==1);++cases;}
 {Test t;auto original=std::make_unique<Fight>(&t.actor);t.actor.primary=original.get();original->Start();original.reset();
  auto replacement=std::make_unique<Fight>(&t.actor);t.actor.primary=replacement.get();t.Kill();t.Tick(10000);
  assert(t.eligible.credits==0&&t.actor.motion.moves==0&&!replacement->firstfight);++cases;}
 std::cout<<"PASS "<<cases<<" actual first-fight methods/controller, native InitSummon/death delivery/FunctionProcessor/lifecycle cases; world services controlled\n";
#endif
}
