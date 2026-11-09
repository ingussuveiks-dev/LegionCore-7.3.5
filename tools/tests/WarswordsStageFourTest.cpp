// Complete production transition and participant gate, native FunctionProcessor.
// Player/map/quest/cast services are controlled; this is not client traversal.
#include <cassert>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
#include "Functions.inc"
enum {CLASS_WARRIOR=1,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3};
struct InstanceScript {virtual ~InstanceScript()=default;virtual void Initialize(){}virtual void onScenarionNextStep(uint32){};};
struct Player;
struct Creature {void UpdateObjectVisibility(){}};
struct Map {
 struct Ref {Player* player;Player* getSource()const{return player;}};
 struct PlayerList:std::list<Ref>{bool isEmpty()const{return empty();}} players;
 using PlayerListType=PlayerList;
 uint32 id=1511,instance=987;InstanceScript* script=nullptr;
 uint32 GetInstanceId()const{return instance;}
 PlayerList const& GetPlayers()const{return players;}
 Creature* GetCreature(uint64){return nullptr;}
};
struct Scenario {uint32 id=944,step=4;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}};
struct Manager {Scenario scenario;bool available=true;Scenario* GetScenario(uint32){return available?&scenario:nullptr;}}manager;
auto sScenarioMgr=&manager;
struct Player {
 Map* map=nullptr;bool world=true,contact=true,alive=true,teleport=false,rewarded=false;
 uint32 cls=1,status=QUEST_STATUS_INCOMPLETE,casts=0;
 std::shared_ptr<std::atomic<bool>> attempt=std::make_shared<std::atomic<bool>>(false);
 FunctionProcessor queue;
 bool IsInWorld()const{return world;}bool CanContact()const{return contact;}bool IsAlive()const{return alive;}
 bool IsBeingTeleported()const{return teleport;}uint32 getClass()const{return cls;}Map* GetMap()const{return map;}
 uint32 GetMapId()const{return map?map->id:0;}uint32 GetInstanceId()const{return map?map->instance:0;}
 InstanceScript* GetInstanceScript()const{return map?map->script:nullptr;}
 bool GetQuestRewardStatus(uint32 id)const{assert(id==40043);return rewarded;}
 uint32 GetQuestStatus(uint32 id)const{assert(id==40043);return status;}
 auto GetQuestCancellationReceipt(uint32 id){assert(id==40043);return attempt;}
 void AddDelayedEvent(uint32 delay,std::function<void()> callback){assert(delay==3000);queue.AddDelayedEvent(delay,std::move(callback));}
 void CastSpell(Player* target,uint32 id){assert(target==this&&id==189740);++casts;}
};
#include "Participant.inc"
struct Probe:InstanceScript {
 Map* instance;bool myscenario=true;std::map<uint32,uint64> objects;
 std::shared_ptr<uint8> sceneLifetime=std::make_shared<uint8>();
 struct Waves {void Reset(){}void Start(){}} waves;
 explicit Probe(Map* m):instance(m){m->script=this;}
 uint32 getScenarionStep(){return manager.scenario.step;}
#include "Initialize.inc"
#include "Step.inc"
};
struct Run {
 Map map,foreign;Player p;std::unique_ptr<Probe> script;
 Run(){manager=Manager{};p.map=&map;foreign.instance=988;map.players.push_back({&p});script=std::make_unique<Probe>(&map);}
 void Queue(){script->onScenarionNextStep(4);}
 void Due(){p.queue.Update(2999);assert(!p.casts);p.queue.Update(1);p.queue.Update(3000);}
};
int main(){unsigned cases=0;
#ifdef BASELINE
 for(unsigned mode=0;mode<3;++mode){Run r;r.Queue();if(mode==0)r.p.map=&r.foreign;if(mode==1){r.p.attempt->store(true);r.p.status=QUEST_STATUS_NONE;}if(mode==2)r.script.reset();r.Due();assert(r.p.casts==1);++cases;}
 std::cout<<"COUNTERFACTUAL pinned6a5b1f80b transition3: actual native queue still invokes189740 after foreign-map departure,quest cancellation or original instance destruction;cast endpoint controlled,not a successful gameplay test\n";
#else
 for(unsigned status:{unsigned(QUEST_STATUS_INCOMPLETE),unsigned(QUEST_STATUS_COMPLETE)}){Run r;r.p.status=status;r.Queue();r.Due();assert(r.p.casts==1);++cases;}
 for(unsigned mode=0;mode<16;++mode)for(bool before:{false,true}){
  Run r;if(!before)r.Queue();
  switch(mode){case 0:r.p.world=false;break;case 1:r.p.contact=false;break;case 2:r.p.alive=false;break;case 3:r.p.teleport=true;break;case 4:r.p.cls=3;break;case 5:r.p.map=nullptr;break;case 6:r.map.id=1220;break;case 7:r.p.map=&r.foreign;break;case 8:r.map.script=nullptr;break;case 9:r.p.rewarded=true;break;case 10:r.p.status=QUEST_STATUS_NONE;break;case 11:manager.available=false;break;case 12:manager.scenario.id=973;break;case 13:r.p.attempt->store(true);r.p.attempt=std::make_shared<std::atomic<bool>>(false);break;case 14:r.script->Initialize();r.script->myscenario=true;break;case 15:manager.scenario.step=5;break;}
  if(before)r.Queue();r.Due();
  // A new valid attempt/reset is allowed to enqueue fresh work; all old work is refused.
  bool fresh=before&&(mode==13||mode==14);
  assert(r.p.casts==unsigned(fresh));++cases;
 }
 {Run r;r.Queue();r.script.reset();r.Due();assert(!r.p.casts);++cases;}
 {Run r;r.Queue();r.p.attempt->store(true);r.p.attempt=std::make_shared<std::atomic<bool>>(false);r.Queue();r.Due();assert(r.p.casts==1);++cases;}
 {Run r;Player visitor;visitor.map=&r.map;visitor.cls=3;r.map.players.push_front({&visitor});r.Queue();r.Due();visitor.queue.Update(3000);assert(r.p.casts==1&&!visitor.casts);++cases;}
 {Run r;r.script->myscenario=false;r.Queue();r.Due();assert(!r.p.casts);++cases;}
 assert(cases==38);std::cout<<"PASS Warswords stage4 ASAN38: complete production transition/participant gate,actual FunctionProcessor2999+1ms,valid incomplete/complete quest,foreign map/instance/script/scenario,death/teleport/logout,abandon/retake,instance reset/destruction,valid visitor selection and retained bootstrap;no manual progress/item grants. Services controlled;client separate\n";
#endif
}
