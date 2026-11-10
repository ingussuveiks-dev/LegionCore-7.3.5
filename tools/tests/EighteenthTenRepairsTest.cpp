#include <algorithm>
#include <cmath>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <iostream>
using uint64=uint64_t;using uint32=uint32_t;using int32=int32_t;
enum{ACTION_1=1,ACTION_TERMINATE_HIDE_PHASE=3,EVENT_1=1,EVENT_2=2,SELECT_TARGET_TOPAGGRO=0};
struct Position{float x=0,y=0;float GetPositionX(){return x;}float GetPositionY(){return y;}void Relocate(float a,float b){x=a;y=b;}};
struct AuraEffect{float amount=0;uint32 stacks=0;float GetAmount(){return amount;}void SetAmount(float a){amount=a;}uint32 GetStackAmount(){return stacks;}};
struct Motion{void MoveTakeoff(uint32,float,float,float){}};
struct Creature{uint64 hp=1,max=1;float x=0,y=0,z=0;bool fly=false;Motion motion;uint64 GetHealth(){return hp;}uint64 GetMaxHealth(){return max;}void SetHealth(uint64 h){hp=std::min(h,max);}void SetMaxHealth(uint64 h){max=h;}uint64 CountPctFromMaxHealth(uint32 n){return max*n/100;}float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}void StopAttack(bool){}void SetDisableGravity(bool){}void SetCanFly(bool b){fly=b;}Motion*GetMotionMaster(){return &motion;}void CastSpell(Creature*,uint32,bool){}bool HasAura(uint32){return true;}};using Unit=Creature;
struct Events{uint32 retry=0;void RescheduleEvent(uint32,uint32 n){retry=n;}void DelayEvent(uint32,uint32){}};
struct Base{Creature*me,*other;Position pos;Events events;uint32 accepted=0,EldersCount=3,jumpTimer=0,timeCount=99,count_adds=0,timer_check=0,wins=0;bool targetPresent=false;AuraEffect effect,*aurEff=&effect,*aura=&effect;std::set<uint64>activeAddGuids;explicit Base(Creature*u):me(u),other(u){}Creature*GetTarget(){return me;}Creature*SelectTarget(uint32){return targetPresent?other:nullptr;}};
static void Require(bool ok,char const*s){if(!ok)throw std::runtime_error(s);}
static int32 CounterfactualSigned32(uint64 n){return n>INT32_MAX?0:int32(n);}
#include "Selected.inc"
int main(int argc,char**argv){try{Creature c;Frame f(&c);bool fixed=false;
#if defined(CASE_TOPPSRAY)
f.pos.x=0;f.pos.y=5;f.Run();fixed=std::isfinite(f.pos.x)&&std::isfinite(f.pos.y)&&f.pos.x==0&&f.pos.y==75;f.pos={0,0};f.Run();
#ifndef COUNTERFACTUAL
Require(f.events.retry==3000,"coincident target retains authored retry without NaN cast");f.pos={3,4};f.Run();Require(std::abs(f.pos.x-45)<0.01f&&std::abs(f.pos.y-60)<0.01f,"nonaxis ray retains actual target angle");
#endif
#elif defined(CASE_FOCUSACTION)||defined(CASE_LEISHIACTION)
f.Run(-7);fixed=f.accepted==0;
#ifndef COUNTERFACTUAL
f.Run(
#ifdef CASE_FOCUSACTION
ACTION_1
#else
ACTION_TERMINATE_HIDE_PHASE
#endif
);Require(f.accepted==1,"matching native action admitted");
#endif
#elif defined(CASE_LORYHEALTH)
c.max=uint64(1)<<40;f.Run(&c);fixed=c.hp==uint64(c.max*0.1f);
#elif defined(CASE_MELJARAKHEAL)
c.max=24000000000ULL;c.hp=1;f.Run();fixed=c.hp==6000000001ULL;
#elif defined(CASE_GARROSHABSORB)
c.max=15000000000ULL;f.Apply();f.Run();fixed=c.hp>14999999000ULL&&f.effect.amount>0;
#elif defined(CASE_NIGHTMOTHERLANDING)
f.Run();fixed=c.fly&&f.jumpTimer==6000;
#ifndef COUNTERFACTUAL
f.targetPresent=true;f.jumpTimer=0;f.Run();Require(f.jumpTimer==6000,"target-present path retains timer");
#endif
#elif defined(CASE_GLOWCAPSTACK)
f.effect.stacks=100;f.Run();fixed=f.timeCount==4;
#ifndef COUNTERFACTUAL
for(auto n:{0u,49u,50u,99u,101u,255u}){f.effect.stacks=n;f.Run();Require(f.timeCount==(n>100?5u:n>=50?4u:3u),"adjacent authored brackets");}
#endif
#elif defined(CASE_FREYAHEALTH)
c.max=10000000000ULL;f.Run(&c);fixed=c.max==16000000000ULL;
#elif defined(CASE_MASTERROSTER)
f.count_adds=3;f.Reset();fixed=f.count_adds==0;f.Join(1);f.Join(2);f.Join(3);f.Die(1);f.Die(1);
#ifndef COUNTERFACTUAL
Require(f.count_adds==2&&!f.wins,"duplicate death cannot consume a second illusion");f.Die(2);f.Die(3);Require(f.wins==1,"last tracked real death wins once");f.Die(3);Require(f.wins==1,"repeat last death ignored");f.Reset();f.Die(99);Require(!f.count_adds,"stale unknown death cannot underflow new attempt");
#endif
#endif
#ifdef COUNTERFACTUAL
Require(!fixed,"pinned old body must reproduce selected defect");
#else
Require(fixed,"repaired production contract");
#endif
std::cout<<"PASS selected repaired/counterfactual contract "<<(argc>1?argv[1]:"")<<"\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
