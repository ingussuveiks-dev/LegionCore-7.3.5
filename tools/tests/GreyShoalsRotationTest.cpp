#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
constexpr int UNIT_FIELD_FLAGS=0,UNIT_FLAG_NON_ATTACKABLE=2,UNIT_FLAG_NOT_SELECTABLE=0x2000000;
enum ReactStates{REACT_PASSIVE,REACT_DEFENSIVE,REACT_AGGRESSIVE};
enum DamageEffectType{DIRECT_DAMAGE,DOT};
constexpr int QUEST_STATUS_INCOMPLETE=1,GO_READY=0,DIFFICULTY_HEROIC=2;
struct Quest {bool IsAllowedInRaid(int){return false;}};
struct Rotation{int64_t ResetTime=200;};
struct Store{Quest quest;Rotation rotation;bool haveQuest=true,haveRotation=true;
 Quest* GetQuestTemplate(uint32){return haveQuest?&quest:nullptr;}Rotation* GetWorldQuest(Quest const*){return haveRotation?&rotation:nullptr;}}store;
auto sQuestDataStore=&store;
namespace GameTime{int64_t now=100;int64_t GetGameTime(){return now;}}
struct WorldObject{bool inWorld=true;int guid=1;virtual ~WorldObject()=default;
 int GetGUID()const{return guid;}bool IsInWorld()const{return inWorld;}
 bool CanNeverSee(WorldObject const*){return false;}bool CanNeverSee2(WorldObject const*){return false;}
 bool IsAlwaysVisibleFor(WorldObject const*){return true;}bool CanAlwaysSee(WorldObject const*){return true;}
 bool See(WorldObject* obj){
#include "RotationVisibilityDispatch.inc"
 return true;}
 virtual bool IsNeverVisible(WorldObject const* =nullptr)const{return !inWorld;}};
struct Unit:WorldObject{};
struct Group{bool isRaidGroup(){return false;}};
struct Map{int mode=0;int GetSpawnMode(){return mode;}int GetDifficultyID(){return mode;}};
struct Player:Unit{bool IsAlive(){return true;}int GetQuestStatus(int){return 1;}Group* GetGroup(){return nullptr;}Map* GetMap(){static Map m;return &m;}};
struct CreatureData{int areaId=8297;};
struct GameObjectData{int areaId=8297;uint64 spawnMask=1;};
struct Creature;
struct CreatureAI{Creature* me;explicit CreatureAI(Creature* c):me(c){}virtual ~CreatureAI()=default;
#include "RotationCreatureDefaults.inc"
 virtual void InitializeAI(){}virtual void JustRespawned(){}virtual void UpdateAI(uint32){}virtual void MoveInLineOfSight(Unit*){}
 virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}};
struct Creature:Unit{
 uint32 entry=115025,map=1220,flags=32768;CreatureData data;bool hasData=true,alive=true,combat=false,casting=false,evade=false;
 bool IsAIEnabled=true,m_isUpdate=false,m_AI_locked=false,deleted=false,preDeleted=false;
 std::string aiName="SmartAI";std::string GetAIName(){return aiName;}
 ReactStates react=REACT_AGGRESSIVE;CreatureAI* i_AI=nullptr;int visibleUpdates=0,combatStops=0,threatClears=0,castStops=0;
 CreatureData const* GetCreatureData()const{return hasData?&data:nullptr;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return map;}
 uint32 GetUInt32Value(int){return flags;}ReactStates GetReactState(){return react;}void SetReactState(ReactStates r){react=r;}
 bool IsAlive(){return alive;}bool isInCombat(){return combat;}void CastStop(){casting=false;++castStops;}
 void CombatStop(bool){combat=false;++combatStops;}void DeleteThreatList(){++threatClears;}
 void SetFlag(int,uint32 f){flags|=f;}void RemoveFlag(int,uint32 f){flags&=~f;}
 void UpdateObjectVisibility(){++visibleUpdates;}CreatureAI* AI()const{return i_AI;}
 bool IsDelete(){return deleted;}bool IsPreDelete(){return preDeleted;}
 bool IsNeverVisible(WorldObject const*)const override;void Update(uint32);
};
struct SmartAI:CreatureAI{int initializes=0,respawns=0,updates=0,los=0,evades=0;using CreatureAI::CreatureAI;
 void InitializeAI()override{++initializes;}void JustRespawned()override{++respawns;}
 void UpdateAI(uint32)override{++updates;}void MoveInLineOfSight(Unit*)override{++los;}
 void EnterEvadeMode(){++evades;me->evade=true;me->combat=false;}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual CreatureAI* GetAI(Creature*)const{return nullptr;}};
