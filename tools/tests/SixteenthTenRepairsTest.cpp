#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <list>
#include <map>
#include <iostream>
using uint8=uint8_t;using int32=int32_t;using uint32=uint32_t;using uint64=uint64_t;using int64=int64_t;
static uint32 invalid=0,counterfactualNarrow=0;
uint32 CounterfactualNarrow32(float v){if(v>4294967295.0){++counterfactualNarrow;return 0;}return uint32(v);}
struct Queue{std::list<std::function<void()>>q;void Add(std::function<void()>f){q.push_back(std::move(f));}void Pump(){auto work=std::move(q);q.clear();for(auto&f:work)f();}};
using ObjectGuid=uint64;
struct GuidList{std::list<ObjectGuid>items;struct iterator{GuidList*list;std::list<ObjectGuid>::iterator at;ObjectGuid operator*(){if(at==list->items.end()){++invalid;return 0;}return *at;}iterator&operator++(){if(at!=list->items.end())++at;return *this;}};bool empty(){return items.empty();}size_t size(){return items.size();}iterator begin(){return {this,items.begin()};}void push_back(ObjectGuid g){items.push_back(g);}};
struct Unit;using Creature=Unit;using Player=Unit;struct AI{uint32 phase=1;uint32 GetData(uint32){return phase;}};
struct Unit{uint64 health=2000000000ULL,max=5000000000ULL;int32 mana=1000;uint32 mapId=1,instanceId=1,calls=0,kills=0,casts=0;bool world=true,alive=true,teleport=false,IsAIEnabled=true;AI ai;Queue q;uint64 GetHealth(){return health;}uint64 GetMaxHealth(){return max;}void SetHealth(uint64 n){health=std::min(n,max);}int64 ModifyHealth(int64 n){uint64 old=health;if(n<0)health=uint64(-n)>=health?0:health-uint64(-n);else health+=std::min<uint64>(n,max-health);return int64(health)-int64(old);}int32 GetPower(uint32){return mana;}void ModifyPower(uint32,int32 n){mana=std::max(0,mana+n);}AI*GetAI(){return &ai;}void Kill(Unit*){++kills;health=0;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instanceId;}bool IsBeingTeleported(){return teleport;}void AddDelayedEvent(uint32,std::function<void()>f){q.Add(std::move(f));}void TeleportTo(uint32,float,float,float,float,uint32){++calls;}void CastSpellDuration(Unit*,uint32,bool,uint32,uint8){++calls;}void CastSpell(Unit*,uint32,bool){++casts;}};
struct SpellInfo{uint32 Id=0;};struct AuraEffect{};static Unit*burrow=nullptr;
namespace ObjectAccessor{Creature*GetCreature(Unit&,ObjectGuid g){return g?burrow:nullptr;}}
struct Events{uint32 count=0,delay=0;void ScheduleEvent(uint32,uint32 n,uint32,uint32){++count;delay=n;}};
uint32 urand(uint32 a,uint32){return a;}
struct Base{Unit*me;Unit*caster;Creature*other=nullptr;uint32 prevented=0;Events events;explicit Base(Unit*u):me(u),caster(u){}Unit*GetCaster(){return caster;}void PreventDefaultAction(){++prevented;}Creature*GetOtherBoss(){return other;}};
#include "Constants.inc"
#include "Selected.inc"
void Check(bool ok,char const*why){if(!ok){std::cerr<<why<<'\n';std::abort();}}
int main(){Unit a,b,p;Frame f(&a);f.other=&b;burrow=&b;
#if defined(CASE_HATCHLINGTIMER)
 f.Reset();
#ifdef COUNTERFACTUAL
 Check(f.SpellVisualCast==0xCDCDCDCD,"old Reset leaves timer indeterminate sentinel");
#else
 Check(f.checking==1000&&f.SpellVisualCast==0,"fresh timer initialized");f.SpellVisualCast=9876;f.Reset();Check(f.SpellVisualCast==0,"every Reset deterministic");
#endif
#elif defined(CASE_MIMIRONHEALTH)
 a.max=9000000000ULL;f.Run(&a);
#ifdef COUNTERFACTUAL
 Check(a.health!=a.max/2,"signed32 cast loses assembled health");
#else
 Check(a.health==a.max/2,"full native half health");for(uint64 h:{1ULL,3ULL,4294967296ULL,12000000000ULL}){a.max=h;for(int i=0;i<3;++i){f.Run(&a);Check(a.health==h/2,"all three identical assembly sites");}}
#endif
#elif defined(CASE_CRYPTHEALTH)||defined(CASE_SQUADHEALTH)
 a.max=uint64(1)<<40;f.Run(&a);
#ifdef COUNTERFACTUAL
 Check(counterfactualNarrow==1,"old float value exceeds uint32;instrumentation avoids UB");
#else
#if defined(CASE_CRYPTHEALTH)
 Check(a.health==uint64(a.max*0.1f)&&a.health>UINT32_MAX,"native-width wounded bootstrap retains original float percent");
#else
 Check(a.health==uint64(a.max*0.9f)&&a.health>UINT32_MAX,"native-width squad health retains original float percent");
#endif
#endif
#elif defined(CASE_SINESTRABARRIER)
 f.HandlePeriodicTick(nullptr);
#ifdef COUNTERFACTUAL
 Check(a.health<a.max&&a.mana>1000,"old signed width damages caster and grants mana");
#else
 Check(a.health==a.max&&a.mana==0&&f.prevented==1,"full heal and bounded actual mana debit");a.health=100;a.mana=99;a.ai.phase=2;f.HandlePeriodicTick(nullptr);Check(a.health==a.max&&a.mana==99,"later phase retains original free refill gate");f.caster=nullptr;f.HandlePeriodicTick(nullptr);Check(f.prevented==3,"missing caster retains default prevention");
#endif
#elif defined(CASE_PYRRHICFOCUS)
 a.max=300000000000ULL;a.health=100000000000ULL;uint64 cost=uint64(0.01f*a.max);f.HandlePeriodicTick(nullptr);
#ifdef COUNTERFACTUAL
 Check(a.health>100000000000ULL,"old int32 cost heals instead of draining");
#else
 Check(a.health==100000000000ULL-cost,"native signed64 health debit");a.health=cost;f.HandlePeriodicTick(nullptr);Check(a.kills==1&&!a.health,"exact native lethal boundary");a.health=cost+1;f.HandlePeriodicTick(nullptr);Check(a.health==1&&a.kills==1,"one health above boundary");
#endif
#elif defined(CASE_TWINHEAL)
 a.max=10000000000ULL;a.health=9000000000ULL;b.max=5000000000ULL;b.health=4000000000ULL;SpellInfo info{SPELL_HEAL_BROTHER};f.SpellHit(&p,&info);
#ifdef COUNTERFACTUAL
 Check(a.health!=9000030000ULL,"old totals/current health truncate");
#else
 Check(a.health==9000030000ULL&&b.health==4000000000ULL,"native-width compensation keeps other boss untouched");a.health=a.max-7;f.SpellHit(&p,&info);Check(a.health==a.max,"cap before numeric conversion/addition");a.max=2;b.max=uint64(1)<<60;b.health=b.max-1;f.SpellHit(&p,&info);Check(b.health==b.max,"large ratio bounded before cast");a.max=0;f.SpellHit(&p,&info);Check(b.health==b.max,"zero maximum avoids division");
#endif
#elif defined(CASE_ANUBBURROW)
 f.Run();
#ifdef COUNTERFACTUAL
 Check(invalid==1&&f.events.count==1,"empty-list sentinel read counterfactual");
#else
 Check(!invalid&&f.events.count==1&&f.events.delay==4000,"empty burrow retains timed retry without read");f._burrowGUID.push_back(7);f.Run();Check(b.casts==1&&f.events.count==2,"present burrow retains native cast and cadence");burrow=nullptr;f.Run();Check(f.events.count==3,"missing actor retains retry");
#endif
#elif defined(CASE_ARMYEXIT)||defined(CASE_EONARDIVE)
 f.Start(&p);p.instanceId=2;p.q.Pump();
#ifdef COUNTERFACTUAL
 Check(p.calls==1,"old delayed payload follows transfer");
#else
 Check(!p.calls,"foreign instance refused");p.instanceId=1;f.Start(&p);p.q.Pump();Check(p.calls==1,"original context payload retained");f.Start(&p);
#if defined(CASE_ARMYEXIT)
 p.teleport=true;
#else
 p.alive=false;
#endif
 p.q.Pump();Check(p.calls==1,"pending transfer/death refuses stale payload");
#endif
#endif
 std::cout<<"PASS selected source arithmetic/lifecycle;instrumented controls and controlled endpoints,not client traversal\n";
}
