#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;
enum {SMART_EVENT_UPDATE_IC=0,SMART_EVENT_UPDATE_OOC=1,SMART_EVENT_OOC_LOS=10,SMART_EVENT_IC_LOS=26,SMART_EVENT_LINK=61,SMART_EVENT_UPDATE=60};
struct SmartScriptHolder
{
 uint32 owner=0,event_id=0,link=0,action=0,kind=0,spell=0;
 struct {struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;} event;
 uint32 GetActionType()const{return action;}uint32 GetEventType()const{return kind;}
};
struct Actor {bool combat=false;bool isInCombat(){return combat;}};
struct SmartScript
{
 std::vector<SmartScriptHolder> mEvents;
 Actor*me=nullptr;uint32 errors=0,timerMin=0,timerMax=0;
 std::vector<uint32> actions,casts;
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessAction(SmartScriptHolder const&);
 void RecalcTimer(SmartScriptHolder&,uint32 min,uint32 max){timerMin=min;timerMax=max;}
 void Init(SmartScriptHolder&e){switch(e.GetEventType()){
#include "CombatTimer.inc"
 default:break;
 }}
 void ProcessEvent(SmartScriptHolder e,void*,uint32=0,uint32=0,bool=false,void* =nullptr,void* =nullptr){
  switch(e.GetEventType()){
  case SMART_EVENT_LINK:ProcessAction(e);break;
#include "CombatDispatch.inc"
  default:break;
  }
 }
};
#define TC_LOG_ERROR(...) (++errors)
#include "FindLinkedEvent.inc"
void SmartScript::ProcessAction(SmartScriptHolder const&e)
{
 actions.push_back(e.event_id);if(e.action==11)casts.push_back(e.spell);
 void*unit=nullptr;uint32 var0=0,var1=0;bool bvar=false;void*spell=nullptr;void*gob=nullptr;
#include "LinkTail.inc"

int main(int argc,char**argv)
{
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);
 std::map<uint32,std::vector<SmartScriptHolder>> before,after;
 for(int version=0;version<2;++version){
  std::ifstream in(argv[version+1]);assert(in);SmartScriptHolder e;
  while(in>>e.owner>>e.event_id>>e.link>>e.kind>>e.action>>e.spell>>e.event.minMaxRepeat.min>>e.event.minMaxRepeat.max>>e.event.minMaxRepeat.repeatMin>>e.event.minMaxRepeat.repeatMax)
   (version?after:before)[e.owner].push_back(e);
  assert(in.eof());
 }
 uint32 cases=0;
 for(uint32 owner:{109357u,109108u,109109u}){
  assert(before.count(owner)&&after.count(owner));
  for(bool fixed:{false,true}){
   SmartScript script;Actor actor;script.me=&actor;script.mEvents=(fixed?after:before)[owner];
   uint32 root=owner==109357?0:0;
   auto row=[&](uint32 id)->SmartScriptHolder&{for(auto&e:script.mEvents)if(e.event_id==id)return e;std::abort();};
   script.ProcessAction(row(root));
   assert(script.errors==(fixed?0u:1u));
   // All prior intro actions, summons/path and six DATA sends survive.
   assert(script.actions.size()==(owner==109357?1u:7u));
   uint32 combatId=owner==109357?1:7;
   for(uint32 id:script.actions)assert(id!=combatId);
   auto combat=row(combatId);
   script.Init(combat);assert(script.timerMin==3000&&script.timerMax==5000);
   auto count=script.actions.size();script.ProcessEvent(combat,nullptr);assert(script.actions.size()==count);
   actor.combat=true;script.ProcessEvent(combat,nullptr);
   assert(script.actions.size()==count+1&&script.actions.back()==combatId&&script.casts.back()==(owner==109357?197639u:202817u));
   assert(script.timerMin==4000&&script.timerMax==6000);
   script.ProcessEvent(combat,nullptr);assert(script.actions.size()==count+2);
   script.me=nullptr;script.ProcessEvent(combat,nullptr);assert(script.actions.size()==count+2);
   ++cases;
  }
 }
 std::cout<<"PASS "<<cases<<" before/after native LINK/timer/combat-dispatch cases: bad reference reproduced, intro actions preserved, combat spells remain independently initialized/repeating and reject noncombat/missing actors; actual cast/path/recipient services and client separate\n";
}