struct GameObject;
struct GameObjectAI{GameObject* go;explicit GameObjectAI(GameObject* g):go(g){}virtual ~GameObjectAI()=default;
#include "RotationGoDefaults.inc"
 virtual void InitializeAI(){}virtual void UpdateAI(uint32){}};
struct GameObject:WorldObject{uint32 entry=266170,map=1220,area=8297;GameObjectData data;bool hasData=true,spawned=true;
 int loot=GO_READY,visibleUpdates=0;Map nativeMap;GameObjectAI* ai=nullptr;std::map<int,int64_t> m_lastUser;
 GameObjectData const* GetGOData()const{return hasData?&data:nullptr;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return map;}
 uint32 GetAreaId(){return area;}Map* GetMap()const{return const_cast<Map*>(&nativeMap);}void UpdateObjectVisibility(){++visibleUpdates;}
 GameObjectAI* AI()const{return ai;}bool IsNeverVisible(WorldObject const*)const override;
 bool InSamePhase(Player*){return true;}float GetInteractionDistance(){return 5;}bool IsWithinDistInMap(Player*,float){return true;}
 bool isSpawned(){return spawned;}int getLootState(){return loot;}};
struct GameObjectScript{explicit GameObjectScript(char const*){}virtual GameObjectAI* GetAI(GameObject*)const{return nullptr;}
 virtual bool OnGossipHello(Player*,GameObject*){return false;}};
#include "grey_shoals.h"
#include "RotationGoFactory.inc"
#include "RotationCreatureVisibility.inc"
#include "RotationGoVisibility.inc"
struct SmartScript{Creature* me;bool IsSmart(Creature*);};
#define TC_LOG_ERROR(...) ((void)0)
#include "RotationSmartAdmission.inc"
#include "RotationLifecycle.inc"
 // The rest of Creature::Update (map/Unit/death/respawn services) is outside
 // this fixture. The extracted prefix proves delivery before those branches.
 m_isUpdate=false;
}
struct DamageDispatcher:Unit{uint32 Hit(Creature* creature,uint32 damage,DamageEffectType damagetype){
#include "RotationScaledDamage.inc"
 return damage;}};
