#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using SpellEffIndex=int;
enum {DUMMY_TRIGGER_COOLDOWN=2,DUMMY_TRIGGER_CAST_OR_REMOVE=6};
#define TC_LOG_DEBUG(...) ((void)0)
struct Unit {
 bool world=true,cooldown=false;unsigned casts=0;uint32 castId=0;Unit* castTarget=nullptr;
 Unit* GetUnitForLinkedSpell(Unit*,Unit* target,int,Unit*){return target;}
 bool IsInWorld()const{return world;}bool HasSpellCooldown(uint32)const{return cooldown;}
 bool HasAura(uint32)const{return false;}
 void CastSpell(Unit* target,uint32 id,bool triggered,void*){assert(triggered);++casts;castId=id;castTarget=target;}
};
struct SpellDummyTrigger {
 int32 spell_trigger=0;int option=6,target=0,caster=0,targetaura=0,effectmask=1,handlemask=8,aura=0,group=0;
 float bp0=0,bp1=0,bp2=0;
};
struct SpellManager {
 std::map<uint32,std::vector<SpellDummyTrigger>> rows;
 auto GetSpellDummyTrigger(uint32 id)const -> std::vector<SpellDummyTrigger> const* {auto it=rows.find(id);return it==rows.end()?nullptr:&it->second;}
} manager;
auto sSpellMgr=&manager;
struct SpellInfo {uint32 Id;};
struct Targets {Unit* target=nullptr;Unit* GetUnitTarget()const{return target;}};
struct Spell {
 SpellInfo const* m_spellInfo;Unit* m_caster;Unit* unitTarget;Unit* m_originalCaster=nullptr;
 Targets m_targets;float damage=0;int effectHandleMode=3;void* m_CastItem=nullptr;
 bool SpellDummyTriggered(SpellEffIndex);
};
#include "ThreeDummy.inc"
int main(){
 for(auto pair:{std::pair<uint32,uint32>{194241,215588},{215591,215592}}){
  Unit caster,target,other;SpellInfo info{pair.first};Spell spell{&info,&caster,&target};spell.damage=float(pair.second);
  assert(!spell.SpellDummyTriggered(0)&&!caster.casts); // Old missing mapping does nothing.
  manager.rows[pair.first]={{int32(pair.second)}};
  spell.effectHandleMode=0;assert(!spell.SpellDummyTriggered(0)&&!caster.casts);
  spell.effectHandleMode=3;assert(!spell.SpellDummyTriggered(1)&&!caster.casts);
  target.world=false;assert(!spell.SpellDummyTriggered(0)&&!caster.casts);target.world=true;
  caster.world=false;assert(!spell.SpellDummyTriggered(0)&&!caster.casts);caster.world=true;
  assert(spell.SpellDummyTriggered(0)&&caster.casts==1&&caster.castId==pair.second&&caster.castTarget==&target);
  spell.unitTarget=&other;assert(spell.SpellDummyTriggered(0)&&caster.casts==2&&caster.castTarget==&other);
  // Option 6 uses CastSpell, preserving native child damage instead of using the spell ID as damage.
 }
 std::cout<<"PASS: actual dummy-trigger filter/option 6 reproduces missing links, rejects wrong handle/effect and out-of-world units, casts the native child once on each hit target without overriding damage.\n";
}
