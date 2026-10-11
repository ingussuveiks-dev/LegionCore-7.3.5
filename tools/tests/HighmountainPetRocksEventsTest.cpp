#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
enum {SMART_EVENT_SPELLHIT=8,SMART_EVENT_ON_SPELLCLICK=73,SMART_EVENT_SPELLHIT_TARGET=31};
uint32 urand(uint32 min,uint32 max){assert(min<=max);return max;} // Reproduce the possible maximum block.
struct Unit {unsigned credits=0;};struct GameObject{};
struct SpellInfo {uint32 Id;struct Misc {struct {uint32 SchoolMask=1;} MiscData;} misc;Misc const* GetMisc()const{return &misc;}};
struct SmartScriptHolder {
 uint32 type=0,timer=0;bool active=true;
 struct {struct {uint32 cooldownMin=0,cooldownMax=0;} spellclick;struct {uint32 spell=0,school=0,cooldownMin=0,cooldownMax=0;} spellHit;} event;
};
struct SmartScript {
 void RecalcTimer(SmartScriptHolder&,uint32,uint32);
 void ProcessAction(SmartScriptHolder&,Unit* unit,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void Dispatch(SmartScriptHolder& e,Unit* unit,SpellInfo const* spell=nullptr){
  if(!e.active)return; // Same initial gate as production ProcessEvent.
  uint32 var0=0,var1=0;bool bvar=false;GameObject* gob=nullptr;
  switch(e.type){
#include "PetRocksEvents.inc"
  }
 }
};
void SmartScript::ProcessAction(SmartScriptHolder&,Unit* unit,uint32,uint32,bool,SpellInfo const*,GameObject*){assert(unit);++unit->credits;}
#include "PetRocksTimer.inc"
int main(){
 SmartScript script;Unit first,second;SmartScriptHolder old;old.type=SMART_EVENT_ON_SPELLCLICK;old.event.spellclick.cooldownMin=189686;
 script.Dispatch(old,&first);script.Dispatch(old,&second);assert(first.credits==1&&second.credits==0&&!old.active&&old.timer==189686);
 for(uint32 spellId:{189687u,189686u,189686u}){
  first={};second={};SmartScriptHolder corrected;corrected.type=SMART_EVENT_SPELLHIT;corrected.event.spellHit.spell=spellId;SpellInfo wrong{1},matching{spellId};
  script.Dispatch(corrected,&first,&wrong);script.Dispatch(corrected,&first);assert(!first.credits);
  script.Dispatch(corrected,&first,&matching);script.Dispatch(corrected,&second,&matching);assert(first.credits==1&&second.credits==1&&corrected.active&&!corrected.timer);
 }
 std::cout<<"PASS: actual SmartAI cases/timer reproduce legacy 189686ms shared block and verify both players immediately receive matching-spell actions, with wrong/missing spells rejected.\n";
}
