#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;using int64=int64_t;using TriggerCastFlags=uint32;using SpellEffIndex=int;using AuraEffectHandleModes=int;using DamageEffectType=int;
constexpr int EFFECT_0=0,EFFECT_1=1,AURA_EFFECT_HANDLE_REAL=1,SPELL_CAST_OK=0,IN_PROGRESS=1;
#define UI64FMTD "%llu"
#define TC_LOG_DEBUG(...) ((void)0)
struct Unit;using Creature=Unit;
struct Position{float GetPositionX(){return 0;}float GetPositionY(){return 0;}float GetPositionZ(){return 0;}float GetOrientation(){return 0;}}lastgtppos;
struct SpellEffect{float value=20;SpellEffect*operator->(){return this;}float CalcValue(Unit* =nullptr)const{return value;}};
struct SpellInfo{uint32 Id=1;SpellEffect Effects[5];};
struct AuraEffect{float amount=0,old=0;uint32 id=1,trigger=2;int misc=1;void*unused=nullptr;Unit*caster=nullptr;float GetAmount()const{return amount;}float GetOldBaseAmount()const{return old;}void SetAmount(float a){amount=a;}AuraEffect*GetEffect(int){return this;}AuraEffect*GetBase(){return this;}bool removed=false;void Remove(){removed=true;}Unit*GetCaster()const{return caster;}uint32 GetId()const{return id;}uint32 GetTriggerSpell()const{return trigger;}int GetMiscValue()const{return misc;}void HandleAuraProcOnHpBelow(struct AuraApplication const*,uint8,bool)const;};
struct AuraApplication{Unit*target;Unit*GetTarget()const{return target;}};
struct DamageInfo{uint32 value;uint32 GetDamage(){return value;}};
struct Events{std::map<uint32,uint32> scheduled;void RescheduleEvent(uint32 id,uint32 delay){scheduled[id]=delay;}};
struct Instance{int state=IN_PROGRESS;uint32 updates=0;int GetBossState(uint32){return state;}uint64 GetGuidData(uint32){return 1;}void SetData(uint32,uint32){++updates;}}instance;
struct Map{bool heroic=false;bool IsHeroic(){return heroic;}}map;
struct Unit{uint64 maximum=1000,health=1000;int32 power=100;bool cooldown=false,has=false,dead=false,failSummon=false;int32 faction=0;uint32 display=0,flags=0,removes=0,casts=0;float lastCustom=-1;uint32 lastSpell=0;AuraEffect effect;std::vector<uint32>removed;Unit*other=nullptr;Unit(){effect.caster=this;}
 uint64 GetMaxHealth()const{return maximum;}uint64 GetHealth()const{return health;}uint64 CountPctFromMaxHealth(int32 p)const{return uint64(maximum*float(p)/100.0f);}float GetHealthPct()const{return float(double(health)*100.0/maximum);}void SetHealth(uint64 h){health=std::min(maximum,h);}void ModifyHealth(int64 d){if(d<0&&uint64(-d)>health)health=0;else SetHealth(uint64(int64(health)+d));}int32 GetPower(uint32){return power;}void SetPower(uint32,int32 p){power=p;}void ModifyPower(uint32,int32 d){power=int32(std::max<int64>(0,std::min<int64>(INT32_MAX,int64(power)+d)));}Unit*ToPlayer(){return this;}bool HasSpellCooldown(uint32){return cooldown;}bool HasAura(uint32){return has;}AuraEffect*GetAura(uint32){return &effect;}AuraEffect*AddAura(uint32,Unit*){has=true;return &effect;}void CastSpell(Unit*,uint32 id,...){++casts;lastSpell=id;}void CastCustomSpell(Unit*,uint32 id,float const*bp,...){++casts;lastSpell=id;lastCustom=*bp;}void CastCustomSpell(uint32 id,int,float bp){++casts;lastSpell=id;lastCustom=float(int32(bp));}void RemoveAurasDueToSpell(uint32 id){removed.push_back(id);}void RemoveFlag(uint32,uint32){}void SetFlag(uint32,uint32){}void SetDisplayId(uint32 i){display=i;}void SetReactState(uint32){}Map*GetMap(){return &map;}void Kill(Unit*,bool){dead=true;}void DespawnOrUnsummon(){++removes;}Unit*SummonCreature(uint32,...){return failSummon?nullptr:other;}void RemoveAllAuras(){}uint64 GetGUID(){return 1;}};
