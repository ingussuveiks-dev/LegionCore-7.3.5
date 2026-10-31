#include <type_traits>
#include "NativeHost.inc"
#include "Policy.inc"
#include "Actor.inc"

// Only these unchanged Erunak methods are needed for the wound policy. Adelee
// above is the entire current actor, including her independent rescue lifecycle.
struct Erunak : ScriptedAI {
 explicit Erunak(Creature* actor):ScriptedAI(actor){}
 EventMap events;bool _introDone=false,wait_heal=false,firstbattle=false,_pendingHealRecovery=false;
#include "ErunakDamage.inc"
#include "ErunakHeal.inc"
#include "ErunakReset.inc"
#include "ErunakLifecycle.inc"
};
using Adel=npc_adel_waterspeaker::npc_adel_waterspeakerAI;
struct Healer : CreatureAI {
 bool cancel=false;unsigned calls=0;
 void HealDone(Unit*,uint32& amount)override{++calls;if(cancel)amount=0;}
};
template<class AI> unsigned Check(){
 unsigned cases=0;
 // Source identities do not restrict ordinary health restoration. Native owner
 // resolution/quest credit belongs to other suites; this hook gives no credit.
 for(int source=0;source<4;++source)for(int mode=0;mode<4;++mode)
 for(int position=0;position<3;++position)for(int input=0;input<10;++input){
  scenario=Scenario{};manager.active=&scenario;warnings=0;
  Creature actor;actor.entry=std::is_same_v<AI,Erunak>?102826:104853;
  actor.health=actor.maximum=1000;actor.flags=0x10200010;
  if(position==1)actor.z=797;if(position==2)actor.mapId=1;
  AI ai(&actor);CreatureAI temporary;Unit healer;Healer filter;
  healer.guid={uint64(source+1),11};healer.IsAIEnabled=true;healer.healAI=&filter;
  uint32 lethal=1000;ai.DamageTakenAfterScaling(&healer,lethal,0);
  assert(!lethal&&ai.wait_heal&&!ai._pendingHealRecovery&&actor.states==UNIT_STATE_STUNNED);
  // A lethal blow is held without reducing health: this is the original false
  // recovery boundary, not an artificial manually-set pending flag.
  if(input==2||input==3)actor.health=1;
  if(input==4)actor.health=399;
  if(mode==1){actor.primary=&temporary;actor.i_disabledAI=&ai;}
  if(mode==2)actor.i_disabledAI=&ai;
  if(mode==3){actor.primary=nullptr;actor.i_disabledAI=&ai;actor.IsAIEnabled=false;}
  filter.cancel=input==0;
  uint32 amount=input==1?0:input==2?398:input==3?399:1;
  healer.DealHeal(&actor,amount);
  if(input==5)ai.Reset();
  if(input==6)actor.alive=false;
  if(input==7)actor.world=false;
  if(input==8)actor.preDelete=true;
  actor.Tick(0);
  bool recovered=input==3||input==4||input==9;
  assert(ai.wait_heal==!recovered);
  assert(actor.removals==unsigned(recovered));
  assert(actor.states==(recovered?0u:UNIT_STATE_STUNNED));
  assert(!ai._pendingHealRecovery&&actor.flags==0x10200010);
  assert(scenario.step==0&&scenario.achievements.progress.empty());
  assert(!actor.poses&&!actor.animations&&!actor.emotes&&!warnings);
  actor.alive=actor.world=true;actor.preDelete=false;
  actor.Tick(10000);assert(actor.removals==unsigned(recovered));
  if(!recovered){
   filter.cancel=false;healer.DealHeal(&actor,1000);actor.Tick(0);
   assert(!ai.wait_heal&&!actor.states&&actor.removals==1);
  }
  // An ordinary later heal never invents another wound or removal.
  healer.DealHeal(&actor,1);actor.Tick(0);assert(actor.removals==1&&!ai._pendingHealRecovery);
  uint32 again=1000;ai.DamageTakenAfterScaling(&healer,again,0);
  assert(!again&&ai.wait_heal&&!ai._pendingHealRecovery);
  actor.Tick(0);assert(ai.wait_heal&&actor.removals==1);
  ++cases;
 }
 return cases;
}
template<class AI> void Counterfactual(){
 scenario=Scenario{};manager.active=&scenario;
 Creature actor;actor.health=actor.maximum=1000;
 AI ai(&actor);Unit healer;Healer filter;filter.cancel=true;
 healer.IsAIEnabled=true;healer.healAI=&filter;
 uint32 lethal=1000;ai.DamageTakenAfterScaling(&healer,lethal,0);assert(!lethal&&ai.wait_heal);
 assert(healer.DealHeal(&actor,1)==0&&actor.health==1000&&filter.calls==1);
 actor.Tick(0);assert(!ai.wait_heal&&!actor.states&&actor.removals==1);
}
int main(){
#ifdef BASELINE
 Counterfactual<Erunak>();Counterfactual<Adel>();
 std::cout<<"PASS COUNTERFACTUAL 3897fd3bf: real held lethal hit leaves full-health wound; pre-HealDone marker clears both actors after a cancelled zero-result heal. This is expected broken behavior, not a successful ordinary test.\n";
#else
 unsigned cases=Check<Erunak>()+Check<Adel>();
 // An already accepted native rescue may animate while an independent wound
 // remains; no successful heal or new rescue criterion is fabricated.
 {scenario=Scenario{};scenario.step=2;scenario.achievements.progress[47123].Counter=1;
  Creature actor;actor.entry=104853;actor.health=actor.maximum=1000;Adel ai(&actor);
  Unit healer;Healer filter;filter.cancel=true;healer.IsAIEnabled=true;healer.healAI=&filter;
  uint32 lethal=1000;ai.DamageTakenAfterScaling(&healer,lethal,0);
  healer.DealHeal(&actor,1);actor.Tick(0);actor.Tick(2000);
  assert(actor.poses==1&&actor.animations==1&&actor.emotes==1);
  assert(ai.wait_heal&&actor.states==UNIT_STATE_STUNNED&&!actor.removals);
  assert(scenario.achievements.progress[47123].Counter==1&&!scenario.achievements.progress[47197].Counter);
  healer.DealHeal(&actor,0);actor.Tick(0);assert(ai.wait_heal);
  filter.cancel=false;healer.DealHeal(&actor,1);actor.Tick(0);
  assert(!ai.wait_heal&&actor.removals==1&&actor.poses==1);
 }
 std::cout<<"PASS "<<cases<<" final-applied wound contexts plus independent accepted-rescue presentation: current/suspended/alias/disabled AI, held lethal full health, cancelled/zero/subthreshold/crossing40/positive overheal, reset, dead/out-of-world/predelete, lower/upper/shared-map and repeated wound/heal. Actual DealHeal prefix/ModifyHealth/Notify/locked lifecycle and full Adelee actor compiled; registry/geometry/scenario/clock adapters controlled. No client or full-chain claim.\n";
#endif
}
