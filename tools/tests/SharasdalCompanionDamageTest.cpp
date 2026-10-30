#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <vector>
using uint8=uint8_t;
using uint32=uint32_t;
using uint64=uint64_t;
using DamageEffectType=uint32;
enum {UNIT_MOD_HEALTH,TOTAL_PCT,UNIT_STATE_STUNNED=8};
template<class V,class P> V CalculatePct(V base,P percent){return V(base*static_cast<float>(percent)/100.0f);}
struct WorldObject {virtual ~WorldObject()=default;};
struct Unit;
struct Creature;
struct CreatureAI {
 virtual ~CreatureAI()=default;
 virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
 virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}
};
struct Unit : WorldObject {
 uint64 hp=64000,maximum=64000;
 uint32 kind=0;
 virtual Creature const* ToCreature()const{return nullptr;}
 uint64 GetHealth()const{return hp;}
 uint64 GetHealth(Unit*)const;
 float GetHealthPct()const{return 100.0f*float(hp)/float(maximum);}
 uint8 GetLevelForTarget(WorldObject const*)const{return 100;}
 float GetModifierValue(uint32,uint32)const{return 1;}
 uint32 Deal(Creature*,uint32);
};
struct InstanceScript {void OnCreatureDamageTaken(Creature*,Unit*,uint32&) {}};
struct Creature : Unit {
 struct ScaleStat {uint64 healthMax=64000;}stat;
 bool scalable=true,statPresent=true,alive=true,IsAIEnabled=true,auraAccepted=true;
 uint32 auraCalls=0,states=0;
 std::function<void()> onAura;
 CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;
 Creature const* ToCreature()const override{return this;}
 Creature* ToCreature(){return this;}
 CreatureAI* AI(){return primary;}
 InstanceScript* GetInstanceScript(){return nullptr;}
 bool HasScalableLevels()const{return scalable;}
 uint8 getLevel()const{return 110;}
 ScaleStat const* GetScaleLevelStat(uint8 level){assert(level==100);return statPresent?&stat:nullptr;}
 uint64 GetMaxHealthByLevel(uint8 level)const{assert(level==100);return stat.healthMax;}
 uint64 GetCreateHealth()const{return maximum;}
 float GetHealthMultiplierForTarget(WorldObject const*)const;
 bool AddAura(uint32 spell,Creature* target){assert(spell==208093&&target==this);++auraCalls;if(onAura)onAura();return auraAccepted;}
 void AddUnitState(uint32 state){assert(state==8);states|=state;}
};
using CreatureLevelStat=Creature::ScaleStat;
#include "Health.inc"
#include "Multiplier.inc"
struct Companion : CreatureAI {
 Creature* me;bool wait_heal=false;std::vector<uint32> speech;
 explicit Companion(Creature* actor):me(actor){}
 void Talk(uint32 group){speech.push_back(group);}
};
struct Erunak : Companion {using Companion::Companion;
#include "ErunakDamage.inc"
};
struct Grash : Companion {using Companion::Companion;
#include "GrashDamage.inc"
};
struct Adel : Companion {using Companion::Companion;
#include "AdelDamage.inc"
};
uint32 Unit::Deal(Creature* victim,uint32 damage){
 DamageEffectType damagetype=0;
 if(victim->IsAIEnabled&&victim->AI())victim->AI()->DamageTaken(this,damage,damagetype);
 uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
 if(health<=damage)victim->alive=false;
 return damage;
}
uint32 oldDeaths=0,oldOverblocks=0;
template<class AI> uint32 Check(std::vector<uint32> const& speech){
 uint32 cases=0;
 for(uint32 kind:{0u,1u,2u,3u})for(float scale:{0.015625f,0.25f,1.0f,16.0f})
 for(uint32 portion:{1u,2u,4u})for(uint32 intensity:{0u,1u,2u,4u,8u})for(uint32 mode:{0u,1u,2u,3u,4u}){
  Creature actor;actor.hp=actor.maximum*portion/4;actor.stat.healthMax=uint64(actor.maximum*scale);
  Unit attacker;attacker.kind=kind;AI content(&actor);CreatureAI temporary;
  actor.primary=mode==1?&temporary:mode==3?nullptr:&content;
  actor.i_disabledAI=mode>=1&&mode<=3?&content:nullptr;actor.IsAIEnabled=mode!=4;
  uint64 health=actor.GetHealth(&attacker);uint32 raw=uint32(std::ceil(health*scale*intensity/4.0f));
  uint32 finalDamage=uint32(raw/actor.GetHealthMultiplierForTarget(&attacker));bool lethal=finalDamage>=health;
#ifdef BASELINE
  bool wound=(mode==0||mode==2)&&raw>=actor.GetHealth();
  if(!wound&&lethal)++oldDeaths;if(wound&&!lethal)++oldOverblocks;
#else
  bool wound=lethal;
#endif
  uint32 expected=wound?0:finalDamage;assert(attacker.Deal(&actor,raw)==expected&&actor.alive==(expected<health));
  assert(content.wait_heal==wound&&actor.auraCalls==(wound?1u:0u)&&actor.states==(wound?8u:0u));
  assert(content.speech==(wound?speech:std::vector<uint32>{}));
  if(actor.alive){assert(attacker.Deal(&actor,raw)==expected);assert(actor.auraCalls==(wound?1u:0u)&&content.speech==(wound?speech:std::vector<uint32>{}));}
  ++cases;
 }
#ifndef BASELINE
 for(bool auraAccepted:{false,true}){
  Creature actor;actor.auraAccepted=auraAccepted;AI content(&actor);actor.primary=&content;Unit attacker;
  actor.onAura=[&]{assert(content.wait_heal);assert(attacker.Deal(&actor,64000)==0);};
  assert(attacker.Deal(&actor,UINT32_MAX)==0&&actor.alive&&actor.auraCalls==1&&content.speech==speech&&actor.states==8);++cases;
 }
 for(bool scalable:{false,true})for(bool present:{false,true}){
  Creature actor;actor.scalable=scalable;actor.statPresent=present;AI content(&actor);actor.primary=&content;Unit attacker;
  uint32 health=uint32(actor.GetHealth(&attacker));assert(attacker.Deal(&actor,health-1)==health-1&&!content.wait_heal);
  assert(attacker.Deal(&actor,health)==0&&actor.alive&&content.wait_heal);++cases;
 }
 {Creature actor;AI content(&actor);uint32 damage=64000;content.DamageTakenAfterScaling(nullptr,damage,0);assert(!damage&&content.wait_heal&&actor.auraCalls==1&&content.speech==speech);++cases;}
#endif
 return cases;
}
int main(){
 uint32 cases=Check<Erunak>({7,8})+Check<Grash>({1,2})+Check<Adel>({2,3});
#ifdef BASELINE
 assert(oldDeaths&&oldOverblocks);
 std::cout<<"PASS "<<cases<<" pinned32bfe713f expected-broken wound contexts: "<<oldDeaths<<" lethal bypasses, "<<oldOverblocks<<" ordinary-hit overblocks; not client incidents\n";
#else
 assert(!oldDeaths&&!oldOverblocks);
 std::cout<<"PASS "<<cases<<" actual three wound handlers/native scaling+target-health/current+suspended cases: lethal hits wound once, ordinary hits remain, original208093/speech/stun and claim-before-reentrant aura preserved, rejected aura still retains original wound mechanics, null safe. No healing/progression/world/client services certified\n";
#endif
}
