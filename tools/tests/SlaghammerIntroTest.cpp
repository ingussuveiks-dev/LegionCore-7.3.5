// Native source22 binding, actual AND/OR grouping/predicates and SmartScript
// dispatch/LOS/repeat/LINK. Criteria and deferred conversation are endpoints.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;
#include "Constants.inc"
using QuestStatus=uint32;constexpr uint32 QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_NONE=0;
constexpr uint32 CONDITION_QUESTTAKEN=9,CONDITION_OBJECT_ENTRY=31,CONDITION_ALIVE=36,CONDITION_SCENARION_STEP=44;
struct Scenario {uint32 id=991,step=1;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}} scenario;
struct Map {bool instance=true;uint32 id=1;Map* ToInstanceMap(){return instance?this:nullptr;}uint32 GetInstanceId()const{return id;}} origin,other;
using InstanceMap=Map;
struct ScenarioMgr {bool available=true;Scenario* GetScenario(uint32 id){return available&&id==1?&scenario:nullptr;}} scenarioMgr;auto sScenarioMgr=&scenarioMgr;
struct Unit {bool player=false,pet=false,alive=true,combat=false,expired=false,los=true,world=true,hostile=false;float distance=1;uint32 quest=3;Unit* owner=nullptr;Map* map=&origin;
 Unit* ToPlayer(){return player?this:nullptr;}Unit* ToUnit(){return this;}bool IsCreature()const{return !player;}bool isPet()const{return pet;}Unit* GetOwner(){return owner;}
 struct Threat {Unit* getHostilTarget(){return nullptr;}} threat;Threat& getThreatManager(){return threat;}
 QuestStatus GetQuestStatus(uint32 id)const{return id==40710?quest:0;}uint32 GetTypeId()const{return player?4:3;}uint32 GetEntry()const{return player?0:101430;}
 bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}bool isInCombat()const{return combat;}bool InInstance()const{return map->instance;}Map* GetMap(){return map;}
 bool IsWithinDistInMap(Unit* unit,float range)const{return unit&&world&&unit->world&&unit->map==map&&unit->distance<=range;}bool IsWithinLOSInMap(Unit* unit)const{return unit&&unit->los;}bool IsHostileTo(Unit*)const{return hostile;}
 Unit* GetGUID(){return this;}
};
using Player=Unit;using WorldObject=Unit;struct GameObject{};struct SpellInfo{};
struct ConditionSourceInfo {Unit* targets[2];ConditionSourceInfo(Unit* a=nullptr,Unit* b=nullptr):targets{a,b}{}};
struct Rule {
 uint32 entry=0,group=0,ElseGroup=0,ConditionType=0,targetIndex=0,ConditionValue1=0,ConditionValue2=0,ReferenceId=0;
 bool isLoaded()const{return ConditionType>0;}
 bool Meets(ConditionSourceInfo& info)const{Unit* object=info.targets[targetIndex];if(!object)return false;bool condMeets=false;
  switch(ConditionType){
#include "QuestCondition.inc"
#include "PlayerCondition.inc"
#include "AliveCondition.inc"
#include "ScenarioCondition.inc"
  }return condMeets;
 }
};
using ConditionList=std::vector<Rule const*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
#define TC_LOG_DEBUG(...) ((void)0)
struct ConditionMgr {
 std::vector<Rule> rules;ConditionReferenceContainer ConditionReferenceStore;
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(WorldObject*,ConditionList const&);
 bool IsObjectMeetToConditions(WorldObject*,WorldObject*,ConditionList const&);
 ConditionList GetConditionsForSmartEvent(uint32 entry,uint32 id,uint32 source)const{ConditionList out;if(!source)for(auto const& r:rules)if(r.entry==entry&&r.group==id+1)out.push_back(&r);return out;}
 bool IsObjectMeetingSmartEventConditions(uint32 entry,uint32 id,uint32 source,Unit* unit,Unit* baseObject)const{
#include "SmartTargetBinding.inc"
  return IsObjectMeetToConditions(sourceInfo,GetConditionsForSmartEvent(entry,id,source));
 }
} conditionMgr;auto sConditionMgr=&conditionMgr;
#include "ConditionGroups.inc"
struct SmartScriptHolder {
 uint32 entryOrGuid=0,source_type=0,event_id=0,link=0,type=0,actionType=0,parameter=0,second=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;struct {uint32 noHostile=0,maxDist=0,cooldownMin=0,cooldownMax=0;} los;} event;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}
};
bool roll_chance_i(uint32){return true;}
struct SmartScript {
 std::vector<SmartScriptHolder> mEvents;Unit actor;Unit* me=&actor;Unit* mLastInvoker=nullptr;uint32 errors=0,criteriaRequests=0,conversationRequests=0;bool advance=true;
 Unit* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker;}bool IsInPhase(uint32)const{return true;}void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
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
#include "LinkEvent.inc"
#include "LosEvent.inc"
 default:break;
 }
}
#include "ActionPrefix.inc"
 if(e.GetActionType()==205){++criteriaRequests;assert(e.parameter==92&&e.second==48121);if(advance&&scenario.id==991&&scenario.step==1)scenario.step=2;}
 if(e.GetActionType()==85){assert(e.parameter==200165);++conversationRequests;}
