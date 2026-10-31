#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
enum { CLASS_SHAMAN=7, QUEST_STATUS_NONE=0, QUEST_STATUS_COMPLETE=1, QUEST_STATUS_INCOMPLETE=3, CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_FIELD_NPC_FLAGS=1,UNIT_FIELD_FLAGS=2 };
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Player;struct Creature;struct Map;struct Unit;
struct ObjectGuid { uint64 high=0,low=0;bool IsPlayer()const{return high==1;}bool IsEmpty()const{return !high&&!low;}explicit operator bool()const{return !IsEmpty();}bool operator==(ObjectGuid other)const{return high==other.high&&low==other.low;} };
struct PlayerRef {Player* player;Player* getSource()const{return player;}};
struct Map {uint32 instanceId=7;std::vector<PlayerRef> players;auto const& GetPlayers()const{return players;}uint32 GetInstanceId()const{return instanceId;}} liveMap,otherMap;
using InstanceMap=Map;
struct WorldLocation {void WorldRelocate(uint32,float,float,float){}};
struct InstanceScript {Map* instance;explicit InstanceScript(Map* map):instance(map){}virtual ~InstanceScript()=default;virtual void CreatureDies(Creature*,Unit*){}virtual void CreatureDiesForScript(Creature*,Unit*){++nativeScriptDeaths;}virtual void OnPlayerEnter(Player*){}virtual WorldLocation* GetClosestGraveYard(float,float,float){return nullptr;}virtual void OnUnitDeath(Unit*){}virtual void OnCreatureCreate(Creature*){}virtual void Update(uint32){}uint32 nativeScriptDeaths=0;};
struct Unit {
 virtual ~Unit()=default;Map* map=&liveMap;ObjectGuid guid{2,1},owner,charmer;bool world=true,phase=true;Player* nearest=nullptr;
 virtual Player const* ToPlayer()const{return nullptr;}Player* ToPlayer(){return const_cast<Player*>(static_cast<Unit const*>(this)->ToPlayer());}virtual Creature* ToCreature(){return nullptr;}
 Map* GetMap()const{return map;}bool IsInWorld()const{return world;}bool InSamePhase(Unit const* u)const{return phase&&u->phase;}
 ObjectGuid GetGUID()const{return guid;}ObjectGuid GetCharmerGUID()const{return charmer;}ObjectGuid GetOwnerGUID()const{return owner;}
#include "OwnerGuid.inc"
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;Player* FindNearestPlayer(float){return nearest;}virtual uint32 GetEntry()const{return 0;}
 void NativeKillDispatch(Creature*);
};
struct QuestStatusData {uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime;};
struct Player:Unit {
 QuestStatusData quest;bool record=true,contact=true,gm=false,teleport=false,rewarded=false,near=true,alive=true;uint32 classId=7,group=0,updates=0,conversations=0;std::function<void()> onCredit;std::vector<std::function<void()>> delayed;
 Player(){guid={1,1};}Player const* ToPlayer()const override{return this;}
 bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool IsBeingTeleported()const{return teleport;}uint32 getClass()const{return classId;}
 bool GetQuestRewardStatus(uint32 id)const{assert(id==40341);return rewarded;}uint32 GetQuestStatus(uint32 id)const{assert(id==40341);return record?quest.Status:0;}QuestStatusData* getQuestStatus(uint32 id){assert(id==40341);return record?&quest:nullptr;}
 bool IsWithinDistInMap(Unit*,float range)const{assert(range==100);return near;}bool IsInSameGroupWith(Player const* other)const{return this==other||(group&&group==other->group);}
 void AddDelayedEvent(uint64,std::function<void()>&& action){delayed.push_back(std::move(action));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void UpdateAchievementCriteria(uint32,uint32);void CreateConversation(uint32 id){assert(id==1693);++conversations;}
 Creature* FindNearestCreature(uint32,float,bool){return nullptr;}
};
struct Creature:Unit {
 uint32 entry=102831;uint64 dbGuid=362076;bool alive=false,preDelete=false;InstanceScript* script=nullptr;uint32 flags=0;
 Creature* ToCreature()override{return this;}uint32 GetEntry()const override{return entry;}uint64 GetDBTableGUIDLow()const{return dbGuid;}bool IsAlive()const{return alive;}bool IsPreDelete()const{return preDelete;}InstanceScript* GetInstanceScript(){return script;}
 void SetUInt32Value(uint32,uint32 value){flags=value;}void SetFlag(uint32,uint32){}void SetVisible(bool){}
 Creature* FindNearestCreature(uint32,float,bool){return nullptr;}struct Voice{void Talk(uint32){}};Voice* AI(){static Voice voice;return &voice;}void AddDelayedEvent(uint64,std::function<void()>&&){assert(false);}
};
struct ObjectAccessor {static Player* GetPlayer(Unit const& source,ObjectGuid guid){for(auto r:source.map->players)if(r.player&&r.player->world&&r.player->GetMap()==source.map&&r.player->guid==guid)return r.player;return nullptr;}};
#include "Owner.inc"
#include "Quest.inc"
#include "Kill.inc"
struct CriteriaTreeEntry {uint32 ID,CriteriaID;};
struct DB2Manager {std::vector<CriteriaTreeEntry const*> current;auto GetCriteriaTreeList(uint32 tree){return tree==47122?&current:nullptr;}} db2;
auto& sDB2Manager=db2;
struct Progress {uint64 Counter=0;};struct Achievement {std::map<uint32,Progress> progress;auto const* GetCriteriaProgressMap()const{return &progress;}};
struct Scenario {uint32 id=1066,step=1,currentTree=47122,_criteriaTreeForUpdate=0;bool reject=false;Achievement achievement;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}auto& GetAchievementMgr(){return achievement;}bool CanUpdateCriteria(uint32,uint32=0)const; } scenario;
#include "ScenarioGate.inc"
struct Manager {Scenario* active=&scenario;Scenario* GetScenario(uint32 instanceId){return instanceId==7?active:nullptr;}} manager;
auto sScenarioMgr=&manager;
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset){assert(type==92&&(asset==49957||asset==49861));++updates;if(asset==49957&&scenario.id==1066&&scenario.step==1&&!scenario.reject&&scenario.CanUpdateCriteria(47197))++scenario.achievement.progress[47197].Counter;if(onCredit)onCredit();}
#include "Waypoint.inc"
#include "Policy.inc"
#include "Instance.inc"
using Script=instance_sharasdal_InstanceMapScript;
struct Run {
 Player owner,neighbor;Unit pet,guardian;Creature guard,ally;Script script{&liveMap};
 Run(){scenario=Scenario{};manager.active=&scenario;warnings=0;static CriteriaTreeEntry guardTree{47197,29695},rescueTree{47123,29659};db2.current={&guardTree,&rescueTree};owner.guid={1,1};neighbor.guid={1,2};liveMap.players={{&owner},{&neighbor}};pet.owner=guardian.owner=owner.guid;guard.script=&script;guard.nearest=&neighbor;ally.entry=102826;ally.dbGuid=362063;neighbor.classId=5;}
 ~Run(){liveMap.players.clear();}
 void Kill(Unit* killer){if(killer)killer->NativeKillDispatch(&guard);else script.CreatureDies(&guard,nullptr);}
 uint64 Count(){return scenario.achievement.progress[47197].Counter;}
};
int main(){
#ifdef BASELINE
 {Run r;r.script.OnUnitDeath(&r.guard);r.Kill(&r.owner);r.script.Update(9000);assert(!r.Count()&&!r.owner.updates&&!r.neighbor.updates);}
 {Run r;r.guard.alive=true;DispatchWaypoint(&r.guard);assert(r.Count()==1&&r.neighbor.updates==1&&!r.owner.updates);}
 std::cout<<"PASS COUNTERFACTUAL c27da5170: real Tidecaller death/native Kill bridge grants no guard criterion; exact old waypoint8 manufactures it for nearest unrelated player while guard lives. No raw delayed actor freed.\n";
#else
 unsigned cases=0;
 {Run r;r.guard.alive=true;DispatchWaypoint(&r.guard);assert(!r.Count()&&!r.neighbor.updates&&!r.owner.updates);}
 for(uint32 state:{1u,3u})for(int source=0;source<6;++source){Run r;r.owner.quest.Status=state;Unit* killers[]={&r.owner,&r.pet,&r.guardian,&r.pet,&r.ally,&r.ally};if(source==3){r.pet.owner={};r.pet.charmer=r.owner.guid;}if(source==5){r.ally.entry=105298;r.ally.dbGuid=362066;}r.script.OnUnitDeath(&r.guard);assert(!r.Count());r.Kill(killers[source]);assert(r.Count()==1&&!r.script.guardPending&&r.owner.updates==1&&!r.neighbor.updates&&r.script.nativeScriptDeaths==1);r.Kill(killers[source]);r.script.Update(60000);assert(r.Count()==1&&r.owner.updates==1&&!warnings);++cases;}
 for(int deny=0;deny<22;++deny){Run r;Unit* killer=&r.owner;switch(deny){case 0:r.guard.alive=true;break;case 1:r.guard.entry=105028;break;case 2:r.guard.dbGuid=362077;break;case 3:r.guard.map=&otherMap;break;case 4:r.guard.world=false;break;case 5:r.guard.preDelete=true;break;case 6:scenario.id=99;break;case 7:scenario.step=2;break;case 8:manager.active=nullptr;break;case 9:r.owner.world=false;break;case 10:r.owner.contact=false;break;case 11:r.owner.gm=true;break;case 12:r.owner.teleport=true;break;case 13:r.owner.map=&otherMap;break;case 14:r.owner.phase=false;break;case 15:r.owner.near=false;break;case 16:r.owner.classId=5;break;case 17:r.owner.rewarded=true;break;case 18:r.owner.record=false;break;case 19:r.owner.quest.Status=0;break;case 20:killer=nullptr;break;case 21:r.pet.owner={1,99};killer=&r.pet;break;}r.Kill(killer);r.script.Update(60000);assert(!r.Count()&&!r.owner.updates&&!r.neighbor.updates&&!r.script.guardPending);++cases;}
 {Run r;r.owner.alive=false;r.Kill(&r.pet);assert(r.Count()==1);}
 {Run r;r.neighbor.classId=7;r.neighbor.group=r.owner.group=4;r.Kill(&r.ally);assert(r.Count()==1&&r.owner.updates+r.neighbor.updates==1);}
 {Run r;r.neighbor.classId=7;r.Kill(&r.ally);assert(!r.Count()&&!r.owner.updates&&!r.neighbor.updates);}
 for(int deny=0;deny<5;++deny){Run r;switch(deny){case 0:r.ally.dbGuid=99;break;case 1:r.ally.entry=104853;break;case 2:r.ally.owner={1,99};break;case 3:r.ally.map=&otherMap;break;case 4:r.ally.phase=false;break;}r.Kill(&r.ally);assert(!r.Count());}
 {Run r;r.owner.classId=5;r.neighbor.classId=7;r.Kill(&r.pet);assert(!r.Count()&&!r.neighbor.updates);}
 {Run r;scenario.achievement.progress[47197].Counter=1;r.Kill(&r.owner);assert(r.Count()==1&&!r.owner.updates);}
 {Run r;scenario.step=0;r.Kill(&r.guardian);assert(r.script.guardPending&&!r.Count());r.script.Update(90000);assert(r.script.guardPending&&!r.owner.updates);scenario.step=1;r.script.Update(0);assert(r.Count()==1&&!r.script.guardPending);}
 {Run r;scenario.step=0;r.Kill(&r.pet);r.guard.world=false;r.guard.preDelete=true;scenario.step=1;r.script.Update(0);assert(r.Count()==1);}
 for(int change=0;change<4;++change){Run r;scenario.step=0;r.Kill(&r.owner);assert(r.script.guardPending);auto held=r.owner.quest.ScriptLifetime;switch(change){case 0:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:r.owner.record=false;break;case 2:r.owner.rewarded=true;break;case 3:r.owner.quest.Status=0;break;}scenario.step=1;r.script.Update(0);assert(!r.Count()&&!r.script.guardPending&&warnings==1);}
 {Run r;scenario.reject=true;r.Kill(&r.owner);assert(r.script.guardPending&&r.owner.updates==1);r.script.Update(999);assert(r.owner.updates==1);r.script.Update(1);assert(r.owner.updates==2);for(int i=0;i<28;++i)r.script.Update(1000);assert(r.script.guardExhausted&&r.owner.updates==30&&warnings==1);r.script.Update(60000);assert(r.owner.updates==30);scenario.reject=false;r.script.OnPlayerEnter(&r.owner);r.script.Update(0);assert(r.Count()==1&&r.owner.updates==31);}
 {Run r;scenario.step=0;r.Kill(&r.owner);auto lease=r.owner.quest.ScriptLifetime;liveMap.players={{&r.neighbor}};scenario.step=1;for(int i=0;i<30;++i)r.script.Update(1000);assert(r.script.guardExhausted&&!r.Count());liveMap.players.push_back({&r.owner});r.script.OnPlayerEnter(&r.neighbor);assert(r.script.guardExhausted);r.script.OnPlayerEnter(&r.owner);r.script.Update(0);assert(r.Count()==1);}
 {Run r;scenario.step=0;r.Kill(&r.owner);r.owner.quest.ScriptLifetime.reset();scenario.step=1;r.script.Update(0);assert(!r.Count()&&!r.script.guardPending&&warnings==1);}
 {Run r;scenario.reject=true;r.owner.onCredit=[&](){r.Kill(&r.owner);r.script.Update(60000);};r.Kill(&r.owner);assert(r.owner.updates==1&&r.script.guardPending);r.owner.onCredit={};scenario.reject=false;r.script.Update(1000);assert(r.Count()==1&&r.owner.updates==2);}
 {Run r;scenario.step=0;r.Kill(&r.owner);scenario.step=2;r.script.Update(0);assert(!r.Count()&&!r.script.guardPending&&warnings==1);}
 {Run r;scenario.step=0;auto original=std::make_unique<Player>();original->guid={1,9};liveMap.players={{original.get()}};r.Kill(original.get());assert(r.script.guardPending);original.reset();liveMap.players.clear();scenario.step=1;r.script.Update(0);assert(!r.script.guardPending&&!r.Count()&&warnings==1);}
 std::cout<<"PASS "<<cases<<" guard matrices plus actual native owner/quest/criteria gate/Kill bridge: player/pet/guardian/charm/both known allies, dead owner, native shared group once, foreign actors/players, stale or destroyed quest owner, real early victory, removed corpse, stage gates, bounded native refusal/re-entry and synchronous re-entry. Geometry, registry, group identity and criterion service remain controlled adapters; no client traversal claimed.\n";
#endif
}
