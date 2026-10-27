#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <set>
#include <map>
using uint32=uint32_t;using uint8=uint8_t;using int32=int32_t;
#include "../../src/server/game/Movement/SkyholdDeparture.h"
#define TC_LOG_DEBUG(...) ((void)0)
enum {EFFECT_MOTION_TYPE,MMCF_UPDATE=1,MOTION_SLOT_CONTROLLED=2};using MovementSlot=int;
using MovementGeneratorType=int;
struct ObjectGuid{int value;ObjectGuid(int v=0):value(v){}static ObjectGuid const Empty;};
ObjectGuid const ObjectGuid::Empty{};
enum {SPELLMOD_DAMAGE,SPELLMOD_DURATION,SPELLMOD_CASTING_TIME,SPELLMOD_COST,SPELLMOD_FLAT,SPELLMOD_PCT,SPELL_ATTR0_ONLY_STEALTHED,SPELL_ATTR11_UNK4};using SpellModOp=int;
struct SpellModifier{int charges=0,op=SPELLMOD_DAMAGE,type=SPELLMOD_PCT;uint32 spellId=199148,mask=536870912;float value=200;void* ownerAura=nullptr;};
struct Spell{std::set<void*> m_appliedMods;};
struct SpellInfo{
 struct{int SpellClassSet=4;uint32 SpellClassMask=536870912;}ClassOptions;
 bool HasAttribute(int)const{return false;}int GetDuration()const{return 0;}bool IsAffectedBySpellMods()const{return true;}
 bool IsAffectedBySpellMod(SpellModifier*)const;
};
struct SpellMgr{SpellInfo info;SpellInfo const* GetSpellInfo(uint32 id){return id==52174||id==199148?&info:nullptr;}}spellMgr;
auto sSpellMgr=&spellMgr;
#define ASSERT(x) assert(x)
template<class T>void AddPct(T& value,float percent){value+=value*percent/100;}
#include "SpellMask.inc"
struct Spline{bool done=false;bool Finalized(){return done;}};
struct Unit;struct Creature;
struct AI{void MovementInform(int,uint32){}};
struct Creature{::AI ai;::AI* AI(){return &ai;}};
struct Unit {
 Spline spline;Spline* movespline=&spline;uint32 damageCasts=0,speedCasts=0;bool talent=false;
 int launchResult=250;bool jumping=false;
 bool IsPlayer(){return true;}void AddUnitState(int){jumping=true;}void ClearUnitState(int){jumping=false;}
 bool IsAlive() const{return true;}uint32 GetMapId() const{return 1220;}
 bool HasAura(int32 id){return id==202163&&talent;}
 void CastSpell(Unit*,uint32 id,bool){++damageCasts;if(id==52174&&!SkipCasterAura(this,202163))++speedCasts;}
 Creature* ToCreature(){return nullptr;}
 bool SkipCasterAura(Unit* caster,int32 hastalent){
#include "CasterAuraGate.inc"
 return false;}
};
namespace ObjectAccessor{Unit* GetUnit(Unit& owner,ObjectGuid){return &owner;}}
struct MovementGenerator {
 virtual ~MovementGenerator()=default;virtual void Initialize(Unit&){};virtual void Finalize(Unit&)=0;
 virtual void Reset(Unit&){};virtual bool Update(Unit&,uint32)=0;virtual int GetMovementGeneratorType()const=0;
};
#include "EffectGenerator.inc"
#include "EffectMethods.inc"
enum{UNIT_STATE_LONG_JUMP=1};
struct JumpArrivalCastArgs{uint32 SpellId=0;ObjectGuid Target;};
namespace Movement{
 constexpr float gravity=19.291105f;
 float computeFallElevation(float t,bool,float velocity){return 0.5f*gravity*t*t+velocity*t;}
 struct SpellEffectExtraData{};
 struct MoveSplineInit{Unit& unit;explicit MoveSplineInit(Unit& u):unit(u){}
  void MoveTo(float,float,float,bool){}void SetParabolic(float,int){}void SetVelocity(float){}void SetFacing(float){}
  void SetSpellEffectExtraData(SpellEffectExtraData const&){}
  int32 Launch(){if(unit.launchResult>0)unit.spline.done=false;return unit.launchResult;}
 };
}
struct MotionMaster{
 Unit* _owner;MovementGenerator* _slot[3]{};int _top=2,_cleanFlag=0;bool _initialize[3]{};
 bool IsStatic(MovementGenerator*){return false;}void DelayedDelete(MovementGenerator* m){DirectDelete(m);}
 void Mutate(MovementGenerator*,MovementSlot);void DirectDelete(MovementGenerator*);
 void MoveJump(float,float,float,float,float,float,uint32,bool,JumpArrivalCastArgs const*,Movement::SpellEffectExtraData const*);
 explicit MotionMaster(Unit* u):_owner(u){}~MotionMaster(){if(_slot[2])DirectDelete(_slot[2]);}
};
#include "MotionMethods.inc"
#include "Jump.inc"
using SpellEffIndex=int;
struct Script{bool prevented=false;void PreventHitDefaultEffect(int){prevented=true;}
#include "SuppressOuter.inc"
#include "SuppressMissile.inc"
};
enum {SPELL_AURA_CHARGE_RECOVERY_MOD,SPELL_AURA_MOD_COOLDOWN,SPELL_AURA_CHARGE_RECOVERY_MULTIPLIER,SPELL_AURA_CHARGE_RECOVERY_AFFECTED_BY_HASTE_REGEN,UNIT_FIELD_MOD_TIME_RATE,SPELL_AURA_MOD_MAX_CHARGES,IN_MILLISECONDS=1000};
struct SpellCategoryEntry{uint32 ID=1211;int32 ChargeRecoveryTime=45000;int MaxCharges=1;};
struct Player{
 bool talent=false,barbarian=false;float GetTotalAuraModifierByMiscValue(int kind,uint32 category)const{if(category!=1211)return 0;if(kind==SPELL_AURA_MOD_MAX_CHARGES&&barbarian)return 2;return kind==SPELL_AURA_CHARGE_RECOVERY_MOD&&talent?-15000.0f:0.0f;}
 float GetTotalAuraModifier(int)const{return 0;}float GetTotalAuraMultiplierByMiscValue(int,uint32)const{return 1;}
 float GetFloatValue(int)const{return 1;}float SpellCooldownModByRate(SpellInfo const*,bool){return 1;}
 uint32 GetSpellCategoryChargesTimer(SpellCategoryEntry const*,SpellInfo const*,bool)const;
 uint8 GetMaxSpellCategoryCharges(SpellCategoryEntry const*)const;
 Spell* m_spellModTakingSpell=nullptr;std::vector<SpellModifier*> m_spellMods[16];
 bool HasItemFitToSpellRequirements(SpellInfo const*)const{return true;}bool HasStealthAura()const{return false;}
 void DropModCharge(SpellModifier*,Spell*)const{}
 bool IsAffectedBySpellmod(SpellInfo const*,SpellModifier*,Spell*)const;
 template<class T>void ApplySpellMod(uint32,SpellModOp,T&,Spell* =nullptr)const;
 int getClass(){return 1;}
};
#include "ChargeTimer.inc"
#include "MaxCharges.inc"
#include "PlayerMask.inc"
#include "ApplyMod.inc"
enum WeaponAttackType{BASE_ATTACK,RANGED_ATTACK};
enum {CLASS_HUNTER=3,SPELL_DAMAGE_CLASS_RANGED=3,SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS,SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS,DOT,SPELL_DIRECT_DAMAGE,LINK_HAS_AURA_ON_CASTER=0};
struct DamageSpell{struct{int DefenseType=2;}Categories;bool IsRangedWeaponSpell()const{return false;}};
struct DamageUnit{
 float ap=10000;Player* ToPlayer(){return nullptr;}int GetTotalAuraModifier(int){return 0;}
 float GetTotalAttackPowerValue(WeaponAttackType){return ap;}
 int32 Damage(){float ApCoeffMod=0.624f;bool calcSPDBonus=false;int32 DoneAdvertisedBenefit=0,DoneTotal=0;int stack=1,damagetype=SPELL_DIRECT_DAMAGE;DamageSpell info;auto spellProto=&info;auto victim=this;
#include "AttackPower.inc"
 return 1+DoneTotal;}
};
int main(){
 // Native controller replacement fires the old descriptor before arrival.
 {Unit old;MotionMaster mm(&old);mm.Mutate(new EffectMovementGenerator(6544,52174,0),2);
  mm.Mutate(new EffectMovementGenerator(94954,0,0),2);assert(old.damageCasts==1);
  old.CastSpell(&old,52174,true);assert(old.damageCasts==2);
  std::cout<<"COUNTERFACTUAL original outer arrival on replacement plus native-data missile emits two 52174 casts; missile dispatch endpoint controlled\n";}
 for(bool talented:{false,true}){
  Unit owner;owner.talent=talented;MotionMaster mm(&owner);Script outer;outer.HandleOuterJump(1);assert(outer.prevented);
  // Ordinary outer default dispatch is skipped, preserving the native94954 spline.
  mm.Mutate(new EffectMovementGenerator(94954,52174,0),2);
  assert(mm._slot[2]->Update(owner,10));assert(!owner.damageCasts);
  Script missile;missile.HandleMissile(2);assert(missile.prevented);
  owner.spline.done=true;assert(!mm._slot[2]->Update(owner,10));
  auto landed=mm._slot[2];mm._slot[2]=nullptr;mm.DirectDelete(landed);
  assert(owner.damageCasts==1&&owner.speedCasts==(talented?1u:0u));
  assert(owner.SkipCasterAura(&owner,202163)==!talented);
  Player player;player.talent=talented;SpellCategoryEntry category;
  assert(player.GetSpellCategoryChargesTimer(&category,nullptr,false)==(talented?30000u:45000u));
  player.talent=false;assert(player.GetSpellCategoryChargesTimer(&category,nullptr,false)==45000);
 }
 for(int cancel:{0,1}){Unit owner;MotionMaster mm(&owner);mm.Mutate(new EffectMovementGenerator(94954,52174,0),2);
  if(cancel)owner.spline.done=true; // interrupted spline without natural Update expiry
  mm.Mutate(new EffectMovementGenerator(100,0,0),2);assert(!owner.damageCasts);}
 for(int duration:{0,250}){Unit owner;owner.launchResult=duration;owner.spline.done=true;MotionMaster mm(&owner);JumpArrivalCastArgs arrival{52174,0};
  mm.MoveJump(10,0,0,0,40,2.5f,94954,false,&arrival,nullptr);
  if(!duration){assert(!mm._slot[2]&&!owner.damageCasts&&!owner.jumping);}
  else{assert(mm._slot[2]&&mm._slot[2]->Update(owner,10));owner.spline.done=true;assert(!mm._slot[2]->Update(owner,10));}}
 {Unit other;MotionMaster mm(&other);mm.Mutate(new EffectMovementGenerator(123,456,0),2);
  mm.Mutate(new EffectMovementGenerator(124,0,0),2);assert(other.damageCasts==1);}
 DamageUnit damage;assert(damage.Damage()==6241);damage.ap=0;assert(damage.Damage()==1);
 {Player warrior;SpellCategoryEntry category;assert(warrior.GetMaxSpellCategoryCharges(&category)==1);warrior.barbarian=true;assert(warrior.GetMaxSpellCategoryCharges(&category)==3);
  SpellModifier mod;float base=6241;warrior.ApplySpellMod(52174,SPELLMOD_DAMAGE,base);assert(base==6241);
  warrior.m_spellMods[SPELLMOD_DAMAGE].push_back(&mod);warrior.ApplySpellMod(52174,SPELLMOD_DAMAGE,base);assert(base==18723);
  mod.mask=1;base=6241;warrior.ApplySpellMod(52174,SPELLMOD_DAMAGE,base);assert(base==6241);
  warrior.m_spellMods[SPELLMOD_DAMAGE].clear();base=6241;warrior.ApplySpellMod(52174,SPELLMOD_DAMAGE,base);assert(base==6241);}
 std::cout<<"PASS native controller and landing gate, both suppression handlers, selected/absent/removed talent charge timing and native aura admission; full spell/target/armor/client endpoints not simulated\n";
}
