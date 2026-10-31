#include "NativeHost.inc"
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(condition,...) assert(condition)
#include "AIDestroy.inc"
#include "Policy.inc"
#include "Actor.inc"
using Lady=npc_lady_zitrin::npc_lady_zitrinAI;
struct Run{
 Player player;Creature lady,grash;CreatureAI original,temporary;std::unique_ptr<Lady> ai;
 Run(){scenario=Scenario{};scenario.step=4;manager.active=&scenario;warnings=0;lady.entry=104856;lady.dbGuid=362095;lady.guid={2,362095};grash.entry=105298;grash.dbGuid=362092;grash.guid={2,362092};grash.primary=&original;ObjectAccessor::creatures={&lady,&grash};ai=std::make_unique<Lady>(&lady);ai->EnterCombat(&player);}
 ~Run(){ObjectAccessor::creatures.clear();ObjectAccessor::players.clear();}
 void OnlyControl(){ai->events.CancelEvent(EVENT_1);ai->events.CancelEvent(EVENT_2);ai->events.CancelEvent(EVENT_3);}
 void Tick(uint32 diff){ai->UpdateAI(diff);}
};
int main(){unsigned cases=0;
#ifdef BASELINE
 {Run r;r.OnlyControl();r.grash.primary=new CreatureAI;r.grash.m_AI_locked=true;assert(!r.grash.AIM_Destroy());r.grash.m_AI_locked=false;assert(r.grash.AIM_Destroy()&&!r.grash.AI());/* Do not invoke the pinned unchecked Talk9. */assert(r.ai->events.GetNextEventTime(EVENT_4)==25000);++cases;}
 std::cout<<"PASS COUNTERFACTUAL585592236 whole Lady/native EventMap+AIM_Destroy: live Grash can have null current AI while native25s control event remains pending; pinned unchecked speech deliberately not invoked. No observed production crash or claim that native243 changes AI.\n";
#else
 for(uint32 mode=0;mode<5;++mode){Run r;r.OnlyControl();if(mode){r.grash.i_disabledAI=mode==4?nullptr:&r.original;r.grash.primary=mode==1?&r.temporary:mode==2?nullptr:mode==3?&r.original:nullptr;}
  r.Tick(24999);assert(r.lady.casts.empty()&&r.original.speech.empty());r.Tick(1);assert(r.lady.casts==std::vector<uint32>{209983}&&r.lady.targets==std::vector<Unit*>{&r.grash}&&r.ai->speech==std::vector<uint32>{2});assert(r.original.speech==(mode==4?std::vector<uint32>{}:std::vector<uint32>{9})&&r.temporary.speech.empty());r.Tick(0);assert(r.lady.casts.size()==1);r.Tick(25000);assert(r.lady.casts.size()==2&&r.ai->speech.size()==2);++cases;
 }
 {Run r;r.OnlyControl();r.grash.primary=new CreatureAI;r.lady.onCast=[&]{assert(r.grash.AIM_Destroy());};r.Tick(25000);assert(!r.grash.AI()&&r.lady.casts==std::vector<uint32>{209983}&&r.ai->speech==std::vector<uint32>{2});r.grash.primary=&r.original;r.lady.onCast={};r.Tick(25000);assert(r.original.speech==std::vector<uint32>{9});++cases;}
 {Run r;r.OnlyControl();r.lady.onCast=[&]{r.grash.primary=&r.temporary;r.grash.i_disabledAI=&r.original;};r.Tick(25000);assert(r.original.speech==std::vector<uint32>{9}&&r.temporary.speech.empty());++cases;}
 {Run r;r.OnlyControl();ObjectAccessor::creatures={&r.lady};r.Tick(25000);assert(r.lady.casts.empty()&&r.ai->speech.empty()&&r.original.speech.empty());ObjectAccessor::creatures.push_back(&r.grash);r.Tick(24999);assert(r.lady.casts.empty());r.Tick(1);assert(r.lady.casts==std::vector<uint32>{209983}&&r.original.speech==std::vector<uint32>{9});++cases;}
 {Run r;r.OnlyControl();r.lady.states=UNIT_STATE_CASTING;r.Tick(25000);assert(r.lady.casts.empty());r.lady.states=0;r.Tick(0);assert(r.lady.casts.size()==1);++cases;}
 {Run r;r.OnlyControl();r.ai->hasVictim=false;r.Tick(50000);assert(r.lady.casts.empty());r.ai->hasVictim=true;r.Tick(24999);assert(r.lady.casts.empty());r.Tick(1);assert(r.lady.casts.size()==1);++cases;}
 {Run r;r.OnlyControl();r.ai->Reset();r.Tick(50000);assert(r.lady.casts.empty()&&r.ai->events.Empty());r.ai->EnterCombat(&r.player);r.OnlyControl();r.Tick(25000);assert(r.lady.casts.size()==1);++cases;}
 {Run r;for(int i=0;i<100;++i)r.Tick(500);assert(std::count(r.lady.casts.begin(),r.lady.casts.end(),209983)==2&&std::count(r.lady.casts.begin(),r.lady.casts.end(),209707)==3&&std::count(r.lady.casts.begin(),r.lady.casts.end(),209934)==1);assert(r.original.speech==std::vector<uint32>({9,9}));++cases;}
 std::cout<<"PASS "<<cases<<" whole current Lady/native EventMap+AIM_Destroy ASAN contexts: original25s event/cast target then original-or-current available Talk9, null/current-suspended/alias, AI removal or replacement during cast, missing actor next-period recovery, cast deferral, no-victim clock, reset/re-entry and retained other combat spells. Cast/registry/victim/geometry services controlled, no assertion native243 itself changes AI, no observed production crash/client traversal/progression grant.\n";
#endif
}
