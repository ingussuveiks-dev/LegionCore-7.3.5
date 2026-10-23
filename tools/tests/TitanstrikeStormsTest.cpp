#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using int32=int32_t;
constexpr uint32 AREATRIGGER_FIELD_SPELL_FOR_VISUALS=201,AREATRIGGER_FIELD_SPELL_XSPELL_VISUAL_ID=202;
constexpr int REACT_DEFENSIVE=1,REACT_PASSIVE=0,POINT_MOTION_TYPE=8,UNIT_STATE_CASTING=1,SPELL_AURA_DISABLE_MOVEMENT_FORCE=1;
constexpr uint32 IN_MILLISECONDS=1000,EVENT_1=1,EVENT_2=2;
struct ObjectGuid {uint64_t n=0;bool IsEmpty()const{return n==0;}bool operator==(ObjectGuid b)const{return n==b.n;}};
struct Player;struct Creature;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}};
struct Ref {Player* p;Player* getSource()const{return p;}};
struct Map {std::vector<Ref> players;auto const& GetPlayers(){return players;}} world;
struct Scenario {uint32 id=1068,step=1;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} progress;
struct ScenarioMgr {Scenario* GetScenario(uint32){return &progress;}} manager;
ScenarioMgr* sScenarioMgr=&manager;
struct Position {float x,y,z,o;Position(float a=0,float b=0,float c=0,float d=0):x(a),y(b),z(c),o(d){}float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}};
#include "../../src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.h"
// The header's native positions need Position, so they are extracted separately.
struct AreaTrigger;
struct Player:Unit {
 ObjectGuid guid{1},force;bool alive=true,vehicle=false,flight=false,disable=false,stun=false;int applied=0,removed=0;
 Player* ToPlayer()override{return this;}bool IsAlive(){return alive;}void* GetVehicle(){return vehicle?this:nullptr;}bool isInFlight(){return flight;}
 bool HasAuraType(int){return disable;}bool HasAura(uint32 s){assert(s==210942);return stun;}ObjectGuid GetForceGUID(){return force;}
 void SendMovementForce(AreaTrigger*,Position=Position(),float=0,uint32=0,bool=false);
};
struct AreaData {float windX=-.508373f,windY=-2.95661f,windZ=0,windSpeed=3;uint32 windType=0;};
struct AreaTrigger {ObjectGuid guid{2};std::vector<ObjectGuid> affected{{1}};uint32 map=1495;
 Unit* caster=nullptr;uint32 visualSpell=210880,visual=49393;
 Unit* GetCaster(){return caster;}void SetUInt32Value(uint32 field,uint32 value){assert(value==0);if(field==201)visualSpell=value;else{assert(field==202);visual=value;}}
 uint32 GetMapId(){return map;}uint32 GetInstanceId(){return 1;}Map* GetMap(){return &world;}bool InSamePhaseId(Player*){return true;}
 ObjectGuid GetGUID(){return guid;}AreaData GetAreaTriggerInfo(){return {};}auto* GetAffectedPlayers(){return &affected;}
};
void Player::SendMovementForce(AreaTrigger* at,Position pos,float speed,uint32 type,bool apply){
 if(apply){assert(speed==3&&pos.y<0&&type==0);force=at->guid;++applied;}else{force={};++removed;}
}
namespace ObjectAccessor {Unit* GetUnit(AreaTrigger&,ObjectGuid g){for(auto r:world.players)if(r.p->guid==g)return r.p;return nullptr;}}
struct AreaTriggerAI {AreaTrigger* at;explicit AreaTriggerAI(AreaTrigger* a):at(a){}virtual bool CreateUnitList(std::list<Unit*>&){return false;}
 virtual void OnUnitEnter(Unit*){}virtual void OnUnitExit(Unit*){}virtual void BeforeRemove(Unit*){}virtual void OnUpdate(uint32){}virtual void CalculateDuration(int32&){} };
