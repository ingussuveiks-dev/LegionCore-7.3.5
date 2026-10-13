#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>
using uint32=uint32_t;using SpellEffIndex=int;
enum {SPELL_EFFECT_HANDLE_HIT_TARGET,UNIT_STATE_ROOT=1,UNIT_STATE_CASTING=2,UNIT_STATE_CHARGING=4,UNIT_STATE_LONG_JUMP=8,CURRENT_GENERIC_SPELL=0,SPELL_INTERRUPT_FLAG_MOVEMENT=1,SPELL_AURA_CAST_WHILE_WALKING2=1,SPELL_AURA_CAST_WHILE_WALKING=2,MOVE_FLIGHT=1,SPELL_EFFECT_KNOCK_BACK_DEST=144,CREATURE_FLAG_EXTRA_IMMUNITY_KNOCKBACK=1};
uint32 getMSTime(){return 123;}
struct EffectInfo {int MiscValue=100,Effect=98;uint32 TriggerSpell=204833;};
struct SpellInfo {uint32 Id=200011,InterruptFlags=0;EffectInfo effect;EffectInfo* GetEffect(int,int){return &effect;}};
struct Spell;
struct Unit {
 bool player=false,boss=false,root=false;uint32 knockTime=0,map=1502;unsigned flags_extra=0;std::vector<int> calls;
 uint32 GetMapId(){return map;}
 Unit* ToCreature(){return player?nullptr:this;} Unit* ToPlayer(){return player?this:nullptr;}
 bool IsPlayer(){return player;} bool isWorldBoss(){return boss;} bool IsDungeonBoss(){return false;}
 Unit* GetCreatureTemplate(){return this;} bool HasUnitState(int state){return root && (state&UNIT_STATE_ROOT);}
 uint32 GetKnockBackTime(){return knockTime;}void SetKnockBackTime(uint32 t){knockTime=t;}
 Spell* GetCurrentSpell(int){return nullptr;}bool IsNonMeleeSpellCast(bool){return false;}bool HasAuraType(int){return false;}
 void InterruptNonMeleeSpells(bool){} float GetSpeedRate(int){return 1;}void GetPosition(float&x,float&y){x=y=0;}
 Unit* GetMotionMaster(){return this;}void Clear(bool){}void StopMoving(){}bool HasAura(int){return false;}
 void JumpTo(float,float){calls.push_back(1);}void KnockbackFrom(float,float,float,float){calls.push_back(1);}
 void CastSpell(Unit* target,uint32 id,bool triggered){assert(target==this && id==204833 && triggered);calls.push_back(2);}
};
struct Targets {bool HasDst(){return true;}};
struct Spell {
 int effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0,damage=100;
 Unit* unitTarget=nullptr;Unit* m_caster=nullptr;Unit* destTarget=nullptr;
 SpellInfo* m_spellInfo=nullptr;Targets m_targets;
 void EffectKnockBack(SpellEffIndex);
};
#include "Knockback.inc"
int main(){
 Unit caster;SpellInfo info;Spell spell;spell.m_caster=&caster;spell.m_spellInfo=&info;
 for(bool player:{false,true}){Unit target;target.player=player;spell.unitTarget=&target;spell.EffectKnockBack(0);assert((target.calls==std::vector<int>{1,2}));}
 for(int mode=0;mode<5;++mode){Unit target;spell.unitTarget=&target;info.Id=200011;info.effect.TriggerSpell=204833;
  if(mode==0)target.root=true;if(mode==1)target.boss=true;if(mode==2)info.Id=999;if(mode==3)info.effect.TriggerSpell=0;
  if(mode==4)caster.map=1522;
  spell.EffectKnockBack(0);assert(target.calls.size()==(mode<2?0:1));
 }
 std::cout<<"PASS: production knockback handler applies native self stun after movement to player and NPC, without affecting immune targets or other spells.\n";
}
