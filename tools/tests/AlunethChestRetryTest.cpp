#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <list>
using uint32=uint32_t;using uint64=uint64_t;using int64=int64_t;
#include "NativeEnums.inc"
#include "CastFlags.inc"
#include "SummonFlags.inc"
constexpr uint32 UNIT_STATE_CASTING=1;
constexpr double M_PI=3.14159265358979323846;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
struct Position{float x=0,y=0,z=0;float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}void SimplePosXYRelocationByAngle(Position&,float,float){assert(false);}};
struct GameObject;struct WorldObject;struct Unit;
using GameObjectMapType=std::vector<GameObject*>;
struct Map{uint32 id=42;bool instance=true,admitted=true;unsigned attempts=0;GameObjectMapType objects;uint32 GetInstanceId(){return id;}Map* ToInstanceMap(){return instance?this:nullptr;}};
using InstanceMap=Map;
struct Scenario{uint32 id=1101,step=6;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct ScenarioManager{Scenario scenario;bool present=true;Scenario*GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}}scenarioManager;auto sScenarioMgr=&scenarioManager;
struct WorldObject{Map*map=nullptr;float x=1117.78f,y=1048.03f,z=311.614f;uint32 phaseId=6068,mask=1,entry=106756;
 virtual ~WorldObject()=default;virtual Unit*ToUnit(){return nullptr;}Map*GetMap()const{return map;}bool InInstance(){return map&&map->instance;}uint32 GetGUIDLow()const{return 370546;}uint32 GetGUID()const{return 370546;}uint32 GetEntry()const{return entry;}uint32 GetPhaseMask()const{return mask;}
 bool InSamePhase(uint32 m)const{return (mask&m)!=0;}bool InSamePhase(WorldObject const*o)const{return (mask&o->mask)&&phaseId==o->phaseId;}
 bool IsInMap(WorldObject const*o)const{return map==o->map;}float GetDistance(WorldObject const*o)const{return std::sqrt((x-o->x)*(x-o->x)+(y-o->y)*(y-o->y)+(z-o->z)*(z-o->z));}
 bool _IsWithinDist(WorldObject const*o,float range,bool,bool)const{return GetDistance(o)<=range;}bool IsWithinDistInMap(WorldObject const*,float,bool=true,bool=false)const;
 bool IsQuestObjectExpired(){return false;}void GetPosition(float&a,float&b,float&c,float&d){a=x;b=y;c=z;d=0;}float GetObjectSize(){assert(false);return 0;}void GetNearPoint2D(Position&,float,float){assert(false);}Position GetFirstCollisionPosition(float,float){assert(false);return {};}
 GameObject*FindNearestGameObject(uint32,float)const;GameObject*SummonGameObject(uint32,float,float,float,float,float,float,float,float,uint32,uint32,void*,bool);
};
struct Unit:WorldObject{bool enabled=true;Unit*ToUnit()override{return this;}bool isInCombat(){return false;}bool HasUnitState(uint32){return false;}};
struct GameObject:WorldObject{uint32 summoner=0,owner=0,lifetime=1;};
namespace Trinity{
#include "NearestClass.inc"
#include "SearcherClass.inc"
#include "NearestBody.inc"
#include "SearcherVisit.inc"
 template<class T>void VisitNearbyGridObject(WorldObject const*source,float,T&visitor){visitor.Visit(source->map->objects);}
}
#include "WithinMap.inc"
#include "FindNearest.inc"
#include "Closest.inc"
GameObject*WorldObject::SummonGameObject(uint32 id,float a,float b,float c,float o,float r0,float r1,float r2,float r3,uint32 time,uint32 viewer,void*list,bool creator){
 assert(id==249814&&a==1117.78f&&b==1048.03f&&c==314.619f&&o==2.09636f&&!r0&&!r1&&!r2&&!r3&&!time&&!viewer&&!list&&!creator);++map->attempts;if(!map->admitted)return nullptr;
 auto*go=new GameObject;go->map=map;go->entry=id;go->x=a;go->y=b;go->z=c;go->phaseId=phaseId;go->mask=mask;go->summoner=GetGUID();go->owner=0;go->lifetime=time;map->objects.push_back(go);return go;
}
struct ConditionSourceInfo{WorldObject*objects[2];ConditionSourceInfo(WorldObject*a,WorldObject*b):objects{a,b}{}};
constexpr uint32 CONDITION_NEAR_GAMEOBJECT=30,CONDITION_SCENARION_STEP=44;
struct Condition{uint32 ElseGroup=0,ConditionType=0,ConditionTarget=1,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;bool NegativeCondition=false;
 bool isLoaded(){return true;}bool Meets(ConditionSourceInfo&info){WorldObject*object=info.objects[ConditionTarget];if(!object)return false;bool condMeets=false;switch(ConditionType){
#include "Conditions.inc"
 default:assert(false);}return NegativeCondition?!condMeets:condMeets;}
};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;using ConditionTypeContainer=std::map<uint32,ConditionList>;using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr{ConditionReferenceContainer ConditionReferenceStore;SmartEventConditionContainer SmartEventConditionStore;
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;bool IsObjectMeetToConditions(ConditionSourceInfo&i,ConditionList const&l)const{return l.empty()||IsObjectMeetToConditionList(i,l);}
 ConditionList GetConditionsForSmartEvent(int64,uint32,uint32);bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*)const;
}conditionManager;auto sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
#include "GetConditions.inc"
#include "EventConditions.inc"
struct SmartScriptHolder{uint32 entryOrGuid=106756,event_id=0,source_type=0,link=0,eventType=0,type=0,targetType=0,timer=0;bool runOnce=false,active=true,enableTimed=true;
 struct{uint32 event_flags=0,event_phase_mask=0,event_chance=100;struct{uint32 min=0,max=0,repeatMin=0,repeatMax=0;}minMaxRepeat;}event;
 struct{struct{uint32 entry=0,despawnTime=0,flags=0;}summonGO;struct{uint32 phase=0;}setEventPhase;struct{uint32 flags=0;}cast;}action;
 struct{float x=0,y=0,z=0,o=0;struct{float range=0,distance=0,angle=0;}randomPos;}target;
 uint32 GetActionType()const{return type;}uint32 GetTargetType()const{return targetType;}uint32 GetEventType()const{return eventType;}uint32 GetScriptType()const{return source_type;}
};
unsigned refusals=0;
bool IsGameObjectValid(SmartScriptHolder&,uint32 id){return id==249814;}
#undef TC_LOG_ERROR
#define TC_LOG_ERROR(...) (++refusals)
bool Validate(SmartScriptHolder&e){switch(e.GetActionType()){
#include "SummonValidation.inc"
default:assert(false);}return true;}
#undef TC_LOG_ERROR
#define TC_LOG_ERROR(...) assert(false)
struct ObjectGuid{static uint32 Empty;};uint32 ObjectGuid::Empty=0;
using ObjectList=std::list<WorldObject*>;
bool roll_chance_i(uint32){assert(false);return false;}uint32 urand(uint32 a,uint32 b){assert(a==b);return a;}float rand_norm(){assert(false);return 0;}
struct SmartScript{Unit*me=nullptr;std::vector<SmartScriptHolder>mEvents,mTimedActionList;uint32 mEventPhase=0,mLastInvoker=0;unsigned teleport=0,data=0,unaura=0,finalSpeech=0;
 WorldObject*GetBaseObject(){return me;}Unit*GetLastInvoker(){return nullptr;}bool IsUnit(WorldObject*o){return o&&o->ToUnit();}ObjectList*GetTargets(SmartScriptHolder const&,Unit*){return nullptr;}
 void ResetBaseObject(){}void SetPhase(uint32);bool IsInPhase(uint32)const;void RecalcTimer(SmartScriptHolder&,uint32,uint32);void InitTimer(SmartScriptHolder&);void OnReset();void UpdateTimer(SmartScriptHolder&,uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,void const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,void const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,void const* =nullptr,GameObject* =nullptr);
 SmartScriptHolder FindLinkedEvent(uint32 id){for(auto const&e:mEvents)if(e.event_id==id)return e;assert(false);return {};}
 void Tick(uint32 diff){if(me->enabled)for(auto&e:mEvents)UpdateTimer(e,diff);}
};
#include "Phase.inc"
#include "Timer.inc"
#include "Reset.inc"
#include "UpdateTimer.inc"
#include "Events.inc"
void SmartScript::ProcessEvent(SmartScriptHolder&e,Unit*unit,uint32 var0,uint32 var1,bool bvar,void const*spell,GameObject*gob){
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "SelectedEvents.inc"
 default:assert(false);}}
