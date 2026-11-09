#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <cmath>
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;using int64=int64_t;using DamageEffectType=int;using AuraEffectHandleModes=int;
struct Unit;using Creature=Unit;
struct StubAI{uint32 attacks=0,gains=0;void AttackStart(Unit*){++attacks;}void GainSentinelAbility(uint32){++gains;}};
static std::map<uint64,Unit*>units;
struct Unit{uint64 maximum=1000,health=1000,guid=1;float scale=1.f;uint32 entry=0,casts=0,despawns=0,summons=0;bool invalid=false;uint32 invalidAccess=0;bool alive=true,player=true,near=true,has=true,cooldown=false,fail=false;StubAI ai;Unit*spawn=nullptr;
 uint64 GetMaxHealth(Unit*v=nullptr)const{return v&&scale!=1.f?uint64(maximum*scale):maximum;}uint64 GetHealth(Unit*v=nullptr)const{return v&&scale!=1.f?uint64(health*scale):health;}float GetHealthPct()const{return maximum?float(100.0*health/maximum):0.f;}uint64 CountPctFromMaxHealth(int pct,Unit*v=nullptr)const{return uint64(GetMaxHealth(v)*float(pct)/100.0f);}void SetHealth(uint64 v){health=std::min(maximum,v);}void ModifyHealth(int64 v){SetHealth(v<0&&uint64(-v)>health?0:uint64(int64(health)+v));}bool IsAlive(){return alive;}bool isDead(){return !alive;}bool IsPlayer(){return player;}bool IsWithinDistInMap(Unit*,float,bool){return near;}bool HasAura(uint32){return has;}bool HasSpellCooldown(uint32){return cooldown;}void RemoveAurasDueToSpell(uint32){}void SetPower(uint32,uint32){}uint32 GetEntry(){return entry;}StubAI*AI(){return &ai;}float GetPositionX(){return 0;}float GetPositionY(){return 0;}float GetPositionZ(){return 0;}Unit*SummonCreature(uint32,...){++summons;if(fail){
#ifdef COUNTERFACTUAL
 spawn->invalid=true;return spawn;
#else
 return nullptr;
#endif
 }return spawn;}void SetDisplayId(uint32){if(invalid)++invalidAccess;}void AddThreat(Unit*,float){}void DespawnOrUnsummon(){++despawns;}static Unit*GetCreature(Unit&,uint64 g){return units[g];}static Unit*GetUnit(Unit&,uint64 g){return units[g];}struct FakeAura{uint8 GetStackAmount(){return 1;}}aura;FakeAura*GetAura(uint32){return &aura;}};
