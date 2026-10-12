#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
enum AreaTriggerActionMoment { AT_ACTION_MOMENT_ENTER=1 };
struct Unit {
 uint32 entry=95153;bool alive=true,combat=true,hostile=true;
 uint32 GetEntry()const{return entry;}bool IsAlive()const{return alive;}bool isInCombat()const{return combat;}
 bool IsValidAttackTarget(Unit* target)const{return target!=this&&target->hostile;}
};
struct AreaTrigger {
 uint32 spell=192179;Unit* caster=nullptr;unsigned despawns=0;
 uint32 GetSpellId()const{return spell;}Unit* GetCaster()const{return caster;}void Despawn(){++despawns;}
};
struct AreaTriggerAI {
 AreaTrigger* at;explicit AreaTriggerAI(AreaTrigger* a):at(a){}
 virtual bool IsValidTarget(Unit*,Unit*,AreaTriggerActionMoment){return false;}
 virtual void OnUpdate(uint32){}
};
#include "WitchqueenArea.inc"
int main(){
 Unit queen,target;AreaTrigger area;area.caster=&queen;at_highmountain_shredding_cyclone ai(&area);
 for(uint32 spell=192179;spell<=192183;++spell){
  area.spell=spell;assert(ai.IsValidTarget(&queen,&target,AT_ACTION_MOMENT_ENTER));
  assert(!ai.IsValidTarget(&queen,&queen,AT_ACTION_MOMENT_ENTER));
  assert(!ai.IsValidTarget(nullptr,&target,AT_ACTION_MOMENT_ENTER));
  assert(!ai.IsValidTarget(&queen,nullptr,AT_ACTION_MOMENT_ENTER));
  target.hostile=false;assert(!ai.IsValidTarget(&queen,&target,AT_ACTION_MOMENT_ENTER));target.hostile=true;
  target.alive=false;assert(!ai.IsValidTarget(&queen,&target,AT_ACTION_MOMENT_ENTER));target.alive=true;
  queen.combat=false;assert(!ai.IsValidTarget(&queen,&target,AT_ACTION_MOMENT_ENTER));ai.OnUpdate(1);queen.combat=true;
  queen.alive=false;assert(!ai.IsValidTarget(&queen,&target,AT_ACTION_MOMENT_ENTER));ai.OnUpdate(1);queen.alive=true;
  queen.entry=79490;assert(!ai.IsValidTarget(&queen,&target,AT_ACTION_MOMENT_ENTER));ai.OnUpdate(1);queen.entry=95153;
 }
 assert(area.despawns==10);ai.OnUpdate(1);assert(area.despawns==10);
 area.caster=nullptr;ai.OnUpdate(1);assert(area.despawns==11);area.caster=&queen;
 for(uint32 spell:{166311u,166327u,166332u,166344u,166345u,192178u,192184u,234807u}){
  area.spell=spell;assert(!ai.IsValidTarget(&queen,&target,AT_ACTION_MOMENT_ENTER));
  queen.combat=false;ai.OnUpdate(1);queen.combat=true;assert(area.despawns==11);
 }
 std::cout<<"PASS: actual cyclone script accepts five native children/hostile living targets only; excludes caster, friendlies, dead/null, other creatures and shared older spells; cleans up death/evade/lost caster.\n";
}
