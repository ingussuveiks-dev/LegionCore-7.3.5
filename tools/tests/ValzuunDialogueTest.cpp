// Actual SmartScript dispatcher, guards, LINK and CALL_TIMED_ACTIONLIST.
// Credit/status changes and AI scheduling are controlled endpoints, not a live quest/client.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;
#include "Constants.inc"
using QuestStatus=uint32;
constexpr uint32 QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_NONE=0,QUEST_STATUS_REWARDED=6;
constexpr uint32 CONDITION_QUESTTAKEN=9,CONDITION_OBJECT_ENTRY=31,CONDITION_ALIVE=36;
struct SmartScriptHolder;struct Unit;
struct AI {unsigned calls=0;Unit* last=nullptr;uint32 list=0;void SetScript9(SmartScriptHolder&,uint32 id,Unit* invoker){++calls;list=id;last=invoker;}};
struct Unit {
 bool player=false,pet=false,alive=true,expired=false,combat=false,smart=true;QuestStatus status=QUEST_STATUS_INCOMPLETE;Unit* owner=nullptr;::AI ai;
 Unit* ToUnit(){return this;}Unit* ToPlayer(){return player?this:nullptr;}Unit* ToCreature(){return player?nullptr:this;}Unit* ToGameObject(){return nullptr;}
 bool IsCreature()const{return !player;}bool isPet()const{return pet;}Unit* GetOwner(){return owner;}
 struct Threat {Unit* getHostilTarget(){return nullptr;}} threat;Threat& getThreatManager(){return threat;}
 QuestStatus GetQuestStatus(uint32 id)const{return id==41919?status:QUEST_STATUS_NONE;}
 uint32 GetTypeId()const{return player?4:3;}uint32 GetEntry()const{return player?0:105333;}
 bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}bool isInCombat()const{return combat;}
 Unit* GetGUID(){return this;}::AI* AI(){return &ai;}
};
using Player=Unit;using Creature=Unit;using GameObject=Unit;using ObjectList=std::vector<Unit*>;struct SpellInfo{};
struct Rule {uint32 group,kind,target,value,second;};using ConditionList=std::vector<Rule>;
struct ConditionSourceInfo {Unit* targets[2];ConditionSourceInfo(Unit* a,Unit* b):targets{a,b}{}};
bool Evaluate(Rule const& rule,ConditionSourceInfo const& info){
 Unit* object=info.targets[rule.target];if(!object)return false;
 uint32 ConditionValue1=rule.value,ConditionValue2=rule.second;bool condMeets=false;
 switch(rule.kind){
#include "QuestCondition.inc"
#include "PlayerCondition.inc"
#include "AliveCondition.inc"
 }
 return condMeets;
}
struct Conditions {
 ConditionList rules;
 ConditionList GetConditionsForSmartEvent(uint32 entry,uint32 id,uint32 source)const{ConditionList out;if(entry==105333&&source==0)for(auto r:rules)if(r.group==id+1)out.push_back(r);return out;}
 bool IsObjectMeetToConditions(ConditionSourceInfo const& info,ConditionList const& list)const{for(auto r:list)if(!Evaluate(r,info))return false;return true;}
 bool IsObjectMeetingSmartEventConditions(uint32 entry,uint32 id,uint32 source,Unit* unit,Unit* baseObject)const{
#include "SmartTargetBinding.inc"
  return IsObjectMeetToConditions(sourceInfo,GetConditionsForSmartEvent(entry,id,source));
 }
} conditions;Conditions* sConditionMgr=&conditions;
struct SmartScriptHolder {
 uint32 entryOrGuid=0,source_type=0,event_id=0,link=0,type=0,actionType=0,parameter=0,targetType=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;struct {uint32 sender=0,action=0,cooldownMin=0,cooldownMax=0;} gossip;} event;
 struct {struct {uint32 id=0;} timedActionList;} action;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}uint32 GetScriptType()const{return source_type;}
};
bool roll_chance_i(uint32){return true;}bool IsSmart(Unit* unit){return unit&&unit->smart;}bool IsSmartGO(Unit*){return false;}
#define ENSURE_AI(type, value) (value)
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_DEBUG(...) ((void)0)
struct SmartScript {
 std::vector<SmartScriptHolder> mEvents;Unit actor;Unit* me=&actor;Unit* mLastInvoker=nullptr;
 unsigned errors=0,creditCalls=0,conversations=0;bool completeOnCredit=false;
 Unit* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker;}bool IsInPhase(uint32)const{return true;}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){return e.targetType==1&&me?new ObjectList{me}:nullptr;}
 void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LinkEvent.inc"
