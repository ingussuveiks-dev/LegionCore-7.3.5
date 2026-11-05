#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
enum{CLASS_ROGUE=4,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,GAMEOBJECT_FIELD_FLAGS=17,GO_FLAG_NOT_SELECTABLE=16,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct ObjectGuid{uint64 low=0,high=0;uint32 GetGUIDLow()const{return uint32(low);}bool operator==(ObjectGuid b)const{return low==b.low&&high==b.high;}bool operator!=(ObjectGuid b)const{return !(*this==b);}};
#include "QuestScope.inc"
static uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Progress{uint64 Counter=0;};
struct Scenario{uint32 id=1078,step=2;bool refuse=false;std::map<uint32,Progress> progress{{48002,{1}},{48005,{1}}};Scenario& GetAchievementMgr(){return *this;}auto GetCriteriaProgressMap(){return &progress;}uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Map{};
struct WorldObject{Map* map=nullptr;ObjectGuid guid;bool world=true,preDelete=false,expired=false,scope=true;uint32 entry=249390,mapId=1607,phase=1,phaseId=1;float distance=1;
 bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}bool IsQuestObjectExpired()const{return expired;}Map* GetMap()const{return map;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return 42;}ObjectGuid GetGUID()const{return guid;}uint32 GetEntry()const{return entry;}
 bool InSamePhase(WorldObject const* p)const{return phase&p->phase;}bool InSamePhaseId(WorldObject const* p)const{return phaseId==p->phaseId;}bool CanTargetQuestObject(WorldObject const* p)const{return scope&&p->scope;}bool IsWithinDistInMap(WorldObject const* p,float r)const{return map==p->map&&p->distance<=r;}
};
struct Player:WorldObject{bool alive=true,contact=true,gm=false,pending=false,visible=true,los=true,rewarded=false,key=true;uint32 klass=4,status=3,requests=0,destroys=0,removals=0;std::shared_ptr<QuestObjectScope> questScope;std::function<void()> onCredit,onDestroy;
 bool IsAlive(){return alive;}bool CanContact(){return contact;}bool isGameMaster(){return gm;}uint32 getClass(){return klass;}bool HasPendingTeleport(){return pending;}bool canSeeOrDetect(WorldObject const*){return visible;}bool IsWithinLOSInMap(WorldObject const*){return los;}bool GetQuestRewardStatus(uint32 id){assert(id==41924);return rewarded;}uint32 GetQuestStatus(uint32 id){assert(id==41924);return status;}
 bool HasItemCount(uint32 id,uint32 count){assert(id==136802&&count==1);return key;}void DestroyItemCount(uint32 id,uint32 count,bool update){assert(id==136802&&count==1&&update&&key);key=false;++destroys;if(onDestroy)onDestroy();}void RemoveAura(uint32 id){assert(id==211684);++removals;}
 auto GetQuestCancellationReceipt(uint32 id){assert(id==41924);if(!status)return std::shared_ptr<QuestCancellationReceipt const>{};if(!questScope){questScope=std::make_shared<QuestObjectScope>();questScope->Owner=guid;}return questScope->CancellationReceipt();}void Abandon(){status=0;if(questScope)questScope->Cancel();questScope.reset();}
 void UpdateAchievementCriteria(uint32,uint32);
};
struct Motion{uint32 moves=0;void MoveRandom(float r){assert(r==15);++moves;}};
struct Creature{uint32 entry=105538,delay=0,talks=0;bool frightened=true;Motion motion;bool HasAura(uint32 id){assert(id==209349);return frightened;}auto GetMotionMaster(){return &motion;}void DespawnOrUnsummon(uint32 ms){delay=ms;}uint32 GetEntry(){return entry;}Creature* AI(){return this;}void Talk(uint32 id){assert(!id);++talks;}};
static std::vector<Creature*> nearby;
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Player*,uint32 entry,float range){assert(range==90);for(auto c:nearby)if(c->entry==entry)out.push_back(c);}
struct GameObject;
struct GameObjectAI{GameObject* go;explicit GameObjectAI(GameObject* p):go(p){}virtual~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual bool GossipUse(Player*){return false;}virtual uint32 GetData(uint32){return 0;}};
struct GameObject:WorldObject{bool spawned=true,selectable=true,instance=true;uint32 changes=0;GameObject* partner=nullptr;
 bool isSpawned(){return spawned;}bool HasFlag(uint32,uint32){return !selectable;}float GetInteractionDistance(){return 5;}void* GetInstanceScript(){return instance?this:nullptr;}GameObject* FindNearestGameObject(uint32 id,float r){assert(id==249391&&r==10);return partner&&partner->world&&partner->entry==id&&InSamePhase(partner)&&partner->distance<=r?partner:nullptr;}void SetPhaseMask(uint32 p,bool update){assert(p==4&&update);phase=p;++changes;}
};
struct GameObjectScript{explicit GameObjectScript(char const*){}virtual~GameObjectScript()=default;virtual GameObjectAI* GetAI(GameObject*)const{return nullptr;}};
struct Manager{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 id){assert(id==42);return present?&scenario:nullptr;}} manager;auto* sScenarioMgr=&manager;
void Player::UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92&&(event==50536||event==50908));++requests;auto& s=manager.scenario;if(s.step==2&&!s.refuse){s.progress[event==50536?48004:48002].Counter=1;if(s.progress[48002].Counter&&s.progress[48004].Counter&&s.progress[48005].Counter)s.step=3;}if(onCredit)onCredit();}
template<class T>struct HashMapHolder{inline static bool _checkLock=false;inline static uint32 _size=1024;inline static std::shared_mutex i_lockVector;inline static std::vector<T*> _objectVector=std::vector<T*>(1024,nullptr);
#include "PlayerFind.inc"
};
struct ObjectAccessor{
#include "GenericLookup.inc"
#include "PlayerLookup.inc"
static GameObject* GetGameObject(WorldObject const&,ObjectGuid);
};
#include "PlayerLookupMethod.inc"
#include "GameObjectLookupMethod.inc"
#include "DoorAI.inc"
struct Fixture{inline static uint32 count=0;Map map,other;Player player;GameObject go,barrier;Creature a,b,c;go_devourer_misc script;std::unique_ptr<GameObjectAI> ai;
 Fixture(){++count;warnings=0;manager={};player.map=go.map=barrier.map=&map;player.guid={1};go.guid={51};barrier.guid={52};barrier.entry=249391;go.partner=&barrier;ai.reset(script.GetAI(&go));a.entry=105538;b.entry=105537;c.entry=105538;c.frightened=false;nearby={&a,&b,&c};std::fill(HashMapHolder<Player>::_objectVector.begin(),HashMapHolder<Player>::_objectVector.end(),nullptr);std::fill(HashMapHolder<GameObject>::_objectVector.begin(),HashMapHolder<GameObject>::_objectVector.end(),nullptr);HashMapHolder<Player>::_objectVector[1]=&player;HashMapHolder<GameObject>::_objectVector[51]=&go;HashMapHolder<GameObject>::_objectVector[52]=&barrier;}
 bool Use(){return ai->GossipUse(&player);}void Untouched(){assert(player.key&&!player.destroys&&!go.changes&&!barrier.changes&&!a.delay&&!b.delay&&!c.delay);}void Complete(bool consume=true){assert(manager.scenario.step==3&&manager.scenario.progress[48004].Counter==1&&go.phase==4&&barrier.phase==4&&player.key==!consume&&player.destroys==uint32(consume)&&a.delay==5000&&b.delay==5000&&a.motion.moves==1&&b.motion.moves==1&&a.talks==1&&!b.talks&&!c.delay);}
};
int main(){
#ifdef DOOR_BASELINE
 for(uint32 mode=0;mode<4;++mode){Fixture f;if(mode==0)manager.scenario.progress.clear();if(mode==1)manager.scenario.step=1;if(mode==2)f.player.klass=1;if(mode==3)manager.scenario.refuse=true;f.ai->GossipHello(&f.player,true);assert(!f.player.key&&f.player.destroys==1&&f.go.phase==4&&f.barrier.phase==1);}
 std::cout<<"COUNTERFACTUAL4 original door: premature/class/refused50536 consumes key and changes control phase before phase-filtered actor lookup. Native lookup/receipts extracted; cast/criteria world modeled\n";
#else
 for(uint32 mode=0;mode<30;++mode){Fixture f;if(mode==0)f.player.world=false;if(mode==1)f.player.preDelete=true;if(mode==2)f.player.alive=false;if(mode==3)f.player.contact=false;if(mode==4)f.player.gm=true;if(mode==5)f.player.klass=1;if(mode==6)f.player.pending=true;if(mode==7)f.player.map=&f.other;if(mode==8)f.player.phase=2;if(mode==9)f.player.phaseId=2;if(mode==10)f.player.visible=false;if(mode==11)f.player.los=false;if(mode==12)f.go.distance=6;if(mode==13)f.player.expired=true;if(mode==14)f.player.scope=false;if(mode==15)f.player.rewarded=true;if(mode==16)f.player.status=0;if(mode==17)f.go.world=false;if(mode==18)f.go.preDelete=true;if(mode==19)f.go.spawned=false;if(mode==20)f.go.expired=true;if(mode==21)f.go.mapId=1;if(mode==22)f.go.instance=false;if(mode==23)f.go.selectable=false;if(mode==24)manager.present=false;if(mode==25)manager.scenario.id=1;if(mode==26)manager.scenario.step=1;if(mode==27)manager.scenario.progress.erase(48002);if(mode==28)manager.scenario.progress.erase(48005);if(mode==29)manager.scenario.step=3;assert(f.Use());f.Untouched();assert(!f.player.requests);}
 {Fixture f;assert(f.ai->GossipHello(&f.player,false));f.Untouched();assert(!f.player.requests);}
 for(uint32 mode=0;mode<2;++mode){Fixture f;if(mode)f.player.status=1;assert(f.Use());f.Complete();f.ai->GossipHello(&f.player,true);f.Use();assert(f.player.requests==1&&f.player.destroys==1);}
 {Fixture f;manager.scenario.refuse=true;f.Use();f.Use();f.Untouched();assert(warnings==1&&f.player.requests==2);manager.scenario.refuse=false;f.Use();f.Complete();}
 for(uint32 mode=0;mode<5;++mode){Fixture f;if(mode==0)f.go.partner=nullptr;if(mode==1)f.barrier.world=false;if(mode==2)f.barrier.preDelete=true;if(mode==3)f.barrier.expired=true;if(mode==4)f.barrier.scope=false;f.Use();f.Use();f.Untouched();assert(warnings==1&&!f.player.requests);f.go.partner=&f.barrier;f.barrier.world=true;f.barrier.preDelete=f.barrier.expired=false;f.barrier.scope=true;f.Use();f.Complete();}
 for(uint32 mode=0;mode<9;++mode){Fixture f;f.player.onCredit=[&]{if(mode==0){f.player.Abandon();f.player.status=3;}if(mode==1)f.player.questScope=std::make_shared<QuestObjectScope>();if(mode==2)f.player.guid.high=1;if(mode==3)HashMapHolder<Player>::_objectVector[1]=nullptr;if(mode==4)f.go.world=false;if(mode==5)f.barrier.guid.high=1;if(mode==6)HashMapHolder<GameObject>::_objectVector[52]=nullptr;if(mode==7)f.player.map=&f.other;if(mode==8)f.player.expired=true;};f.Use();f.Untouched();assert(f.player.requests==1);}
 {Fixture f;f.player.onCredit=[&]{f.Use();f.ai->GossipHello(&f.player,true);};f.player.onDestroy=[&]{f.Use();};f.Use();f.Complete();assert(f.player.requests==1);}
 {Fixture f;f.player.key=false;manager.scenario.step=3;manager.scenario.progress[48004].Counter=1;f.Use();assert(f.go.phase==4&&f.barrier.phase==4&&!f.player.destroys&&!f.player.requests);}
 {Fixture f;assert(f.ai->GossipUse(nullptr));f.Untouched();assert(!f.player.requests);}
 for(uint32 entry:{249809u,248992u}){Fixture f;f.go.entry=entry;assert(!f.Use());f.Untouched();assert(f.ai->GossipHello(&f.player,true)&&f.go.phase==4&&f.barrier.phase==1&&f.player.key&&!f.player.destroys&&f.player.requests==uint32(entry==249809)&&f.player.removals==uint32(entry==249809));}
 std::cout<<"PASS "<<Fixture::count<<" ASAN full door class/native lookup/receipt contexts: admission, real prior leafs/ack before key/phase, captured barrier before phase change, missing actor/refused credit retry, cancellation/fullGUID/reentry and no-key earned-checkpoint cleanup. Cast/criteria/geometry services modeled; native/client separate\n";
#endif
}
