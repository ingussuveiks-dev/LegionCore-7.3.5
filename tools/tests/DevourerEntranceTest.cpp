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
enum{CLASS_ROGUE=4,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_STATE_CASTING=1,UNIT_STATE_STUNNED=2};
#include "SceneStatus.inc"
using SpellCastResult=uint32;enum{SPELL_CAST_OK=0,SPELL_FAILED_ERROR=1};
struct ObjectGuid{uint64 high=1,low=0;bool IsEmpty()const{return !low;}void Clear(){low=0;}bool operator==(ObjectGuid const&o)const{return high==o.high&&low==o.low;}};
#include "QuestScope.inc"
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
static uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Progress{uint64 Counter=0;};
struct Scenario{uint32 id=1078,step=4;std::map<uint32,Progress> progress;Scenario& GetAchievementMgr(){return *this;}auto GetCriteriaProgressMap(){return &progress;}uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}bool CanUpdateCriteria(uint32 tree)const{return step==4&&(tree==53045||tree==47367);}};
struct Player;struct Map{Scenario scenario;std::list<Player*> players;};
struct WorldObject{
 Map* map=nullptr;ObjectGuid guid;bool world=true,preDelete=false,alive=true,expired=false,scope=true;uint32 mapId=1607,phase=1;float x=0,z=92;
 bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return 1;}auto GetMap()const{return map;}auto GetGUID()const{return guid;}float GetPositionZ()const{return z;}
 bool InSamePhase(WorldObject const*o)const{return phase==o->phase;}bool CanTargetQuestObject(WorldObject const*o)const{return scope&&o->scope;}float GetDistance(WorldObject const*o)const{return std::sqrt((x-o->x)*(x-o->x)+(z-o->z)*(z-o->z));}bool IsWithinDistInMap(WorldObject const*o,float range)const{return map==o->map&&GetDistance(o)<=range;}
};
struct Unit:WorldObject{Player* owner=nullptr;Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return owner;}};
struct Player:Unit{
 bool gm=false,contact=true,pending=false,rewarded=false,sceneUnavailable=false;uint32 klass=4,status=3,scene=0,casts=0,launchState=SCENE_LAUNCH;SpellCastResult castResult=0;std::shared_ptr<QuestObjectScope> scopeState;std::vector<uint32> attempts;uint32 blocked=0;std::function<void()> onCast,onCredit;
 Player(){owner=this;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return pending;}uint32 getClass()const{return klass;}bool GetQuestRewardStatus(uint32 id)const{assert(id==41924);return rewarded;}uint32 GetQuestStatus(uint32 id)const{assert(id==41924);return status;}
 auto GetQuestCancellationReceipt(uint32 id){assert(id==41924);if(!scopeState){scopeState=std::make_shared<QuestObjectScope>();scopeState->Owner=guid;}return scopeState->CancellationReceipt();}
 void Abandon(){status=0;if(scopeState)scopeState->Cancel();scopeState.reset();}
 SpellCastResult CastSpell(Player* target,uint32 id){assert(target==this&&id==209626);++casts;if(castResult==0&&!sceneUnavailable)scene=launchState;if(onCast)onCast();return castResult;}
 bool HasSceneStatus(uint32 id,uint32 state)const{assert(id==1254);return scene>=state;}
 void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92);attempts.push_back(event);if(event!=blocked&&map->scenario.step==4){if(event==50404)map->scenario.progress[53045].Counter=1;if(event==50299)map->scenario.progress[47367].Counter=1;if(map->scenario.progress[53045].Counter&&map->scenario.progress[47367].Counter)map->scenario.step=5;}if(onCredit)onCredit();}
};
struct InstanceScript{};
struct Creature:Unit{uint32 entry=105660,melees=0;bool instanceAvailable=true,combat=false;InstanceScript instance;std::vector<uint32> combatCasts;uint32 GetEntry()const{return entry;}InstanceScript* GetInstanceScript(){return instanceAvailable?&instance:nullptr;}void GetPlayerListInGrid(std::list<Player*>& out,float range){assert(range==25);if(map)out=map->players;}bool HasUnitState(uint32)const{return false;}};
struct Registry{Map* map=nullptr;Scenario* GetScenario(uint32){return map?&map->scenario:nullptr;}} registry;
#define sScenarioMgr (&registry)
namespace ObjectAccessor{static std::map<uint64,Player*> players;Player* GetPlayer(WorldObject const&,ObjectGuid guid){auto at=players.find(guid.low);return at==players.end()?nullptr:at->second;}}
enum class HighGuid{Conversation};struct Generator{uint32 Generate(){return 1;}};struct Manager{Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}} manager;
#define sObjectMgr (&manager)
struct Conversation{bool CreateConversation(uint32,uint32,Unit*,void*,Unit&){return false;}};
struct ScriptedAI{Creature*me;explicit ScriptedAI(Creature*p):me(p){}virtual~ScriptedAI()=default;virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void JustDied(Unit*){}virtual void UpdateAI(uint32){}void SetCanSeeEvenInPassiveMode(bool){}bool UpdateVictim(){return me->combat;}void DoCast(uint32 id){me->combatCasts.push_back(id);}void DoMeleeAttackIfReady(){++me->melees;}};
#include "EntranceAI.inc"
using Entrance=npc_akaari_shadowgore_lastAI;
struct Fixture{
 inline static uint32 cases=0;Map map,other;Player rogue,visitor;Creature source;Entrance ai;
 Fixture():ai(&source){++cases;warnings=0;registry.map=&map;source.map=rogue.map=visitor.map=&map;rogue.guid.low=101;visitor.guid.low=102;rogue.x=10;visitor.x=2;visitor.gm=true;map.players={&visitor,&rogue};ObjectAccessor::players={{101,&rogue},{102,&visitor}};}
 ~Fixture(){ObjectAccessor::players.clear();registry.map=nullptr;}void Tick(uint32 t=1000){ai.UpdateAI(t);}void Complete(){assert(ai.introDone&&ai.entryVisual&&map.scenario.step==5);assert(rogue.attempts==std::vector<uint32>({50404,50299}));}
};
int main(){
 for(uint32 mode=0;mode<26;++mode){Fixture f;switch(mode){case 0:f.rogue.gm=true;break;case 1:f.rogue.klass=1;break;case 2:f.rogue.status=0;break;case 3:f.rogue.rewarded=true;f.rogue.status=QUEST_STATUS_NONE;break;case 4:f.rogue.world=false;break;case 5:f.rogue.preDelete=true;break;case 6:f.rogue.alive=false;break;case 7:f.rogue.contact=false;break;case 8:f.rogue.pending=true;break;case 9:f.rogue.map=&f.other;break;case 10:f.rogue.phase=2;break;case 11:f.rogue.z=90;break;case 12:f.rogue.x=30;break;case 13:f.rogue.expired=true;break;case 14:f.rogue.scope=false;break;case 15:f.source.world=false;break;case 16:f.source.preDelete=true;break;case 17:f.source.alive=false;break;case 18:f.source.entry=105542;break;case 19:f.source.mapId=1;break;case 20:f.source.instanceAvailable=false;break;case 21:f.source.expired=true;break;case 22:f.source.scope=false;break;case 23:f.map.scenario.id=1;break;case 24:f.map.scenario.step=3;break;case 25:f.map.scenario.step=6;break;}f.Tick();assert(!f.ai.introDone&&!f.rogue.casts&&f.rogue.attempts.empty());}
 for(uint32 status:{3u,1u}){Fixture f;f.rogue.status=status;f.Tick(999);assert(!f.rogue.casts);f.Tick(1);f.Complete();f.ai.Reset();f.Tick();assert(f.rogue.casts==1&&f.rogue.attempts.size()==2);}
 for(uint32 blocked:{50404u,50299u}){Fixture f;f.rogue.blocked=blocked;f.Tick();assert(!f.ai.introDone&&f.ai.entryVisual&&warnings==1);f.rogue.blocked=0;f.Tick(999);assert(f.rogue.attempts.size()==2);f.Tick(1);assert(f.ai.introDone&&f.ai.entryVisual&&f.rogue.attempts.size()==3&&f.rogue.attempts.back()==blocked&&f.rogue.casts==1&&warnings==1);}
 {Fixture f;f.rogue.castResult=1;f.rogue.scene=SCENE_COMPLETE;f.Tick();assert(f.ai.introDone&&!f.ai.entryVisual&&warnings==1);f.ai.Reset();f.rogue.castResult=0;f.Tick(999);assert(f.rogue.casts==1);f.Tick(1);f.Complete();assert(f.rogue.casts==2&&warnings==1);}
 {Fixture f;f.rogue.sceneUnavailable=true;f.Tick();assert(f.ai.introDone&&!f.ai.entryVisual&&warnings==1);f.rogue.sceneUnavailable=false;f.Tick();f.Complete();}
 {Fixture f;f.rogue.castResult=1;f.Tick();auto receipt=f.ai.entryReceipt;f.rogue.Abandon();f.rogue.status=3;f.rogue.castResult=0;f.Tick();assert(receipt->load()&&!f.ai.entryReceipt&&f.ai.entryPlayer.IsEmpty()&&f.rogue.casts==1);}
 {Fixture f;f.rogue.castResult=1;f.Tick();f.ai.entryPlayer.high=2;f.rogue.castResult=0;f.Tick();assert(f.ai.entryPlayer.IsEmpty()&&f.rogue.casts==1);}
 {Fixture f;f.rogue.onCredit=[&]{f.ai.UpdateEntry(1000);};f.Tick();f.Complete();assert(f.ai.checktimer==1000&&f.rogue.casts==1);}
 {Fixture f;f.rogue.onCast=[&]{f.rogue.Abandon();};f.Tick();assert(!f.ai.introDone&&f.rogue.attempts.empty());f.visitor.gm=false;f.Tick();assert(f.ai.introDone&&f.visitor.casts==1&&f.visitor.attempts.size()==2);}
 {Fixture f;f.rogue.z=f.visitor.z=90;f.source.combat=true;f.ai.events.ScheduleEvent(EVENT_2,1);f.Tick();assert(f.ai.events.GetTimer()==1000&&f.source.combatCasts==std::vector<uint32>({224990})&&f.source.melees==1&&!f.ai.introDone);}
 {Fixture f;f.map.scenario.step=5;f.map.scenario.progress[53045].Counter=f.map.scenario.progress[47367].Counter=1;f.Tick();assert(!f.rogue.casts&&f.rogue.attempts.empty());}
 {Fixture f;f.rogue.launchState=SCENE_TRIGER;f.Tick();f.Complete();}
 std::cout<<"PASS "<<Fixture::cases<<" full final-Akaari AI/native EventMap/quest-receipt ASAN contexts; modeled scene/criteria services, native tests separate\n";
}
