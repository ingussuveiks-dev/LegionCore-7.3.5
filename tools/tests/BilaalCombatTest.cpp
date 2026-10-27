#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
#include "NativeEnums.inc"
constexpr uint32 UNIT_STATE_CASTING=1;
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS};
#include "CastFlags.inc"
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) ((void)0)
#define ENSURE_AI(type,ai) (ai)
struct SmartAI{void SetCombatMove(bool){}};
struct Unit;
struct WorldObject{virtual ~WorldObject()=default;virtual Unit* ToUnit(){return nullptr;}virtual bool IsUnit(){return false;}int GetGUID(){return 0;} };
struct Unit:WorldObject{
 bool combat=true,aura=false,busy=false;float pct=100;uint64 maximum=100;unsigned calls=0,accepted=0,interrupts=0;SpellCastResult result=SPELL_CAST_OK;SmartAI ai;
 Unit* ToUnit()override{return this;}bool IsUnit()override{return true;}
 bool isInCombat(){return combat;}uint64 GetMaxHealth(){return maximum;}float GetHealthPct(){return pct;}
 bool HasAura(uint32){return aura;}SmartAI* AI(){return &ai;}void GetPosition(float&x,float&y,float&z){x=y=z=0;}
 bool HasUnitState(uint32){return busy;}
 SpellCastResult CastSpell(Unit*,uint32 id,bool triggered){assert(id==209863);assert(!triggered);++calls;if(result==SPELL_CAST_OK)++accepted;return result;}
 template<class...T>SpellCastResult CastSpell(float,float,float,uint32,T...){assert(false);return SPELL_FAILED_BAD_TARGETS;}
 void InterruptNonMeleeSpells(bool){++interrupts;}
};
struct GameObject{SpellCastResult CastSpell(Unit*,uint32){assert(false);return SPELL_FAILED_BAD_TARGETS;}};
using ObjectList=std::list<WorldObject*>;
struct SmartScriptHolder{
 uint32 entryOrGuid=104502,event_id=2,source_type=0,link=0,type=11,targetType=2,eventType=2,timer=0;bool active=true,runOnce=false,enableTimed=true;
 struct{uint32 event_flags=0,event_phase_mask=0;struct{uint32 min=0,max=80,repeatMin=0,repeatMax=0;}minMaxRepeat;}event;
 struct{struct{uint32 spell=209863,flags=0;}cast;}action;
 struct{float x=0,y=0,z=0;struct{uint32 dist=50;}playerDistance;}target;
 uint32 GetActionType()const{return type;}uint32 GetTargetType()const{return targetType;}uint32 GetEventType()const{return eventType;}uint32 GetScriptType()const{return source_type;}
};
struct ObjectGuid{static int Empty;};int ObjectGuid::Empty=0;
using ConditionList=int;struct ConditionSourceInfo{ConditionSourceInfo(void*,void*){}};
struct Conditions{ConditionList GetConditionsForSmartEvent(uint32,uint32,uint32){return 0;}bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList){return true;}}conditions;
auto sConditionMgr=&conditions;
bool IsSpellValid(SmartScriptHolder&,uint32 spell){return spell==209863;}
bool Validate(SmartScriptHolder&e){switch(e.GetActionType()){
#include "Validation.inc"
 default:return false;}return true;}
struct SmartScript{
 Unit* me=nullptr;GameObject* go=nullptr;WorldObject* victim=nullptr;bool nullTargets=false;std::vector<SmartScriptHolder>mEvents,mTimedActionList;int mLastInvoker=0;
 bool IsInPhase(uint32){return true;}void* GetBaseObject(){return me;}bool IsUnit(WorldObject*o){return o->IsUnit();}
 ObjectList* GetTargets(SmartScriptHolder&,Unit*){return nullTargets?nullptr:new ObjectList(victim?ObjectList{victim}:ObjectList{});}
 void RecalcTimer(SmartScriptHolder&e,uint32 minimum,uint32 maximum){assert(minimum==maximum);e.timer=minimum;e.active=!minimum;}
 void ProcessAction(SmartScriptHolder&e,Unit*unit=nullptr,uint32=0,uint32=0,bool=false,void* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&e,Unit*unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,void*spell=nullptr,GameObject*gob=nullptr){
#include "HealthEvent.inc"
 }
 void SetPhase(int){}void ResetBaseObject(){}void InitTimer(SmartScriptHolder&e){e.timer=0;e.active=true;}void ProcessEventsFor(int){}
 void OnReset();void UpdateTimer(SmartScriptHolder&e,uint32 diff);
 void Tick(uint32 diff){for(auto&e:mEvents)UpdateTimer(e,diff?diff:1);}
};
void SmartScript::ProcessAction(SmartScriptHolder&e,Unit*unit,uint32,uint32,bool,void*,GameObject*){
 e.runOnce=true; // actual native pre-dispatch latch; extraction verifies placement
 switch(e.GetActionType()){
#include "CastAction.inc"
 default:assert(false);}}
