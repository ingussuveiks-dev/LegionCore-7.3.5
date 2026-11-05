#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using Milliseconds=std::chrono::milliseconds;
enum{CLASS_ROGUE=4,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,UNIT_STATE_CASTING=1,UNIT_STATE_STUNNED=2};
struct ObjectGuid{uint64 high=1,low=0;bool IsEmpty()const{return !low;}void Clear(){low=0;}bool operator==(ObjectGuid const&o)const{return high==o.high&&low==o.low;}bool operator!=(ObjectGuid const&o)const{return !(*this==o);}};
#include "QuestScope.inc"
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
static uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Scenario{uint32 id=1078,step=3;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}};
struct Player;struct Map{Scenario scenario;std::list<Player*> players;};
struct WorldObject{
 Map* map=nullptr;ObjectGuid guid;bool world=true,preDelete=false,alive=true,expired=false,scope=true;uint32 mapId=1607,phase=1;float x=0,z=92;
 bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return 1;}auto GetMap()const{return map;}auto GetGUID()const{return guid;}float GetPositionZ()const{return z;}float GetVisibilityRange()const{return 100;}bool InSamePhaseId(WorldObject const*o)const{return phase==o->phase;}
 float GetExactDist2d(WorldObject const* o)const{return std::fabs(x-o->x);}bool los=true;bool IsWithinLOSInMap(WorldObject const*o)const{return los&&o->los;}bool InSamePhase(WorldObject const*o)const{return phase==o->phase;}bool CanTargetQuestObject(WorldObject const*o)const{return scope&&o->scope;}float GetDistance(WorldObject const*o)const{return std::sqrt((x-o->x)*(x-o->x)+(z-o->z)*(z-o->z));}bool IsWithinDistInMap(WorldObject const*o,float range)const{return map==o->map&&GetDistance(o)<=range;}
};
struct Unit:WorldObject{virtual Player* ToPlayer(){return nullptr;}Player* owner=nullptr;Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return owner;}};
struct Player:Unit{
 bool gm=false,contact=true,pending=false,rewarded=false;uint32 klass=4,status=3;std::shared_ptr<QuestObjectScope> scopeState;std::vector<uint32> attempts;
 Player(){owner=this;}Player* ToPlayer()override{return this;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return pending;}uint32 getClass()const{return klass;}bool GetQuestRewardStatus(uint32 id)const{assert(id==41924);return rewarded;}uint32 GetQuestStatus(uint32 id)const{assert(id==41924);return status;}
 auto GetQuestCancellationReceipt(uint32 id){assert(id==41924);if(!scopeState){scopeState=std::make_shared<QuestObjectScope>();scopeState->Owner=guid;}return scopeState->CancellationReceipt();}
 void Abandon(){status=0;if(scopeState)scopeState->Cancel();scopeState.reset();}
 void UpdateAchievementCriteria(uint32,uint32){assert(false&&"Intro must never grant progression");}
};
struct InstanceScript{};
struct Position{float x,y,z;float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}};
struct Creature:Unit{uint32 entry=105542,melees=0,state=0;bool instanceAvailable=true,combat=false;InstanceScript instance;std::vector<uint32> combatCasts;bool isInCombat()const{return combat;}uint32 GetEntry()const{return entry;}InstanceScript* GetInstanceScript(){return instanceAvailable?&instance:nullptr;}void GetPlayerListInGrid(std::list<Player*>& out,float range){assert(range==25);if(map)out=map->players;}bool HasUnitState(uint32 mask)const{return state&mask;}Position GetNearPosition(float radius,float angle){return {x+radius*std::cos(angle),radius*std::sin(angle),z};}void CastSpell(float,float,float,uint32 id,bool trigger){assert(id==210157&&trigger);combatCasts.push_back(id);}};
struct Registry{Map* map=nullptr;Scenario* GetScenario(uint32){return map?&map->scenario:nullptr;}} registry;
#define sScenarioMgr (&registry)
namespace ObjectAccessor{static std::map<uint64,Player*> players;Player* GetPlayer(WorldObject const&,ObjectGuid guid){auto at=players.find(guid.low);return at==players.end()?nullptr:at->second;}}
enum class HighGuid{Conversation};struct Generator{uint32 Generate(){return 1;}};struct Manager{Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}} manager;
#define sObjectMgr (&manager)
struct Conversation{
 inline static std::vector<Conversation*> owned;inline static bool refuse=false;inline static std::function<void()> hook;inline static uint32 attempts=0;
 uint32 id=0;ObjectGuid caster;bool removed=false;uint32 duration=0;
 bool CreateConversation(uint32,uint32 value,Unit* target,void*,Unit& anchor){assert(target==&anchor&&value==1884);++attempts;if(auto f=hook)f();if(refuse)return false;id=value;caster=target->GetGUID();duration=7500;owned.push_back(this);return true;}
 uint32 GetDuration()const{return duration;}void Remove(){removed=true;}
 static void Clear(){for(auto p:owned)delete p;owned.clear();refuse=false;hook={};attempts=0;}
 static std::vector<uint32> Ids(){std::vector<uint32> ids;for(auto p:owned)if(!p->removed)ids.push_back(p->id);return ids;}
};
struct ScriptedAI{Creature*me;explicit ScriptedAI(Creature*p):me(p){}virtual~ScriptedAI()=default;virtual void MoveInLineOfSight(Unit*){}virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void JustDied(Unit*){}virtual void UpdateAI(uint32){}void SetCanSeeEvenInPassiveMode(bool){}bool UpdateVictim(){return me->combat;}void DoCast(uint32 id){me->combatCasts.push_back(id);}void DoMeleeAttackIfReady(){++me->melees;}};