#include "LinkTail.inc"
static std::vector<std::vector<std::string>> Read(char const* path){std::ifstream file(path);std::string line;std::vector<std::vector<std::string>> out;while(std::getline(file,line)){std::stringstream stream(line);std::string cell;std::vector<std::string> row;while(std::getline(stream,cell,'\t'))row.push_back(cell);if(!line.empty()&&line.back()=='\t')row.push_back("");out.push_back(row);}return out;}
int main(int argc,char** argv){
 if(argc!=3)return 1;std::vector<SmartScriptHolder> actions;for(auto const& f:Read(argv[1])){if(f.size()!=31)return 2;auto n=[&](size_t i){return uint32(std::stoul(f[i]));};if(n(0)!=101430)continue;SmartScriptHolder e;e.entryOrGuid=n(0);e.source_type=n(1);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_flags=n(8);e.event.los={n(9),n(10),n(11),n(12)};e.actionType=n(14);e.parameter=n(15);e.second=n(16);actions.push_back(e);}assert(actions.size()==10);
 std::vector<Rule> fixed,old;for(auto const& f:Read(argv[2])){if(f.size()!=14)return 3;auto n=[&](size_t i){return uint32(std::stoul(f[i]));};Rule r;r.entry=n(2);r.group=n(1);r.ElseGroup=n(4);r.ConditionType=n(5);r.targetIndex=n(6);r.ConditionValue1=n(7);r.ConditionValue2=n(8);fixed.push_back(r);if(r.entry!=101430)old.push_back(r);}assert(fixed.size()==13&&old.size()==1);
 Unit player;player.player=true;
 conditionMgr.rules=old;scenario={1037,1};SmartScript baseline;baseline.mEvents=actions;baseline.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);assert(baseline.criteriaRequests==1&&baseline.conversationRequests==1&&baseline.errors==1);
 conditionMgr.rules=fixed;
 for(uint32 wrong:{1037u,1092u,933u}){scenario={wrong,1};SmartScript s;s.mEvents=actions;s.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);assert(!s.criteriaRequests&&!s.conversationRequests&&!s.errors&&!s.mEvents[0].runOnce);scenario={991,1};s.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);assert(s.criteriaRequests==1&&s.conversationRequests==1&&s.errors==1);}
 for(bool advance:{false,true}){scenario={991,1};SmartScript s;s.mEvents=actions;s.advance=advance;s.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);assert(s.criteriaRequests==1&&s.conversationRequests==1&&s.errors==1);assert(scenario.step==(advance?2u:1u));s.ProcessEventsFor(SMART_EVENT_OOC_LOS,&player);assert(s.criteriaRequests==1&&s.conversationRequests==1&&s.errors==1);}
 for(unsigned mode=0;mode<14;++mode){scenario={991,1};scenarioMgr.available=true;origin.instance=true;SmartScript s;s.mEvents=actions;Unit p=player;switch(mode){case 0:scenario.step=0;break;case 1:scenario.step=2;break;case 2:p.quest=0;break;case 3:p.quest=1;break;case 4:p.quest=6;break;case 5:p.alive=false;break;case 6:p.player=false;p.pet=true;p.owner=&player;break;case 7:p.player=false;break;case 8:p.distance=71;break;case 9:p.los=false;break;case 10:s.actor.combat=true;break;case 11:s.actor.expired=true;break;case 12:scenarioMgr.available=false;break;case 13:origin.instance=false;break;}s.ProcessEventsFor(SMART_EVENT_OOC_LOS,&p);assert(!s.criteriaRequests&&!s.conversationRequests&&!s.errors&&!s.mEvents[0].runOnce);}
 origin.instance=true;scenarioMgr.available=true;scenario={991,1};SmartScript null; // keep null invoker distinct from actor fallback
 null.mEvents=actions;null.ProcessEventsFor(SMART_EVENT_OOC_LOS,nullptr);assert(!null.criteriaRequests&&!null.conversationRequests&&!null.errors);
 // Cultist DATA producer remains actor-scoped and admits pet/guardian/null invokers.
 scenario={991,2};Unit cultist,pet;pet.pet=true;pet.owner=&player;for(Unit* killer:{&player,&pet,static_cast<Unit*>(nullptr)})assert(conditionMgr.IsObjectMeetingSmartEventConditions(101122,1,0,killer,&cultist));
 std::puts("PASS native AND/OR/source22/LOS/repeat/LINK: old wrong-Warrior criterion+Priest conversation counterfactual; repaired correct991/1 and post-credit991/2, rejects other shared scenarios and invalid invokers without consuming once; cultist pet/null producer retained. Legitimate Priest still reaches known missingLINK2 (expected counter), NOT repaired or hidden. Criteria/cast endpoints and client traversal not certified.");
}
