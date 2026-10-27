#include <list>
namespace Dispatch
{
#include "NativeEnums.inc"
#include "CastFlags.inc"
#undef TC_LOG_DEBUG
#define TC_LOG_DEBUG(...) ((void)0)
#define ENSURE_AI(type,ai) (ai)
constexpr uint32 UNIT_FIELD_NPC_FLAGS=1;
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS};
struct SpellInfo{};struct Unit;struct InstanceScript{std::function<void(uint32,ObjectGuid)>callback;void SetGuidData(uint32 f,ObjectGuid g){callback(f,g);}};
struct WorldObject{virtual ~WorldObject()=default;InstanceScript*instance=nullptr;bool expired=false;virtual Unit*ToUnit(){return nullptr;}virtual bool IsUnit(){return false;}bool IsQuestObjectExpired(){return expired;}InstanceScript*GetInstanceScript(){return instance;}ObjectGuid GetGUID(){return {2};}};
struct Motion{unsigned clears=0,moves=0;void Clear(){++clears;}void MovePoint(uint32 id,float,float,float){assert(id==0);++moves;}};
struct CombatAI{void SetCombatMove(bool){}};
struct Unit:WorldObject{SpellCastResult result=SPELL_CAST_OK;unsigned casts=0;bool fly=true,gravity=true;uint32 npcFlags=1;Motion motion;CombatAI ai;
 Unit*ToUnit()override{return this;}bool IsUnit()override{return true;}bool isInCombat(){return false;}bool HasAura(uint32){return false;}void GetPosition(float&x,float&y,float&z){x=y=z=0;}void GetPosition(float&x,float&y,float&z,float&o){x=y=z=o=0;}void InterruptNonMeleeSpells(bool){}CombatAI*AI(){return &ai;}
 SpellCastResult CastSpell(Unit*,uint32 id,bool trigger){assert(id==223908&&!trigger);++casts;return result;}SpellCastResult CastSpell(float,float,float,uint32,bool){assert(false);return result;}void SetUInt32Value(uint32 f,uint32 value){assert(f==UNIT_FIELD_NPC_FLAGS);npcFlags=value;}Motion*GetMotionMaster(){return &motion;}void SetCanFly(bool b){fly=b;}void SetDisableGravity(bool b){gravity=b;}
};
struct GameObject{SpellCastResult CastSpell(Unit*,uint32){assert(false);return SPELL_FAILED_BAD_TARGETS;}};
using ObjectList=std::list<WorldObject*>;
struct SmartScriptHolder{
 uint32 entryOrGuid=104546,event_id=0,source_type=0,link=0,type=0,eventType=40,targetType=1;bool active=true,runOnce=false;
 struct{uint32 event_flags=1,event_phase_mask=0;struct{uint32 pointID=1,pathID=104546;}waypoint;}event;
 struct{struct{uint32 spell=223908,flags=0;}cast;struct{uint32 field=104546;}setInstanceData64;struct{uint32 flag=0;}unitFlag;struct{int targetX=0,targetY=0,targetZ=0;bool flymode=true;}movetoz;}action;
 struct{float x=0,y=0,z=0;}target;
 uint32 GetEventType()const{return eventType;}uint32 GetActionType()const{return type;}uint32 GetTargetType()const{return targetType;}
};
using ConditionList=int;struct ConditionSourceInfo{ConditionSourceInfo(WorldObject*,WorldObject*){}};
struct Conditions{bool allow=true;bool IsObjectMeetingSmartEventConditions(uint32,uint32,uint32,Unit*,WorldObject*){return allow;}int GetConditionsForSmartEvent(uint32,uint32,uint32){return 0;}bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList){return allow;}}conditions;auto*sConditionMgr=&conditions;
struct SmartScript{
 Unit*me=nullptr;GameObject*go=nullptr;std::vector<SmartScriptHolder>mEvents;uint32 path=104546,oldPortalAttempts=0,conversation=0;bool nullTargets=false;
 WorldObject*GetBaseObject(){return me;}bool IsInPhase(uint32){return true;}bool IsUnit(WorldObject*o){return o->IsUnit();}uint32 GetPathId(){return path;}void RecalcTimer(SmartScriptHolder&,uint32 a,uint32 b){assert(a==500&&b==500);}
 ObjectList*GetTargets(SmartScriptHolder&e,Unit*){if(nullTargets)return nullptr;auto l=new ObjectList;switch(e.GetTargetType()){
#include "SelfTarget.inc"
 default:assert(false);}return l;}
 SmartScriptHolder FindLinkedEvent(uint32 id){for(auto&e:mEvents)if(e.event_id==id)return e;return {};}
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
#include "EventLoop.inc"
void SmartScript::ProcessEvent(SmartScriptHolder&e,Unit*unit,uint32 var0,uint32 var1,bool bvar,SpellInfo const*spell,GameObject*gob){
#include "EventPrefix.inc"
 switch(e.GetEventType()){
 case SMART_EVENT_LINK:ProcessAction(e,unit,var0,var1,bvar,spell,gob);break;
#include "WaypointEvent.inc"
 default:assert(false);}}
void SmartScript::ProcessAction(SmartScriptHolder&e,Unit*unit,uint32 var0,uint32 var1,bool bvar,SpellInfo const*spell,GameObject*gob){e.runOnce=true;switch(e.GetActionType()){
#include "CastAction.inc"
#include "InstanceAction.inc"
#include "NpcFlagAction.inc"
#include "MoveZAction.inc"
 case SMART_ACTION_SUMMON_CONVERSATION:++conversation;break; // optional presentation boundary
 case SMART_ACTION_SUMMON_GO:++oldPortalAttempts;break; // controlled failed insertion boundary
 default:assert(false);}
#include "LinkTail.inc"
}
void Check(bool mage){
 for(auto result:{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS}){
  Test t(mage);Unit actor;InstanceScript instance;actor.instance=&instance;instance.callback=[&](uint32 f,ObjectGuid g){assert(f==104546);t.portal.Arrived(&t.map,t.death,g);};
  SmartScript s;s.me=&actor;actor.result=result;
  SmartScriptHolder reached;reached.event_id=2;reached.type=SMART_ACTION_SUMMON_CONVERSATION;reached.link=3;
  SmartScriptHolder cast;cast.event_id=3;cast.eventType=61;cast.type=SMART_ACTION_CAST;cast.link=4;
  SmartScriptHolder old;old.event_id=4;old.eventType=61;old.type=SMART_ACTION_SUMMON_GO;old.link=5;
  SmartScriptHolder flags;flags.event_id=5;flags.eventType=61;flags.type=SMART_ACTION_SET_NPC_FLAG;flags.link=6;
  SmartScriptHolder fly;fly.event_id=6;fly.eventType=61;fly.type=SMART_ACTION_MOVE_Z;
  s.mEvents={reached,cast,old,flags,fly};s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,1);
  assert(actor.casts==1&&s.oldPortalAttempts==(result==SPELL_FAILED_BAD_TARGETS?0u:1u));assert(!t.portal.arrived);
  std::cout<<"COUNTERFACTUAL native cast/LINK: failed223908 aborts portal/cleanup; accepted/busy proceeds to controlled one-shot failed GO insertion with no retry.\n";
  actor.npcFlags=1;actor.fly=actor.gravity=true;actor.casts=0;s.oldPortalAttempts=0;cast.link=0;
  SmartScriptHolder arrival;arrival.event_id=7;arrival.type=SMART_ACTION_SET_INST_DATA64;arrival.link=5;
  s.mEvents={reached,cast,flags,fly,arrival};t.map.failInsert=1;
  s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,2);assert(!actor.casts&&!t.portal.arrived);
  s.path=1;s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,1);assert(!actor.casts&&!t.portal.arrived);s.path=104546;
  conditions.allow=false;s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,1);assert(!actor.casts);conditions.allow=true;
  s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,1);assert(actor.casts==1&&t.portal.arrived&&!t.portal.done&&!actor.npcFlags&&!actor.fly&&!actor.gravity&&actor.motion.moves>=1);
  t.ticks();t.success();auto count=t.map.creates;for(int i=0;i<10;++i)s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,1);assert(t.map.creates==count&&actor.casts==1&&!s.oldPortalAttempts);
 }
 // The scoped SELF action always has the real base actor. Null base/invoker
 // is rejected before target dispatch; a missing instance retains diagnostics.
 SmartScript s;SmartScriptHolder e;e.type=SMART_ACTION_SET_INST_DATA64;s.ProcessAction(e);Unit actor;s.me=&actor;auto before=errors;s.ProcessAction(e);assert(errors==before+1);
}
}
