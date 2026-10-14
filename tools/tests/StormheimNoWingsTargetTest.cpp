#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;using SpellCastResult=int;
constexpr uint32 EFFECT_0=0,TARGET_UNIT_TARGET_ANY=25,SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1;
constexpr uint32 UNIT_FIELD_FLAGS=1,UNIT_FLAG_PVP_ATTACKABLE=8,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,
 SPELL_ATTR6_ASSIST_IGNORE_IMMUNE_FLAG=8,REP_NEUTRAL=4,CREATURE_TYPEFLAGS_TREAT_AS_RAID_UNIT=0x4000000;
#include "TargetFlags.inc"
struct SpellInfo;struct Item{};
struct Unit {struct Template {uint32 TypeFlags[2]{};} data;uint32 guid=1,flags=8,reaction=4;mutable uint32 assists=0;Unit const* ToUnit()const{return this;}Unit const* ToCreature()const{return this;}Template const* GetCreatureTemplate()const{return &data;}uint32 GetGUID()const{return guid;}uint32 GetCritterGUID()const{return 0;}
 bool HasFlag(uint32,uint32 flag)const{return (flags&flag)!=0;}uint32 GetReactionTo(Unit const*)const{return reaction;}
 bool _IsValidAttackTarget(Unit const*,SpellInfo const*)const{return false;}bool _IsValidAssistTarget(Unit const*,SpellInfo const*)const;
 bool IsInPartyWith(Unit const*)const{return false;}bool IsInRaidWith(Unit const*)const{return false;}bool IsOnVehicle(Unit const*)const{return false;}};
using WorldObject=Unit;
struct SpellInfo {struct Effect {uint32 TargetA=21;} effect;Effect* Effects[1]{&effect};bool HasAttribute(uint32)const{return false;}uint32 GetExplicitTargetMask()const{return effect.TargetA==21?TARGET_FLAG_UNIT_ALLY:TARGET_FLAG_UNIT;}SpellCastResult CheckExplicitTarget(Unit const*,WorldObject const*,Item const*)const;};
bool Unit::_IsValidAssistTarget(Unit const* target,SpellInfo const* bySpell)const{++assists;
#include "AssistImmunity.inc"
#include "AssistReaction.inc"
 return true;
}
#include "ExplicitTarget.inc"
int main(){Unit player,point;point.guid=2;point.flags=768;SpellInfo gun;assert(gun.CheckExplicitTarget(&player,&point,nullptr)==SPELL_FAILED_BAD_TARGETS);assert(player.assists==1);
 auto spellInfo=&gun;
#include "GunTargetFix.inc"
 assert(gun.effect.TargetA==25&&gun.CheckExplicitTarget(&player,&point,nullptr)==SPELL_CAST_OK);assert(player.assists==1);
 SpellInfo unrelated;assert(unrelated.CheckExplicitTarget(&player,&point,nullptr)==SPELL_FAILED_BAD_TARGETS);
 assert(gun.CheckExplicitTarget(&player,nullptr,nullptr)==SPELL_FAILED_BAD_TARGETS);
 std::cout<<"PASS: production gun correction through native explicit-target validation; protected/neutral point reaches scripted checks, unrelated ally spells remain protected.\n";
}
