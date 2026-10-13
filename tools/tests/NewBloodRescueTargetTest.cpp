#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
constexpr uint32 UNIT_FLAG_PVP_ATTACKABLE=8, UNIT_FLAG_IMMUNE_TO_PC=256, UNIT_FLAG_IMMUNE_TO_NPC=512;
constexpr uint32 UNIT_FIELD_FLAGS=1, SPELL_ATTR6_ASSIST_IGNORE_IMMUNE_FLAG=8;
constexpr uint32 UNIT_FIELD_BYTES_2=2, UNIT_BYTE2_FLAG_PVP=1;
constexpr uint32 CREATURE_TYPEFLAGS_TREAT_AS_RAID_UNIT=0x4000000, CREATURE_TYPEFLAGS_CAN_ASSIST=0x1000;
struct SpellInfo {
 struct Misc { struct Data {uint32 Attributes[14]{};} MiscData;} misc;
 Misc* GetMisc(){return &misc;}
 bool HasAttribute(uint32 mask)const{return (misc.MiscData.Attributes[6]&mask)!=0;}
};
struct CreatureTemplate {uint32 TypeFlags[2]{};};
struct Creature {
 uint32 flags=0;CreatureTemplate data;
 bool HasFlag(uint32,uint32 mask)const{return (flags&mask)!=0;}
 uint32 GetByteValue(uint32,uint32)const{return 0;}
 Creature const* ToCreature()const{return this;}
 CreatureTemplate const* GetCreatureTemplate()const{return &data;}
 bool Assist(Creature const* target,SpellInfo const* bySpell)const{
#include "RescueAssistImmunity.inc"
#include "RescueAssistCreature.inc"
  return true;
 }
};
int main(){
 SpellInfo spell;spell.misc.MiscData.Attributes[6]=4608; // Native 26972 Attributes[6].
 Creature player,captive;player.flags=UNIT_FLAG_PVP_ATTACKABLE;
 captive.flags=UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC;
 assert(!player.Assist(&captive,&spell)); // Before: fails before SpellHit.
 auto spellInfo=&spell;
#include "RescueSpellFix.inc"
 assert(player.Assist(&captive,&spell));
 SpellInfo normal;assert(!player.Assist(&captive,&normal));
 assert(!player.Assist(&captive,nullptr));
 std::cout<<"PASS: production rescue correction and Unit assist gates; ordinary assistance remains protected\n";
}
