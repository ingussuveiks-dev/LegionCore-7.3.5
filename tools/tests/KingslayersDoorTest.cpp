#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>
#include <iostream>
using uint32=uint32_t;using uint64=uint64_t;
enum{GAMEOBJECT_TYPE_DOOR=0,CLASS_ROGUE=4,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,CRITERIA_TYPE_SCRIPT_EVENT_2=92,GAMEOBJECT_FIELD_FLAGS=1,GO_FLAG_IN_USE=1,GO_FLAG_LOCKED=2,GO_FLAG_NOT_SELECTABLE=16,GO_NOT_READY=0,GO_READY=1,GO_ACTIVATED=2,GO_JUST_DEACTIVATED=3,GO_STATE_ACTIVE=0,GO_STATE_READY=1};
struct Player;struct GameObject;
struct Progress{uint64 Counter=0;};
struct Scenario{
 uint32 id=1123,tree=48584;std::map<uint32,Progress> progress;
 auto& GetAchievementMgr(){return *this;}auto GetCriteriaProgressMap()const{return &progress;}
 uint32 GetScenarioId()const{return id;}
 bool CanUpdateCriteria(uint32 leaf)const{return tree==48576?leaf==48577:tree==48584?(leaf==48597||leaf==48585):false;}
};
struct Map{uint32 id=1620,instance=1,scripts=0;void ScriptsStart(int,uint64,Player*,GameObject*){++scripts;}};
struct ScenarioMgr{Scenario* value=nullptr;Scenario* GetScenario(uint32){return value;}} manager;auto sScenarioMgr=&manager;int sGameObjectScripts=0;
struct Unit{virtual ~Unit()=default;};
struct World{
 Map* map=nullptr;bool world=true,expired=false,scope=true;
 bool IsInWorld()const{return world;}bool IsQuestObjectExpired()const{return expired;}bool CanTargetQuestObject(World const*)const{return scope;}
 Map* GetMap()const{return map;}uint32 GetMapId()const{return map?map->id:0;}uint32 GetInstanceId()const{return map?map->instance:0;}
};
struct Motion{uint32 moves=0;void MovePoint(uint32 point,float x,float y,float z){assert(point==0&&x==-8625.50f&&y==415.93f&&z==103.7f);++moves;}};
struct Creature:World{Motion motion;uint32 removals=0;auto GetMotionMaster(){return &motion;}void RemoveAura(uint32 id){assert(id==132653);++removals;}};
struct Player:World{
 bool preDelete=false,contact=true,alive=true,gm=false,teleport=false,phase=true,visible=true,near=true,failCredit=false;
 uint32 klass=4;std::map<uint32,uint32> quests{{42504,3}};std::map<uint32,bool> rewarded;std::vector<uint32> events;std::function<void(uint32)> onCredit;Creature* garona=nullptr;
 bool IsPreDelete()const{return preDelete;}bool CanContact()const{return contact;}bool IsAlive()const{return alive;}bool isGameMaster()const{return gm;}uint32 getClass()const{return klass;}bool HasPendingTeleport()const{return teleport;}
 bool InSamePhase(World*)const{return phase;}bool canSeeOrDetect(World*)const{return visible;}bool IsWithinDistInMap(World*,float distance)const{assert(distance==5);return near;}
 bool GetQuestRewardStatus(uint32 id){return rewarded[id];}uint32 GetQuestStatus(uint32 id){return quests[id];}
 Creature* FindNearestCreature(uint32 entry,float range,bool living){assert(entry==108214&&range==60&&living);return garona;}
 void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92);events.push_back(event);auto s=manager.value;if(s&&!failCredit){uint32 leaf=event==51549?48577:48597;if(s->CanUpdateCriteria(leaf)){++s->progress[leaf].Counter;if(leaf==48577)s->tree=48584;else if(s->progress[48585].Counter)s->tree=48586;}}if(onCredit)onCredit(event);}
};
struct GameObject:World{
 uint32 entry=251053,kind=0,flags=0,loot=GO_READY,pose=GO_STATE_READY,activations=0;uint64 db=133953;bool spawned=true,failActivation=false;std::function<void()> onActivate;
 uint32 GetEntry()const{return entry;}uint64 GetDBTableGUIDLow()const{return db;}uint32 GetGoType()const{return kind;}bool isSpawned()const{return spawned;}
 uint32 getLootState()const{return loot;}uint32 GetGoState()const{return pose;}bool HasFlag(uint32 field,uint32 bits)const{assert(field==1);return(flags&bits)!=0;}float GetInteractionDistance()const{return 5;}
 void UseDoorOrButton(uint32 time,bool alternative,Player*){assert(!time&&!alternative);++activations;if(!failActivation&&loot==GO_READY){loot=GO_ACTIVATED;pose=GO_STATE_ACTIVE;flags|=GO_FLAG_IN_USE;if(onActivate)onActivate();}}
};
struct GameObjectAI{GameObject* go;explicit GameObjectAI(GameObject* source):go(source){}virtual ~GameObjectAI()=default;virtual bool GossipUse(Player*){return false;}virtual bool GossipHello(Player*,bool){return false;}};
#include "DoorAI.inc"
struct Run{
 Map map;Scenario scenario;GameObject door;Player player;Creature garona;go_kingslayers_doorAI ai{&door};
 Run(){door.map=player.map=garona.map=&map;player.garona=&garona;manager.value=&scenario;}
 uint64 Count(uint32 leaf){return scenario.progress[leaf].Counter;}
};
int main(){uint32 cases=0;
 for(uint32 quest:{42504u,42627u})for(uint32 stage:{48576u,48584u})for(bool hello:{false,true}){
  Run r;r.player.quests={{quest,QUEST_STATUS_INCOMPLETE}};r.scenario.tree=stage;
  if(hello)assert(r.ai.GossipHello(&r.player,true));else assert(r.ai.GossipUse(&r.player));
  assert(r.door.pose==GO_STATE_ACTIVE&&r.door.activations==1&&r.map.scripts==1&&r.Count(48597)==1&&r.scenario.tree==48584);
  assert(r.Count(48577)==(stage==48576?1u:0u));assert(r.garona.motion.moves==1&&r.garona.removals==1);
  auto events=r.player.events;r.ai.GossipUse(&r.player);r.ai.GossipHello(&r.player,false);r.ai.GossipHello(&r.player,true);
  assert(r.player.events==events&&r.door.activations==1&&r.map.scripts==1&&r.garona.motion.moves==1);++cases;
 }
 for(uint32 variant=0;variant<24;++variant)for(bool hello:{false,true}){
  Run r;bool delegate=false;
  switch(variant){case 0:r.door.db=1;delegate=true;break;case 1:r.map.id=1220;delegate=true;break;case 2:r.door.entry=1;delegate=true;break;case 3:r.door.kind=1;delegate=true;break;
  case 4:r.door.world=false;break;case 5:r.door.spawned=false;break;case 6:r.door.expired=true;break;case 7:r.door.flags=GO_FLAG_NOT_SELECTABLE;break;
  case 8:r.door.flags=GO_FLAG_LOCKED;break;case 9:r.player.world=false;break;case 10:r.player.preDelete=true;break;case 11:r.player.contact=false;break;
  case 12:r.player.alive=false;break;case 13:r.player.gm=true;break;case 14:r.player.klass=1;break;case 15:r.player.teleport=true;break;
  case 16:r.player.phase=false;break;case 17:r.player.visible=false;break;case 18:r.player.near=false;break;case 19:r.player.expired=true;break;
  case 20:r.player.scope=false;break;case 21:r.door.scope=false;break;case 22:r.player.quests.clear();break;case 23:r.player.rewarded[42504]=true;break;}
  bool result=hello?r.ai.GossipHello(&r.player,true):r.ai.GossipUse(&r.player);assert(result!=delegate&&r.door.activations==0&&r.player.events.empty()&&r.map.scripts==0);++cases;
 }
 {Run r;r.ai.GossipUse(nullptr);r.ai.GossipHello(nullptr,true);assert(!r.door.activations);++cases;}
 {Run r;manager.value=nullptr;r.ai.GossipUse(&r.player);assert(!r.door.activations);++cases;}
 {Run r;r.scenario.id=943;r.ai.GossipUse(&r.player);assert(!r.door.activations);++cases;}
 {Run r;r.scenario.tree=48586;r.ai.GossipUse(&r.player);assert(!r.door.activations);++cases;}
 {Run r;r.scenario.tree=48586;r.scenario.progress[48597].Counter=1;r.ai.GossipUse(&r.player);assert(r.door.activations==1&&r.player.events.empty());++cases;}
 {Run r;r.player.quests[42504]=QUEST_STATUS_COMPLETE;r.ai.GossipUse(&r.player);assert(r.Count(48597)==1);++cases;}
 {Run r;r.player.garona=nullptr;r.ai.GossipUse(&r.player);assert(r.Count(48597)==1);++cases;}
 {Run r;r.garona.scope=false;r.ai.GossipUse(&r.player);assert(r.Count(48597)==1&&!r.garona.motion.moves);++cases;}
 {Run r;r.door.failActivation=true;r.ai.GossipUse(&r.player);assert(r.door.activations==1&&r.player.events.empty()&&!r.ai.opening);r.door.failActivation=false;r.ai.GossipUse(&r.player);assert(r.Count(48597)==1&&r.map.scripts==1);++cases;}
 {Run r;r.door.onActivate=[&]{r.ai.GossipUse(&r.player);};r.player.onCredit=[&](uint32){r.ai.GossipHello(&r.player,true);};r.ai.GossipUse(&r.player);assert(r.door.activations==1&&r.Count(48597)==1&&r.map.scripts==1);++cases;}
 {Run r;r.scenario.tree=48576;r.player.onCredit=[&](uint32 id){if(id==51549)r.player.quests.clear();};r.ai.GossipUse(&r.player);assert(r.Count(48577)==1&&!r.Count(48597)&&!r.ai.opening);r.player.onCredit={};r.player.quests[42504]=3;r.ai.GossipUse(&r.player);assert(r.Count(48597)==1&&r.door.activations==1&&r.map.scripts==1);++cases;}
 {Run r;r.player.failCredit=true;r.ai.GossipUse(&r.player);assert(!r.Count(48597)&&!r.garona.motion.moves);r.player.failCredit=false;r.ai.GossipUse(&r.player);assert(r.Count(48597)==1&&r.door.activations==1);++cases;}
 {Run r;r.scenario.progress[48585].Counter=1;r.ai.GossipUse(&r.player);assert(r.Count(48597)==1&&r.scenario.tree==48586&&r.Count(48585)==1);++cases;}
 {Run r;for(uint32 i=0;i<4;++i)r.ai.GossipHello(&r.player,false);assert(!r.door.activations&&r.player.events.empty());++cases;}
 std::cout<<"PASS "<<cases<<" ASAN actual complete door AI contexts: stage4/5, both quests, actual activation acknowledgement boundary, report/no/null/wrong source and physical/quest/scope admission, missing Garona, failed activation/credit, retry/reentry, abandoned during transition, reentrant use, shared once-only leaves and independent Althea victory; native mesh/Spell tests separate\n";
}
