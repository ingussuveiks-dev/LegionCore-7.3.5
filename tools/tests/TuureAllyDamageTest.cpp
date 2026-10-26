// Native core extraction/adapters and established cross-product boundary cases
// come from the matching project's SilverHandAllyDamageTest harness.
#include "NativeHost.inc"
enum {CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct EventMap {void Reset(){}};
struct Companion:CreatureAI {
 Creature* me=nullptr;bool wait_heal=false,_pendingHealRecovery=false,_introHealDone=true;
 EventMap events;std::vector<uint32> speech;void Talk(uint32 id){speech.push_back(id);}
};
struct Boros:Companion {
#include "BorosDamage.inc"
#include "BorosHeal.inc"
#include "BorosLifecycle.inc"
#include "BorosReset.inc"
};
struct Allies:Companion {
#include "AlliesDamage.inc"
#include "AlliesHeal.inc"
#include "AlliesLifecycle.inc"
#include "AlliesReset.inc"
};
#include "NativeCases.inc"
int main(){
 uint32 cases=Check<Boros>({10})+Check<Allies>({10});CheckHealing<Boros>();CheckHealing<Allies>();
 Unit attacker;Creature fallback;fallback.scalable=false;assert(fallback.GetHealth(&attacker)==64000&&fallback.GetHealthMultiplierForTarget(&attacker)==1);
 fallback.scalable=true;fallback.statPresent=false;assert(fallback.GetHealth(&attacker)==64000);
 fallback.statPresent=true;fallback.currentHealth=1;fallback.stat.healthMax=1;assert(fallback.GetHealth(&attacker)==1);
 std::cout<<"PASS "<<cases<<" actual Tuure companion guard/heal/reset/lifecycle with native target health/multiplier/damage and heal/ModifyHealth/locked lifecycle dispatch. ";
#if defined(TUURE_ALLY_ALLY_BASELINE)
 std::cout<<"Labelled7608b3f2 counterfactual: raw guards miss scaled lethal hits and can wound on a nonlethal high-scale hit. Not passing ordinary behavior.\n";
#elif defined(TUURE_ALLY_HEAL_BASELINE)
 std::cout<<"Isolated pinned7608b3f2 HealReceived counterfactual with current final-damage guard: crossing40pct requires another heal.\n";
#else
 std::cout<<"Final lethal wound once, recovery only after actual positive heal and observed40pct; zero/cancelled/subthreshold, intervening damage/dead/unloaded/predelete/reset/re-wound/replacement/reentrant/primary/disabled/alias cases pass. Labels/aura endpoints are adapters; initial Boros95pct sequence is preserved and still open. Client fight/heal/VFX pending.\n";
#endif
}
