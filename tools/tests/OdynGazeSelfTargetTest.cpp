#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
using uint32=uint32_t;
#define TC_LOG_DEBUG(...) ((void)0)
enum {CONDITION_CLASS=15};
enum { TARGET_FLAG_UNIT_MASK=2,TARGET_FLAG_CORPSE_MASK=4,TARGET_FLAG_GAMEOBJECT_MASK=8,
 TARGET_FLAG_UNIT_ENEMY=16,TARGET_FLAG_UNIT=32,TARGET_FLAG_UNIT_RAID=64,
 TARGET_FLAG_UNIT_PARTY=128,TARGET_FLAG_UNIT_ALLY=256,TARGET_FLAG_DEST_LOCATION=512,TARGET_FLAG_SOURCE_LOCATION=1024,SPELL_CAST_OK=0 };
struct Player;
struct Unit {
 uint32 mask=1;Unit*ToUnit(){return this;}bool IsUnit(){return true;}void*ToGameObject(){return nullptr;}void*ToCorpse(){return nullptr;}
 bool IsCreature(){return false;}Player*ToPlayer(){return nullptr;}Unit*getVictim(){return nullptr;}
 uint32 getClassMask(){return mask;}uint32 GetSelection(){return 0;}
};
using WorldObject=Unit;
struct Player:Unit{};
namespace ObjectAccessor { Unit*GetUnit(Unit&,uint32){return nullptr;} }
struct SpellInfo {uint32 GetExplicitTargetMask(){return 0;}int CheckExplicitTarget(Unit*,Unit*){return SPELL_CAST_OK;}};
struct SpellCastTargets {
 Unit*target=nullptr;Unit*GetUnitTarget()const{return target;}Unit*GetObjectTarget()const{return target;}
 void RemoveObjectTarget(){target=nullptr;}void SetUnitTarget(Unit*p){target=p;}
 bool HasDst(){return false;}bool HasSrc()const{return false;}void SetDst(Unit&){}void SetSrc(Unit&){}void RemoveDst(){}void RemoveSrc(){}
};
struct Guid {static uint32 Empty;};uint32 Guid::Empty=0;
using ObjectGuid=Guid;
// GUID identity is irrelevant to removal; retain it as a concrete endpoint.
#define GetGUID() GetSelection()
struct Spell {SpellCastTargets m_targets;Unit*m_originalTarget=nullptr;uint32 m_originalTargetGUID=0;Unit*m_caster;SpellInfo*m_spellInfo;void InitExplicitTargets(SpellCastTargets const&);};
#include "ExplicitTargets.inc"
struct Condition {uint32 ConditionTarget=0,ConditionValue1=1;bool Meets(Unit*caster,Unit*target){Unit*object=ConditionTarget?target:caster;
 // Exact native null-object and class predicate, extracted by the runner.
#include "ConditionObject.inc"
 bool condMeets=false;
 switch(15){
#include "ClassCondition.inc"
 }
 return condMeets;}};
int main(int argc,char**argv){
 assert(argc==2);std::ifstream rows(argv[1]);assert(rows);std::vector<Condition> fixed;
 uint32 target,value;while(rows>>target>>value)fixed.push_back({target,value});assert(fixed.size()==1);
 for(uint32 mask:{1u,2u,4u,8u,16u,32u,64u,128u,256u,512u,1024u,2048u}){
  Unit player;player.mask=mask;SpellInfo info;Spell spell{{},nullptr,0,&player,&info};
  SpellCastTargets targets{&player};spell.InitExplicitTargets(targets);assert(!spell.m_targets.GetObjectTarget());
  Condition oldCaster{0,1},oldTarget{1,1};
  bool before=oldCaster.Meets(&player,spell.m_targets.GetObjectTarget())&&oldTarget.Meets(&player,spell.m_targets.GetObjectTarget());
  assert(!before);
  bool after=true;for(auto&c:fixed)after=after&&c.Meets(&player,spell.m_targets.GetObjectTarget());
  assert(after==(mask==1));
 }
 std::cout<<"PASS native explicit-target removal + null/class conditions: old Warrior self cast rejected; repaired Warrior admitted, other11 classes rejected. Real spell/aura/client UI remain separate checks.\n";
}