struct AuraEffect{};struct DamageInfo{uint32 damage;uint32 GetDamage()const{return damage;}};struct ProcEventInfo{DamageInfo*damage;DamageInfo*GetDamageInfo(){return damage;}};
struct Events{std::map<uint32,uint32>scheduled;void Reset(){scheduled.clear();}void ScheduleEvent(uint32 e,uint32 d){scheduled[e]=d;}void RescheduleEvent(uint32 e,uint32 d){scheduled[e]=d;}};
struct Instance{uint32 state=0,updates=0;uint32 GetBossState(uint32){return state;}void SetBossState(uint32,uint32 v){state=v;++updates;}};
struct Base{Unit*me,*ownerUnit,*target,*combat_trigger=nullptr;Instance*instance;Instance ownInstance;Events events;uint32 HealTimer=0,healthPct=90,donehppct=100,timerpower=0,actualtimer=0,loaspirittimer=0,ability=1,m_uiSummonSkeletonTimer=0,m_uiSummonSuppressorTimer=0,talks=0,prevents=0,removes=0;bool isProc=false,despawn=false,bIntro=false,needresetevents=false,heroic=false;uint64 NearbyGUID[3]{11,12,13},m_auiLackeyGUID[4]{11,12,13,14};Unit*healed=nullptr;Base(Unit*u):me(u),ownerUnit(u),target(u),instance(&ownInstance){}Unit*GetTarget(){return target;}Unit*GetUnitOwner(){return ownerUnit;}void PreventDefaultAction(){++prevents;}void DoCast(Unit*t,uint32,bool=false){++me->casts;healed=t;}void DoCastVictim(uint32){++me->casts;}void Talk(uint32){++talks;}void ResetEvents(){}void RemovePassenger(bool){++removes;}uint32 DungeonMode(uint32 a,uint32 b){return heroic?b:a;}};
#define DUNGEON_MODE(a,b) DungeonMode(a,b)
#define CAST_AI(T,p) (p)
#include "Constants.inc"
#include "Selected.inc"
void Check(bool ok,char const*why){if(!ok){std::cerr<<why<<'\n';std::abort();}}
int main(){Unit a,b,c,d;a.spawn=&b;units[11]=&b;units[12]=&c;units[13]=nullptr;units[14]=&d;Frame f(&a);auto heal=[&]{f.HealTimer=0;
#if defined(CASE_DELRISSA)
 f.Heal(1);
#endif
 };
#if defined(CASE_HADRONOX)
 a.maximum=30000000000ULL;a.health=10000000000ULL;uint64 expected=a.health+a.CountPctFromMaxHealth(10);f.KilledUnit(&b);
#ifdef COUNTERFACTUAL
 Check(a.health!=expected,"original leech narrowing counterfactual");
#else
 Check(a.health==expected,"native ten percent heal");a.health=a.maximum-1;f.KilledUnit(&b);Check(a.health==a.maximum,"heal capped");b.has=false;a.health=50;f.KilledUnit(&b);f.KilledUnit(nullptr);Check(a.health==50,"poison/null gate");b.has=true;a.alive=false;f.KilledUnit(&b);Check(a.health==50,"dead gate");
#endif
#elif defined(CASE_SENTINEL)
 b.maximum=10000000000ULL;b.health=1000000000ULL;c.alive=false;uint64 expected=b.health+b.CountPctFromMaxHealth(50);f.JustDied(&d);
#ifdef COUNTERFACTUAL
 Check(b.health!=expected,"original sentinel heal narrowed");
#else
 Check(b.health==expected&&b.ai.gains==1&&c.ai.gains==0,"living buddy only heal and ability");b.health=b.maximum-1;f.JustDied(nullptr);Check(b.health==b.maximum,"heal cap and null killer");
#endif
#elif defined(CASE_VALITHRIAHEALTH)
 a.maximum=12000000000ULL;f.Setup();
#ifdef COUNTERFACTUAL
 Check(a.health!=6000000000ULL,"original half health narrowing");
#else
 Check(a.health==a.CountPctFromMaxHealth(50),"reset half");a.health=1;f.IntroHealth();Check(a.health==a.CountPctFromMaxHealth(50),"intro half");a.maximum=1001;f.Setup();Check(a.health==500,"ordinary odd half");
#endif
#elif defined(CASE_FIRESWORN)
 a.maximum=10000000000ULL;a.health=3000000000ULL;uint32 damage=1;f.Hit(&b,damage,0);
#ifdef COUNTERFACTUAL
 Check(a.despawns==1&&damage==0,"original early eruption from signed health");
#else
 Check(a.despawns==0&&damage==1,"high health chip not eruption");a.maximum=1000;a.health=1000;damage=900;f.Hit(&b,damage,0);Check(!a.despawns,"strict threshold equality");damage=901;f.Hit(nullptr,damage,0);Check(a.despawns==1&&damage==0,"threshold crossed eruption");a.health=5;damage=UINT32_MAX;f.Hit(&b,damage,0);Check(a.despawns==2&&damage==0,"overkill bounded");a.maximum=1000;a.health=1000;a.scale=0.1f;damage=91;f.Hit(&b,damage,0);Check(a.despawns==3&&damage==0,"post-scaling target-relative threshold");
#endif
#elif defined(CASE_DELRISSA)
 a.health=900;b.health=100;c.health=300;d.health=200;heal();
#ifdef COUNTERFACTUAL
 Check(f.healed==&d,"original last lower target counterfactual");
#else
 Check(f.healed==&b,"lowest living regardless iteration");b.alive=false;heal();Check(f.healed==&d,"dead skipped");a.maximum=10000000000ULL;a.health=6000000000ULL;b.alive=true;b.maximum=10000000000ULL;b.health=5000000000ULL;c.alive=d.alive=false;heal();Check(f.healed==&b,"full width lower target");b.health=a.health;heal();Check(f.healed==&a,"tie retains self");units.clear();heal();Check(f.healed==&a,"missing adds self fallback");
#endif
#elif defined(CASE_FINKLE)
 a.health=20000;a.maximum=100000;DamageInfo hit{1000};float absorb=0;f.Absorb(nullptr,hit,absorb);
#ifdef COUNTERFACTUAL
 Check(absorb>1000,"original unsigned nonlethal excess");
#else
 Check(absorb==0,"nonlethal no absorb");hit.damage=25000;f.Absorb(nullptr,hit,absorb);Check(absorb==5001,"leave one health");a.health=9999;absorb=0;f.Absorb(nullptr,hit,absorb);Check(!absorb,"health admission threshold");a.health=10000;hit.damage=10000;f.Absorb(nullptr,hit,absorb);Check(absorb==1,"equal lethal");a.maximum=10000000000ULL;a.health=5000000000ULL;hit.damage=UINT32_MAX;absorb=0;f.Absorb(nullptr,hit,absorb);Check(!absorb,"full width nonlethal");f.target=nullptr;f.Absorb(nullptr,hit,absorb);
#endif
#elif defined(CASE_COUNCIL)
 a.health=200;a.maximum=1000;f.PassengerBoarded(&b,0,true);
#ifdef COUNTERFACTUAL
 Check(f.donehppct>100,"original low health possession unsigned floor");
#else
 Check(f.donehppct==1,"low clamp");for(uint32 hp:{250u,259u,260u,1000u}){a.health=hp;f.PassengerBoarded(&b,0,true);Check(f.donehppct>=1&&f.donehppct<=75,"all release floors");}Check(f.donehppct==75,"full-health25-point window");f.PassengerBoarded(&b,0,false);Check(f.timerpower==0&&f.actualtimer==0,"native removal cleanup");
#endif
#elif defined(CASE_NEFARIAN)
 uint32 damage=1;f.Hit(&b,damage,0);
#ifdef COUNTERFACTUAL
 Check(a.casts==1&&f.healthPct==0,"original first chip consumed all checkpoints");
#else
 Check(a.casts==0&&f.healthPct==90,"chip above90");a.health=900;f.Hit(&b,damage,0);Check(a.casts==1&&f.healthPct==80,"cross first threshold");a.health=899;f.Hit(&b,damage,0);Check(a.casts==1,"no duplicate same decile");damage=UINT32_MAX;f.Hit(nullptr,damage,0);Check(a.casts==2&&f.healthPct==0,"overkill once");f.Hit(&b,damage,0);Check(a.casts==2,"consumed boundary");a.maximum=10000000000ULL;a.health=9500000000ULL;f.healthPct=90;damage=1;f.Hit(&b,damage,0);Check(a.casts==2,"high hp no premature threshold");a.maximum=1000;a.health=1000;a.scale=0.1f;damage=11;f.Hit(&b,damage,0);Check(a.casts==3&&f.healthPct==80,"target-relative scaled crossing");
#endif
#elif defined(CASE_REFLECTION)
 a.health=100;DamageInfo hit{101};ProcEventInfo info{&hit};f.OnProc_0(nullptr,info);
#ifdef COUNTERFACTUAL
 Check(!f.isProc&&f.prevents==1,"original overkill underflow rejects proc");
#else
 Check(f.isProc&&!f.prevents,"lethal admitted");f.OnProc_1(nullptr,info);Check(!f.isProc&&!f.prevents,"paired effect consumes once");f.OnProc_1(nullptr,info);Check(f.prevents==1,"repeated second effect rejected");a.health=600;hit.damage=100;f.OnProc_0(nullptr,info);Check(!f.isProc&&f.prevents==2,"strict50 equality");hit.damage=101;f.OnProc_0(nullptr,info);Check(f.isProc,"strict below50");f.OnProc_1(nullptr,info);a.cooldown=true;f.OnProc_0(nullptr,info);Check(!f.isProc,"cooldown rejection");f.ownerUnit=nullptr;f.OnProc_0(nullptr,info);
#endif
#elif defined(CASE_VALITHRIATRIGGER)
 a.fail=true;
#ifdef COUNTERFACTUAL
 // Instrument missing spawn with a sentinel, avoiding deliberate null UB in the baseline.
 f.MoveInLineOfSight(&c);Check(f.bIntro&&f.talks==1&&a.summons==1&&b.invalidAccess==1,"original consumes intro before unguarded spawn result");
#else
 f.MoveInLineOfSight(&c);Check(!f.bIntro&&!f.talks&&f.events.scheduled.empty()&&!f.instance->updates,"failed spawn leaves start retryable");a.fail=false;f.MoveInLineOfSight(&c);Check(f.bIntro&&f.talks==1&&f.events.scheduled.size()==8&&a.summons==2,"retry initializes once");f.MoveInLineOfSight(&c);Check(a.summons==2&&f.talks==1,"repeat LOS no duplicate");f.bIntro=false;c.player=false;f.MoveInLineOfSight(&c);Check(a.summons==2,"pet cannot start");c.player=true;c.near=false;f.MoveInLineOfSight(&c);Check(a.summons==2,"distance gate");
#endif
#endif
 std::cout<<"PASS selected source regression; counterfactual controls and controlled endpoints, not full encounter traversal\n";
}
