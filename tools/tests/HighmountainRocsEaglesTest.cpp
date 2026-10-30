#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32=uint32_t;
#define TC_LOG_DEBUG(...) ((void)0)
enum {SMART_EVENT_SPELLHIT=8,SMART_EVENT_SPELLHIT_TARGET=31,SMART_EVENT_LINK=61,SMART_EVENT_RESET=25,
 SMART_EVENT_FLAG_NOT_REPEATABLE=1,SMART_EVENT_FLAG_DONT_RESET=256,SMART_EVENT_FLAG_EVENT_NON_COMBAT=512,SMART_ACTION_MOVE_Z=202};
bool roll_chance_i(uint32){return true;}
uint32 urand(uint32 min,uint32 max){assert(min<=max);return max;}
struct ObjectGuid {int value=0;static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
struct Unit {
 bool quest=true;int guid=1;ObjectGuid GetGUID()const{return {guid};}
};
struct GameObject {};
struct SpellInfo {uint32 Id;struct Misc {struct {uint32 SchoolMask=1;} MiscData;} misc;Misc const* GetMisc()const{return &misc;}};
struct Motion {float x=0,y=0,z=0;bool cleared=false;void Clear(){cleared=true;}void MovePoint(int,float a,float b,float c){x=a;y=b;z=c;}};
struct Creature:Unit {
 bool flying=false,noGravity=false,expired=false;Motion motion;
 bool IsQuestObjectExpired()const{return expired;}
 bool isInCombat()const{return false;}Motion* GetMotionMaster(){return &motion;}
 void SetCanFly(bool on){flying=on;}void SetDisableGravity(bool on){noGravity=on;}
 void GetPosition(float& x,float& y,float& z,float& o){x=4970.65f;y=3950.95f;z=799.212f;o=1.29f;}
};
struct SmartScriptHolder {
 uint32 type=SMART_EVENT_SPELLHIT,timer=0,event_id=0,source_type=0;int entryOrGuid=94991;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;struct {uint32 spell=188432,school=0,cooldownMin=0,cooldownMax=0;} spellHit;} event;
 struct {struct {int targetX=0,targetY=0,targetZ=15,flymode=0;} movetoz;} action;
 uint32 GetEventType()const{return type;}
};
using ConditionList=bool;
struct ConditionSourceInfo {Unit* actor;ConditionSourceInfo(Unit* unit,Creature*):actor(unit){}};
struct Conditions {
 bool enabled=true;
 bool GetConditionsForSmartEvent(int,uint32,uint32)const{return enabled;}
 bool IsObjectMeetToConditions(ConditionSourceInfo const& info,bool guarded)const{return !guarded || (info.actor && info.actor->quest);}
} conditions;
auto sConditionMgr=&conditions;
struct SmartScript {
 // This older scope has no linked-cast opt-in; unexpected use must fail.
 template<class... Args>void QueueLinkedCastRetry(Args const&...){assert(false);}
 template<class... Args>uint32 BeginLinkedCastAttempt(Args const&...){assert(false);return 0;}
 template<class... Args>bool FinishLinkedCastAttempt(Args const&...){assert(false);return false;}
 void UpdateLinkedCastRetries(uint32){}
 void ResetLinkedCastRetries(){}

 Creature creature;Creature* me=&creature;ObjectGuid mLastInvoker;Unit* invoker=nullptr;unsigned launches=0,resetCalls=0;
 std::vector<SmartScriptHolder> mEvents{SmartScriptHolder{}};
 Creature* GetBaseObject(){return me;}bool IsInPhase(uint32)const{return true;}
 Unit* GetLastInvoker(){return invoker;}
 void SetPhase(int){}void ResetBaseObject(){}void InitTimer(SmartScriptHolder& e){e.timer=0;e.active=true;}
 void ProcessEventsFor(int event){assert(event==SMART_EVENT_RESET);++resetCalls;}
 void OnReset();void RecalcTimer(SmartScriptHolder&,uint32,uint32);
 void ProcessAction(SmartScriptHolder&,Unit*,uint32,uint32,bool,SpellInfo const*,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit*,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void Move(SmartScriptHolder& e){switch(SMART_ACTION_MOVE_Z){
#include "EagleMovement.inc"
 }}
};
#include "EagleEventEntry.inc"
    switch(e.GetEventType()){
#include "EagleSpellHit.inc"
    }
}
#include "EagleActionEntry.inc"
    // World service boundary: SQL action 80 starts the timed rescue list.
    invoker=unit;++launches;
}
#include "EagleTimer.inc"
#include "EagleReset.inc"
int main(){
 Unit first,second;second.guid=2;SpellInfo release{188432},wrong{188434};
 SmartScript legacy;auto& old=legacy.mEvents[0];
 legacy.ProcessEvent(old,&first,0,0,false,&release);
 legacy.ProcessEvent(old,&first,0,0,false,&release);
 assert(legacy.launches==2); // The legacy flag permits restarting the rescue.
 SmartScript fixed;auto& event=fixed.mEvents[0];event.event.event_flags=SMART_EVENT_FLAG_NOT_REPEATABLE;
 first.quest=false;fixed.ProcessEvent(event,&first,0,0,false,&release);
 assert(!event.runOnce&&!fixed.launches); // Invalid player cannot consume the captive.
 first.quest=true;fixed.ProcessEvent(event,&first,0,0,false,&wrong);fixed.ProcessEvent(event,&first);
 assert(!event.runOnce&&!fixed.launches);
 fixed.creature.expired=true;fixed.ProcessEvent(event,&first,0,0,false,&release);
 assert(!event.runOnce&&!fixed.launches);
 fixed.creature.expired=false;
 fixed.ProcessEvent(event,&first,0,0,false,&release);
 assert(event.runOnce&&fixed.launches==1&&fixed.mLastInvoker.value==1);
 for(int i=0;i<8;++i)fixed.ProcessEvent(event,&first,0,0,false,&release);
 fixed.ProcessEvent(event,&second,0,0,false,&release);
 assert(fixed.launches==1&&fixed.mLastInvoker.value==1);
 fixed.OnReset();assert(!event.runOnce&&fixed.resetCalls==1);
 fixed.ProcessEvent(event,&second,0,0,false,&release);assert(fixed.launches==2&&fixed.mLastInvoker.value==2);
 SmartScriptHolder movement;
 movement.action.movetoz={15,0,0,0};fixed.Move(movement);
 assert(fixed.creature.motion.x==4985.65f&&fixed.creature.motion.z==799.212f);
 movement.action.movetoz={0,0,15,0};fixed.Move(movement);
 assert(fixed.creature.motion.cleared&&fixed.creature.flying&&fixed.creature.noGravity);
 assert(fixed.creature.motion.x==4970.65f&&fixed.creature.motion.y==3950.95f&&fixed.creature.motion.z==814.212f);
 std::cout<<"PASS: actual SmartAI dispatch/action gates reproduce repeat start, reject invalid quest/wrong spell, preserve first invoker, block repeated/concurrent release until reset, and move upward with flight/gravity flags.\n";
}
