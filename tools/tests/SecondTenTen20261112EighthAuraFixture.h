#pragma once
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace EighthAura {
using uint32=uint32_t;using uint64=uint64_t;
enum{SPELL_SHADOWTOUCHED=244055,SPELL_FLAMETOUCHED=244054,SPELL_SCHOOL_MASK_FIRE=4,SPELL_SCHOOL_MASK_SHADOW=32,SPELL_SHADOWSCAR_FILTER=245149,SPELL_BURNING_FLASH=245021,NPC_SHATUG=122135,EFFECT_0=0,EFFECT_1=1,SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN=87,SPELL_AURA_MOD_ABSORBTION_PERCENT=179};
static void Require(bool value,char const*message){if(!value)throw std::runtime_error(message);}
template<class T,class U>static T CalculatePct(T amount,U pct){return T(amount*float(pct)/100.f);}
struct Unit;struct InstanceScript;struct Cast{uint32 spell;float amount;};
struct Unit{uint64 guid=1;InstanceScript*instance=nullptr;std::vector<Cast>casts;uint64 GetGUID(){return guid;}InstanceScript*GetInstanceScript(){return instance;}template<class...T>void CastCustomSpell(Unit*,uint32 id,float*amount,T...){casts.push_back({id,*amount});}void CastSpell(Unit*,uint32 id,bool){casts.push_back({id,0});}};using Creature=Unit;
struct Map{Creature*boss=nullptr;Creature*GetCreature(uint64){return boss;}};
struct InstanceScript{Map*instance=nullptr;uint64 GetGuidData(uint32){return 1;}};
struct SpellInfo{uint32 Id=1,schoolMask=SPELL_SCHOOL_MASK_SHADOW;uint32 GetSchoolMask()const{return schoolMask;}};
struct DamageInfo{uint64 damage;SpellInfo const*info=nullptr;DamageInfo(uint64 amount,bool healing):damage(healing?amount:std::min<uint64>(amount,UINT32_MAX)){}uint64 GetDamage(){return damage;}SpellInfo const*GetSpellInfo(){return info;}};
struct ProcEventInfo{DamageInfo*damage=nullptr;Unit*actor=nullptr;DamageInfo*GetDamageInfo(){return damage;}Unit*GetActor(){return actor;}};
struct AuraEffect{float amount=10;float GetAmount()const{return amount;}};
struct Hook{void operator+=(int){}};
struct AuraScript{Unit*caster=nullptr,*target=nullptr;uint32 id=SPELL_FLAMETOUCHED;Hook OnEffectProc;virtual~AuraScript()=default;virtual void Register(){}Unit*GetCaster(){return caster;}Unit*GetTarget(){return target;}Unit*GetUnitOwner(){return target;}uint32 GetId(){return id;}};
#define PrepareAuraScript(T) public:
#define AuraEffectProcFn(...) 0
#include "SelectedAura.inc"
#undef PrepareAuraScript
#undef AuraEffectProcFn
static void RunAuraCase(int test){Unit owner,actor;actor.guid=2;AuraEffect effect;SpellInfo source;DamageInfo damage(5000000000ull,true);damage.info=&source;ProcEventInfo event{&damage,&actor};
if(test==8){spell_dru_natures_guardian script;script.target=&owner;script.OnProc(&effect,event);Require(owner.casts.size()==1&&owner.casts.back().amount==500000000.f,"Nature Guardian must preserve the actual wide healing input");owner.casts.clear();source.Id=227034;script.OnProc(&effect,event);Require(owner.casts.empty(),"Nature Guardian cannot trigger itself");damage.info=nullptr;script.OnProc(&effect,event);Require(owner.casts.size()==1,"A healing record without a source spell remains eligible");event.damage=nullptr;script.OnProc(&effect,event);Require(owner.casts.size()==1,"A missing healing record cannot trigger another heal");}
else if(test==9){spell_felhounds_touched_proc script;script.target=&owner;script.OnProc(&effect,event);Require(owner.casts.empty(),"A vanished original imprint caster cannot issue a child spell");Unit caster,boss;Map map;map.boss=&boss;InstanceScript instance;instance.instance=&map;caster.instance=&instance;script.caster=&caster;script.OnProc(&effect,event);Require(boss.casts.size()==1,"A live original caster retains its native encounter child cast");script.id=SPELL_SHADOWTOUCHED;source.schoolMask=SPELL_SCHOOL_MASK_FIRE;script.caster=nullptr;script.OnProc(&effect,event);Require(owner.casts.size()==1,"The independent shadow-imprint branch retains its target cast");event.damage=nullptr;script.OnProc(&effect,event);event.damage=&damage;event.actor=nullptr;script.OnProc(&effect,event);Require(owner.casts.size()==1,"Incomplete proc records cannot issue imprint child spells");}
else throw std::runtime_error("unknown aura case");}
}