int main(){
 WorldObject seer;Unit passerby;npc_grey_shoals factory;go_grey_shoals_supplies supply;
 for(uint32 entry:{115025u,115026u,115031u,115751u}){
  Creature c;c.entry=entry;std::unique_ptr<CreatureAI> owner(factory.GetAI(&c));assert(owner);c.i_AI=owner.get();
  auto& ai=*static_cast<npc_grey_shoals_rotation*>(c.i_AI);ai.InitializeAI();
  assert(ai.initializes==1&&!c.IsNeverVisible(&seer));ai.UpdateAI(1);ai.MoveInLineOfSight(&passerby);assert(ai.updates==1&&ai.los==1);
  c.combat=true;c.casting=true;c.flags|=0x10000000;GameTime::now=200;
  // Visibility and damage enforce expiry even before the lifecycle tick.
  assert(c.IsNeverVisible(&seer)&&!seer.See(&c));DamageDispatcher hit;assert(hit.Hit(&c,0xffffffff,DIRECT_DAMAGE)==0);assert(hit.Hit(&c,100,DOT)==0);
  c.Update(1);assert(!c.combat&&!c.casting&&c.react==REACT_PASSIVE&&c.evade&&ai.evades==1);
  assert((c.flags&ai.GateFlags)==ai.GateFlags&&(c.flags&0x10000000));
  int refresh=c.visibleUpdates;c.Update(1);assert(c.visibleUpdates==refresh);
  ai.UpdateAI(1);ai.MoveInLineOfSight(&passerby);assert(ai.updates==1&&ai.los==1);
  // Corpses still get the lifecycle callback; a fresh grid object is hidden
  // before InitializeAI/its first Update, because visibility queries the clock.
  c.alive=false;c.Update(1);assert(c.IsNeverVisible(&seer));
  c.flags=32768;c.react=REACT_AGGRESSIVE;ai.JustRespawned();assert(c.IsNeverVisible(&seer)&&c.react==REACT_PASSIVE);
  GameTime::now=100;store.rotation.ResetTime=300;c.Update(1);
  assert(!c.IsNeverVisible(&seer)&&c.react==REACT_AGGRESSIVE&&!(c.flags&ai.GateFlags));
  assert(hit.Hit(&c,100,DIRECT_DAMAGE)==100);ai.UpdateAI(1);assert(ai.updates==2);store.rotation.ResetTime=200;
  SmartScript admission{&c};assert(admission.IsSmart(&c));c.aiName="";assert(!admission.IsSmart(&c));
 }
 // Scope is the DB home area, not current movement position. No summons or
 // shared entries elsewhere acquire a rotation AI.
 for(int kind=0;kind<4;++kind){Creature c;if(kind==0)c.hasData=false;if(kind==1)c.map=1498;if(kind==2)c.data.areaId=1;if(kind==3)c.entry=1;assert(!factory.GetAI(&c));}
 store.haveRotation=false;
 Creature loaded;std::unique_ptr<CreatureAI> loadedAI(factory.GetAI(&loaded));loaded.i_AI=loadedAI.get();assert(loaded.IsNeverVisible(&seer));
 loaded.flags|=UNIT_FLAG_NON_ATTACKABLE; // Existing gate bit is preserved on reactivation.
 Creature protectedNpc;protectedNpc.flags|=UNIT_FLAG_NON_ATTACKABLE;std::unique_ptr<CreatureAI> protectedAI(factory.GetAI(&protectedNpc));protectedNpc.i_AI=protectedAI.get();protectedNpc.Update(1);
 store.haveRotation=true;protectedNpc.Update(1);assert(protectedNpc.flags&UNIT_FLAG_NON_ATTACKABLE);
 store.haveQuest=false;assert(loaded.IsNeverVisible(&seer));store.haveQuest=true;
 Creature ordinary;CreatureAI ordinaryAI(&ordinary);ordinary.i_AI=&ordinaryAI;assert(!ordinary.IsNeverVisible(&seer));
 ordinary.inWorld=false;assert(ordinary.IsNeverVisible(&seer));
 GameObject rack;std::unique_ptr<GameObjectAI> rackAI(supply.GetAI(&rack));assert(rackAI);rack.ai=rackAI.get();rackAI->InitializeAI();assert(!rack.IsNeverVisible(&seer));
 GameTime::now=200;assert(rack.IsNeverVisible(&seer));rackAI->UpdateAI(1);int updates=rack.visibleUpdates;rackAI->UpdateAI(1);assert(rack.visibleUpdates==updates);
 Player player;assert(supply.OnGossipHello(&player,&rack));
 GameTime::now=100;store.rotation.ResetTime=300;rack.spawned=false;rack.loot=1;rackAI->UpdateAI(1);assert(!rack.IsNeverVisible(&seer)&&!rack.spawned&&rack.loot==1);
 // Existing per-user and challenge invisibility must not be bypassed.
 rack.m_lastUser[seer.guid]=200;assert(rack.IsNeverVisible(&seer));rack.m_lastUser.clear();rack.data.spawnMask=256;rack.nativeMap.mode=DIFFICULTY_HEROIC;assert(rack.IsNeverVisible(&seer));
 for(int kind=0;kind<4;++kind){GameObject go;if(kind==0)go.hasData=false;if(kind==1)go.map=1;if(kind==2)go.data.areaId=1;if(kind==3)go.entry=241741;assert(!supply.GetAI(&go));}
 std::cout<<"PASS: four SmartAI actors, pre-update expiry visibility/scaled damage, combat stop, corpse/respawn/new rotation, scope isolation, retained flags and native GO visibility gates\n";
}
