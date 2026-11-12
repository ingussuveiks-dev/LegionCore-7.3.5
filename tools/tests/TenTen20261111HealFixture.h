#ifndef TEN_TEN_20261111_HEAL_FIXTURE_H
#define TEN_TEN_20261111_HEAL_FIXTURE_H
// Complete production spell classes, with controlled cast/aura/spatial endpoints.
#include "HealingAmount.h"
#include <array>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
using SpellEffIndex=uint32;using AuraEffectHandleModes=uint32;using AuraRemoveMode=uint32;using Targets=uint32;
constexpr uint32 RANGED_ATTACK=2,QUEST_STATUS_INCOMPLETE=3,EFFECT_0=0,EFFECT_1=1,EFFECT_2=2,EFFECT_3=3,EFFECT_4=4,IN_MILLISECONDS=1000,AURA_REMOVE_BY_EXPIRE=1,AURA_REMOVE_BY_ENEMY_SPELL=2,SPELL_EFFECT_DUMMY=3,SPELL_EFFECT_HEAL=10,SPELL_EFFECT_WEAPON_PERCENT_DAMAGE=31,TARGET_SRC_CASTER=22,TARGET_GAMEOBJECT_SRC_AREA=51,TARGET_UNIT_SRC_AREA_ENTRY=7,SPELL_AURA_SCHOOL_ABSORB=69,SPELL_AURA_PERIODIC_DUMMY=226,AURA_EFFECT_HANDLE_REAL=1;
template<class T,class U>T CalculatePct(T base,U pct){return T(base*static_cast<float>(pct)/100.f);}template<class T,class U>T AddPct(T&base,U pct){return base+=CalculatePct(base,pct);}
static void Require(bool v,char const*m){if(!v)throw std::runtime_error(m);}
struct Unit;struct Player;struct Creature;struct WorldObject{virtual~WorldObject()=default;uint32 entry=0;uint32 GetEntry(){return entry;}virtual Unit*ToUnit(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}};
using ObjectGuid=uint64;using GuidList=std::list<ObjectGuid>;
struct AuraEffect;
struct Aura{bool removed=false;uint32 stack=1;AuraEffect*effect=nullptr;void Remove(){removed=true;}uint32 GetStackAmount(){return stack;}AuraEffect*GetEffect(uint32){return effect;}GuidList targets;int32 duration=1000;GuidList GetEffectTargets(){return targets;}void RemoveEffectTarget(ObjectGuid g){targets.remove(g);}void AddEffectTarget(ObjectGuid g){targets.push_back(g);}int32 GetDuration(){return duration;}void SetDuration(int32 d){duration=d;}};
struct AuraEffect{uint32 ticks=3;uint32 GetTotalTicks()const{return ticks;}void SetAmount(float a){amount=a;}float amount=0;Aura base;float GetAmount()const{return amount;}Aura*GetBase()const{auto self=const_cast<AuraEffect*>(this);self->base.effect=self;return &self->base;}};
struct Cast{Unit*actor;Unit*target;uint32 spell;float amount;};static std::vector<Cast>casts;
struct AreaTrigger{bool removed=false;void Despawn(){removed=true;}};
struct Unit:WorldObject{bool alive=true,phase=true;uint32 map=1220,interrupts=0;std::list<AreaTrigger*>areas;std::array<uint32,3>autoattacks{};bool IsAlive(){return alive&&health>0;}bool HasPhaseId(uint32 id){return phase&&id==5165;}uint32 GetMapId(){return map;}void GetAreaObjectList(std::list<AreaTrigger*>&out,uint32){out=areas;}uint32 GetAutoattackSpellId(uint32 i){return autoattacks.at(i);}void InterruptSpell(uint32,bool){++interrupts;}Unit*owner=nullptr;bool combat=false;std::map<uint32,ObjectGuid>auraOwners;Unit*GetOwner(){return owner;}Unit*GetAnyOwner(){return owner;}bool IsFullHealth(){return health==maxHealth;}float GetHealthPct(){return 100.f*float(health)/float(maxHealth);}bool isInCombat(){return combat;}uint64 health=100,maxHealth=100;ObjectGuid guid=1;float distance=25;bool pvp=false;uint32 history=0;std::map<uint32,AuraEffect>effects;std::set<uint32>auras;Unit*ToUnit()override{return this;}virtual Player*ToPlayer(){return nullptr;}uint64 GetMaxHealth(){return maxHealth;}uint64 GetHealth(){return health;}uint64 CountPctFromMaxHealth(int32 pct,Unit* =nullptr){return CalculatePct(maxHealth,pct);}ObjectGuid GetGUID(){return guid;}float GetDistance(Unit*){return distance;}bool HasAura(uint32 id,ObjectGuid who=0){return auras.count(id)!=0&&(!who||!auraOwners.count(id)||auraOwners[id]==who);}AuraEffect*GetAuraEffect(uint32 id,uint32){auto it=effects.find(id);return it==effects.end()||it->second.base.removed?nullptr:&it->second;}Aura*GetAura(uint32 id,ObjectGuid who=0){if(who&&auraOwners.count(id)&&auraOwners[id]!=who)return nullptr;auto e=GetAuraEffect(id,0);return e?e->GetBase():nullptr;}template<class...T>void CastCustomSpell(Unit*target,uint32 id,float*amount,void*,void*,bool,T...){casts.push_back({this,target,id,*amount});}void CastCustomSpell(uint32 id,uint32,int32 amount,Unit*target,bool){casts.push_back({this,target,id,float(amount)});}void CastSpell(Unit*target,uint32 id,bool){casts.push_back({this,target,id,0});}bool CanPvPScalar(){return pvp;}uint32 GetDamageTakenInPastSecs(uint32,bool,bool){return history;}};
struct Creature:Unit{Creature*ToCreature()override{return this;}};
struct Group{bool raid=false;bool isRaidGroup(){return raid;}};
struct Pet:Unit{};
struct Player:Unit{Pet*pet=nullptr;Unit*vehicle=nullptr;uint32 questStatus=QUEST_STATUS_INCOMPLETE,credits=0;std::map<uint32,float>floatFields;Pet*GetPet(){return pet;}Unit*GetVehicleBase(){return vehicle;}uint32 GetQuestStatus(uint32){return questStatus;}void KilledMonsterCredit(uint32){++credits;}bool InSamePhase(Unit*other){return phase&&other->phase;}float GetFloatValue(uint32 key){return floatFields[key];}void ModSpellChargeCooldown(uint32,int32){}Group*group=nullptr;Group*GetGroup(){return group;}Player*ToPlayer()override{return this;}float GetSpellCooldownDelay(uint32){return 1;}void ModifySpellCooldown(uint32,int32){}};
namespace ObjectAccessor{static Unit*GetUnit(Unit&,ObjectGuid){return nullptr;}}
struct EffectInfo{float BasePoints=0;float value=0;float CalcValue(Unit* =nullptr){return value;}};
struct SpellInfo{struct ChargeCategory{uint32 ChargeRecoveryTime=0;}Category;uint32 Id=0;EffectInfo storage[5];EffectInfo*Effects[5]={&storage[0],&storage[1],&storage[2],&storage[3],&storage[4]};EffectInfo*GetEffect(uint32 i){return Effects[i];}};
struct Spell{uint32 parentCount=0;uint32 GetTargetParentCount(){return parentCount;}bool crit=false;bool IsCritForTarget(Unit*){return crit;}SpellInfo info;SpellInfo*GetSpellInfo(){return &info;}};
struct SpellMgr{SpellInfo info;SpellInfo*GetSpellInfo(uint32){return &info;}}spellMgr;static SpellMgr*sSpellMgr=&spellMgr;
struct DamageInfo{explicit DamageInfo(uint64 value=0):amount(std::min<uint64>(value,UINT32_MAX)){}Unit*attacker=nullptr;SpellInfo const*spell=nullptr;Unit*GetAttacker(){return attacker;}SpellInfo const*GetSpellInfo(){return spell;}uint64 amount=0;uint64 GetDamage(){return amount;}};struct HealInfo{uint64 amount=0;uint64 GetHeal(){return amount;}};
struct ProcEventInfo{Spell*spell=nullptr;HealInfo*heal=nullptr;DamageInfo*damage=nullptr;Unit*actor=nullptr;Spell*GetSpell(){return spell;}HealInfo*GetHealInfo(){return heal;}DamageInfo*GetDamageInfo(){return damage;}Unit*GetActor(){return actor;}};
struct Application{uint32 mode=AURA_REMOVE_BY_EXPIRE;uint32 GetRemoveMode(){return mode;}};
struct Hook{template<class T>void operator+=(T){}};
static bool rollChance=false;static bool roll_chance_i(float){return rollChance;}
struct ScriptBase{int32 hitDamageValue=0;void SetHitDamage(int32 value){hitDamageValue=value;}Unit*GetUnitOwner(){return target;}Spell spell;Spell*GetSpell(){return &spell;}Unit*explicitTarget=nullptr;Unit*GetExplTargetUnit(){return explicitTarget?explicitTarget:target;}Unit*caster=nullptr,*target=nullptr;SpellInfo info;uint64 healing=0;int32 legacy=0;bool exact=false,removed=false,prevented=false;Application app;Hook BeforeHit,AfterHit,AfterEffectAbsorb,OnObjectAreaTargetSelect,OnEffectHitTarget,OnHit,DoEffectCalcAmount,OnEffectAbsorb,AfterEffectApply,OnEffectRemove,DoCheckProc,OnEffectProc,AfterCast;
 virtual~ScriptBase()=default;virtual void Register(){}virtual bool Validate(SpellInfo const*){return true;}virtual bool Load(){return true;}bool ValidateSpellInfo(std::initializer_list<uint32>){return true;}Unit*GetCaster(){return caster;}Unit*GetTarget(){return target;}Unit*GetHitUnit(){return target;}SpellInfo*GetSpellInfo(){return &info;}Application*GetTargetApplication(){return &app;}void Remove(uint32){removed=true;}void PreventDefaultAction(){prevented=true;}void Configure(uint64 value,bool channel=true){healing=value;exact=channel;legacy=channel?0:int32(value);}int32 GetHitHeal(){return legacy;}uint64 GetHitHeal64(){return exact?healing:legacy>0?uint64(legacy):0;}void SetHitHeal(int32 value){legacy=value;exact=false;}void SetHitHeal64(uint64 value){healing=value;exact=true;}};using SpellScript=ScriptBase;using AuraScript=ScriptBase;
#define PrepareSpellScript(T) public:
#define PrepareAuraScript(T) public:
#define SpellObjectAreaTargetSelectFn(...) 0
#define SpellEffectFn(...) 0
#define SpellHitFn(...) 0
#define AuraEffectCalcAmountFn(...) 0
#define AuraEffectAbsorbFn(...) 0
#define AuraEffectApplyFn(...) 0
#define AuraCheckProcFn(...) 0
#define AuraEffectProcFn(...) 0
#define SpellCastFn(...) 0
namespace GameObjects{enum{Azerite=281307,Azerite2=272471};}
struct GameObject:WorldObject{std::vector<std::pair<uint32,std::function<void()>>>queue;uint32 animations=0;void AddDelayedEvent(uint32 delay,std::function<void()>fn){queue.push_back({delay,std::move(fn)});}void SendCustomAnim(uint32 id,bool despawn){Require(id==1&&!despawn,"Authored capture-node animation");++animations;}};
struct BGBase{uint32 initialized=0;void UpdateCapturePoint(uint32,uint32,GameObject*,void*,bool){++initialized;}};

#endif