#include "SelectEvent.inc"
#include "NoParamEvents.inc"
 default:break;
 }
}
#include "ActionPrefix.inc"
 switch(e.GetActionType()){
#include "ListAction.inc"
 // Controlled endpoints retain original credit timing and the later quest branch.
 case 33:++creditCalls;if(completeOnCredit&&e.parameter==105333&&unit)unit->status=QUEST_STATUS_COMPLETE;break;
 case 206:if(e.parameter==1832)++conversations;break;
 default:break;
 }
#include "LinkTail.inc"
static std::vector<std::vector<std::string>> Read(char const* path){
 std::ifstream file(path);std::string line;std::vector<std::vector<std::string>> out;
 while(std::getline(file,line)){std::stringstream stream(line);std::string field;std::vector<std::string> row;while(std::getline(stream,field,'\t'))row.push_back(field);if(!line.empty()&&line.back()=='\t')row.push_back("");out.push_back(row);}return out;
}
static std::vector<SmartScriptHolder> Actions(char const* path){
 std::vector<SmartScriptHolder> out;for(auto const& f:Read(path)){if(f.size()!=31)return {};auto n=[&](size_t i){return uint32(std::stoul(f[i]));};if(n(0)!=105333)continue;
 SmartScriptHolder e;e.entryOrGuid=n(0);e.source_type=n(1);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_flags=n(8);e.event.gossip={n(9),n(10),n(11),n(12)};e.actionType=n(14);e.parameter=n(15);e.action.timedActionList.id=n(15);e.targetType=n(21);out.push_back(e);}return out;
}
int main(int argc,char** argv){
 if(argc!=4)return 1;auto original=Actions(argv[1]),fixed=Actions(argv[2]);if(original.size()!=4||fixed.size()!=4)return 2;
 ConditionList rules;for(auto const& f:Read(argv[3])){if(f.size()!=14)return 3;rules.push_back({uint32(std::stoul(f[1])),uint32(std::stoul(f[5])),uint32(std::stoul(f[6])),uint32(std::stoul(f[7])),uint32(std::stoul(f[8]))});}if(rules.size()!=5)return 4;
 Unit first;first.player=true;Unit second=first;
 conditions.rules.clear();SmartScript old;old.mEvents=original;
 old.ProcessEventsFor(SMART_EVENT_GOSSIP_HELLO,&first);if(old.actor.ai.calls!=1)return 5;
 old.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&first,19472,0);if(old.creditCalls!=1||old.errors!=1||old.actor.ai.calls!=1)return 6;
 conditions.rules=rules;
 for(bool finish:{false,true}){SmartScript s;s.mEvents=fixed;s.completeOnCredit=finish;first.status=QUEST_STATUS_INCOMPLETE;
  s.ProcessEventsFor(SMART_EVENT_GOSSIP_HELLO,&first);if(s.actor.ai.calls||s.creditCalls)return 7;
  s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&first,19472,0);if(s.errors||s.creditCalls!=1||s.actor.ai.calls!=1||s.actor.ai.last!=&first||s.actor.ai.list!=10533300)return 8;
  if(finish&&first.status!=QUEST_STATUS_COMPLETE)return 9;
  s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&second,19472,0);if(s.errors||s.actor.ai.calls!=2||s.actor.ai.last!=&second)return 10;
  // A new legitimate interaction (or retry) must not inherit actor-wide runOnce.
  first.status=QUEST_STATUS_INCOMPLETE;s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&first,19472,0);if(s.actor.ai.calls!=3)return 11;
 }
 first.status=QUEST_STATUS_INCOMPLETE;
 for(unsigned mode=0;mode<9;++mode){SmartScript s;s.mEvents=fixed;Unit p=first;p.status=mode==0?QUEST_STATUS_NONE:mode==1?QUEST_STATUS_COMPLETE:mode==2?QUEST_STATUS_REWARDED:QUEST_STATUS_INCOMPLETE;if(mode==3)p.alive=false;if(mode==4){p.player=false;p.pet=true;p.owner=&first;}if(mode==5)p.player=false;if(mode==6)s.actor.expired=true;Unit* invoker=mode==7?nullptr:&p;
  s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,invoker,mode==8?19473:19472,0);if(s.creditCalls||s.actor.ai.calls||s.errors)return 12;
 }
 SmartScript missing;missing.mEvents=fixed;missing.actor.smart=false;missing.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&first,19472,0);if(missing.actor.ai.calls||missing.errors)return 13;
 SmartScript later;later.mEvents=fixed;later.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&first,19472,1);if(later.creditCalls!=1||later.conversations!=1||later.actor.ai.calls||later.errors)return 14;
 std::puts("PASS extracted native HELLO/password/LINK/condition/player/AI handoff, last-objective completion, second-player/retry, dead/null/pet/guardian/inactive/wrong-menu/expired/non-Smart refusals and preserved41920 branch. Credit/group awards, source9 scheduling, client audio/timing not simulated.");
}
