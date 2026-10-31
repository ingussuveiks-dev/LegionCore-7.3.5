// Full current Erunak, unchanged quest/lifecycle/function/path/conversation
// dependencies; only actor services remain controlled as in the upper suite.
#include "ReplyHost.inc"
struct ReplyRun:Run{
 ReplyRun(){scenario.step=0;actor.instance.step=0;actor.db=362063;actor.z=245;adel.db=362078;adel.z=258;adel.i_AI=new CreatureAI(&adel);}
 ~ReplyRun(){if(adel.i_disabledAI&&adel.i_disabledAI!=adel.i_AI)delete adel.i_disabledAI;delete adel.i_AI;adel.i_AI=adel.i_disabledAI=nullptr;}
 void BeginReply(){Content()->MovementInform(WAYPOINT_MOTION_TYPE,8);}
 void ReplyTick(uint32 diff){adel.functions.Update(diff);}
};
int main(){
 _set_abort_behavior(0,_CALL_REPORTFAULT);
#ifdef BASELINE
 {ReplyRun r;r.BeginReply();assert(r.adel.talks==std::vector<uint32>{0}&&r.adel.functions.SizeQueue()==1);assert(r.adel.AIM_Destroy());assert(!r.adel.AI()&&r.adel.functions.SizeQueue()==1);/* The pinned callback's unchecked null-AI dereference is deliberately not invoked. */}
 std::cout<<"PASS EXPECTED UNSAFE c33b7e1f2 actual whole waypoint callback remains queued after native AIM_Destroy clears current AI; source pins its unchecked AI()->Talk1. Same Creature capture remains within its native Unit queue, not an escaped AI pointer. Null callback deliberately not invoked; no observed crash claimed.\n";
#else
 unsigned cases=0;
 {ReplyRun r;r.BeginReply();r.ReplyTick(4999);assert(r.adel.talks==std::vector<uint32>{0});r.ReplyTick(1);assert(r.adel.talks==std::vector<uint32>({0,1}));r.ReplyTick(10000);assert(r.adel.talks.size()==2);++cases;}
 {ReplyRun r;assert(r.adel.AIM_Destroy());r.BeginReply();r.ReplyTick(5000);assert(r.adel.talks.empty()&&r.adel.functions.Empty());++cases;}
 for(uint32 elapsed:{0u,1u,2500u,4999u}){ReplyRun r;r.BeginReply();r.ReplyTick(elapsed);assert(r.adel.AIM_Destroy());r.ReplyTick(5000-elapsed);assert(r.adel.talks==std::vector<uint32>{0}&&r.adel.functions.Empty());++cases;}
 for(uint32 mode=0;mode<3;++mode){ReplyRun r;r.adel.i_disabledAI=r.adel.i_AI;r.adel.i_AI=mode==0?nullptr:mode==1?new CreatureAI(&r.adel):r.adel.i_disabledAI;r.BeginReply();r.ReplyTick(5000);assert(r.adel.talks==std::vector<uint32>({0,1}));++cases;}
 for(uint32 mode=0;mode<3;++mode){ReplyRun r;r.BeginReply();r.adel.i_disabledAI=r.adel.i_AI;r.adel.i_AI=mode==0?nullptr:mode==1?new CreatureAI(&r.adel):r.adel.i_disabledAI;r.ReplyTick(5000);assert(r.adel.talks==std::vector<uint32>({0,1}));++cases;}
 {ReplyRun r;r.BeginReply();assert(r.adel.AIM_Destroy());r.adel.i_AI=new CreatureAI(&r.adel);r.ReplyTick(5000);assert(r.adel.talks==std::vector<uint32>({0,1}));++cases;}
 for(uint32 state:{uint32(UNIT_STATE_STUNNED),uint32(UNIT_STATE_CASTING)}){ReplyRun r;r.BeginReply();r.adel.states|=state;r.ReplyTick(5000);assert(r.adel.talks==std::vector<uint32>({0,1}));++cases;}
 for(uint32 invalid=0;invalid<3;++invalid){ReplyRun r;r.BeginReply();if(invalid==0)r.adel.world=false;if(invalid==1)r.adel.preDelete=true;if(invalid==2)r.adel.alive=false;r.ReplyTick(5000);assert(r.adel.talks==std::vector<uint32>{0}&&r.adel.functions.Empty());++cases;}
 {ReplyRun r;r.actor.adel=nullptr;r.BeginReply();r.ReplyTick(5000);assert(r.adel.talks.empty());++cases;}
 {ReplyRun r;r.BeginReply();uint32 other=0;r.adel.AddDelayedEvent(5000,[&]{++other;});assert(r.adel.AIM_Destroy());r.ReplyTick(5000);assert(other==1&&r.adel.talks==std::vector<uint32>{0});++cases;}
 {ReplyRun r;r.BeginReply();assert(r.actor.AIM_Destroy());r.ReplyTick(5000);assert(r.adel.talks==std::vector<uint32>({0,1}));++cases;}
 {ReplyRun r;r.BeginReply();r.adel.m_AI_locked=true;assert(!r.adel.AIM_Destroy());r.adel.m_AI_locked=false;r.ReplyTick(5000);assert(r.adel.talks==std::vector<uint32>({0,1}));++cases;}
 {ReplyRun r;r.actor.instance.step=3;r.BeginReply();r.ReplyTick(5000);assert(r.adel.talks.empty()&&!factoryCalls&&!r.player.calls);++cases;}
 std::cout<<"PASS "<<cases<<" full-current Erunak/native FunctionProcessor+AIM_Destroy reply ASAN cases: native Adelee0/5000ms1, absent current AI before/after request, actual AI destruction/replacement, current/suspended/current-null/alias, stunned/casting Unit clock, dead/deleting/out-of-world skip, missing actor, unrelated Unit work and original Erunak removal. Same Creature owns queued capture; no quest/progression/path/owner policy repair or client traversal claimed.\n";
#endif
}
