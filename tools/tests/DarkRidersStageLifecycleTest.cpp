#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <list>
#include <functional>
using uint32=uint32_t;using uint64=uint64_t;using ObjectGuid=uint64;
using ReactStates=uint32;constexpr ReactStates REACT_PASSIVE=0,REACT_AGGRESSIVE=2;
constexpr uint32 UNIT_FIELD_FLAGS=0,GAMEOBJECT_FIELD_FLAGS=0,UNIT_FLAG_NON_ATTACKABLE=2,GO_FLAG_NOT_SELECTABLE=16,GO_FLAG_IN_USE=1;
constexpr uint32 UNIT_FIELD_NPC_FLAGS=1,UNIT_FIELD_CREATED_BY_SPELL=2,TEMPSUMMON_MANUAL_DESPAWN=8;
using GuidList=std::list<ObjectGuid>;
struct Position{Position(float,float,float,float){}};
#define TC_LOG_INFO(...) ((void)0)
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
struct Creature;struct CreatureAI {void MoveInLineOfSight_Safe(struct Player*){}};
struct Unit{};
constexpr int QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1;
struct InstanceMap;
struct Player:Unit{
 bool aura=false;ObjectGuid GetGUID(){return 42;}
 bool controller=false;uint32 followerAttempts=0;std::function<void()> summonFollower;
 InstanceMap* map=nullptr;GuidList followers;InstanceMap* GetMap(){return map;}GuidList* GetSummonList(uint32){return &followers;}
 bool IsInWorld(){return true;}bool IsAlive(){return true;}bool IsBeingTeleported(){return false;}
 uint32 GetMapId(){return 1533;}bool GetQuestRewardStatus(uint32){return false;}int GetQuestStatus(uint32){return QUEST_STATUS_INCOMPLETE;}
 Creature* FindNearestCreature(uint32,float){return nullptr;}bool InSamePhase(Creature*){return true;}uint32 GetGUIDLow(){return 1;}
 float GetExactDist(Creature*){return 100;}bool HasAura(uint32 id){return id==199626?controller:aura;}
 void CastSpell(Player* target,uint32 id,bool triggered){if(id==199626){assert(target==this&&triggered);controller=true;++followerAttempts;if(summonFollower)summonFollower();}else aura=true;}
 void RemoveAurasDueToSpell(uint32 id){if(id==198654)aura=false;if(id==199626)controller=false;}
};
struct Creature{
 uint32 entry=0,flags=0;uint64 guid=0,hp=100000,targetHp=1000;bool summoned=false,alive=true;ReactStates reaction=REACT_AGGRESSIVE;
 uint32 GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}bool isSummon(){return summoned;}bool IsAlive(){return alive;}
 bool IsAIEnabled=true,world=true;bool IsInWorld(){return world;}uint32 GetDBTableGUIDLow(){return db;}CreatureAI* AI(){static CreatureAI ai;return &ai;}
 bool IsWithinLOSInMap(Player*){return true;}
 uint32 db=0,createdBy=199625,npcFlags=2,retired=0;ObjectGuid owner=42;std::function<Creature*(Creature*,uint32)> summon;
 Creature* ToTempSummon(){return this;}ObjectGuid GetSummonerGUID(){return owner;}void DespawnOrUnsummon(){++retired;}
 void SetUInt32Value(uint32 field,uint32 value){assert(field==UNIT_FIELD_NPC_FLAGS);npcFlags=value;}
 Creature* SummonCreature(uint32 entry,Position,uint32 type){assert(type==8);return summon?summon(this,entry):nullptr;}
 uint64 GetHealth(Unit*){return targetHp;}uint64 GetHealth(){return hp;}void SetHealth(uint64 value){hp=targetHp=value;}
 uint32 GetUInt32Value(uint32 field){return field==UNIT_FIELD_CREATED_BY_SPELL?createdBy:flags;}void SetFlag(uint32,uint32 bits){flags|=bits;}void RemoveFlag(uint32,uint32 bits){flags&=~bits;}
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
 std::vector<Player*> players;template<class F>void ApplyOnEveryPlayer(F f){for(auto p:players)f(p);}
 uint32 GetInstanceId(){return 1;}Creature* GetCreature(uint64 guid){return creatures.count(guid)?creatures.at(guid):nullptr;}
 GameObject* GetGameObject(uint64 guid){return objects.count(guid)?objects.at(guid):nullptr;}
};
struct InstanceScript{
 InstanceMap* instance;explicit InstanceScript(InstanceMap* map):instance(map){}virtual ~InstanceScript()=default;
 virtual void OnCreatureCreate(Creature*){}virtual void OnCreatureRemove(Creature*){}
 virtual void OnGameObjectCreate(GameObject*){}virtual void OnGameObjectRemove(GameObject*){}
 virtual void OnPlayerEnter(Player*){}virtual void OnPlayerLeave(Player*){}virtual void onScenarionNextStep(uint32){}virtual void Update(uint32){}
 virtual void SetData(uint32,uint32){}
 virtual void OnCreatureDamageTaken(Creature*,Unit*,uint32&){}
};
struct InstanceMapScript{
 InstanceMapScript(char const*,uint32){}virtual ~InstanceMapScript()=default;virtual InstanceScript* GetInstanceScript(InstanceMap*)const=0;
};
#include "LifecycleInstance.inc"
struct WorldObject{InstanceScript* script;InstanceScript* GetInstanceScript(){return script;}};
constexpr uint32 SMART_ACTION_SET_INST_DATA=34;
void NativeGuideRequest(InstanceScript* script,bool invokerFallback=false,uint32 field=10085001){
 WorldObject object{script};WorldObject* unit=invokerFallback?&object:nullptr;
 auto GetBaseObject=[&]()->WorldObject*{return invokerFallback?nullptr:&object;};
 struct Row{int entryOrGuid=-369334;uint32 GetEventType(){return 61;}struct{struct{uint32 field=10085001,data=1;}setInstanceData;}action;}e;
 e.action.setInstanceData.field=field;
 switch(SMART_ACTION_SET_INST_DATA){
#include "RevilInstanceAction.inc"
 }
}
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
 Player departing;departing.aura=true;script.OnPlayerLeave(&departing);assert(!departing.aura);
 // Infinite native contact aura must be removed even if Scenario teardown
 // precedes the leave callback. Preserve actors from unrelated scenarios.
 departing.aura=true;script.spiritPlayers.insert(departing.GetGUID());manager.progress=nullptr;
 script.OnPlayerLeave(&departing);assert(!departing.aura&&script.spiritPlayers.empty());
 // Real production handoff: no clone for a wrong stage, retry a missing
 // actor/failed summon, one clone despite repeat requests, no respawn after
 // the native guide intentionally despawns. Retire only our actual follower.
 Scenario handoff;manager.progress=&handoff;InstanceMap hm;instance_dark_riders_artifact::Instance hs(&hm);
 Player escortOwner;escortOwner.map=&hm;hm.players.push_back(&escortOwner);
 Creature escort{101286,768,81},demon{1860,0,82},foreign{101286,768,83},wrongSpell{101286,768,84};
 escort.summoned=demon.summoned=foreign.summoned=wrongSpell.summoned=true;
 foreign.owner=43;wrongSpell.createdBy=99;
 for(auto c:{&escort,&demon,&foreign,&wrongSpell}){hm.creatures[c->guid]=c;escortOwner.followers.push_back(c->guid);}
 NativeGuideRequest(&hs);assert(!hs.barrierGuideRequested&&!escort.retired);
 handoff.step=1;hs.SetData(1,1);assert(!hs.barrierGuideRequested);
 NativeGuideRequest(&hs);assert(hs.barrierGuideRequested&&!hs.barrierGuideStarted&&escort.retired==1);
 Creature initial{100850,0,85},guide{101282,768,86};initial.db=369334;guide.summoned=true;guide.owner=initial.guid;
 hm.creatures[initial.guid]=&initial;hs.OnCreatureCreate(&initial);uint32 attempts=0;
 initial.summon=[&](Creature* parent,uint32 entry)->Creature*{assert(parent==&initial&&entry==101282);++attempts;if(attempts==1)return nullptr;hm.creatures[guide.guid]=&guide;hs.OnCreatureCreate(&guide);return &guide;};
 hs.Update(250);assert(attempts==1&&!hs.barrierGuideStarted);hs.Update(250);assert(attempts==2&&hs.barrierGuideStarted&&guide.npcFlags==0);
 for(int i=0;i<8;++i){NativeGuideRequest(&hs,true);hs.Update(250);}assert(attempts==2&&!demon.retired&&!foreign.retired&&!wrongSpell.retired);
 Creature otherGuide{101282,768,87};otherGuide.summoned=true;otherGuide.owner=999;hs.OnCreatureCreate(&otherGuide);assert(otherGuide.npcFlags==2);
 hs.OnCreatureRemove(&guide);hm.creatures.erase(guide.guid);hs.Update(250);assert(attempts==2);
 handoff.step=3;uint32 before=escort.retired;hs.Update(250);assert(escort.retired==before); // Later escort retained.
 otherGuide.owner=initial.guid;hs.OnCreatureCreate(&otherGuide);assert(otherGuide.npcFlags==2); // Native wounded/finale flags retained.
 // The entrance speaker may already have despawned and lost its invoker.
 // Completion persists in the instance. Respect200ms, retry a failed summon
 // or stale controller, and stop recreating the follower at the barrier.
 Scenario intro;manager.progress=&intro;InstanceMap im;instance_dark_riders_artifact::Instance is(&im);Player ip;ip.map=&im;im.players.push_back(&ip);
 Creature follower{101286,768,91};follower.summoned=true;follower.world=false;im.creatures[91]=&follower;ip.controller=true;
 ip.summonFollower=[&]{if(ip.followerAttempts>1){follower.world=true;ip.followers={91};}};
 is.Update(1000);assert(!ip.followerAttempts&&!is.entranceFollowerRequested);
 NativeGuideRequest(&is,true,DarkRidersRevil::EntranceFollowerRequest);is.Update(199);assert(!ip.followerAttempts);is.Update(1);assert(ip.followerAttempts==1&&!DarkRidersRevil::HasFollower(&ip));
 is.Update(999);assert(ip.followerAttempts==1);is.Update(1);assert(ip.followerAttempts==2&&DarkRidersRevil::HasFollower(&ip));
 for(int i=0;i<8;++i){NativeGuideRequest(&is,false,DarkRidersRevil::EntranceFollowerRequest);is.Update(1000);}assert(ip.followerAttempts==2);
 follower.world=false;is.Update(1000);assert(ip.followerAttempts==3&&follower.world);
 intro.step=1;uint32 castBefore=ip.followerAttempts;NativeGuideRequest(&is,false,DarkRidersRevil::EntranceFollowerRequest);is.Update(1000);assert(ip.followerAttempts==castBefore);
 for(uint32 otherScenario:{0u,1026u,1014u}){
  Scenario shared;shared.id=otherScenario;manager.progress=&shared;InstanceMap otherMap;instance_dark_riders_artifact::Instance untouched(&otherMap);
  Creature actor{101257,0x400,6};GameObject object;otherMap.creatures[6]=&actor;otherMap.objects[20]=&object;untouched.OnCreatureCreate(&actor);untouched.OnGameObjectCreate(&object);untouched.Update(250);
  NativeGuideRequest(&untouched);assert(!untouched.barrierGuideRequested&&!untouched.barrierGuideStarted);
  assert(actor.flags==0x400&&!object.flags);uint32 amount=200000;untouched.OnCreatureDamageTaken(&actor,&pet,amount);assert(amount==200000);
 }
 std::cout<<"PASS: complete production instance lifecycle; delayed Scenario construction, native GO Use admission, every stage/actor, scaled barrier/finale guards, native flags, removal/re-entry and shared-scenario isolation\n";
}
