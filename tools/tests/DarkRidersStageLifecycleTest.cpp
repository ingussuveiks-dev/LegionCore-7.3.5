#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
using uint32=uint32_t;using uint64=uint64_t;using ObjectGuid=uint64;
using ReactStates=uint32;constexpr ReactStates REACT_PASSIVE=0,REACT_AGGRESSIVE=2;
constexpr uint32 UNIT_FIELD_FLAGS=0,GAMEOBJECT_FIELD_FLAGS=0,UNIT_FLAG_NON_ATTACKABLE=2,GO_FLAG_NOT_SELECTABLE=16,GO_FLAG_IN_USE=1;
#define TC_LOG_INFO(...) ((void)0)
struct Unit{};struct Player:Unit{};
struct Creature{
 uint32 entry=0,flags=0;uint64 guid=0,hp=100000,targetHp=1000;bool summoned=false,alive=true;ReactStates reaction=REACT_AGGRESSIVE;
 uint32 GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}bool isSummon(){return summoned;}bool IsAlive(){return alive;}
 uint64 GetHealth(Unit*){return targetHp;}uint64 GetHealth(){return hp;}void SetHealth(uint64 value){hp=targetHp=value;}
 uint32 GetUInt32Value(uint32){return flags;}void SetFlag(uint32,uint32 bits){flags|=bits;}void RemoveFlag(uint32,uint32 bits){flags&=~bits;}
 ReactStates GetReactState(){return reaction;}void SetReactState(ReactStates value){reaction=value;}
};
struct GameObject{
 uint32 entry=245994,flags=0,uses=0;uint64 guid=20;
 uint32 GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}uint32 GetUInt32Value(uint32){return flags;}
 void SetFlag(uint32,uint32 bits){flags|=bits;}void RemoveFlag(uint32,uint32 bits){flags&=~bits;}
 bool isSpawned(){return true;}bool HasFlag(uint32,uint32 bits){return flags&bits;}
 void Use(){
#include "LifecycleGoAdmission.inc"
  ++uses;
 }
};
struct Scenario{uint32 id=988,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Manager{Scenario* progress=nullptr;Scenario* GetScenario(uint32){return progress;}}manager;auto sScenarioMgr=&manager;
struct InstanceMap{
 std::map<uint64,Creature*> creatures;std::map<uint64,GameObject*> objects;
 uint32 GetInstanceId(){return 1;}Creature* GetCreature(uint64 guid){return creatures.count(guid)?creatures.at(guid):nullptr;}
 GameObject* GetGameObject(uint64 guid){return objects.count(guid)?objects.at(guid):nullptr;}
};
struct InstanceScript{
 InstanceMap* instance;explicit InstanceScript(InstanceMap* map):instance(map){}virtual ~InstanceScript()=default;
 virtual void OnCreatureCreate(Creature*){}virtual void OnCreatureRemove(Creature*){}
 virtual void OnGameObjectCreate(GameObject*){}virtual void OnGameObjectRemove(GameObject*){}
 virtual void OnPlayerEnter(Player*){}virtual void onScenarionNextStep(uint32){}virtual void Update(uint32){}
 virtual void OnCreatureDamageTaken(Creature*,Unit*,uint32&){}
};
struct InstanceMapScript{
 InstanceMapScript(char const*,uint32){}virtual ~InstanceMapScript()=default;virtual InstanceScript* GetInstanceScript(InstanceMap*)const=0;
};
#include "LifecycleInstance.inc"
int main(){
 Scenario scenario;InstanceMap map;instance_dark_riders_artifact::Instance script(&map);Unit player,pet;
 Creature barrier{100850,0x400,1},guardian{101257,0x400,2},finalBoss{100344,0x400,3},summoned{100850,768,4};summoned.summoned=true;
 GameObject scythe,other{246436,0,0,21};
 for(auto c:{&barrier,&guardian,&finalBoss,&summoned}){map.creatures[c->guid]=c;script.OnCreatureCreate(c);}
 for(auto go:{&scythe,&other}){map.objects[go->guid]=go;script.OnGameObjectCreate(go);}
 // Actor registration may precede Scenario construction. Retry must enforce
 // the same state after that constructor, including stationary login.
 assert(!scythe.flags&&guardian.flags==0x400);manager.progress=&scenario;script.Update(250);
 assert(scythe.flags&GO_FLAG_NOT_SELECTABLE);scythe.Use();assert(!scythe.uses);
 for(auto c:{&barrier,&guardian,&finalBoss}){assert(c->flags&2);uint32 damage=200000;script.OnCreatureDamageTaken(c,&pet,damage);assert(!damage&&c->hp==100000);}
 for(uint32 step=0;step<7;++step){
  scenario.step=step;script.onScenarionNextStep(step);
  for(auto c:{&barrier,&guardian,&finalBoss,&summoned}){
   bool allowed=DarkRidersArtifact::ActorAvailable(c->entry,c->summoned,step);
   assert(bool(c->flags&2)==!allowed);assert((c->flags&(c==&summoned?768:0x400))==(c==&summoned?768:0x400));
   assert(c->reaction==(allowed?REACT_AGGRESSIVE:REACT_PASSIVE));
  }
  assert(bool(scythe.flags&GO_FLAG_NOT_SELECTABLE)==(step!=3));assert(!other.flags);
 }
 // Correct native sequence: entrance -> barrier -> Conservator -> clickable
 // artifact -> final approach -> defeat. Events remain external; Sync never
 // awards a criterion merely because an actor was spawned or clicked early.
 scenario.step=1;script.Sync();
 for(Unit* attacker:{&player,&pet}){uint32 lethal=200000;barrier.hp=100000;barrier.targetHp=1000;script.OnCreatureDamageTaken(&barrier,attacker,lethal);assert(!lethal&&barrier.hp==1&&barrier.alive);}
 scenario.step=2;script.Sync();uint32 lethal=200000;script.OnCreatureDamageTaken(&guardian,&pet,lethal);assert(lethal==200000);
 scenario.step=3;script.Sync();scythe.Use();assert(scythe.uses==1);assert(!(summoned.flags&2)&&summoned.flags==768);
 scenario.step=4;script.Sync();assert(!(finalBoss.flags&2));scenario.step=5;script.Sync();lethal=200000;script.OnCreatureDamageTaken(&finalBoss,&pet,lethal);assert(!lethal&&finalBoss.hp==1);
 // Reset/re-entry, missing actor, late grid load and an independently set
 // NON_ATTACKABLE bit retain ownership of native flags.
 script.OnCreatureRemove(&guardian);map.creatures.erase(guardian.guid);script.OnGameObjectRemove(&scythe);map.objects.erase(scythe.guid);
 scenario.step=0;script.Sync();map.creatures[guardian.guid]=&guardian;guardian.flags=0x400;script.OnCreatureCreate(&guardian);assert(guardian.flags&2);
 Creature nativeProtected{101257,2,5};nativeProtected.reaction=REACT_PASSIVE;map.creatures[5]=&nativeProtected;script.OnCreatureCreate(&nativeProtected);scenario.step=2;script.Sync();assert(nativeProtected.flags==2&&nativeProtected.reaction==REACT_PASSIVE);
 for(uint32 otherScenario:{0u,1026u,1014u}){
  Scenario shared;shared.id=otherScenario;manager.progress=&shared;InstanceMap otherMap;instance_dark_riders_artifact::Instance untouched(&otherMap);
  Creature actor{101257,0x400,6};GameObject object;otherMap.creatures[6]=&actor;otherMap.objects[20]=&object;untouched.OnCreatureCreate(&actor);untouched.OnGameObjectCreate(&object);untouched.Update(250);
  assert(actor.flags==0x400&&!object.flags);uint32 amount=200000;untouched.OnCreatureDamageTaken(&actor,&pet,amount);assert(amount==200000);
 }
 std::cout<<"PASS: complete production instance lifecycle; delayed Scenario construction, native GO Use admission, every stage/actor, scaled barrier/finale guards, native flags, removal/re-entry and shared-scenario isolation\n";
}