struct SummonList{explicit SummonList(Creature*){}void DespawnAll(){}};

#include "ZirusAI.inc"
using Entrance=ZirusIntroAI;
struct Fixture{
 inline static uint32 cases=0;Map map,other;Player rogue,visitor;Creature source;Entrance ai;
 Fixture():ai(&source){++cases;warnings=0;registry.map=&map;map.scenario.step=3;source.map=rogue.map=visitor.map=&map;rogue.guid.low=101;visitor.guid.low=102;rogue.x=10;visitor.x=2;visitor.gm=true;map.players={&visitor,&rogue};ObjectAccessor::players={{101,&rogue},{102,&visitor}};}
 ~Fixture(){Conversation::Clear();ObjectAccessor::players.clear();registry.map=nullptr;}
 void Tick(uint32 t=1000){ai.UpdateAI(t);}void Accepted(){assert(ai.introDone&&Conversation::Ids()==std::vector<uint32>{1884});assert(Conversation::owned.back()->caster==rogue.guid&&map.scenario.step==3&&rogue.attempts.empty());}
};
int main(){
 for(uint32 mode=0;mode<30;++mode){Fixture f;
  if(mode==0)f.rogue.gm=true;if(mode==1)f.rogue.klass=1;if(mode==2)f.rogue.status=0;if(mode==3)f.rogue.rewarded=true;if(mode==4)f.rogue.alive=false;if(mode==5)f.rogue.world=false;if(mode==6)f.rogue.preDelete=true;if(mode==7)f.rogue.contact=false;if(mode==8)f.rogue.pending=true;if(mode==9)f.rogue.map=&f.other;if(mode==10)f.rogue.phase=2;if(mode==11)f.rogue.x=26;if(mode==12)f.rogue.z+=40;if(mode==13)f.rogue.los=false;if(mode==14)f.rogue.scope=false;if(mode==15)f.source.scope=false;if(mode==16)f.rogue.expired=true;if(mode==17)f.source.expired=true;if(mode==18)f.source.world=false;if(mode==19)f.source.preDelete=true;if(mode==20)f.source.alive=false;if(mode==21)f.source.entry=105660;if(mode==22)f.source.mapId=0;if(mode==23)f.source.instanceAvailable=false;if(mode==24)f.map.scenario.id=0;if(mode==25)f.map.scenario.step=2;if(mode==26)f.map.scenario.step=4;if(mode==27)registry.map=nullptr;if(mode==28)f.rogue.x=-26;if(mode==29)f.source.los=false;
  f.ai.MoveInLineOfSight(&f.rogue);f.Tick();assert(!f.ai.introDone&&Conversation::Ids().empty()&&!Conversation::attempts);
 }
 {Fixture f;Unit pet;pet.owner=&f.rogue;f.ai.MoveInLineOfSight(nullptr);f.ai.MoveInLineOfSight(&pet);f.ai.MoveInLineOfSight(&f.source);f.ai.MoveInLineOfSight(&f.visitor);assert(!Conversation::attempts);f.Tick();f.Accepted();f.ai.Reset();f.ai.MoveInLineOfSight(&f.rogue);f.Tick();f.Accepted();assert(Conversation::attempts==1);}
 {Fixture f;f.rogue.status=QUEST_STATUS_COMPLETE;f.ai.MoveInLineOfSight(&f.rogue);f.Accepted();}
 {Fixture f;Conversation::refuse=true;f.ai.MoveInLineOfSight(&f.rogue);for(uint32 i=0;i<999;++i)f.Tick(1);assert(Conversation::attempts==1&&warnings==1&&!f.ai.introDone);f.Tick(1);assert(Conversation::attempts==2&&warnings==1);Conversation::refuse=false;f.Tick(999);assert(Conversation::attempts==2);f.Tick(1);f.Accepted();}
 for(uint32 mode=0;mode<12;++mode){Fixture f;Conversation::hook=[&]{if(mode==0)f.ai.Reset();if(mode==1){f.rogue.Abandon();f.rogue.status=3;}if(mode==2)ObjectAccessor::players.erase(101);if(mode==3)f.rogue.guid.high=2;if(mode==4)f.rogue.map=&f.other;if(mode==5)f.rogue.scopeState=std::make_shared<QuestObjectScope>();if(mode==6)f.source.world=false;if(mode==7)f.map.scenario.step=4;if(mode==8)f.rogue.pending=true;if(mode==9)f.rogue.los=false;if(mode==10)f.rogue.expired=true;if(mode==11)f.source.alive=false;};f.ai.MoveInLineOfSight(&f.rogue);assert(Conversation::attempts==1&&!f.ai.introDone&&Conversation::Ids().empty());}
 {Fixture f;Conversation::hook=[&]{f.ai.MoveInLineOfSight(&f.rogue);f.Tick(99999);};f.ai.MoveInLineOfSight(&f.rogue);f.Accepted();assert(Conversation::attempts==1&&f.ai.introTimer==1000&&!f.ai.introBusy);}
 // Differential: actual pinned/repaired combat callbacks, native EventMap;
 // cast/stun delays, no-victim pause, refused presentation never change attacks.
 for(uint32 mode=0;mode<4;++mode){Fixture f;Creature prior;prior.map=&f.map;ZirusBeforeAI before(&prior);before.EnterCombat(&f.rogue);f.ai.EnterCombat(&f.rogue);Conversation::refuse=mode==3;
  for(uint32 tick=0;tick<100000;++tick){prior.combat=f.source.combat=mode!=1||tick>10000;prior.state=f.source.state=mode==2&&tick<15000?UNIT_STATE_CASTING:mode==2&&tick<18000?UNIT_STATE_STUNNED:0;before.UpdateAI(1);f.Tick(1);assert(prior.combatCasts==f.source.combatCasts&&prior.melees==f.source.melees);}
  assert(!f.source.combatCasts.empty()&&f.rogue.attempts.empty()&&f.map.scenario.step==3);
 }
 std::cout<<"PASS "<<Fixture::cases<<" ASAN extracted actual intro/Reset/LOS/combat contexts with native EventMap/quest receipts: admission, stationary1s retry, exactly once, factory callback cancellation/reset/reentry/fullGUID/scope replacement and100000-tick combat differential. Map/Conversation services modeled; native whole-class and client separate\n";
}
