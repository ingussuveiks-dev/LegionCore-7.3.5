// Production conditions/ElseGroups and one-shot SmartScript LOS dispatch.
// Spell/cast result, delayed work and conversation playback are endpoints.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;using int64=int64_t;
#include "Constants.inc"
struct Unit;struct Player;struct Creature;struct Condition;
struct InstanceMap {uint32 id=7;uint32 GetInstanceId(){return id;}};
struct Map:InstanceMap {InstanceMap* ToInstanceMap(){return this;}};
struct Scenario {uint32 id=1137,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} scenario;
struct ScenarioMgr {Scenario* GetScenario(uint32 id){return id==7?&scenario:nullptr;}} manager;auto sScenarioMgr=&manager;
struct WorldObject {
 bool player=false,creature=false,alive=true;uint32 mapId=1625,classMask=512;float distance=0;Map map;
 Player* ToPlayer();Unit* ToUnit();bool IsCreature(){return creature;}uint32 GetTypeId(){return player?4:3;}
 uint32 GetEntry(){return 0;}uint32 GetMapId(){return mapId;}bool InInstance(){return mapId==1625;}Map* GetMap(){return &map;}
};
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct Threat {Unit* target=nullptr;Unit* getHostilTarget(){return target;}};
struct Unit:WorldObject {Threat threat;Unit* owner=nullptr;bool pet=false;
 ObjectGuid GetGUID(){return {this};}uint32 getClassMask(){return classMask;}bool IsAlive(){return alive;}
 Threat& getThreatManager(){return threat;}bool isPet(){return pet;}Unit* GetOwner(){return owner;}
};
struct Player:Unit {QuestStatus status=QUEST_STATUS_INCOMPLETE;bool rewarded=false;
 Player(){player=true;}QuestStatus GetQuestStatus(uint32 id){assert(id==42765);return status;}bool GetQuestRewardStatus(uint32 id){assert(id==42765);return rewarded;}
};
struct Creature:Unit {bool combat=false,los=true,hostile=false,expired=false;
 Creature(){creature=true;}bool isInCombat(){return combat;}bool IsWithinDistInMap(Unit* u,float range){return u&&u->distance<=range;}
 bool IsWithinLOSInMap(Unit*){return los;}bool IsHostileTo(Unit*){return hostile;}bool IsQuestObjectExpired(){return expired;}
};
Player* WorldObject::ToPlayer(){return player?static_cast<Player*>(this):nullptr;}
Unit* WorldObject::ToUnit(){return static_cast<Unit*>(this);}
struct ConditionSourceInfo {WorldObject* mConditionTargets[2];Condition* mLastFailedCondition=nullptr;
 ConditionSourceInfo(WorldObject* a,WorldObject* b):mConditionTargets{a,b}{}
};
struct Condition {
 uint32 ElseGroup=0,ReferenceId=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0;
 bool NegativeCondition=false;bool isLoaded(){return true;}bool Meets(ConditionSourceInfo& sourceInfo){
  WorldObject* object=sourceInfo.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;
  switch(ConditionType){
#include "Predicates.inc"
  }
  if(NegativeCondition)condMeets=!condMeets;return condMeets;
 }
};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {
 ConditionList rows;ConditionReferenceContainer ConditionReferenceStore;bool baseline=false;
 ConditionList GetConditionsForSmartEvent(uint32,uint32 id,uint32){return id==0&&!baseline?rows:ConditionList{};}
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& list)const{return list.empty()||IsObjectMeetToConditionList(info,list);}
 bool IsObjectMeetingSmartEventConditions(uint32 entry,uint32 id,uint32 source,Unit* u,WorldObject* base){ConditionSourceInfo info(u,base);return IsObjectMeetToConditions(info,GetConditionsForSmartEvent(entry,id,source));}
} conditionManagerInstance;auto sConditionMgr=&conditionManagerInstance;
#define TC_LOG_DEBUG(...) ((void)0)
#include "ElseGroups.inc"
struct SpellInfo{};struct GameObject{};
struct SmartScriptHolder {
 uint32 entryOrGuid=109413,source_type=0,event_id=0,link=0,type=0,actionType=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;
  struct {uint32 noHostile=1,maxDist=16,cooldownMin=0,cooldownMax=0;} los;
 } event;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}
};
bool roll_chance_i(uint32){return true;}
struct SmartScript {
 Creature actor;Creature* me=&actor;std::vector<SmartScriptHolder> mEvents;ObjectGuid mLastInvoker;uint32 casts=0,lists=0,errors=0;
 Creature* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}bool IsInPhase(uint32){return true;}
 void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
