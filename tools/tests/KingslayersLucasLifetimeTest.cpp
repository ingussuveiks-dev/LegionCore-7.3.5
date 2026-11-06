// Exact Lucas LOS slice/follow-up and native EventMap/FunctionProcessor.
// Creature, motion and conversation factories are modeled; native publication
// and the whole shared AI have a separate real-core probe.
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;
using Milliseconds=std::chrono::milliseconds;
#define ASSERT assert
static uint32 urand(uint32 low,uint32){return low;}
#include "Functions.inc"
#include "Events.inc"
enum class HighGuid{Conversation};
struct Generator{uint64 Generate(){return 1;}};struct ObjectManager{template<HighGuid>Generator* GetGenerator(){static Generator value;return &value;}} manager;
auto sObjectMgr=&manager;
static std::vector<uint32> requested;static bool acceptFactory;static uint32 moves;
struct Unit{};
struct Movement{void MovePoint(int id,float x,float y,float z){assert(id==0&&abs(x+8527.94f)<0.01f&&abs(y-442.f)<0.01f&&abs(z-106.01f)<0.01f);++moves;}};
struct Creature:Unit{uint32 entry=108399;float distance=1;FunctionProcessor functions;Movement movement;uint32 GetEntry(){return entry;}float GetExactDist2d(Unit*){return distance;}Movement* GetMotionMaster(){return &movement;}void AddDelayedEvent(uint64 delay,std::function<void()>&& job){functions.AddDelayedEvent(delay,std::move(job));}};
struct Conversation;static std::vector<Conversation*> published;
struct Conversation{bool CreateConversation(uint64,uint32 id,Unit*,void*,Unit&){requested.push_back(id);if(acceptFactory)published.push_back(this);return acceptFactory;}};
static void Clear(){for(auto scene:published)delete scene;published.clear();requested.clear();moves=0;}
struct ProbeAI{
 Creature* me;bool introDone=false;explicit ProbeAI(Creature* source):me(source){}
#ifndef LUCAS_BASELINE
 EventMap lucasEvents;
#include "LucasUpdate.inc"
#endif
 void MoveInLineOfSight(Unit* who){
#include "LucasLos.inc"
 }
 void Update(uint32 diff){
#ifndef LUCAS_BASELINE
 UpdateLucas(diff);
#else
 (void)diff;
#endif
 }
};
static void Tick(Creature& actor,ProbeAI* ai,uint32 ms){actor.functions.Update(ms);if(ai)ai->Update(ms);}
static size_t Count(uint32 id){return std::count(requested.begin(),requested.end(),id);}
int main(int argc,char**argv){
 if(argc>1&&std::string(argv[1])=="destroy"){Creature actor;Unit player;auto ai=std::make_unique<ProbeAI>(&actor);ai->MoveInLineOfSight(&player);ai.reset();Tick(actor,nullptr,5000);
#ifdef LUCAS_BASELINE
 std::cerr<<"Expected old freed-AI read missing\n";return 2;
#else
 assert(Count(2483)==1&&!Count(2476)&&!moves);Clear();std::cout<<"PASS destroyed AI has no follow-up callback\n";return 0;
#endif
 }
 uint32 cases=0;
 for(bool accepted:{false,true}){acceptFactory=accepted;
  {Creature actor;Unit player;ProbeAI ai(&actor);ai.MoveInLineOfSight(&player);ai.MoveInLineOfSight(&player);assert(Count(2483)==1&&!Count(2476)&&!moves);Tick(actor,&ai,4900);assert(!Count(2476));Tick(actor,&ai,100);assert(Count(2476)==1&&moves==1);Tick(actor,&ai,50000);assert(Count(2476)==1&&moves==1);Clear();++cases;}
  {Creature actor;Unit player;ProbeAI ai(&actor);ai.MoveInLineOfSight(&player);Tick(actor,&ai,10000);assert(Count(2483)==1&&Count(2476)==1&&moves==1);Clear();++cases;}
  for(float distance:{130.f,130.01f}){Creature actor;actor.distance=distance;Unit player;ProbeAI ai(&actor);ai.MoveInLineOfSight(&player);Tick(actor,&ai,10000);assert(Count(2483)==(distance<=130?1:0)&&Count(2476)==(distance<=130?1:0));Clear();++cases;}
#ifndef LUCAS_BASELINE
  for(uint32 before:{0u,4900u}){Creature actor;Unit player;auto old=std::make_unique<ProbeAI>(&actor);old->MoveInLineOfSight(&player);Tick(actor,old.get(),before);old.reset();Tick(actor,nullptr,10000);assert(Count(2483)==1&&!Count(2476)&&!moves);auto next=std::make_unique<ProbeAI>(&actor);next->MoveInLineOfSight(&player);Tick(actor,next.get(),5000);assert(Count(2483)==2&&Count(2476)==1&&moves==1);Clear();++cases;}
  {Creature actor;actor.entry=108057;Unit player;ProbeAI ai(&actor);ai.MoveInLineOfSight(&player);Tick(actor,&ai,10000);assert(requested.empty()&&!moves);Clear();++cases;}
  {auto actor=std::make_unique<Creature>();Unit player;auto ai=std::make_unique<ProbeAI>(actor.get());ai->MoveInLineOfSight(&player);ai.reset();actor.reset();assert(!Count(2476));Clear();++cases;}
#endif
 }
 std::cout<<"PASS "<<cases<<" ASAN exact Lucas cue/native clocks: modeled accepted/refused factories, original2483/5s2476/move once, boundary/long tick";
#ifndef LUCAS_BASELINE
 std::cout<<", destroyed/replaced AI cancels old work and fresh LOS retries, other-role untouched";
#endif
 std::cout<<". Model services, not native conversation/client traversal\n";
}
