#pragma once
#include <algorithm>
#include <array>
#include <bitset>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#include "HealingAmount.h"
namespace TenthSpell {
using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
#include "TenthSpellLabels.inc"
static void Require(bool b,char const*s){if(!b)throw std::runtime_error(s);}
template<class T,class U>static T CalculatePct(T v,U p){return T(v*float(p)/100.f);}
static uint32 urand(float lo,float){return uint32(lo);}static void ApplyPercentModFloatVar(float&v,float pct,bool apply){v=apply?v*(1+pct/100):v/(1+pct/100);}
struct Player;struct Creature;struct Unit;
struct Hook{void operator+=(int){}};
struct Effect{float BasePoints=50;float CalcValue(Unit*)const{return BasePoints;}};
struct SpellInfo{Effect e[32];Effect*Effects[32];SpellInfo(){for(int i=0;i<32;++i)Effects[i]=e+i;}};
struct AuraEffect{float amount=6000000000.f;float GetAmount()const{return amount;}};
struct DamageInfo{uint32 damage;Unit*attacker=nullptr;uint32 GetDamage(){return damage;}Unit*GetAttacker(){return attacker;}};
struct ItemTemplate{float lo=10,hi=20;uint32 delay=2000;mutable int damageCalls=0;void GetDamage(uint32,float&a,float&b)const{++damageCalls;a=lo;b=hi;}uint32 GetDelay()const{return delay;}};
struct Item{ItemTemplate info;ItemTemplate*GetTemplate(){return &info;}uint32 GetItemLevel(){return 1;}};
struct AreaTrigger{float distance=1;bool consumed=false;void CastAction(){consumed=true;}};
struct InstanceScript{std::map<uint32,uint32>data;std::set<uint32>allowed;uint32 GetData(uint32 id){return data[id];}void SetData(uint32 id,uint32 v){data[id]=v;}uint64 GetGuidData(uint32){return 0;}void HandleGameObject(uint64,bool){}};
static bool IsNextStageAllowed(InstanceScript*p,uint32 s){return p->allowed.count(s)!=0;}
namespace WorldPackets{namespace Misc{struct PhaseShiftDataPhase{explicit PhaseShiftDataPhase(uint32){}};}}
struct Session{void SendSetPhaseShift(std::vector<WorldPackets::Misc::PhaseShiftDataPhase>const&,std::vector<uint16>const&,std::vector<uint16>const&,std::vector<uint16>const&,int){}};
struct Talk{void SendQuestQueryResponse(uint32){}};struct AreaTriggerEntry{uint32 ID;};
struct Unit{virtual~Unit()=default;virtual Player*ToPlayer(){return nullptr;}uint32 type=TYPEID_UNIT,map=1,instanceId=1;bool world=true;uint64 maximum=7000000000ull,health=4000000000ull;AuraEffect aura;std::list<uint64>summons;std::vector<uint32>casts;std::vector<AreaTrigger*>fragments;std::vector<std::function<void()>>delayed;std::map<uint32,float>fields;
 uint64 GetMaxHealth(){return maximum;}uint64 GetHealth(){return health;}uint32 GetTypeId(){return type;}uint32 GetMapId(){return map;}uint32 GetInstanceId(){return instanceId;}bool IsInWorld(){return world;}AuraEffect const*GetAuraEffect(uint32,int){return &aura;}std::list<uint64>*GetSummonList(uint32){return &summons;}template<class...T>void CastCustomSpell(Unit*,uint32 id,T...){casts.push_back(id);}void CastSpell(Unit*,uint32 id,bool=false){casts.push_back(id);}void AddAura(uint32 id,Unit*){casts.push_back(id);}void RemoveAurasDueToSpell(uint32){}void DespawnOrUnsummon(uint32){}void GetAreaObjectList(std::list<AreaTrigger*>&out,std::vector<uint32>const&){for(auto a:fragments)if(!a->consumed)out.push_back(a);}float GetDistance(AreaTrigger*a){return a->distance;}void AddDelayedEvent(uint32,std::function<void()>f){delayed.push_back(f);}void Dispatch(){auto q=delayed;delayed.clear();for(auto&f:q)f();}float GetFloatValue(uint32 f){return fields[f];}
};
struct Creature:Unit{float power=140;float GetTotalAttackPowerValue(uint32){return power;}float GetTotalAuraMultiplierByMiscMask(uint32,uint32){return 1;}float GetModifierValue(uint32,uint32 kind){return kind==BASE_PCT?1.f:0.f;}void SetStatFloatValue(uint32 id,float v){fields[id]=v;}void SetFloatValue(uint32 id,float v){fields[id]=v;}float GetTotalAuraModifier(uint32){return 0;}void DoAction(uint32){}Creature*AI(){return this;}};
struct Player:Unit{Item*main=nullptr,*off=nullptr;InstanceScript*instance=nullptr;Session session;Talk talk;Talk*PlayerTalkClass=&talk;uint32 credit=0,teleports=0;float haste=0;Player(){type=TYPEID_PLAYER;}Player*ToPlayer()override{return this;}Item*GetItemByPos(uint32,uint32 slot){return slot==EQUIPMENT_SLOT_MAINHAND?main:off;}float GetRatingBonusValue(uint32){return haste;}InstanceScript*GetInstanceScript(){return instance;}void UpdateAchievementCriteria(uint32,uint32,uint32){++credit;}Creature*FindNearestCreature(uint32,float){return nullptr;}Session*GetSession(){return &session;}void TeleportTo(uint32,float,float,float,float){++teleports;}};
namespace ObjectAccessor{static Creature*GetCreature(Unit&,uint64){return nullptr;}}
struct AuraScript{Unit*owner=nullptr;SpellInfo info;bool removed=false;Hook DoEffectCalcAmount,OnEffectAbsorb;virtual~AuraScript()=default;virtual void Register(){}Unit*GetUnitOwner(){return owner;}SpellInfo const*GetSpellInfo(){return &info;}void Remove(){removed=true;}};
using SpellCastResult=int;using SpellEffIndex=int;
struct SpellScript{Unit*caster=nullptr,*target=nullptr;SpellInfo info;int32 damage=7;Hook OnCheckCast,OnCast,OnEffectLaunchTarget;virtual~SpellScript()=default;virtual void Register(){}Unit*GetCaster(){return caster;}Unit*GetHitUnit(){return target;}SpellInfo const*GetSpellInfo(){return &info;}void SetHitDamage(int32 d){damage=d;}};
#define PrepareAuraScript(T) public:
#define PrepareSpellScript(T) public:
#define AuraEffectCalcAmountFn(...) 0
#define AuraEffectAbsorbFn(...) 0
#define SpellCheckCastFn(...) 0
#define SpellCastFn(...) 0
#define SpellEffectFn(...) 0
#include "TenthSpellSelected.inc"
#undef PrepareAuraScript
#undef PrepareSpellScript
#undef AuraEffectCalcAmountFn
#undef AuraEffectAbsorbFn
#undef SpellCheckCastFn
#undef SpellCastFn
#undef SpellEffectFn
static void Run(int n){
 if(n==1){Unit owner,attacker;spell_gen_shadowy_reflection_absorb s;s.owner=&owner;float amount=0;bool recalc=false;s.CalculateAmount(nullptr,amount,recalc);DamageInfo d{2000000000u,&attacker};for(int i=0;i<6;++i){float a=0;s.Absorb(nullptr,d,a);Require(a==1000000000.f,"A six-billion absorb pool must retain each legal ordinary hit");Require(s.removed==(i==5),"Absorb expires exactly when its wide pool is spent");}spell_gen_shadowy_reflection_absorb fractional;owner.aura.amount=1;fractional.owner=&owner;fractional.CalculateAmount(nullptr,amount,recalc);d.damage=1;float a=9;fractional.Absorb(nullptr,d,a);Require(a==0&&!fractional.removed,"Fractional damage cannot spend an integer absorb point");}
 else if(n==2){Player p;Creature c;Item main,off;off.info.lo=30;off.info.hi=40;off.info.delay=1000;p.main=&main;p.off=&off;TenthClone s;s.owner=&p;s.me=&c;s.RecalcDamage();Require(off.info.damageCalls==1,"Clone must calculate its off-hand from the actual off-hand item");Require(c.fields[UNIT_FIELD_MIN_DAMAGE]==60&&c.fields[UNIT_FIELD_MIN_OFF_HAND_DAMAGE]==15,"Distinct weapons retain the existing clone damage formula");s.entryId=69792;s.RecalcDamage();Require(c.fields[UNIT_FIELD_MIN_DAMAGE]==75,"Green clone combines the native two damage channels");s.entryId=69791;p.main=p.off=nullptr;s.RecalcDamage();Require(c.fields[UNIT_FIELD_MIN_DAMAGE]==20&&c.fields[UNIT_FIELD_MIN_OFF_HAND_DAMAGE]==10,"Unarmed clone uses initialized native attack speeds");p.main=&main;s.RecalcHast();main.info.delay=3000;s.RecalcHast();Require(c.fields[UNIT_FIELD_ATTACK_ROUND_BASE_TIME+BASE_ATTACK]==3000,"Unchanged haste still refreshes equipment speed");p.main=nullptr;p.off=&off;s.RecalcDamage();Require(off.info.damageCalls==3,"Off-hand-only recalculation remains defined");}
 else if(n==3){Player p;InstanceScript i;p.instance=&i;TenthHarvest s;AreaTriggerEntry at{8698};i.allowed.insert(STAGE_7);for(uint32 state:{uint32(DONE),uint32(TO_BE_DECIDED)}){i.data[DATA_ESSENCE_OF_ORDER_EVENT]=state;Require(!s.OnTrigger(&p,&at,true)&&p.casts.empty()&&!p.credit,"Completed or reserved Essence trigger cannot replay its scene or fall through");}at.ID=8702;i.allowed.clear();i.allowed.insert(STAGE_8);Require(!s.OnTrigger(&p,&at,true)&&!p.teleports,"A blocked stage-seven trigger cannot fall through to stage-eight teleport");at.ID=8698;i.data[DATA_ESSENCE_OF_ORDER_EVENT]=NOT_STARTED;Require(s.OnTrigger(&p,&at,true)&&p.casts.size()==3,"Eligible Essence starts its authored scene and visibility sequence");Require(!s.OnTrigger(&p,&at,true)&&p.casts.size()==3,"Repeated scene trigger retains exactly-once admission");p.instance=nullptr;Require(!s.OnTrigger(&p,&at,true),"No instance cannot use a stale shared instance pointer");}
 else if(n==4){for(int transition=0;transition<4;++transition){Unit c;AreaTrigger near,far;far.distance=50;c.fragments={&near,&far};spell_dh_spirit_bomb_SpellScript s;s.caster=&c;s.info.e[0].BasePoints=10;Require(s.CheckCast()==SPELL_FAILED_SUCCESS,"Nearby fragment admits Spirit Bomb");s.HandleOnCast();Require(near.consumed&&!far.consumed&&c.delayed.size()==1,"Only nearby fragments are consumed before the native delay");if(transition==0)++c.map;if(transition==1)++c.instanceId;if(transition==2)c.world=false;c.Dispatch();Require(c.casts.size()==(transition==3?1:0),"Spirit Bomb cannot explode after its caster changes map or instance");s.HandleOnCast();Require(c.delayed.empty(),"No eligible fragments cannot schedule another explosion");}}
 else if(n==6){Unit c;Player target;spell_pal_holy_wrath_SpellScript s;s.caster=&c;s.target=&target;s.info.e[3].BasePoints=50;s.HandleOnHit(0);Require(s.damage==1500000000,"Holy Wrath must retain wide missing health before applying its native percentage");Creature npc;s.target=&npc;s.info.e[2].BasePoints=200;s.HandleOnHit(0);Require(s.damage==INT32_MAX,"Native signed damage channel saturates rather than wraps");c.health=c.maximum;s.damage=7;s.HandleOnHit(0);Require(s.damage==7,"Full health preserves native ordinary hit handling");c.health=c.maximum+1;s.HandleOnHit(0);Require(s.damage==7,"Inconsistent over-max health cannot underflow");}
 else throw std::runtime_error("unknown spell case");
}
}
namespace TenthTick {
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;
#include "TenthTickLabels.inc"
struct TickEffect{uint32 Effect=SPELL_EFFECT_APPLY_AURA,ApplyAuraName=SPELL_AURA_PERIODIC_WEAPON_PERCENT_DAMAGE,ApplyAuraPeriod=3000;};
struct SpellInfo{int32 duration=15000;uint32 EffectMask=1;std::bitset<512>HasAuraBit;TickEffect e[32];TickEffect*Effects[32];SpellInfo(){HasAuraBit.set(SPELL_AURA_PERIODIC_WEAPON_PERCENT_DAMAGE);for(int i=0;i<32;++i)Effects[i]=e+i;}int32 GetDuration()const{return duration;}uint32 GetMaxTicks()const;};
#include "TenthTicks.inc"
static void Run(){SpellInfo s;TenthSpell::Require(s.GetMaxTicks()==5,"Periodic weapon damage must derive five ticks from its native duration and period");s.duration=0;TenthSpell::Require(s.GetMaxTicks()==1,"Zero duration retains one tick");s.duration=60000;TenthSpell::Require(s.GetMaxTicks()==10,"Native thirty-second cap remains");s.e[0].ApplyAuraPeriod=0;TenthSpell::Require(s.GetMaxTicks()==6,"Missing period retains legacy fallback");}
}