#define TC_LOG_ERROR(...) ++errors
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LosEvent.inc"
#include "LinkEvent.inc"
 }
}
#include "ActionPrefix.inc"
 if(e.actionType==85)++casts;else if(e.actionType==80)++lists;
#include "LinkTail.inc"
static std::vector<std::string> Fields(std::string const& line){std::stringstream ss(line);std::string cell;std::vector<std::string> f;while(std::getline(ss,cell,'\t'))f.push_back(cell);return f;}
static std::vector<SmartScriptHolder> ReadEvents(char const* path){std::ifstream input(path);assert(input);std::string line;std::vector<SmartScriptHolder> rows;
 while(std::getline(input,line)){auto f=Fields(line);assert(f.size()==31);auto n=[&](size_t i){return uint32(std::stoul(f[i]));};if(n(1)!=0)continue;SmartScriptHolder e;
 e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
 e.event.los={n(9),n(10),n(11),n(12)};e.actionType=n(14);rows.push_back(e);
 }return rows;
}
int main(int argc,char** argv){
 if(argc!=3)return 1;auto events=ReadEvents(argv[1]);assert(events.size()==2);
 std::ifstream input(argv[2]);std::string line;std::vector<Condition> parsed;
 while(std::getline(input,line)){auto f=Fields(line);assert(f.size()==14);auto n=[&](size_t i){return uint32(std::stoul(f[i]));};parsed.push_back({n(4),0,n(5),n(6),n(7),n(8),n(9),n(10)!=0});}
 assert(parsed.size()==14);for(auto& c:parsed)conditionManagerInstance.rows.push_back(&c);
 Player player;Unit pet,guardian;pet.creature=true;pet.pet=true;pet.owner=&player;guardian.creature=true;guardian.owner=&player;Creature neighbor;
 unsigned passed=0;conditionManagerInstance.baseline=true;
 for(Unit* wrong:{&pet,&guardian,static_cast<Unit*>(&neighbor)}){
  SmartScript before;before.mEvents=events;before.ProcessEventsFor(SMART_EVENT_OOC_LOS,wrong);before.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);
  assert(before.casts==1&&before.lists==1&&before.mLastInvoker.unit==wrong&&before.mEvents[0].runOnce);++passed;
 }
 std::printf("EXPECTED_BROKEN %u NPC/pet/guardian consumed first LOS before player\n",passed);conditionManagerInstance.baseline=false;passed=0;
 for(Unit* wrong:{&pet,&guardian,static_cast<Unit*>(&neighbor),static_cast<Unit*>(nullptr)}){
  SmartScript after;after.mEvents=events;after.ProcessEventsFor(SMART_EVENT_OOC_LOS,wrong);assert(!after.casts&&!after.lists&&!after.mEvents[0].runOnce);
  after.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);after.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);assert(after.casts==1&&after.lists==1&&after.mLastInvoker.unit==&player&&!after.errors);++passed;
 }
 for(uint32 step=0;step<9;++step)for(auto status:{QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED})for(bool alive:{false,true})for(uint32 mask:{1u,2u,4u,8u,16u,32u,64u,128u,256u,512u,1024u,2048u}){
  player=Player{};player.status=status;player.alive=alive;player.classMask=mask;scenario.step=step;SmartScript test;test.mEvents=events;
  test.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);bool allowed=step==0&&alive&&mask==512&&(status==QUEST_STATUS_INCOMPLETE||status==QUEST_STATUS_COMPLETE);
  assert(test.casts==unsigned(allowed)&&test.lists==unsigned(allowed)&&test.mEvents[0].runOnce==allowed);++passed;
 }
 for(unsigned mode=0;mode<7;++mode){player=Player{};scenario=Scenario{};SmartScript test;test.mEvents=events;
  if(mode==0)player.mapId=1;if(mode==1)scenario.id=1;if(mode==2)player.rewarded=true;if(mode==3)player.distance=16.01f;
  if(mode==4)test.actor.los=false;if(mode==5)test.actor.combat=true;if(mode==6)test.actor.expired=true;
  if(mode==2)player.status=QUEST_STATUS_COMPLETE;
  test.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);assert(!test.casts&&!test.lists&&!test.mEvents[0].runOnce);++passed;
 }
 std::printf("PASS %u real-predicate/ElseGroup/one-shot LOS cases; native217775 cast/list endpoints and30s timer unchanged. Failed cast/conversation completion remain separate.\n",passed);
}
