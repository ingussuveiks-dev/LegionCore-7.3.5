#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <iostream>
using uint32=uint32_t;using uint64=uint64_t;using FunctionList=std::multimap<uint64,std::function<void()>>;
#define TC_COMMON_API
#include "NativeFunctionProcessor.inc"
#include "NativeFunctionMethods.inc"
uint32 engagementRequests=0;
#include "Support.inc"
struct Run {
 Creature actor,helya;std::unique_ptr<Player>owner=std::make_unique<Player>();std::unique_ptr<IntroAI>ai;
 Run(){actor.guid=98;helya.entry=99043;actor.helya=&helya;actor.nearbyPlayers={owner.get()};scenarioManager={};ObjectAccessor::players={{1,owner.get()}};ObjectAccessor::creatures={{helya.guid,&helya}};warningRequests=recoveryRequests=engagementRequests=Conversation::removed=Conversation::deleted=0;NewAI();actor.onElapsed=[this](uint32 diff){if(ai)ai->TickVisual(diff);};}
 void NewAI(){ai=std::make_unique<IntroAI>(&actor);ai->Reset();}
 ~Run(){ai.reset();ObjectAccessor::players.clear();ObjectAccessor::creatures.clear();while(!Conversation::objects.empty())delete Conversation::objects.back();}
 void Start(){ai->MoveInLineOfSight(owner.get());}void Tick(uint32 diff){while(diff){auto slice=std::min(diff,1000u);actor.Advance(slice);diff-=slice;}}
 uint32 Pending(){return actor.m_Functions.Size()+actor.m_Functions.SizeQueue();}
};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);std::cout<<std::unitbuf;
#ifdef LIFETIME_BASELINE
 for(uint32 elapsed:{0u,23999u,24000u,30999u}){Run r;r.Start();auto dialogue=Conversation::objects.back();r.Tick(elapsed);assert(r.Pending()==1);r.ai.reset();assert(r.Pending()==1&&dialogue->world);r.actor.m_Functions.KillAllFunctions();r.actor.Advance(0);assert(!r.Pending());}
 std::cout<<"PASS EXPECTED BROKEN661a25685: actual native Unit queue retains raw-AI24/7 closures after deletion; owned843 remains. Dangling closures discarded without invoking them, no fabricated crash.\n";
#else
 uint32 cases=0;
 for(uint32 status:{1u,3u})for(uint32 elapsed:{0u,1u,23999u,24000u,30999u,31000u}){
  Run r;r.owner->status=status;r.Start();auto dialogue=Conversation::objects.back();auto unrelated=new Conversation;unrelated->world=true;r.Tick(elapsed);auto casts=r.helya.casts;auto attacks=engagementRequests;
  r.ai.reset();assert(!dialogue->world&&unrelated->world&&Conversation::removed==1);uint32 independent=0;r.actor.AddDelayedEvent(1,[&]{++independent;});r.Tick(100000);assert(r.helya.casts==casts&&engagementRequests==attacks&&!r.Pending()&&independent==1&&unrelated->world);++cases;
 }
 for(uint32 elapsed:{0u,23999u,24000u,30999u}){Run r;r.Start();auto original=Conversation::objects.back();r.Tick(elapsed);auto before=r.helya.casts;r.ai.reset();r.NewAI();r.Start();assert(!original->world);r.Tick(31000);assert(engagementRequests==1&&r.helya.casts==before+1&&!r.Pending());++cases;}
 for(uint32 stalled:{31000u,100000u}){Run r;r.Start();r.actor.Advance(stalled);assert(r.helya.casts==1&&!engagementRequests&&r.Pending()==1);r.actor.Advance(6999);assert(!engagementRequests);r.actor.Advance(1);assert(engagementRequests==1&&!r.Pending());++cases;}
 {Run r;r.Start();r.helya.onCast=[&]{r.ai.reset();};r.Tick(24000);assert(!r.ai&&r.helya.casts==1&&!engagementRequests&&!r.Pending()&&Conversation::removed==1);r.Tick(100000);assert(!engagementRequests);++cases;}
 {Run r;r.Start();r.actor.onAuraRemove=[&]{r.ai.reset();};r.Tick(31000);assert(!r.ai&&!engagementRequests&&!r.Pending()&&Conversation::removed==1);++cases;}
 {Run r;r.Start();r.actor.onAuraRemove=[&]{r.ai->Reset();};r.Tick(31000);assert(!engagementRequests&&!r.ai->introDone&&!r.ai->introPending&&!r.Pending());r.actor.onAuraRemove={};r.Start();r.Tick(31000);assert(engagementRequests==1);++cases;}
 {Run r;r.Start();r.helya.onCast=[&]{ObjectAccessor::players.clear();r.owner.reset();};r.Tick(24000);assert(!r.owner&&!engagementRequests&&!r.ai->introDone&&!r.Pending()&&Conversation::removed==1);++cases;}
 {Run r;r.Start();r.helya.onCast=[] {ObjectAccessor::creatures.clear();};r.Tick(24000);assert(!engagementRequests&&!r.ai->introDone&&!r.Pending()&&Conversation::removed==1);++cases;}
 {Run r;r.Start();r.actor.onAuraRemove=[&]{ObjectAccessor::players.clear();r.owner.reset();};r.Tick(31000);assert(!r.owner&&!engagementRequests&&!r.ai->introDone&&!r.Pending()&&Conversation::removed==1);++cases;}
 {Run r;r.Start();r.actor.onAuraRemove=[] {ObjectAccessor::creatures.clear();};r.Tick(31000);assert(!engagementRequests&&!r.ai->introDone&&!r.Pending()&&Conversation::removed==1);++cases;}
 {Run r;r.Start();auto dialogue=Conversation::objects.back();r.ai->RemoveIntroConversation();assert(!dialogue->world&&Conversation::removed==1);r.ai.reset();r.Tick(100000);assert(Conversation::removed==1&&!engagementRequests);++cases;}
 // Map-local cleanup deliberately does not remove an object in another map.
 // Its natural expiry/transfer recovery is separate from this memory repair.
 {Run r;r.Start();auto dialogue=Conversation::objects.back();r.actor.map=&otherMap;r.ai.reset();r.Tick(100000);assert(dialogue->world&&!engagementRequests&&!r.Pending());++cases;}
 std::cout<<"PASS "<<cases<<" ASAN contexts: actual production intro/weak24+7 closures/destructor with native Unit AddDelayedEvent/FunctionProcessor and extracted native map accessors. AI deletion/replacement before both deadlines and after cast/aura-removal reentry, destroyed original Player, fresh GUID participant resolution, Reset during aura removal, late31/100s frame retaining full7s, owned843-only cleanup and unrelated Unit work retained. World/actual casts/object lifecycle/engagement endpoints controlled; map-transfer dialogue expiry/client/full944 remain separate.\n";
#endif
}