uint32 urand(uint32 a,uint32){return a;}
struct EventMap {std::map<uint32,uint32> timers;void Reset(){timers.clear();}void RescheduleEvent(uint32 id,uint32 ms){timers[id]=ms;}void CancelEvent(uint32 id){timers.erase(id);}void Update(uint32 d){for(auto& p:timers)p.second=p.second>d?p.second-d:0;}uint32 ExecuteEvent(){for(auto p:timers)if(!p.second){timers.erase(p.first);return p.first;}return 0;}};
struct Motion {int moves=0;Position destination;void MovePoint(int,float x,float y,float z,bool){++moves;destination={x,y,z};}};
struct Vehicle {int releases=0;void RemoveAllPassengers(){++releases;}};
struct Creature:Unit {float x=0,y=0,z=0;bool combat=false,casting=false,alive=true,hasAura=false;uint32 dbguid=0,map=1495;int casts=0,areasRemoved=0,despawns=0;uint32 lastSpell=0;Position destination;Motion motion;Vehicle vehicle;
 Creature* ToCreature()override{return this;}
 uint32 GetMapId(){return map;}
 void SetReactState(int){}float GetDistance(Position p){return std::hypot(x-p.x,y-p.y);}float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}
 bool isInCombat(){return combat;}bool HasUnitState(int){return casting;}
 void CastSpell(float a,float b,float c,uint32 s,bool){destination={a,b,c};lastSpell=s;++casts;}
 void CastSpell(Unit*,uint32 s,bool){lastSpell=s;hasAura=true;++casts;}
 Motion* GetMotionMaster(){return &motion;}Vehicle* GetVehicleKit(){return &vehicle;}void RemoveAllAreaObjects(){++areasRemoved;}void DespawnOrUnsummon(uint32){++despawns;}
 uint32 GetDBTableGUIDLow(){return dbguid;}bool HasAura(uint32 s){assert(s==210880);return hasAura;}void RemoveAurasDueToSpell(uint32 s){assert(s==210880);hasAura=false;}
};
#include "TitanstrikeStormArea.inc"
struct ScriptedAI {Creature* me;bool combatMovement=true;explicit ScriptedAI(Creature* c):me(c){}virtual ~ScriptedAI()=default;virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void UpdateAI(uint32){}virtual void IsSummonedBy(Unit*){}virtual void MovementInform(uint32,uint32){}
 void SetCombatMovement(bool enabled){combatMovement=enabled;}
 void DoCast(Unit*,uint32,bool=true){}void DoCastVictim(uint32){}bool UpdateVictim(){return true;}void DoMeleeAttackIfReady(){}
};
using CreatureAI=ScriptedAI;
struct SmartAI:ScriptedAI {using ScriptedAI::ScriptedAI;};
struct npc_unga_banana_ship:ScriptedAI {using ScriptedAI::ScriptedAI;};
struct CreatureScript {explicit CreatureScript(const char*){}virtual CreatureAI* GetAI(Creature*)const=0;};
#include "TitanstrikeInvisibleFactory.inc"
#include "TitanstrikeStormActors.inc"
struct Instance {std::map<uint64_t,Creature*> creatures;uint32 GetInstanceId(){return 1;}Creature* GetCreature(ObjectGuid g){return creatures[g.n];}};
struct BaseInstance {virtual void Update(uint32){}};
struct FloorInstance:BaseInstance {Instance* instance;std::vector<ObjectGuid> trashList;uint8 windshaperDiedCount=0;
#include "TitanstrikeStormFloor.inc"
};
int main(){
 Creature shared;npc_unga_banana_ship_script factory;auto ai=factory.GetAI(&shared);assert(dynamic_cast<SmartAI*>(ai));delete ai;
 shared.map=1048;ai=factory.GetAI(&shared);assert(dynamic_cast<npc_unga_banana_ship*>(ai));delete ai;
 Player player;world.players={{&player}};AreaTrigger a,b;b.guid={3};at_titanstrike_spirit_winds first(&a),second(&b);
 // Only the lower Hunter force box loses its duplicate full-length visual.
 // The upper native assembly, player casters, other stages/scenarios/maps and
 // missing casters retain their visual selection and infinite duration.
 Creature lowerVisual,upperVisual;lowerVisual.dbguid=365439;upperVisual.dbguid=365440;
 for(int context=0;context<7;++context){
  AreaTrigger visualArea;visualArea.caster=&lowerVisual;int32 duration=-1;
  if(context==1)visualArea.caster=&upperVisual;
  if(context==2)visualArea.caster=nullptr;
  if(context==3)visualArea.caster=&player;
  if(context==4)progress.step=2;
  if(context==5)progress.id=1082;
  if(context==6)visualArea.map=1048;
  at_titanstrike_spirit_winds visualAI(&visualArea);visualAI.CalculateDuration(duration);
  assert(duration==-1);
  if(context==0){assert(!visualArea.visual&&!visualArea.visualSpell);visualAI.OnUnitEnter(&player);assert(player.force==visualArea.guid);visualAI.BeforeRemove(&player);}
  else assert(visualArea.visual==49393&&visualArea.visualSpell==210880);
  progress.id=1068;progress.step=1;
 }
 player.applied=player.removed=0;
 std::list<Unit*> tracked;assert(first.CreateUnitList(tracked)&&tracked.size()==1);
 first.OnUnitEnter(&player);assert(player.force==a.guid&&player.applied==1);second.OnUnitEnter(&player);assert(player.applied==1);
 second.OnUnitExit(&player);assert(player.force==a.guid&&player.removed==0);first.OnUnitExit(&player);second.OnUpdate(1);assert(player.force==b.guid);
 player.vehicle=true;second.OnUpdate(1);assert(player.force.IsEmpty());player.vehicle=false;second.OnUpdate(1);assert(player.force==b.guid);
 player.stun=true;second.OnUpdate(1);assert(player.force.IsEmpty());player.stun=false;player.disable=true;second.OnUpdate(1);assert(player.force.IsEmpty());
 player.disable=false;player.force={999};second.BeforeRemove(&player);assert(player.force.n==999);player.force={};second.OnUpdate(1);second.BeforeRemove(&player);assert(player.force.IsEmpty());
 for(uint32 other:{909,1082}){progress.id=other;first.OnUpdate(1);assert(player.force.IsEmpty());}progress.id=1068;first.OnUpdate(1);progress.step=2;first.OnUpdate(1);assert(player.force.IsEmpty());progress.step=1;
 // Actual corridor Update starts only the two DB emitters and never cleans a
 // released tornado. It keeps the floor until all three guardians are dead.
 Instance map;Creature lower,upper,rune,tornado;lower.dbguid=365439;upper.dbguid=365440;rune.dbguid=123;tornado.dbguid=456;
 map.creatures={{1,&lower},{2,&upper},{3,&rune},{4,&tornado}};FloorInstance floor;floor.instance=&map;floor.trashList={{1},{2},{3},{4}};
 floor.Update(500);assert(lower.casts==1&&upper.casts==1&&!rune.casts&&!tornado.casts);floor.Update(500);assert(lower.casts==1);
 floor.windshaperDiedCount=2;floor.Update(500);assert(lower.hasAura);floor.windshaperDiedCount=3;floor.Update(500);assert(!lower.hasAura&&lower.areasRemoved==1&&!tornado.areasRemoved);
 floor.windshaperDiedCount=0;floor.Update(500);progress.step=2;floor.Update(500);assert(!upper.hasAura);progress.step=1;
 // Each native lane supplies an explicit destination despite target106's
 // missing channel, and continues emitting after combat starts.
 for(unsigned i=0;i<3;++i){Creature caster;caster.x=windshaperPos[i].x;caster.y=windshaperPos[i].y;caster.z=windshaperPos[i].z;
  npc_sr_spectral_windshaperAI ai(&caster);assert(!ai.combatMovement);ai.Reset();ai.EnterCombat(&player);caster.combat=true;ai.UpdateAI(1000);assert(caster.lastSpell==210976+i&&caster.destination.x==caster.x);
  Creature vortex;vortex.x=caster.x;vortex.y=caster.y;vortex.z=caster.z;npc_sr_fierce_windsAI storm(&vortex);storm.IsSummonedBy(&caster);caster.alive=false;storm.UpdateAI(500);assert(vortex.motion.moves==1&&vortex.motion.destination.y==windEndPos[i].y&&!vortex.despawns);
  storm.MovementInform(POINT_MOTION_TYPE,2);assert(!vortex.despawns);storm.MovementInform(POINT_MOTION_TYPE,1);assert(vortex.vehicle.releases==1&&vortex.areasRemoved==1&&vortex.despawns==1);
 }
 Creature displaced;npc_sr_spectral_windshaperAI unknown(&displaced);unknown.Reset();unknown.UpdateAI(6000);assert(!displaced.casts);
 npc_sr_fierce_windsAI bad(&displaced);bad.IsSummonedBy(&player);bad.UpdateAI(500);assert(displaced.despawns==1&&displaced.vehicle.releases==1);
 Creature stuck;stuck.x=windshaperPos[0].x;stuck.y=windshaperPos[0].y;npc_sr_fierce_windsAI timeout(&stuck);timeout.IsSummonedBy(&player);timeout.UpdateAI(30000);assert(stuck.vehicle.releases==1&&stuck.despawns==1);
 std::cout<<"PASS: production wind actors, corridor lifecycle and AT callbacks; native lanes, combat emissions, caster death independence, endpoint/timeout release, overlap/foreign force, vehicle/stun/death/stage cleanup\n";
}
