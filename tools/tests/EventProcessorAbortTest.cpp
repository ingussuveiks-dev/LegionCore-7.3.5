#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
using uint32=uint32_t;using uint64=uint64_t;
#include "EventProcessor.inc"
#include "EventMethods.inc"
struct State{bool referenced=true,finished=false;unsigned executes=0,aborts=0,effects=0,destroyed=0,premature=0;uint64 add=0,due=0;};
struct Held:BasicEvent{State& state;std::function<void()> onAbort;explicit Held(State& s):state(s){}~Held(){++state.destroyed;state.premature+=state.referenced;state.add=m_addTime;state.due=m_execTime;}bool Execute(uint64,uint32)override{++state.executes;return true;}bool IsDeletable()const override{return !state.referenced;}void Abort(uint64)override{++state.aborts;if(!state.finished){state.finished=true;++state.effects;if(onAbort)onAbort();}}};
struct Once:BasicEvent{unsigned& count;explicit Once(unsigned& c):count(c){}bool Execute(uint64,uint32)override{++count;return true;}};
static unsigned cases=0;
static void Check(bool current,bool old){
#ifdef EVENT_ABORT_BASELINE
 assert(old);
#else
 assert(current);
#endif
 ++cases;
}
int main(){
 // Finished-but-current spells survive KillAllEvents(false) until the unit's
 // later _UpdateSpells releases the reference. Cancellation must not execute.
 for(unsigned route=0;route<3;++route){State state;EventProcessor queue;queue.Update(50);auto event=new Held(state);queue.AddEvent(event,queue.CalculateTime(1));if(route==0)queue.KillAllEvents(false);if(route==1)event->to_Abort=true;if(route==2){state.finished=true;queue.KillAllEvents(false);}queue.Update(1);Check(!state.destroyed&&!state.executes,state.destroyed==1&&state.premature==1&&!state.executes);
#ifndef EVENT_ABORT_BASELINE
  auto aborts=state.aborts;queue.Update(0);assert(state.aborts==aborts&&!state.destroyed);state.referenced=false;queue.Update(1);assert(state.destroyed==1&&!state.premature&&state.add==50&&state.due==52);queue.Update(500);assert(state.destroyed==1&&!state.executes&&state.effects==(route==2?0u:1u));
#endif
 }
 {State state;EventProcessor queue;auto event=new Held(state);queue.AddEvent(event,1000);queue.KillAllEvents(false);unsigned normal=0;queue.AddEvent(new Once(normal),1);queue.Update(1);assert(normal==1&&!state.destroyed);queue.Update(999);Check(!state.destroyed,state.destroyed==1&&state.premature==1);
#ifndef EVENT_ABORT_BASELINE
  state.referenced=false;queue.Update(1);assert(state.destroyed==1&&!state.premature);
#endif
 }
 // Ordinary deletable/direct-abort events (including vehicle joins) retain
 // their immediate Abort/destructor behavior and never Execute.
 {State state;state.referenced=false;EventProcessor queue;auto event=new Held(state);event->to_Abort=true;queue.AddEvent(event,1);queue.Update(1);assert(state.destroyed==1&&state.effects==1&&!state.executes&&!state.premature);++cases;}
 {State state;EventProcessor queue;auto event=new Held(state);event->onAbort=[&]{state.referenced=false;};queue.AddEvent(event,1);queue.KillAllEvents(false);assert(state.destroyed==1&&!state.premature&&state.effects==1);++cases;}
 {State state;EventProcessor queue;auto event=new Held(state);unsigned added=0;event->onAbort=[&]{queue.AddEvent(new Once(added),queue.CalculateTime(1));};queue.AddEvent(event,1);event->to_Abort=true;queue.Update(1);queue.Update(1);Check(!state.destroyed&&added==1&&state.effects==1,state.destroyed==1&&state.premature==1&&added==1);
#ifndef EVENT_ABORT_BASELINE
  state.referenced=false;queue.Update(1);assert(state.destroyed==1&&!state.premature&&state.effects==1);
#endif
 }
 {State state;{EventProcessor queue;queue.AddEvent(new Held(state),1);queue.KillAllEvents(true);assert(state.destroyed==1);}assert(state.destroyed==1);++cases;}
 {State state;{EventProcessor queue;queue.AddEvent(new Held(state),1);}assert(state.destroyed==1);++cases;}
 {State states[32];EventProcessor queue;for(auto& state:states){auto event=new Held(state);event->to_Abort=true;queue.AddEvent(event,1);}queue.Update(1);
#ifndef EVENT_ABORT_BASELINE
  for(unsigned t=0;t<100;++t)queue.Update(1);for(auto& state:states){assert(!state.destroyed&&!state.executes&&state.effects==1);state.referenced=false;}queue.Update(1);for(auto& state:states)assert(state.destroyed==1&&!state.premature);
#else
  for(auto& state:states)assert(state.destroyed==1&&state.premature==1);
#endif
  ++cases;
 }
#ifdef EVENT_ABORT_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native EventProcessor contexts: aborted held/current events delete prematurely; normal/forced controls retained\n";
#else
 std::cout<<"PASS "<<cases<<" ASAN whole native EventProcessor contexts: defer aborted referenced events, no Execute, unchanged add-time, next-tick exactly-once destruction, unrelated scheduling/direct vehicle-like abort/reentrant abort/forced cleanup and32 retained events\n";
#endif
}