namespace ObjectAccessor{Unit*target=nullptr;Unit*GetUnit(Unit&,uint64){return target;}}
struct Manager{bool found=true;SpellInfo info;SpellInfo const*GetSpellInfo(uint32){return found?&info:nullptr;}}manager;auto sSpellMgr=&manager;
struct Base{Unit*me;Unit*caster=nullptr;Unit*target=nullptr;Instance*instance=&::instance;Events events;SpellInfo spell;bool toDie=false;uint8 donesp=0,donecp=0;float lasthppct=0;bool phase_two=true;uint32 effectValue=20;Base(Unit*c):me(c),caster(c),target(c){}virtual~Base()=default;virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}Unit*GetCaster(){return caster;}Unit*GetTarget(){return target;}Unit*GetHitUnit(){return target;}SpellInfo*GetSpellInfo(){return &spell;}int32 GetEffectValue(){return effectValue;}uint32 prevents=0;void PreventDefaultAction(){++prevents;}template<class...T>void DoCast(T...){++me->casts;}void SpawnWave(){}struct Summons{bool empty(){return true;}void DespawnEntry(uint32){}}summons;};
#include "Constants.inc"
#include "Selected.inc"
void Check(bool ok,char const*why){if(!ok){std::cerr<<why<<'\n';std::abort();}}bool Near(float a,float b){return std::abs(double(a)-b)<=std::max(1.0,std::abs(double(b))*1e-6);}
int main(){Unit u,v;u.other=&v;
#if defined(CASE_HPRTHRESHOLD)
#error incorrect macro
#elif defined(CASE_HPTHRESHOLD)
 AuraEffect effect;effect.caster=&u;effect.amount=20;AuraApplication app{&v};v.maximum=30000000000ULL;v.health=4000000000ULL;effect.HandleAuraProcOnHpBelow(&app,AURA_EFFECT_HANDLE_REAL,true);
#ifdef COUNTERFACTUAL
 Check(u.casts==0,"original threshold truncation must miss below-health proc");
#else
 Check(u.casts==1,"full-width below-health proc");u.casts=0;v.health=7000000000ULL;effect.HandleAuraProcOnHpBelow(&app,AURA_EFFECT_HANDLE_REAL,true);Check(!u.casts,"above threshold must not proc below branch");effect.misc=0;effect.HandleAuraProcOnHpBelow(&app,AURA_EFFECT_HANDLE_REAL,true);Check(u.casts==1,"native above branch");u.casts=0;v.health=6000000000ULL;effect.HandleAuraProcOnHpBelow(&app,AURA_EFFECT_HANDLE_REAL,true);Check(!u.casts,"strict equality no proc");effect.caster=nullptr;effect.HandleAuraProcOnHpBelow(&app,AURA_EFFECT_HANDLE_REAL,true);Check(!u.casts,"missing caster no proc");
#endif
#else
 Frame f(&u);
#if defined(CASE_DIVINE)
 u.health=500;DamageInfo hit{600};float amount=0;f.Absorb(nullptr,hit,amount);
#ifdef COUNTERFACTUAL
 Check(u.lastCustom>1e18f,"old unsigned health gap creates enormous unwanted heal");
#else
 Check(u.lastCustom<0&&Near(amount,300),"above20percent no supplemental heal");u.health=100;u.lastCustom=-1;f.Absorb(nullptr,hit,amount);Check(Near(u.lastCustom,100)&&Near(amount,600),"below20percent heal only positive gap and absorb lethal");u.cooldown=true;u.casts=0;f.Absorb(nullptr,hit,amount);Check(!u.casts,"cooldown refusal preserved");u.cooldown=false;u.has=true;f.Absorb(nullptr,hit,amount);Check(!u.casts,"forbearance refusal preserved");
#endif
#elif defined(CASE_FROSTSHIELD)
 u.maximum=20000000000ULL;AuraEffect effect;effect.old=100;float amount=5000000000.f;bool recalc=false;f.CalculateAmount(&effect,amount,recalc);
#ifdef COUNTERFACTUAL
 Check(amount<0,"old frost cap signed narrowing");
#else
 Check(Near(amount,4000000000.f),"20percent cap at native health width");u.maximum=1000;amount=10;f.CalculateAmount(&effect,amount,recalc);Check(Near(amount,110),"uncapped old amount addition preserved");f.caster=nullptr;amount=10;f.CalculateAmount(&effect,amount,recalc);Check(amount==10,"missing caster unchanged");
#endif
#elif defined(CASE_DECIMATE)
 u.maximum=30000000000ULL;u.health=u.maximum;f.HandleScript(0);
#ifdef COUNTERFACTUAL
 Check(u.health!=6000000000ULL,"old decimate truncates target health");
#else
 Check(u.health==6000000000ULL,"native20percent destination preserved");u.health=100;f.HandleScript(0);Check(u.health==100,"already below threshold untouched");
#endif
#elif defined(CASE_ALAR)
 v.maximum=10000000000ULL;v.health=8000000000ULL;ObjectAccessor::target=&v;u.health=100;uint32 damage=100;f.DamageTaken(&v,damage,0);
#ifdef COUNTERFACTUAL
 Check(v.health!=7700000000ULL,"old Alar signed truncation");
#else
 Check(v.health==7700000000ULL&&damage==0&&f.toDie,"ember native3percent once");f.DamageTaken(&v,damage,0);Check(v.health==7700000000ULL,"repeated callback cannot replay");Frame retry(&u);v.health=10;damage=100;retry.DamageTaken(&v,damage,0);Check(v.health==1,"original one-health floor");
#endif
#elif defined(CASE_GARROSH)
 u.maximum=v.maximum=12000000000ULL;u.health=8000000000ULL;u.power=77;f.Enter(&v);
#ifdef COUNTERFACTUAL
 Check(v.health!=u.health,"old realm transfer truncates health");
#else
 Check(v.health==u.health&&v.power==77,"full enter health/power");v.health=7000000000ULL;f.Return(&v);Check(u.health==7000000000ULL&&v.removes==1,"full return before despawn");u.health=8200000000ULL;f.Final();Check(v.health==8200000000ULL,"final clone preserves full health");u.failSummon=true;u.health=1000;auto prior=v.health;f.Final();Check(v.health==prior,"failed clone grants no clone health");
#endif
#elif defined(CASE_GARROSHHEAL)
 u.maximum=12000000000ULL;u.health=1000000000ULL;f.Heal();
#ifdef COUNTERFACTUAL
 Check(u.health!=8200000000ULL,"old signed final-phase heal damages Garrosh instead");
#else
 Check(u.health==8200000000ULL,"native60percent heal remainspositive");u.maximum=1000;u.health=100;f.Heal();Check(u.health==700,"ordinary60percent heal unchanged");u.health=900;f.Heal();Check(u.health==1000,"native max-health cap");
#endif
#elif defined(CASE_IMMERSEUS)
 u.maximum=1000;u.health=1000;f.lasthp=10;f.donesp=20;f.DoAction(ACTION_INTRO_PHASE_ONE);
#ifdef COUNTERFACTUAL
 Check(u.health==u.maximum,"old unsigned split decrement refills instead of1percent floor");
#else
 Check(u.health==10,"insufficient saved health gets authored1percent floor");u.maximum=30000000000ULL;f.lasthp=20000000000ULL;f.donesp=10;f.DoAction(ACTION_INTRO_PHASE_ONE);Check(Near(float(u.health),17000000000.f),"high split checkpoint/decrement preserves nativefloat precision");f.donesp=0;f.DoAction(ACTION_INTRO_PHASE_ONE);Check(Near(float(u.health),17000000000.f),"zero completions retains checkpoint");u.power=0;f.DoAction(ACTION_INTRO_PHASE_ONE);Check(u.dead,"native energy0 completion retained");
#endif
#elif defined(CASE_MIASMA)
 u.maximum=5000000000ULL;u.has=true;u.effect.amount=2000000000.f;DamageInfo hit{1};float amount=1000000000.f;f.AfterAbsorb(nullptr,hit,amount);
#ifdef COUNTERFACTUAL
 Check(!Near(u.effect.amount,3000000000.f),"old signed barrier cannot retain3b");
#else
 Check(Near(u.effect.amount,3000000000.f)&&Near(u.lastCustom,3000000000.f),"barrier and native cosmetic float cast retain3b");amount=4000000000.f;f.AfterAbsorb(nullptr,hit,amount);Check(Near(u.effect.amount,5000000000.f),"native100percent cap");u.maximum=1000;u.effect.amount=0;amount=200;f.AfterAbsorb(nullptr,hit,amount);Check(u.lastSpell==ancientbarrierbar[0],"low visual tier");amount=200;f.AfterAbsorb(nullptr,hit,amount);Check(u.lastSpell==ancientbarrierbar[1],"middle visual tier");
#endif
#elif defined(CASE_MANABARRIER)
 u.maximum=6000000000ULL;u.health=2000000000ULL;u.power=1000;f.HandlePeriodicTick(nullptr);
#ifdef COUNTERFACTUAL
 Check(u.health!=u.maximum,"old signed missing health damages instead of restoring");
#else
 Check(u.health==u.maximum&&u.power==0&&f.prevents==1,"full health restoration,finite native mana depletion");u.maximum=1000;u.health=900;u.power=200;f.HandlePeriodicTick(nullptr);Check(u.health==1000&&u.power==100,"ordinary healing/mana amount unchanged");f.caster=nullptr;f.HandlePeriodicTick(nullptr);Check(u.power==100,"missing caster no change");
#endif
#endif
#endif
 std::cout<<"PASS selected production source callback/health dataflow;controlled endpoints,not full native encounter/client traversal\n";}
