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
struct ObjectGuid{uint64 high=1,low=0;bool IsEmpty()const{return !low;}void Clear(){low=0;}bool operator==(ObjectGuid const&o)const{return high==o.high&&low==o.low;}bool operator!=(ObjectGuid const&o)const{return !(*this==o);}};
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
 bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return 1;}auto GetMap()const{return map;}auto GetGUID()const{return guid;}float GetPositionZ()const{return z;}float GetVisibilityRange()const{return 100;}bool InSamePhaseId(WorldObject const*o)const{return phase==o->phase;}
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
struct Creature:Unit{uint32 entry=105660,melees=0;bool instanceAvailable=true,combat=false;InstanceScript instance;std::vector<uint32> combatCasts;bool isInCombat()const{return combat;}uint32 GetEntry()const{return entry;}InstanceScript* GetInstanceScript(){return instanceAvailable?&instance:nullptr;}void GetPlayerListInGrid(std::list<Player*>& out,float range){assert(range==25);if(map)out=map->players;}bool HasUnitState(uint32)const{return false;}};
struct Registry{Map* map=nullptr;Scenario* GetScenario(uint32){return map?&map->scenario:nullptr;}} registry;
#define sScenarioMgr (&registry)
namespace ObjectAccessor{static std::map<uint64,Player*> players;Player* GetPlayer(WorldObject const&,ObjectGuid guid){auto at=players.find(guid.low);return at==players.end()?nullptr:at->second;}}
enum class HighGuid{Conversation};struct Generator{uint32 Generate(){return 1;}};struct Manager{Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}} manager;
#define sObjectMgr (&manager)
struct Conversation{
 inline static std::vector<Conversation*> owned;inline static bool refuse=false;inline static std::function<void()> hook;inline static uint32 attempts=0;
 uint32 id=0;ObjectGuid caster;bool removed=false;uint32 duration=0;
 bool CreateConversation(uint32,uint32 value,Unit* target,void*,Unit& anchor){assert(target==&anchor&&value>=1942&&value<=1947);++attempts;if(auto f=hook)f();if(refuse)return false;id=value;caster=target->GetGUID();duration=std::vector<uint32>{7800,9400,8200,9500,6700,7350}.at(id-1942);owned.push_back(this);return true;}
 uint32 GetDuration()const{return duration;}void Remove(){removed=true;}
 static void Clear(){for(auto p:owned)delete p;owned.clear();refuse=false;hook={};attempts=0;}
 static std::vector<uint32> Ids(){std::vector<uint32> ids;for(auto p:owned)if(!p->removed)ids.push_back(p->id);return ids;}
};
struct ScriptedAI{Creature*me;explicit ScriptedAI(Creature*p):me(p){}virtual~ScriptedAI()=default;virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void JustDied(Unit*){}virtual void UpdateAI(uint32){}void SetCanSeeEvenInPassiveMode(bool){}bool UpdateVictim(){return me->combat;}void DoCast(uint32 id){me->combatCasts.push_back(id);}void DoMeleeAttackIfReady(){++me->melees;}};
#include "EntranceAI.inc"
using Entrance=npc_akaari_shadowgore_lastAI;
struct Fixture{
 inline static uint32 cases=0;Map map,other;Player rogue,visitor;Creature source;Entrance ai;
 Fixture():ai(&source){++cases;warnings=0;registry.map=&map;source.map=rogue.map=visitor.map=&map;rogue.guid.low=101;visitor.guid.low=102;rogue.x=10;visitor.x=2;visitor.gm=true;map.players={&visitor,&rogue};ObjectAccessor::players={{101,&rogue},{102,&visitor}};}
 ~Fixture(){Conversation::Clear();ObjectAccessor::players.clear();registry.map=nullptr;}void Tick(uint32 t=1000){ai.UpdateAI(t);}void Complete(){assert(ai.introDone&&ai.entryVisual&&map.scenario.step==5);assert(rogue.attempts==std::vector<uint32>({50404,50299}));}
};
int main(){uint32 cases=0;
 for(uint32 mode=0;mode<5;++mode){Fixture f;f.map.scenario.step=5;f.source.combat=true;Unit pet;pet.owner=&f.rogue;pet.map=&f.map;Unit wild;wild.map=&f.map;Unit* caller=mode==0?static_cast<Unit*>(&f.rogue):mode<3?&pet:mode==3?&wild:nullptr;f.ai.EnterCombat(caller);assert(Conversation::Ids()==std::vector<uint32>{1942}&&Conversation::owned[0]->caster==(mode<3?f.rogue.guid:f.source.guid)&&f.ai.convers==1943&&f.ai.combatVoiceDelay==7800);for(uint32 i=0;i<100000;++i)f.Tick(1);assert(Conversation::Ids()==std::vector<uint32>({1942,1943,1944,1945,1946})&&f.source.combatCasts.size()==18&&f.rogue.attempts.empty());++cases;}
 {Fixture f;f.source.combat=true;f.map.scenario.step=5;Conversation::refuse=true;f.ai.EnterCombat(&f.rogue);assert(f.ai.convers==1942&&warnings==1&&Conversation::attempts==1);f.ai.UpdateCombatVoice(999);assert(Conversation::attempts==1);Conversation::refuse=false;f.ai.UpdateCombatVoice(1);assert(f.ai.convers==1943&&Conversation::Ids()==std::vector<uint32>{1942});f.ai.UpdateCombatVoice(10000);assert(Conversation::attempts==2);++cases;}
 for(uint32 mode=0;mode<9;++mode){Fixture f;f.source.combat=true;f.map.scenario.step=5;f.rogue.x=200;f.ai.EnterCombat(&f.rogue);assert(!Conversation::attempts&&warnings==1&&f.ai.convers==1942);f.rogue.x=10;
  if(mode==0)f.ai.combatOpener.high=2;if(mode==1)ObjectAccessor::players.erase(101);if(mode==2)f.rogue.map=&f.other;if(mode==3)f.rogue.phase=2;if(mode==4)f.rogue.scope=false;if(mode==5)f.rogue.contact=false;if(mode==6)f.rogue.world=false;if(mode==7)f.rogue.preDelete=true;if(mode==8)f.rogue.expired=true;
  f.ai.UpdateCombatVoice(1000);assert(!Conversation::attempts&&f.ai.convers==1942);f.rogue.map=&f.map;f.rogue.phase=1;f.rogue.scope=f.rogue.contact=f.rogue.world=true;f.rogue.preDelete=f.rogue.expired=false;ObjectAccessor::players[101]=&f.rogue;f.ai.combatOpener=f.rogue.guid;f.ai.UpdateCombatVoice(1000);assert(Conversation::attempts==1&&Conversation::owned[0]->caster==f.rogue.guid);++cases;
 }
 {Fixture f;f.source.combat=true;f.map.scenario.step=5;Conversation::refuse=true;f.ai.EnterCombat(&f.rogue);auto receipt=f.ai.combatOpeningReceipt;f.rogue.Abandon();f.rogue.status=3;Conversation::refuse=false;f.ai.UpdateCombatVoice(1000);assert(receipt->load()&&Conversation::Ids().empty()&&f.ai.convers==1943&&!f.ai.combatVoicePending);f.ai.Reset();f.ai.EnterCombat(&f.rogue);assert(Conversation::Ids()==std::vector<uint32>{1942});++cases;}
 {Fixture f;f.source.combat=true;f.map.scenario.step=5;f.rogue.x=200;f.ai.EnterCombat(&f.rogue);f.rogue.x=10;f.rogue.scopeState=std::make_shared<QuestObjectScope>();f.ai.UpdateCombatVoice(1000);assert(Conversation::Ids().empty()&&f.ai.convers==1943&&!f.ai.combatVoicePending);++cases;}
 for(uint32 mode=0;mode<3;++mode){Fixture f;f.source.combat=true;f.map.scenario.step=5;Conversation::hook=[&]{if(mode==0)f.rogue.Abandon();else if(mode==1)f.ai.Reset();else{f.ai.UpdateCombatVoice(1000);f.ai.RequestCombatVoice();}};f.ai.EnterCombat(&f.rogue);assert(Conversation::attempts==1);assert(Conversation::Ids()==(mode==2?std::vector<uint32>{1942}:std::vector<uint32>{}));assert(mode==2?f.ai.convers==1943:!f.ai.combatVoicePending);++cases;}
 {Fixture f;f.source.combat=true;f.map.scenario.step=5;f.rogue.status=0;f.ai.EnterCombat(&f.rogue);assert(Conversation::Ids()==std::vector<uint32>{1942}&&!f.ai.combatOpeningReceipt&&f.rogue.attempts.empty());++cases;}
 {Fixture f;f.source.combat=true;f.map.scenario.step=5;Conversation::refuse=true;f.ai.EnterCombat(&f.rogue);for(uint32 i=0;i<100000;++i){if(i==20000)Conversation::refuse=false;f.Tick(1);}assert(Conversation::Ids()==std::vector<uint32>({1942,1943,1944,1945,1946})&&f.source.combatCasts.size()==18&&warnings==1&&f.rogue.attempts.empty());++cases;}
 std::cout<<"PASS "<<cases<<" ASAN whole final-AI opening flow/native EventMap/quest receipt contexts: actual owner binding/fullGUID, null/NPC fallback, missing original recipient/same-owner recovery, failed factory/1s retry/no replay, explicit cancel/reaccept/current receipt, reentry/reset during admission, five-line sequence and18unchanged ability requests; subtype/native interfaces/client separate\n";
}