#include "Reset.inc"
#include "UpdateTimer.inc"
int main(int argc,char**argv){assert(argc==2);std::ifstream input(argv[1]);assert(input);std::vector<SmartScriptHolder>original;uint32 entry,id,threshold;while(input>>entry>>id>>threshold){SmartScriptHolder e;e.entryOrGuid=entry;e.event_id=id;e.event.minMaxRepeat.max=threshold;original.push_back(e);}assert(original.size()==6);
 for(uint32 actor:{104502u,106708u}){
  Unit boss,target;SmartScript s;s.me=&boss;s.victim=&target;for(auto e:original)if(e.entryOrGuid==actor)s.mEvents.push_back(e);
  boss.pct=10;for(int i=0;i<20;++i)s.Tick(50);assert(boss.accepted==60);
  std::cout<<"COUNTERFACTUAL "<<actor<<": original three unbounded health events accept60 casts in20 eligible updates; actual spell effects are controlled endpoints.\n";
  boss.calls=boss.accepted=0;for(auto&e:s.mEvents){e.event.event_flags=1;e.action.cast.flags=SMARTCAST_RETRY_HEALTH_THRESHOLD;assert(Validate(e));}s.OnReset();
  boss.pct=81;s.Tick(0);assert(!boss.calls);boss.pct=80;s.Tick(0);assert(boss.accepted==1);for(int i=0;i<20;++i)s.Tick(100);assert(boss.accepted==1);
  boss.pct=50;s.Tick(0);assert(boss.accepted==2);boss.pct=20;s.Tick(0);assert(boss.accepted==3);for(int i=0;i<20;++i)s.Tick(100);assert(boss.accepted==3);
  boss.pct=100;s.OnReset();s.Tick(0);assert(boss.accepted==3);boss.pct=1;s.Tick(0);assert(boss.accepted==6); // scaled one-hit threshold crossing; no progress from this policy
  for(auto result:{SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS}){s.OnReset();boss.calls=boss.accepted=0;boss.result=result;s.Tick(0);assert(boss.calls==3&&!boss.accepted);for(auto&e:s.mEvents)assert(!e.runOnce&&e.timer==500&&!e.active);boss.result=SPELL_CAST_OK;s.Tick(499);assert(boss.calls==3);s.Tick(1);assert(boss.calls==3);s.Tick(1);assert(boss.accepted==3);s.Tick(1000);assert(boss.accepted==3);}
  for(int missing=0;missing<3;++missing){s.OnReset();boss.calls=boss.accepted=0;WorldObject nonunit;s.victim=missing==2?&nonunit:nullptr;s.nullTargets=missing==1;s.Tick(0);assert(!boss.calls);for(auto&e:s.mEvents)assert(!e.runOnce&&e.timer==500);s.victim=&target;s.nullTargets=false;s.Tick(500);assert(!boss.calls);s.Tick(1);assert(boss.accepted==3);}
  s.OnReset();boss.calls=boss.accepted=0;boss.busy=true;s.Tick(1000);assert(!boss.calls);boss.busy=false;boss.combat=false;s.Tick(1000);assert(!boss.calls);boss.combat=true;boss.maximum=0;s.Tick(1000);assert(!boss.calls);boss.maximum=100;s.Tick(2);assert(boss.accepted==3);
  for(int bad=0;bad<8;++bad){auto e=s.mEvents.front();if(bad==0)e.type=SMART_ACTION_INVOKER_CAST;if(bad==1)e.eventType=0;if(bad==2)e.source_type=9;if(bad==3)e.targetType=8;if(bad==4)e.event.event_flags=0;if(bad==5)e.link=1;if(bad==6)e.action.cast.flags|=SMARTCAST_TRIGGERED;if(bad==7)e.type=SMART_ACTION_CAST_CUSTOM;assert(!Validate(e));}
  {auto aura=s.mEvents.front();aura.type=SMART_ACTION_ADD_AURA;aura.action.cast.flags=256;aura.event.event_flags=0;assert(Validate(aura));} // action75 param2 is an amount, not cast flags
  // Existing unflagged behavior, including one-shot failure consumption and
  // SPELL_IN_PROGRESS handling, remains identical outside this opt-in.
  s.mEvents={original.front()};s.mEvents.front().event.event_flags=1;s.OnReset();boss.result=SPELL_FAILED_BAD_TARGETS;boss.calls=0;s.Tick(0);s.Tick(500);assert(boss.calls==1&&s.mEvents.front().runOnce);s.OnReset();boss.result=SPELL_FAILED_SPELL_IN_PROGRESS;boss.calls=0;s.Tick(0);s.Tick(1000);assert(boss.calls==1);
 }
 std::cout<<"PASS actual native health gate/ordinary cast/Reset: both native Bilaal shapes, three accepted thresholds per combat, rejected/busy/missing victim500ms retries, reset/skip thresholds/combat gates and eight loader refusals; ordinary aura and legacy callers unchanged. Death/credit/real damage/client not simulated.\n";
}
