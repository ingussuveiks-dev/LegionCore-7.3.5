// Actual native dispatch/condition/LINK/action fragments; world/criteria endpoints controlled.
#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <set>
using uint32=std::uint32_t;using int64=std::int64_t;
enum SMART_EVENT{SMART_EVENT_LINK=61,SMART_EVENT_EVENTOBJECT_ONTRIGGER=89,SMART_EVENT_EVENTOBJECT_OFFTRIGGER=90};
enum{CONDITION_SCENARION_STEP=44,SMART_EVENT_FLAG_NOT_REPEATABLE=1,SMART_EVENT_FLAG_EVENT_NON_COMBAT=2,SMART_TARGET_NONE=0,SMART_TARGET_ACTION_INVOKER=7,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205,SMART_ACTION_REMOVEAURASFROMSPELL=28};
enum CriteriaTypes{SCRIPT_EVENT_2=92};
static unsigned errors=0;
#define TC_LOG_DEBUG(...) ((void)0)
template<class... Args>static void ObserveError(char const*,char const*,Args...){++errors;}
#define TC_LOG_ERROR(...) ObserveError(__VA_ARGS__)
#define SI64FMTD "%lld"
struct Unit;struct Player;struct SpellInfo{};struct GameObject{};
struct ObjectGuid{Unit* value=nullptr;};
struct InstanceMap{uint32 GetInstanceId()const{return 42;}};
struct Map{InstanceMap instance;InstanceMap* ToInstanceMap(){return &instance;}};
struct WorldObject{Map map;bool instanced=true,expired=false;virtual ~WorldObject()=default;bool InInstance(){return instanced;}Map* GetMap(){return &map;}bool IsQuestObjectExpired(){return expired;}virtual Unit* ToUnit(){return nullptr;}};
struct Aura{void ModStackAmount(int){assert(false);}};
struct Unit:WorldObject{bool alive=true,empowered=true;unsigned removals=0;Unit* ToUnit()override{return this;}virtual Player* ToPlayer(){return nullptr;}Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer();}bool IsAlive()const{return alive;}ObjectGuid GetGUID(){return {this};}uint32 GetGUIDLow(){return 9;}bool isInCombat(){return false;}void RemoveAllAuras(){assert(false);}Aura* GetAura(uint32){return nullptr;}void RemoveAurasDueToSpell(uint32 id){assert(id==217815);empowered=false;++removals;}};
struct Scenario{uint32 id=1146,step=2;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}};
struct ScenarioManager{Scenario data;bool present=true;Scenario* GetScenario(uint32 id){return present&&id==42?&data:nullptr;}}manager;
auto sScenarioMgr=&manager;
struct Player:Unit{unsigned calls=0,accepted=0;Player* ToPlayer()override{return this;}void UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32,uint32,Unit*,bool){assert(type==92&&asset==52537);++calls;if(manager.data.id==1146&&manager.data.step==2){++accepted;manager.data.step=3;}}};
#include "CreditPlayer.inc"
struct ConditionSourceInfo{WorldObject* mConditionTargets[2];ConditionSourceInfo(WorldObject* a,WorldObject* b):mConditionTargets{a,b}{}};
struct Condition{uint32 ElseGroup=0,ConditionType=44,ConditionTarget=0,ConditionValue1=1146,ConditionValue2=2,ConditionValue3=0,ReferenceId=0;bool NegativeCondition=false;bool isLoaded(){return true;}bool Meets(ConditionSourceInfo& info){auto object=info.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;switch(ConditionType){
#include "ScenarioCondition.inc"
default:assert(false);}return NegativeCondition?!condMeets:condMeets;}};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;using ConditionTypeContainer=std::map<uint32,ConditionList>;using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr{ConditionReferenceContainer ConditionReferenceStore;SmartEventConditionContainer SmartEventConditionStore;bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;bool IsObjectMeetToConditions(ConditionSourceInfo& i,ConditionList const& c)const{return c.empty()||IsObjectMeetToConditionList(i,c);}bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*)const;ConditionList GetConditionsForSmartEvent(int64,uint32,uint32);}conditionManager;
auto sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
#include "EventConditions.inc"
#include "Lookup.inc"
struct SmartScriptHolder{int64 entryOrGuid=157;uint32 event_id=0,source_type=13,link=0,type=89,actionType=205,targetType=7;bool active=true,runOnce=false;struct{uint32 event_phase_mask=0,event_flags=0;struct{uint32 id=0,cooldownMin=0,cooldownMax=0;}areatrigger;}event;struct{struct{uint32 type=92,misc1=52537,misc2=0,misc3=0;}achievementCriteria;struct{uint32 spell=217815;int stack=0;}removeAura;}action;uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}uint32 GetTargetType()const{return targetType;}};
using SmartAIEventList=std::vector<SmartScriptHolder>;using ObjectList=std::list<WorldObject*>;
struct SmartScript{WorldObject* base=nullptr;Unit* me=nullptr;SmartAIEventList mEvents;ObjectGuid mLastInvoker;WorldObject* GetBaseObject(){return base;}bool IsInPhase(uint32){return true;}bool IsUnit(WorldObject* o){return o&&o->ToUnit();}ObjectList* GetTargets(SmartScriptHolder const& e,Unit* trigger){auto* l=new ObjectList;switch(e.GetTargetType()){
#include "InvokerTarget.inc"
default:assert(false);}if(l->empty()){delete l;return nullptr;}return l;}
 void RecalcTimer(SmartScriptHolder&,uint32 a,uint32 b){assert(!a&&!b);}SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){
#include "EventPrefix.inc"
switch(e.GetEventType()){case SMART_EVENT_LINK:ProcessAction(e,unit,var0,var1,bvar,spell,gob);break;
#include "TriggerEvent.inc"
default:assert(false);}}
 void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,SpellInfo const* spell=nullptr,GameObject* gob=nullptr){if(GetBaseObject()&&GetBaseObject()->IsQuestObjectExpired())return;
#include "ActionStart.inc"
switch(e.GetActionType()){
#include "CriteriaAction.inc"
#include "RemoveAction.inc"
default:assert(false);}
#include "LinkTail.inc"
};
#include "Dispatch.inc"
#include "FindLink.inc"
int main(int argc,char**argv){assert(argc==2);std::ifstream in(argv[1]);unsigned source=0,cleanupType=0;assert(in>>source>>cleanupType);assert((source==10&&cleanupType==89)||(source==13&&cleanupType==61));unsigned cases=0;
 for(unsigned stage=0;stage<7;++stage)for(unsigned repeat=0;repeat<2;++repeat){manager={};manager.data.step=stage;conditionManager={};Condition c;conditionManager.SmartEventConditionStore[{157,source}][1].push_back(&c);Player p;WorldObject event;SmartScript script;script.base=&event;SmartScriptHolder root;root.link=1;SmartScriptHolder cleanup;cleanup.event_id=1;cleanup.type=cleanupType;cleanup.actionType=28;script.mEvents={root,cleanup};auto before=errors;script.ProcessEventsFor(SMART_EVENT_EVENTOBJECT_ONTRIGGER,&p,157);
  if(source==13&&cleanupType==61){assert(p.empowered==(stage!=2)&&p.removals==unsigned(stage==2)&&p.calls==unsigned(stage==2)&&p.accepted==unsigned(stage==2)&&manager.data.step==(stage==2?3:stage)&&errors==before);script.ProcessEventsFor(SMART_EVENT_EVENTOBJECT_ONTRIGGER,&p,157);assert(p.removals==unsigned(stage==2)&&p.accepted==unsigned(stage==2));}
  else{assert(!p.empowered&&p.removals>=1);if(cleanupType==89)assert(errors==before+1);else assert(errors==before);}
  ++cases;
 }
 if(source==13&&cleanupType==61){for(unsigned kind=0;kind<4;++kind){manager={};conditionManager={};Condition c;conditionManager.SmartEventConditionStore[{157,source}][1].push_back(&c);Player p;WorldObject event;SmartScript script;script.base=&event;SmartScriptHolder root;root.link=1;SmartScriptHolder cleanup;cleanup.event_id=1;cleanup.type=61;cleanup.actionType=28;script.mEvents={root,cleanup};if(kind==0)manager.present=false;if(kind==1)manager.data.id=972;if(kind==2)p.instanced=false;if(kind==3)event.expired=true;script.ProcessEventsFor(SMART_EVENT_EVENTOBJECT_ONTRIGGER,&p,157);assert(p.empowered&&!p.calls&&!p.removals);++cases;}
  assert(cases==18&&!errors);std::cout<<"PASS Hodir landing source18 ASAN:actual native13 lookup/44 stage gate/dispatch/invoker/205/LINK/28,only step2 accepts52537 then removes217815 after3 transition,repeat/missing/wrong-map-scenario/expired controls;criteria/world endpoints controlled\n";
 }else{assert(cases==14);std::cout<<"COUNTERFACTUAL Hodir landing source14:original10/89 graph strips aura at wrong stages and emits14 expected invalid-LINK diagnostics;not ordinary successful traversal\n";}
 return 0;
}
