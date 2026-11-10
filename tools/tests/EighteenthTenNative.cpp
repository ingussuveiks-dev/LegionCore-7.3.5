#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
enum{ACTION_TERMINATE_HIDE_PHASE=3};
struct ObservedEffect18{float amount=0;uint32 stacks=100;float GetAmount(){return amount;}void SetAmount(float a){amount=a;}uint32 GetStackAmount(){return stacks;}};
struct ObservedEvents18{uint32 retry=0;void RescheduleEvent(uint32,uint32 n){retry=n;}void DelayEvent(uint32,uint32){}};
struct NativeBase{Creature*me,*other;Position pos;ObservedEvents18 events;uint32 accepted=0,EldersCount=3,jumpTimer=0,timeCount=99,count_adds=0,timer_check=0,wins=0;bool targetPresent=false;ObservedEffect18 effect,*aurEff=&effect,*aura=&effect;std::set<uint64>activeAddGuids;explicit NativeBase(Creature*u):me(u),other(u){}Creature*GetTarget(){return me;}Creature*SelectTarget(uint32){return targetPresent?other:nullptr;}};
#include "EighteenthNativeScripts.inc"
using namespace EighteenthNative;
int main(int argc,char**argv){try{InitializeAltheaGuardsNative(argc,argv);
dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999777,DIFFICULTY_N_SCENARIO,&parent);Actor a(&map,299,10631,{52,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&a)),"Actor insertion");
NToppsRay ray(&a);ray.pos.Relocate(52,808,805);ray.Run();Require(std::isfinite(ray.pos.GetPositionX())&&ray.pos.GetPositionY()==878,"Actual native Position axis projection");ray.pos=a.GetPosition();ray.Run();Require(ray.events.retry==3000,"Actual coincident Position retry");
NFocusAction focus(&a);focus.Run(-1);focus.Run(ACTION_1);Require(focus.accepted==1,"Matching native ACTION_1 admission;payload observed");NLeiShiAction lei(&a);lei.Run(0);lei.Run(3);Require(lei.accepted==1,"Hide action admission;payload observed");
a.SetMaxHealth(uint64(1)<<40);NLoryHealth lory(&a);lory.Run(&a);Require(a.GetHealth()==uint64(a.GetMaxHealth()*0.1f),"Native wounded health64");
a.SetMaxHealth(24000000000ULL);a.SetHealth(1);NMeljarakHeal heal(&a);heal.Run();Require(a.GetHealth()==6000000001ULL,"Native quarter-health helper width selected contract");
a.SetMaxHealth(15000000000ULL);NGarroshAbsorb absorb(&a);absorb.Apply();absorb.Run();Require(a.GetHealth()>14999999000ULL&&absorb.effect.amount>0,"Native health64 with observed float amount boundary");
NNightmotherLanding landing(&a);landing.Run();Require(landing.jumpTimer==6000,"Native failed target takeoff arms recovery");a.SetCanFly(false);a.SetDisableGravity(false);
NGlowcapStack glow(&a);glow.Run();Require(glow.timeCount==4,"100-stack selected branch with observed aura");
a.SetMaxHealth(10000000000ULL);NFreyaHealth freya(&a);freya.Run(&a);Require(a.GetMaxHealth()==16000000000ULL,"Native elder bonus64");
NMasterRoster master(&a);master.count_adds=3;master.Reset();master.Join(1);master.Join(2);master.Join(3);master.Die(1);master.Die(1);Require(master.count_adds==2&&!master.wins,"Selected GUID-roster state model repeated death");master.Die(2);master.Die(3);Require(master.wins==1,"Selected roster completion model");
CloseAltheaNative();std::cout<<"PASS native ten selected cases:actual Position projection and Unit health mutations/takeoff;selected action,stack and Master roster models with observed endpoints. Original full helper actor graph,real aura/spell/death/brawler report dispatch and client traversal not executed;no character writes/full-core ASAN\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
