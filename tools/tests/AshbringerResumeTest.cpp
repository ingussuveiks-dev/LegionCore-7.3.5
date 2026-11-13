#define ASHBRINGER_RESUME_TEST
#include "AshbringerDominationRetryTest.cpp"
#include <cstdlib>
int main(){
 _set_abort_behavior(0,_CALL_REPORTFAULT);
#ifdef ASHBRINGER_RESUME_BASELINE
 Unit target;Creature boss;ActorAI original;boss.disabled=&original;target.boss=&boss;
 Aura aura;aura.target=&target;aura.application=true;aura.OnRemove(nullptr,0);
 assert(original.actions==0&&boss.ai.actions==1&&!warnings);
 std::cout<<"COUNTERFACTUAL pinned2e6f0d739 complete OnRemove ignores suspended content and sends combat resume to the replacement. Missing-current dereference is not deliberately executed.\n";
#else
 uint32 cases=0;
 for(bool targetPresent:{false,true})for(bool bossPresent:{false,true})for(bool currentPresent:{false,true})for(bool disabledPresent:{false,true}){
  warnings=0;Unit target;Creature boss;ActorAI original;boss.missing=!currentPresent;boss.disabled=disabledPresent?&original:nullptr;target.boss=bossPresent?&boss:nullptr;
  Aura aura;aura.target=targetPresent?&target:nullptr;aura.application=true;aura.OnRemove(nullptr,0);
  bool admitted=targetPresent&&bossPresent;
  assert(original.actions==uint32(admitted&&disabledPresent));
  assert(boss.ai.actions==uint32(admitted&&!disabledPresent&&currentPresent));
  assert(warnings==uint32(admitted&&!disabledPresent&&!currentPresent));
  ++cases;
 }
 std::cout<<"PASS "<<cases<<" ASAN complete OnRemove target/boss/current/suspended combinations:original content wins,ordinary current fallback retained,missing target/actor no-op,missing both AI safely diagnosed without dispatch. No HP/credit/loot/timer/retry or actor fabrication.\n";
#endif
}
