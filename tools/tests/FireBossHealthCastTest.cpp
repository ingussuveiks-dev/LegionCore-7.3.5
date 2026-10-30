#include <cassert>
#include <cstdlib>
#include <sstream>
#include <string>
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
 SpellCastResult CastSpell(Unit*,uint32 id,bool triggered){assert(id==196327);assert(!triggered);++calls;if(result==SPELL_CAST_OK)++accepted;return result;}
 template<class...T>SpellCastResult CastSpell(float,float,float,uint32,T...){assert(false);return SPELL_FAILED_BAD_TARGETS;}
 void InterruptNonMeleeSpells(bool){++interrupts;}
};
struct GameObject{SpellCastResult CastSpell(Unit*,uint32){assert(false);return SPELL_FAILED_BAD_TARGETS;}};
using ObjectList=std::list<WorldObject*>;
struct SmartScriptHolder{
 uint32 entryOrGuid=99615,event_id=15,source_type=0,link=0,type=11,targetType=2,eventType=2,timer=0;bool active=true,runOnce=false,enableTimed=true;
 struct{uint32 event_flags=0,event_phase_mask=0;struct{uint32 min=0,max=80,repeatMin=0,repeatMax=0;}minMaxRepeat;}event;
 struct{struct{uint32 spell=196327,flags=0;}cast;}action;
 struct{float x=0,y=0,z=0;struct{uint32 dist=50;}playerDistance;}target;
 uint32 GetActionType()const{return type;}uint32 GetTargetType()const{return targetType;}uint32 GetEventType()const{return eventType;}uint32 GetScriptType()const{return source_type;}
};
struct ObjectGuid{static int Empty;};int ObjectGuid::Empty=0;
using ConditionList=int;struct ConditionSourceInfo{ConditionSourceInfo(void*,void*){}};
struct Conditions{ConditionList GetConditionsForSmartEvent(uint32,uint32,uint32){return 0;}bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList){return true;}}conditions;
auto sConditionMgr=&conditions;
bool IsSpellValid(SmartScriptHolder&,uint32 spell){return spell==196327;}
bool Validate(SmartScriptHolder&e){switch(e.GetActionType()){
#include "Validation.inc"
 default:return false;}return true;}
struct SmartScript{
 // This older scope has no linked-cast opt-in; unexpected use must fail.
 template<class... Args>void QueueLinkedCastRetry(Args const&...){assert(false);}
 template<class... Args>uint32 BeginLinkedCastAttempt(Args const&...){assert(false);return 0;}
 template<class... Args>bool FinishLinkedCastAttempt(Args const&...){assert(false);return false;}
 void UpdateLinkedCastRetries(uint32){}
 void ResetLinkedCastRetries(){}

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
void SmartScript::ProcessAction(SmartScriptHolder&e,Unit*unit,uint32 var0,uint32 var1,bool bvar,void*spell,GameObject*gob){
 e.runOnce=true; // actual native pre-dispatch latch; extraction verifies placement
 switch(e.GetActionType()){
#include "CastAction.inc"
 default:assert(false);}}
#include "Reset.inc"
#include "UpdateTimer.inc"
int main(int argc,char**argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;std::vector<SmartScriptHolder> rows;
 while(std::getline(input,line)){std::stringstream ss(line);std::vector<std::string> f;std::string cell;while(std::getline(ss,cell,'\t'))f.push_back(cell);assert(f.size()==31);auto n=[&](unsigned i){return uint32(std::stoul(f[i]));};if(n(0)!=99615||n(1)!=0||(n(2)!=15&&n(2)!=16))continue;
  SmartScriptHolder e;e.entryOrGuid=n(0);e.source_type=n(1);e.event_id=n(2);e.link=n(3);e.eventType=n(5);e.event.event_flags=n(8);e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};e.type=n(14);e.action.cast={n(15),n(16)};e.targetType=n(21);assert(Validate(e));rows.push_back(e);
 }assert(rows.size()==2&&rows[0].event.minMaxRepeat.max==80&&rows[1].event.minMaxRepeat.max==20);
 Unit boss,target;SmartScript s;s.me=&boss;s.victim=&target;s.mEvents=rows;
 boss.pct=100;s.Tick(1);assert(!boss.calls);boss.pct=80;s.Tick(1);assert(boss.accepted==1);s.Tick(1000);assert(boss.accepted==1);boss.pct=20;s.Tick(1);assert(boss.accepted==2);s.Tick(1000);assert(boss.accepted==2);
 boss.pct=10;
 for(auto result:{SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS}){
  s.OnReset();boss.calls=boss.accepted=0;boss.result=result;s.Tick(1);assert(boss.calls==2&&!boss.accepted);for(auto&e:s.mEvents)assert(!e.runOnce&&e.timer==500&&!e.active);
  boss.result=SPELL_CAST_OK;s.Tick(499);assert(boss.calls==2);s.Tick(1);assert(boss.calls==2);s.Tick(1);assert(boss.accepted==2);s.Tick(1000);assert(boss.accepted==2);
 }
 for(int missing=0;missing<3;++missing){s.OnReset();boss.calls=boss.accepted=0;WorldObject nonunit;s.victim=missing==2?&nonunit:nullptr;s.nullTargets=missing==1;s.Tick(1);assert(!boss.calls);for(auto&e:s.mEvents)assert(!e.runOnce&&e.timer==500);s.victim=&target;s.nullTargets=false;s.Tick(500);assert(!boss.calls);s.Tick(1);assert(boss.accepted==2);}
 s.OnReset();boss.calls=boss.accepted=0;boss.busy=true;s.Tick(1000);assert(!boss.calls);boss.busy=false;boss.combat=false;s.Tick(1000);assert(!boss.calls);boss.combat=true;boss.maximum=0;s.Tick(1000);assert(!boss.calls);boss.maximum=100;s.Tick(2);assert(boss.accepted==2);
 // The former flags0 policy consumed each one-shot even on a rejected cast.
 s.mEvents=rows;for(auto&e:s.mEvents)e.action.cast.flags=0;s.OnReset();boss.calls=boss.accepted=0;boss.result=SPELL_FAILED_BAD_TARGETS;s.Tick(1);assert(boss.calls==2&&!boss.accepted);boss.result=SPELL_CAST_OK;s.Tick(1000);assert(boss.calls==2&&!boss.accepted);
 for(int bad=0;bad<8;++bad){auto e=rows.front();if(bad==0)e.type=SMART_ACTION_INVOKER_CAST;if(bad==1)e.eventType=0;if(bad==2)e.source_type=9;if(bad==3)e.targetType=8;if(bad==4)e.event.event_flags=0;if(bad==5)e.link=1;if(bad==6)e.action.cast.flags|=SMARTCAST_TRIGGERED;if(bad==7)e.type=SMART_ACTION_CAST_CUSTOM;assert(!Validate(e));}
 std::cout<<"PASS actual Fire SQL threshold shapes + production CAST/health/UpdateTimer/reset/loader: native196327 once at80/20, missing/nonunit/null victim and rejected/busy casts retain500ms retry until acceptance; no repeats after success, combat/zeroHP/fresh reset and invalid flag scope. Original flags0 failure-loss baseline reproduced; spell/world endpoints controlled.\n";
}
