#include <algorithm>
#include <limits>
#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;using uint64=uint64_t;using DamageEffectType=int;
constexpr int SMART_EVENT_DAMAGED=0;
#define TC_LOG_INFO(...) ((void)0)
struct Creature;struct InstanceScript;
struct DefaultAI{void DamageTakenAfterScaling(struct Unit*,uint32&,DamageEffectType){}};
using CreatureAI=DefaultAI;
struct Unit{bool pet=false;uint32 Deal(Creature*,uint32);};
struct Creature{
 bool IsAIEnabled=true,aiPresent=true;DefaultAI defaultAI;DefaultAI* i_disabledAI=nullptr;DefaultAI* AI(){return aiPresent?&defaultAI:nullptr;}
 uint32 entry=102532;uint64 hp=100000,targetHp=1000;float multiplier=0.001f;bool alive=true;InstanceScript* instance=nullptr;
 uint32 GetEntry(){return entry;}bool IsAlive(){return alive;}uint64 GetHealth(){return hp;}uint64 GetHealth(Unit*){return targetHp;}
 void SetHealth(uint64 v){hp=v;targetHp=v;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}
 Creature* ToCreature(){return this;}InstanceScript* GetInstanceScript(){return instance;}
};
#include "ProtectFinale.inc"
struct Script{void ProcessEventsFor(int,Unit*,uint32){}};
struct SmartAI{Creature* me;uint32 mInvincibilityHpLevel=1;Script script;Script* GetScript(){return &script;}void DamageTaken(Unit*,uint32&,DamageEffectType);};
#include "SmartDamage.inc"
struct InstanceScript{uint32 scenario=1026,step=5;void OnCreatureDamageTaken(Creature* c,Unit* a,uint32& damage){DarkRidersArtifact::ProtectFinale(c,a,damage,scenario);
#ifndef APOCALYPSE_BASELINE
 DarkRidersArtifact::ProtectUnholyFinale(c,a,damage,scenario,step);
#endif
}};
uint32 Unit::Deal(Creature* victim,uint32 damage)
{
 DamageEffectType damagetype=0;
 SmartAI ai{victim};if(victim->IsAIEnabled&&victim->aiPresent)ai.DamageTaken(this,damage,0);
 // Native Unit damage scaling and instance hook; actual cached health is
 // captured after AI but before scaling. Unrelated aura/threat logic omitted.
 uint64 health=victim->GetHealth(this);
#include "NativeScaledHook.inc"
 if(health<=damage)victim->alive=false;
 return damage;
}
int main(){
 Unit player,pet,guardian;pet.pet=guardian.pet=true;InstanceScript instance;
#ifdef APOCALYPSE_BASELINE
 Creature broken;broken.instance=&instance;auto amount=player.Deal(&broken,2);assert(amount>=1000&&!broken.alive&&broken.hp==100000);
 std::cout<<"EXPECTED FAIL counterfactual: pinned9731fc183 native SmartAI accepts raw2, actual Unit scaling makes lethal~2000, shared instance does not protect Apocalypse102532, death model reached before1pct finale. No real server crash/client test.\n";
#else
 for(Unit* attacker:{&player,&pet,&guardian})for(uint32 step:{4u,5u})for(float scale:{0.001f,0.01f,1.0f,100.0f})for(bool enabled:{false,true}){
  instance.step=step;Creature boss;boss.instance=&instance;boss.multiplier=scale;boss.IsAIEnabled=enabled;
  // Larger one-shot bypasses the pre-AI guard entirely when AI is unavailable.
  auto damage=attacker->Deal(&boss,200000);assert(!damage&&boss.alive&&boss.hp==1);
  damage=attacker->Deal(&boss,200000);assert(!damage&&boss.alive&&boss.hp==1);
 }
 instance.step=5;Creature scaled;scaled.instance=&instance;assert(player.Deal(&scaled,2)==0&&scaled.hp==1&&scaled.alive);
 Creature missing;missing.instance=&instance;missing.aiPresent=false;assert(pet.Deal(&missing,2)==0&&missing.hp==1&&missing.alive);
 Creature ordinary;ordinary.instance=&instance;ordinary.multiplier=1;assert(player.Deal(&ordinary,2)==2&&ordinary.hp==100000&&ordinary.alive);
 for(uint32 scenario:{0u,988u,1014u,1037u}){instance.scenario=scenario;Creature other;other.instance=&instance;assert(player.Deal(&other,2)>0&&!other.alive&&other.hp==100000);}
 instance.scenario=1026;for(uint32 step:{0u,1u,2u,3u,6u,99u}){instance.step=step;Creature other;other.instance=&instance;assert(player.Deal(&other,2)>0&&!other.alive&&other.hp==100000);}
 instance.step=5;Creature other;other.entry=123;uint32 damage=500000;DarkRidersArtifact::ProtectUnholyFinale(&other,&player,damage,1026,5);assert(damage==500000&&other.hp==100000);
 other.entry=102532;other.alive=false;DarkRidersArtifact::ProtectUnholyFinale(&other,&player,damage,1026,5);assert(damage==500000);
 other.alive=true;DarkRidersArtifact::ProtectUnholyFinale(&other,nullptr,damage,1026,5);assert(damage==500000);
 DarkRidersArtifact::ProtectUnholyFinale(nullptr,&player,damage,1026,5);assert(damage==500000);
 other.hp=100000;other.targetHp=0;damage=0;DarkRidersArtifact::ProtectUnholyFinale(&other,&player,damage,1026,5);assert(!damage&&other.hp==100000);
 Creature absent;assert(player.Deal(&absent,2)>0&&!absent.alive);
 std::cout<<"PASS actual pre-scaling SmartAI and Unit final-scaling/hook: lethal raw2 counterexample repaired; Apocalypse102532/1026stage4..5, player/pet/guardian, four scale ranges, repeated/one-hit/AI-disabled-or-missing, nonlethal, unrelated actors/scenarios/stages/null/dead/absent instance. No progression service exists in this host. Native dialogue/finale endpoints separately tested; client/full combat unverified.\n";
#endif
}
