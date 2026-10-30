#include "NativeHost.inc"
struct EventMap {void Reset(){}};
struct Companion : CreatureAI {
 Creature* me=nullptr;bool wait_heal=false,_pendingHealRecovery=false;EventMap events;
 std::vector<uint32> speech;
 void Talk(uint32 group){speech.push_back(group);}
};
struct Erunak : Companion {
#include "ErunakDamage.inc"
#include "ErunakHeal.inc"
#include "ErunakLifecycle.inc"
#include "ErunakReset.inc"
};
struct Grash : Companion {
#include "GrashDamage.inc"
#include "GrashHeal.inc"
#include "GrashLifecycle.inc"
#include "GrashReset.inc"
};
struct Adel : Companion {
#include "AdelDamage.inc"
#include "AdelHeal.inc"
#include "AdelLifecycle.inc"
#include "AdelReset.inc"
};
#include "NativeCases.inc"
int main(){
 uint32 cases=Check<Erunak>({7,8})+Check<Grash>({1,2})+Check<Adel>({2,3});
 CheckHealing<Erunak>();CheckHealing<Grash>();CheckHealing<Adel>();
#ifdef SHARASDAL_HEAL_BASELINE
 std::cout<<"PASS pinned c3c3ce355 expected-broken native-heal baseline: crossing40pct leaves each companion stunned until a second heal; "<<cases<<" final-damage contexts retained\n";
#else
 std::cout<<"PASS "<<cases<<" retained damage contexts plus three actual native DealHeal/ModifyHealth/locked current+suspended lifecycle matrices: observed40pct after positive incoming heal clears original wound once; zero/cancel/subthreshold/stale reset/re-wound/lifetime boundaries retained. Grash50/90pct story tails explicitly outside this host; actual client/full heal pipeline separate\n";
#endif
}
