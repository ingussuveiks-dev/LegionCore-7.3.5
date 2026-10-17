// Complete native SpellInfo::CheckTarget and CheckExplicitTarget with isolated
// world services. The spell's real packed attributes are emitted by the catalog.
namespace TorchTargets {
struct Attribute {uint32 index,mask;};
#include "TorchAttributes.inc"
enum {SPELL_FAILED_TARGET_AFFECTING_COMBAT=20,SPELL_FAILED_TARGET_NOT_GHOST,SPELL_FAILED_CANT_CAST_ON_TAPPED,SPELL_FAILED_TARGET_NO_POCKETS,SPELL_FAILED_TARGET_NO_WEAPONS,SPELL_FAILED_TARGET_NOT_PLAYER,SPELL_FAILED_TARGETS_DEAD,SPELL_FAILED_TARGET_IS_PLAYER,SPELL_FAILED_BM_OR_INVISGOD,SPELL_FAILED_TARGET_AURASTATE,SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED,
 SPELL_ATTR0_CU_PICKPOCKET=1,SPELL_ATTR0_CU_NEGATIVE=2,SPELL_AURA_GHOST=1,CREATURE_TYPEMASK_HUMANOID_OR_UNDEAD=3,MECHANIC_DISARM=3,BASE_ATTACK=0,UNIT_FIELD_VIRTUAL_ITEMS=0,CORPSE_BONES=0,UNIT_STATE_IN_FLIGHT=1,
 SPELL_AURA_PREVENT_RESURRECTION=2,SPELL_EFFECT_SELF_RESURRECT=94,SPELL_EFFECT_RESURRECT=18,SPELL_EFFECT_RESURRECT_NEW=113,SPELL_EFFECT_RESURRECT_WITH_AURA=172,SPELLFAMILY_WARLOCK=5};
using AuraStateType=int;
constexpr uint32 TARGET_FLAG_UNIT_MASK=0x7FFFF,TARGET_FLAG_GAMEOBJECT_MASK=0x4000,TARGET_FLAG_CORPSE_MASK=0x8200,TARGET_FLAG_DEST_LOCATION=0x40,TARGET_FLAG_GAMEOBJECT_ITEM=0x4000,
 TARGET_FLAG_UNIT_ENEMY=0x80,TARGET_FLAG_UNIT_ALLY=0x100,TARGET_FLAG_UNIT_RAID=4,TARGET_FLAG_UNIT_PARTY=8,TARGET_FLAG_UNIT_MINIPET=0x10000,TARGET_FLAG_UNIT_PASSENGER=0x100000,
 TARGET_FLAG_CORPSE_ALLY=0x8000,TARGET_FLAG_CORPSE_ENEMY=0x200,TARGET_FLAG_UNIT_DEAD=0x400,TARGET_UNIT_MASTER=27;
struct Unit;struct Player;struct Creature;struct Corpse;struct Item{};
struct WorldObject {virtual Unit const* ToUnit()const{return nullptr;}virtual Player const* ToPlayer()const{return nullptr;}virtual Creature const* ToCreature()const{return nullptr;}virtual Corpse const* ToCorpse()const{return nullptr;}virtual bool IsPlayer()const{return false;}bool IsWithinDist3d(void const*,float,bool){return near;}bool IsWithinLOS(float,float,float){return los;}bool near=true,los=true;};
struct Unit:WorldObject {bool alive=true,combat=false,flight=false;uint32 entry=0,flags=0;Unit const* ToUnit()const override{return this;}bool isInCombat()const{return combat;}bool HasAuraType(int)const{return false;}bool IsAlive()const{return alive;}bool CanFreeMove()const{return true;}bool IsControlledByPlayer()const{return IsPlayer();}bool HasUnitState(int)const{return flight;}bool IsVehicle()const{return false;}bool HasAuraState(int,void const*,Unit const*)const{return true;}bool HasAura(uint32)const{return false;}uint32 GetCreatureTypeMask()const{return 1;}uint32 GetUInt32Value(uint32)const{return 0;}uint32 GetEntry()const{return entry;}uint64 GetGUID()const{return 1;}bool _IsValidAttackTarget(Unit const*,void const*)const{return false;}bool _IsValidAssistTarget(Unit const*,void const*)const{return false;}bool IsInPartyWith(Unit const*)const{return false;}bool IsInRaidWith(Unit const*)const{return false;}uint64 GetCritterGUID()const{return 0;}bool IsOnVehicle(Unit const*)const{return false;}bool IsValidDesolateHostTarget(Unit const*,void const*)const{return true;}};
struct Creature:Unit {Creature const* ToCreature()const override{return this;}bool hasLootRecipient()const{return false;}bool isTappedBy(Player const*)const{return false;}};
struct Player:Unit {Player const* ToPlayer()const override{return this;}bool IsPlayer()const override{return true;}bool IsVisible()const{return true;}bool isGameMaster()const{return false;}bool GetWeaponForAttack(int)const{return false;}bool IsUseEquipedWeapon(bool)const{return false;}};
struct Corpse:WorldObject {Corpse const* ToCorpse()const override{return this;}int GetType()const{return 1;}uint64 GetOwnerGUID()const{return 0;}};
namespace ObjectAccessor {Player* FindPlayer(uint64){return nullptr;}}
struct SpellInfo {
 uint32 Id=186745,attrs[14]{696254848,1056,273170437,269681152,8388736,393224,4608,0,0,1048576,0,0,0,0},AttributesCu[1]{};
 struct {uint32 Targets=64,TargetCreatureType=0;}TargetRestrictions;
 struct {uint32 Mechanic=0,Category=0;}Categories;struct {uint32 SpellClassSet=0;}ClassOptions;
 struct {uint32 TargetAuraState=0,ExcludeTargetAuraState=0,TargetAuraSpell=0,ExcludeTargetAuraSpell=0;}AuraRestrictions;
 bool HasAttribute(Attribute a)const{return attrs[a.index]&a.mask;}bool HasEffect(int)const{return false;}
 bool IsPositive()const{return !(AttributesCu[0]&SPELL_ATTR0_CU_NEGATIVE);}
 bool IsRequiringDeadTarget()const;bool IsAllowingDeadTarget()const;
 bool CheckTargetCreatureType(Unit const*)const;
 uint32 GetExplicitTargetMask()const{return 64;}
 SpellCastResult CheckTarget(Unit const*,WorldObject const*,bool)const;
 SpellCastResult CheckExplicitTarget(Unit const*,WorldObject const*,Item const*)const;
};
#include "TorchDeadTarget.inc"
#include "TorchTarget.inc"
#include "TorchExplicit.inc"
#include "TorchCreatureType.inc"
struct Position {float m_positionX=0,m_positionY=0,m_positionZ=0;};
struct WorldObjectSpellTargetCheck {Unit* _caster;SpellInfo* _spellInfo;bool operator()(WorldObject* t){return _spellInfo->CheckTarget(_caster,t,true)==SPELL_CAST_OK;}};
struct WorldObjectSpellAreaTargetCheck:WorldObjectSpellTargetCheck {Position position;Position* _position=&position;float _range=5;bool _allowObjectSize=true;bool operator()(WorldObject*);};
#include "TorchArea.inc"
void Verify(){
 SpellInfo info;Player player;Creature helper;helper.flags=33555200;
 assert(info.CheckExplicitTarget(&player,nullptr,nullptr)==SPELL_CAST_OK); // Destination64, no selected unit required.
 for(auto id:{94340,94341,94342}){helper.entry=id;assert(info.CheckTarget(&player,&helper,true)==SPELL_CAST_OK);}
 WorldObjectSpellAreaTargetCheck area;area._caster=&player;area._spellInfo=&info;assert(area(&helper));helper.near=false;assert(!area(&helper));helper.near=true;
 // Native186745 has CAN_TARGET_NOT_IN_LOS. This bypass is preserved, not silently treated as LOS validation.
 helper.los=false;assert(area(&helper));info.attrs[2]&=~SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS.mask;assert(!area(&helper));helper.los=true;
 helper.alive=false;assert(info.CheckTarget(&player,&helper,true)==SPELL_CAST_OK); // Native spell permits dead area targets; objective credit still caps normally.
 helper.alive=true;helper.flight=true;assert(info.CheckTarget(&player,&helper,true)!=SPELL_CAST_OK);
}
}