void SmartScript::ProcessAction(SmartScriptHolder&e,Unit*unit,uint32 var0,uint32 var1,bool bvar,void const*spell,GameObject*gob){
#include "ActionPrefix.inc"
 switch(e.GetActionType()){
#include "SummonGo.inc"
#include "SetPhaseAction.inc"
 case SMART_ACTION_SET_DATA:++data;break;
 case SMART_ACTION_TELEPORT:++teleport;break;
 case SMART_ACTION_REMOVEAURASFROMSPELL:++unaura;break;
 case SMART_ACTION_TALK:if(e.event_id==6)++finalSpeech;break;
 default:assert(false);}
#include "Links.inc"
}
struct Shape{std::vector<SmartScriptHolder>events;std::vector<Condition>conditions;std::vector<uint32>groups;};
Shape Load(char const*ep,char const*cp){Shape result;std::ifstream events(ep),conditions(cp);assert(events&&conditions);SmartScriptHolder e;uint32 p1,p2,p3;while(events>>e.entryOrGuid>>e.source_type>>e.event_id>>e.link>>e.eventType>>e.event.event_phase_mask>>e.event.event_chance>>e.event.event_flags>>e.event.minMaxRepeat.min>>e.event.minMaxRepeat.max>>e.event.minMaxRepeat.repeatMin>>e.event.minMaxRepeat.repeatMax>>e.type>>p1>>p2>>p3>>e.targetType>>e.target.x>>e.target.y>>e.target.z>>e.target.o){e.action.summonGO={p1,p2,p3};e.action.setEventPhase.phase=p1;result.events.push_back(e);}Condition c;uint32 group;while(conditions>>group>>c.ElseGroup>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition){result.groups.push_back(group);result.conditions.push_back(c);}return result;}
struct Run{Map map;Unit actor;Shape shape;SmartScript script;
 Run(Shape const&s):shape(s){scenarioManager={};conditionManager={};actor.map=&map;script.me=&actor;script.mEvents=shape.events;for(auto&e:script.mEvents)script.InitTimer(e);for(size_t i=0;i<shape.conditions.size();++i)conditionManager.SmartEventConditionStore[{106756,0}][shape.groups[i]].push_back(&shape.conditions[i]);}
 ~Run(){for(auto*o:map.objects)delete o;}
 void Existing(uint32 id,uint32 phase=6068,bool far=false){auto*o=new GameObject;o->entry=id;o->map=&map;o->phaseId=phase;if(far)o->x+=200;map.objects.push_back(o);}
 void OneIntro(){assert(script.teleport==1&&script.data==1&&script.unaura==1&&script.finalSpeech==1);}
};
int main(int argc,char**argv){_set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==5);Shape old=Load(argv[1],argv[2]),now=Load(argv[3],argv[4]);assert(old.events.size()==6&&now.events.size()==8&&old.conditions.size()==2&&now.conditions.size()==8);
 auto valid=*std::find_if(now.events.begin(),now.events.end(),[](auto const&e){return e.event_id==7;});assert(Validate(valid));assert(!(valid.event.event_flags&~SMART_EVENT_FLAGS_ALL));
 for(uint32 bad=0;bad<4;++bad){auto e=valid;if(bad==0)e.targetType=1;if(bad==1)e.eventType=61;if(bad==2)e.event.event_phase_mask=0;if(bad==3)e.event.event_flags=256;assert(!Validate(e));}assert(refusals==4);
 {Run r(old);r.map.admitted=false;r.script.Tick(1);r.OneIntro();assert(r.map.attempts==1);for(int i=0;i<10;++i)r.script.Tick(2001);assert(r.map.attempts==1&&r.map.objects.empty());r.script.OnReset();r.script.Tick(1);assert(r.script.teleport==2);std::cout<<"COUNTERFACTUAL native original LINK consumes failed creation; ordinary polls never retry and Reset repeats player teleport. Controlled Map rejection, no production crash.\n";}
 {Run r(now);r.map.admitted=false;r.script.Tick(1);r.OneIntro();assert(r.map.attempts==1&&r.script.mEventPhase==1);r.script.OnReset();r.script.Tick(1999);assert(r.map.attempts==1);r.script.Tick(1);assert(r.map.attempts==1);r.map.admitted=true;r.script.Tick(1);assert(r.map.attempts==2&&r.map.objects.size()==1);auto*o=r.map.objects.front();assert(o->entry==249814&&!o->owner&&!o->lifetime&&o->summoner==370546);r.script.OnReset();for(int i=0;i<5;++i)r.script.Tick(2001);assert(r.map.attempts==2);r.OneIntro();}
 for(uint32 id:{249814u,248574u}){Run r(now);r.Existing(id);r.script.OnReset();for(int i=0;i<5;++i)r.script.Tick(2001);assert(!r.map.attempts&&!r.script.teleport&&!r.script.finalSpeech);}
 for(uint32 variant=0;variant<3;++variant){Run r(now);r.Existing(variant==2?12345:249814,variant==0?6140:6068,variant==1);r.script.OnReset();r.script.Tick(1);r.OneIntro();assert(r.map.attempts==1&&r.map.objects.size()==2);}
 {Run r(now);r.map.admitted=false;r.script.Tick(1);r.actor.enabled=false;r.map.admitted=true;r.script.Tick(20000);assert(r.map.attempts==1);r.actor.enabled=true;r.script.Tick(2001);assert(r.map.attempts==2);r.OneIntro();}
 for(uint32 wrong=0;wrong<3;++wrong){Run r(now);if(wrong==0)scenarioManager.scenario.id=1065;if(wrong==1)scenarioManager.scenario.step=7;if(wrong==2)scenarioManager.present=false;r.script.OnReset();r.script.Tick(2001);assert(!r.map.attempts&&!r.script.teleport);}
 {Run r(now);scenarioManager.scenario.step=5;r.script.OnReset();r.script.Tick(2001);assert(!r.map.attempts);scenarioManager.scenario.step=6;r.script.Tick(1);assert(r.map.attempts==1);r.OneIntro();}
 {Run r(now);r.map.admitted=false;r.script.Tick(1);for(size_t i=0;i<r.shape.conditions.size();++i)if(r.shape.groups[i]==9)r.shape.conditions[i].ConditionTarget=0;r.script.OnReset();r.map.admitted=true;r.script.Tick(2001);assert(r.map.attempts==1&&r.script.mEventPhase==0);std::cout<<"COUNTERFACTUAL reset condition target0 loses the null-invoker base; repaired target1 restores phase and pending native retry above.\n";}
 std::cout<<"PASS actual native UPDATE/LINK/RESET, pre-dispatch latch/LINK copy, phased2s retry/DONT_RESET, source22 getter+ElseGroups/scenario+phase-visible nearest GO predicates: failed creation/strict timer/Reset/no duplicate intro, existing chest/exit/new AI, wrong phase/range/entry/scenario, unavailable AI deferred then resumed, null invoker. Map insertion and unchanged dialogue/teleport endpoints controlled; no progression grant/client rendering proof.\n";
}
