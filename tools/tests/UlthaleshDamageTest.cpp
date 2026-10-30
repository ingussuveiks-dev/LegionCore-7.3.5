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
 bool IsAIEnabled=false;DefaultAI defaultAI;DefaultAI* i_disabledAI=nullptr;DefaultAI* AI(){return &defaultAI;}
 uint32 entry=100344;uint64 hp=100000,targetHp=1000;float multiplier=0.001f;bool alive=true;InstanceScript* instance=nullptr;
 uint32 GetEntry(){return entry;}bool IsAlive(){return alive;}uint64 GetHealth(){return hp;}uint64 GetHealth(Unit*){return targetHp;}
 void SetHealth(uint64 v){hp=v;targetHp=v;}float GetHealthMultiplierForTarget(Unit*){return multiplier;}
 Creature* ToCreature(){return this;}InstanceScript* GetInstanceScript(){return instance;}
};
#include "ProtectFinale.inc"
struct Script{void ProcessEventsFor(int,Unit*,uint32){}};
struct SmartAI{Creature* me;uint32 mInvincibilityHpLevel=1;Script script;Script* GetScript(){return &script;}void DamageTaken(Unit*,uint32&,DamageEffectType);};
#include "SmartDamage.inc"
struct InstanceScript{uint32 scenario=988;void OnCreatureDamageTaken(Creature* c,Unit* a,uint32& damage){DarkRidersArtifact::ProtectFinale(c,a,damage,scenario);}};
uint32 Unit::Deal(Creature* victim,uint32 damage)
{
 DamageEffectType damagetype=0;
 SmartAI ai{victim};if(victim->entry==100850)ai.mInvincibilityHpLevel=0;ai.DamageTaken(this,damage,0);
 // Native Unit damage scaling and instance hook; actual cached health is
 // captured after AI but before scaling. Unrelated aura/threat logic omitted.
 uint64 health=victim->GetHealth(this);
#include "NativeScaledHook.inc"
 if(health<=damage)victim->alive=false;
 return damage;
}
int main()
{
 for(uint32 entry:{100344u,100850u})for(bool pet:{false,true})for(float scale:{0.001f,0.01f,1.0f,100.0f})
 {
  Unit attacker;attacker.pet=pet;InstanceScript instance;Creature boss;boss.entry=entry;boss.instance=&instance;boss.multiplier=scale;
  auto damage=attacker.Deal(&boss,200000);assert(!damage&&boss.alive&&boss.hp==1);
  damage=attacker.Deal(&boss,200000);assert(!damage&&boss.alive&&boss.hp==1);
 }
 // A raw hit below the native invincibility threshold becomes lethal only
 // AFTER scaling. This is the failure the old SmartAI-only guard missed.
 Unit attacker;InstanceScript instance;Creature scaled;scaled.instance=&instance;
 assert(attacker.Deal(&scaled,2)==0&&scaled.hp==1&&scaled.alive);
 Creature ordinary;ordinary.instance=&instance;ordinary.multiplier=1;
 assert(attacker.Deal(&ordinary,2)==2&&ordinary.hp==100000);
 for(auto scenario:{0u,1026u,1014u}){Creature other;other.instance=&instance;instance.scenario=scenario;assert(attacker.Deal(&other,2)>0&&other.hp==100000);}
 instance.scenario=988;Creature other;other.entry=123;uint32 damage=500000;
 DarkRidersArtifact::ProtectFinale(&other,&attacker,damage,988);assert(damage==500000&&other.hp==100000);
 other.entry=100344;other.alive=false;DarkRidersArtifact::ProtectFinale(&other,&attacker,damage,988);assert(damage==500000);
 other.alive=true;DarkRidersArtifact::ProtectFinale(&other,nullptr,damage,988);assert(damage==500000);
 std::cout<<"PASS: native SmartAI pre-scaling hook and Unit post-scaling instance hook; one-shot, pet, repeated hit and shared-map scenario isolation\n";
}
