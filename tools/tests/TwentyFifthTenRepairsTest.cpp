#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
#include "Constants.inc"
static void Require(bool b,char const*m){if(!b)throw std::runtime_error(m);}
struct Aura{uint32 stacks=0;uint32 GetStackAmount()const{return stacks;}};
struct Unit;using Creature=Unit;struct Map{Unit*controller=nullptr;Unit*GetCreature(uint32){return controller;}};
struct Instance{Map*instance;uint32 GetGuidData(uint32){return 1;}};
struct Unit{uint64 hp=100,relativeHp=10;uint32 entry=0;std::map<uint32,Aura>auras;uint64 GetHealth(Unit*u=nullptr){return u?relativeHp:hp;}bool HealthBelowPct(uint32 n){return hp<n;}uint32 GetEntry(){return entry;}Aura const*GetAura(uint32 n){auto i=auras.find(n);return i==auras.end()?nullptr:&i->second;}void StopAttack(){}template<class...T>void SetFlag(T...){}void RemoveAurasDueToSpell(uint32 n){auras.erase(n);}Unit*AI(){return this;}void DoAction(int32){++actions;}uint32 actions=0;void RemoveFlag(uint32,uint32){}void SetReactState(uint32){}void Attack(Unit*,bool){}struct Motion{void MoveChase(Unit*){}}motion;Motion*GetMotionMaster(){return &motion;} };
struct Events{uint32 phase=1,scheduled=0;bool IsInPhase(uint32 n){return phase==n;}void SetPhase(uint32 n){phase=n;}template<class...T>void RescheduleEvent(T...){++scheduled;}template<class...T>void ScheduleEvent(T...){++scheduled;}};
struct Base{Creature*me;Instance*instance=nullptr;Events events;bool finalPhase=false,_stacked=false,stacksdone=false,soulpower=false,killedByRitualStrike=false,active=true,done=false,explose=false;Unit*selected=nullptr;uint32 berserkPct=20,soulcount=4,casts=0;explicit Base(Creature*c):me(c){}Unit*SelectTarget(uint32,uint32,float,bool){return selected;}bool HealthBelowPct(uint32 n){return me->HealthBelowPct(n);}bool HealthAbovePct(uint32 n){return me->hp>n;}void DoCast(Creature*,uint32,bool=false){++casts;}void DoCast(uint32 n){DoCast(me,n);} };
static uint32 urand(uint32 a,uint32){return a;}
#include "Selected.inc"
template<class F>static void Hit(F&f,Unit*p,uint32&d){
#ifdef COUNTERFACTUAL
 d=std::max<uint32>(1,d/10);f.DamageTaken(p,d,0);d*=10;
#else
 f.DamageTakenAfterScaling(p,d,0);
#endif
}
int main(){try{Unit me,p,controller;Frame f(&me);Map map{&controller};Instance instance{&map};uint32 d=10;bool fixed=false;
#if defined(CASE_XAVIUSSTACKED)
 me.auras[221315].stacks=10;Hit(f,&p,d);fixed=f._stacked&&d==10;
#ifndef COUNTERFACTUAL
 Frame shortStack(&me);me.auras[221315].stacks=9;d=10;Hit(shortStack,&p,d);Require(!shortStack._stacked,"nine stacks cannot award ten-stack criterion");me.auras.clear();Frame missing(&me);d=10;Hit(missing,&p,d);Require(!missing._stacked,"missing aura cannot award");
#endif
#elif defined(CASE_NARAXASRAVENOUS)
 me.auras[SPELL_RAVENOUS].stacks=6;Hit(f,&p,d);fixed=f.stacksdone&&d==10;
#ifndef COUNTERFACTUAL
 Frame low(&me);me.auras[SPELL_RAVENOUS].stacks=5;d=10;Hit(low,&p,d);Require(!low.stacksdone,"five stacks cannot award six-stack criterion");me.auras.clear();Frame absent(&me);d=10;Hit(absent,&p,d);Require(!absent.stacksdone,"missing aura cannot award");
#endif
#elif defined(CASE_BRONJAHMSOULPOWER)
 Hit(f,&p,d);fixed=f.soulpower&&d==10;
#ifndef COUNTERFACTUAL
 Frame low(&me);low.soulcount=3;d=10;Hit(low,&p,d);Require(!low.soulpower,"three souls cannot award four-soul criterion");Frame phase(&me);me.hp=20;d=1;Hit(phase,&p,d);Require(phase.events.scheduled==2&&phase.events.phase==PHASE_2,"native low-health soulstorm events retained");
#endif
#elif defined(CASE_SVALARITUALHULK)
 p.entry=CREATURE_SVALA_SORROWGRAVE;Hit(f,&p,d);fixed=f.killedByRitualStrike&&d==10;
#ifndef COUNTERFACTUAL
 Frame other(&me);p.entry=1;d=10;Hit(other,&p,d);Require(!other.killedByRitualStrike,"ordinary killer cannot award ritual criterion");d=10;Hit(other,nullptr,d);Require(!other.killedByRitualStrike,"null environmental source cannot become ritual");
#endif
#elif defined(CASE_CHANNELERTRANSFER)
 Hit(f,&p,d);fixed=f.casts==1&&d==10;
#ifndef COUNTERFACTUAL
 Frame nonlethal(&me);d=1;Hit(nonlethal,&p,d);Require(!nonlethal.casts&&d==1,"nonlethal hit cannot transfer soul");
#endif
#elif defined(CASE_RAIGONNWEAKSPOT)
 f.instance=&instance;Hit(f,&p,d);fixed=controller.actions==1&&d==10;
#ifndef COUNTERFACTUAL
 Frame missing(&me);d=10;Hit(missing,&p,d);Require(controller.actions==1,"absent instance cannot notify unrelated controller");map.controller=nullptr;f.instance=&instance;d=10;Hit(f,&p,d);Require(controller.actions==1,"missing controller safe");
#endif
#elif defined(CASE_ELEGONDISCHARGE)
 Hit(f,&p,d);fixed=!d&&!f.active&&f.casts==2;
#ifndef COUNTERFACTUAL
 d=10;Hit(f,&p,d);Require(!d&&f.casts==2,"charge discharges once");OtherFrame focus(&me);d=10;Hit(focus,&p,d);Require(!d&&!focus.active&&focus.casts==1,"focus inactivates once");d=10;Hit(focus,&p,d);Require(!d&&focus.casts==1,"inactive focus retains protection");focus.active=true;d=10;Hit(focus,&p,d);Require(!d&&focus.casts==2,"native reactivation can inactivate again");
#endif
#elif defined(CASE_LIVINGSANDRETURN)
 Hit(f,&p,d);fixed=!d&&f.done&&f.casts==2;
#ifndef COUNTERFACTUAL
 d=1;Hit(f,&p,d);Require(!d&&f.casts==2,"returned trap immune to queued nonlethal hits");f.DoAction(ACTION_ACTIVE);Require(f.done,"missing storm target retains trap");f.selected=&p;f.DoAction(ACTION_ACTIVE);Require(!f.done,"real target admits storm reactivation");d=10;Hit(f,&p,d);Require(!d&&f.casts==4,"native later storm may reactivate and return same actor");
#endif
#elif defined(CASE_AZUREFOGFREEZE)
 me.entry=NPC_AZURE_FOG;Hit(f,&p,d);fixed=f.explose&&f.casts==1&&d==10;
#ifndef COUNTERFACTUAL
 d=10;Hit(f,&p,d);Require(f.casts==1,"azure freeze once");Frame crimson(&me);me.entry=NPC_CRIMSON_FOG;d=10;Hit(crimson,&p,d);Require(!crimson.explose&&!crimson.casts&&d==10,"crimson cannot acquire blue-fog explosion");
#endif
#elif defined(CASE_CRACKLINGFLARE)
 Hit(f,&p,d);fixed=f.done&&f.casts==1&&d==10;
#ifndef COUNTERFACTUAL
 d=10;Hit(f,&p,d);Require(f.casts==1,"cauterizing flare once");
#endif
#else
#error Unknown case
#endif
#ifdef COUNTERFACTUAL
 Require(!fixed,"pinned raw-hook scaled-lethal counterfactual must remain broken");std::cout<<"PASS expected-broken pinned control\n";
#else
 Require(fixed,"repaired selected lethal lifecycle");std::cout<<"PASS repaired production extract under ASAN;not full encounter/client traversal\n";
#endif
 return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
