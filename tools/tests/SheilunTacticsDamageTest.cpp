#include <cassert>
#include <cstdint>
#include <map>
#include <list>
#include <functional>
#include <cmath>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using SpellGroup=uint32;
#include "Enums.inc"
#include "Percent.inc"
#include "AddPercent.inc"
enum{SPELL_SCHOOL_MASK_NORMAL=1,USM_ARMOR,USM_MELEE_AP,USM_RANGE_AP,PLAYER_FIELD_MOD_DAMAGE_DONE_PERCENT};
struct SpellInfo{};struct AuraEffect;struct Player;
using AuraEffectList=std::list<AuraEffect*>;
struct SpellManager{bool AddSameEffectStackRuleSpellGroups(SpellInfo const*,int32,std::map<SpellGroup,int32>&){return false;}}spellManager;auto*sSpellMgr=&spellManager;
struct Unit{
 float m_auraModifiersGroup[UNIT_MOD_END][MODIFIER_TYPE_END]{};bool modifiable=true;uint32 physicalUpdates=0;std::map<AuraType,AuraEffectList>effects;
 Unit(){for(auto&mod:m_auraModifiersGroup)mod[BASE_PCT]=mod[TOTAL_PCT]=1;}
 virtual~Unit()=default;virtual bool IsPlayer()const{return false;}virtual Player*ToPlayer(){return nullptr;}
 bool CanModifyStats()const{return modifiable;}void UpdateDamagePhysical(uint32){++physicalUpdates;}
 uint32 GetStatByAuraGroup(UnitMods){return 0;}uint32 GetSpellSchoolByAuraGroup(UnitMods){return 0;}
 void UpdateStats(uint32){assert(false);}void UpdateArmor(){assert(false);}void UpdateMaxHealth(){assert(false);}void UpdateResistances(uint32){assert(false);}void UpdateAttackPowerAndDamage(bool=false){assert(false);}
 bool HandleStatModifier(UnitMods,UnitModifierType,float,bool);
 AuraEffectList const&GetAuraEffectsByType(AuraType type)const{static AuraEffectList empty;auto found=effects.find(type);return found==effects.end()?empty:found->second;}
 float GetTotalAuraMultiplier(AuraType,std::function<bool(AuraEffect const*)>const&)const;
 float GetTotalAuraMultiplierByMiscMask(AuraType,uint32)const;
};
struct Player:Unit{float percent=1;bool IsPlayer()const override{return true;}Player*ToPlayer()override{return this;}void SendUpdateStat(uint32){assert(false);}void ApplyPercentModFloatValue(uint32 field,float amount,bool apply){assert(field==PLAYER_FIELD_MOD_DAMAGE_DONE_PERCENT);ApplyPercentModFloatVar(percent,amount,apply);}};
struct AuraApplication{Unit*target;Unit*GetTarget()const{return target;}};
struct AuraEffect{
 float amount;int32 misc;SpellInfo info;float GetAmount()const{return amount;}int32 GetMiscValue()const{return misc;}SpellInfo const*GetSpellInfo()const{return &info;}
 void HandleModDamagePercentDone(AuraApplication const*,uint8,bool)const;
};
#define TC_LOG_ERROR(...) assert(false)
#include "Stats.inc"
#include "Multiplier.inc"
#include "Mask.inc"
#include "DamageHandler.inc"
bool Near(float a,float b){return std::fabs(a-b)<0.00001f;}
int main(){
 unsigned cases=0;
 // Amounts/types/mask are exact matching26972 catalog expectations, checked by
 // Test-SheilunTactics -VerifyNativeCatalog. No new production coefficients.
 AuraEffect done{25,127},taken{50,127};
 for(uint8 mode:{uint8(0),uint8(AURA_EFFECT_HANDLE_REAL),uint8(AURA_EFFECT_HANDLE_SEND_FOR_CLIENT),uint8(AURA_EFFECT_HANDLE_CHANGE_AMOUNT),uint8(AURA_EFFECT_HANDLE_REAPPLY),uint8(AURA_EFFECT_HANDLE_STAT)}){
  Unit actor;AuraApplication application{&actor};bool applies=(mode&(AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK|AURA_EFFECT_HANDLE_STAT))!=0;
  done.HandleModDamagePercentDone(&application,mode,true);
  for(auto mod:{UNIT_MOD_DAMAGE_MAINHAND,UNIT_MOD_DAMAGE_OFFHAND,UNIT_MOD_DAMAGE_RANGED})assert(Near(actor.m_auraModifiersGroup[mod][TOTAL_PCT],applies?1.25f:1.f));
  assert(actor.physicalUpdates==(applies?3u:0u));done.HandleModDamagePercentDone(&application,mode,false);
  for(auto mod:{UNIT_MOD_DAMAGE_MAINHAND,UNIT_MOD_DAMAGE_OFFHAND,UNIT_MOD_DAMAGE_RANGED})assert(Near(actor.m_auraModifiersGroup[mod][TOTAL_PCT],1));++cases;
 }
 {Player actor;AuraApplication application{&actor};done.HandleModDamagePercentDone(&application,AURA_EFFECT_HANDLE_REAL,true);assert(Near(actor.percent,1.25f)&&actor.physicalUpdates==3);done.HandleModDamagePercentDone(&application,AURA_EFFECT_HANDLE_REAL,false);assert(Near(actor.percent,1));++cases;}
 {Unit actor;actor.modifiable=false;AuraApplication application{&actor};done.HandleModDamagePercentDone(&application,AURA_EFFECT_HANDLE_REAL,true);assert(!actor.physicalUpdates&&Near(actor.m_auraModifiersGroup[UNIT_MOD_DAMAGE_MAINHAND][TOTAL_PCT],1.25f));++cases;}
 {AuraApplication missing{nullptr};done.HandleModDamagePercentDone(&missing,AURA_EFFECT_HANDLE_REAL,true);++cases;}
 {Unit actor;AuraApplication application{&actor};AuraEffect magical{25,2};magical.HandleModDamagePercentDone(&application,AURA_EFFECT_HANDLE_REAL,true);assert(!actor.physicalUpdates&&Near(actor.m_auraModifiersGroup[UNIT_MOD_DAMAGE_MAINHAND][TOTAL_PCT],1));++cases;}
 for(uint32 school=1;school<=128;school<<=1){Unit actor;actor.effects[SPELL_AURA_MOD_DAMAGE_PERCENT_DONE]={&done};actor.effects[SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN]={&taken};
  assert(Near(actor.GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE,school),school<=64?1.25f:1));
  assert(Near(actor.GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN,school),school<=64?1.5f:1));
  actor.effects.clear();assert(Near(actor.GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE,school),1)&&Near(actor.GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN,school),1));++cases;
 }
 {Unit actor;AuraApplication application{&actor};for(uint32 i=0;i<40;++i){done.HandleModDamagePercentDone(&application,AURA_EFFECT_HANDLE_REAL,true);assert(Near(actor.m_auraModifiersGroup[UNIT_MOD_DAMAGE_RANGED][TOTAL_PCT],1.25f));done.HandleModDamagePercentDone(&application,AURA_EFFECT_HANDLE_REAL,false);assert(Near(actor.m_auraModifiersGroup[UNIT_MOD_DAMAGE_RANGED][TOTAL_PCT],1));}++cases;}
 std::cout<<"PASS "<<cases<<" native aura79 dispatch body/Unit HandleStatModifier/Percent helpers and aura-mask multiplier consumers under ASAN: real three weapon TOTAL_PCT25pct apply/remove, mode/null/player guards, same-effect service controlled, all seven schools87/taken50pct and removal/baseline,40 reversible cycles. Full spell damage/cast/health engine/client not modeled; no invented Balanced reduction.\n";
}
