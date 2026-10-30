#include <algorithm>
#include <limits>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
using uint8=uint8_t;
using uint32=uint32_t;
using uint64=uint64_t;
using DamageEffectType=uint32;
enum {UNIT_MOD_HEALTH,TOTAL_PCT};
template<class V,class P> V CalculatePct(V base,P percent){return V(base*static_cast<float>(percent)/100.0f);}
struct WorldObject {virtual ~WorldObject()=default;};
struct Unit;
struct Creature;
struct CreatureAI {
 virtual ~CreatureAI()=default;
 virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
 virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}
};
struct Threat {
 uint32 calls=0;Creature* target=nullptr;int percent=0;
 void modifyThreatPercent(Creature* value,int amount){++calls;target=value;percent=amount;}
};
struct Unit : WorldObject {
 uint64 hp=64000,maximum=64000;
 uint32 kind=0;Threat threat;
 virtual Creature const* ToCreature()const{return nullptr;}
 uint64 GetHealth()const{return hp;}
 uint64 GetHealth(Unit*)const;
 float GetHealthPct()const{return 100.0f*float(hp)/float(maximum);}
 uint8 GetLevelForTarget(WorldObject const*)const{return 100;}
 float GetModifierValue(uint32,uint32)const{return 1;}
 Threat& getThreatManager(){return threat;}
 uint32 Deal(Creature*,uint32);
};
struct InstanceScript {void OnCreatureDamageTaken(Creature*,Unit*,uint32&) {}};
struct Creature : Unit {
 struct ScaleStat {uint64 healthMax=64000;} stat;
 bool scalable=true,statPresent=true,alive=true,IsAIEnabled=true;
 uint32 entry=105911;
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
};
using CreatureLevelStat=Creature::ScaleStat;
#include "Health.inc"
#include "Multiplier.inc"
struct Companion : CreatureAI {
 Creature* me;
 explicit Companion(Creature* actor):me(actor){}
#include "Damage.inc"
};
uint32 Unit::Deal(Creature* victim,uint32 damage){
 DamageEffectType damagetype=0;
 if(victim->IsAIEnabled&&victim->AI())victim->AI()->DamageTaken(this,damage,damagetype);
 uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
 if(health<=damage)victim->alive=false;
 return damage;
}
int main(){
 uint32 cases=0;
#ifdef BASELINE
 uint32 oldDeaths=0,oldOverblocks=0;
#endif
 for(uint32 entry:{105911u,105912u})for(uint32 kind:{0u,1u,2u,3u})
 for(float scale:{0.015625f,0.25f,1.0f,16.0f})for(uint32 portion:{1u,2u,4u})
 for(uint32 intensity:{0u,1u,2u,4u,8u})for(uint32 mode:{0u,1u,2u,3u,4u}){
  Creature actor;actor.entry=entry;actor.hp=actor.maximum*portion/4;actor.stat.healthMax=uint64(actor.maximum*scale);
  Unit attacker;attacker.kind=kind;Companion content(&actor);CreatureAI temporary;
  actor.primary=mode==1?&temporary:mode==3?nullptr:&content;
  actor.i_disabledAI=(mode>=1&&mode<=3)?&content:nullptr;actor.IsAIEnabled=mode!=4;
  uint64 health=actor.GetHealth(&attacker);
  uint32 raw=uint32(std::ceil(health*scale*intensity/4.0f));
  uint32 finalDamage=uint32(raw/actor.GetHealthMultiplierForTarget(&attacker));
  bool lethal=finalDamage>=health;
#ifdef BASELINE
  bool oldClaim=(mode==0||mode==2)&&raw>=actor.GetHealth();
  uint32 expected=oldClaim?0:finalDamage;
  if(!oldClaim&&lethal)++oldDeaths;
  if(oldClaim&&!lethal)++oldOverblocks;
#else
  uint32 expected=lethal?0:finalDamage;
#endif
  uint32 result=attacker.Deal(&actor,raw);
  assert(result==expected&&actor.alive==(expected<health));
  assert(attacker.threat.calls==(expected==0&&raw!=0?1u:0u));
  if(attacker.threat.calls)assert(attacker.threat.target==&actor&&attacker.threat.percent==-90);
  if(actor.alive){uint32 before=attacker.threat.calls;assert(attacker.Deal(&actor,raw)==expected);assert(attacker.threat.calls==before+(expected==0&&raw!=0?1:0));}
  ++cases;
 }
#ifdef BASELINE
 assert(oldDeaths&&oldOverblocks);
 std::cout<<"PASS "<<cases<<" pinned a148a34b5 expected-broken cases: "<<oldDeaths<<" lethal bypasses, "<<oldOverblocks<<" ordinary-hit overblocks; controlled counterfactual, not a live client incident\n";
#else
 for(bool scalable:{false,true})for(bool statPresent:{false,true}){
  Creature actor;actor.scalable=scalable;actor.statPresent=statPresent;Companion content(&actor);actor.primary=&content;
  Unit attacker;uint64 hp=actor.GetHealth(&attacker);assert(attacker.Deal(&actor,uint32(hp-1))==hp-1&&actor.alive&&!attacker.threat.calls);
  assert(attacker.Deal(&actor,uint32(hp))==0&&actor.alive&&attacker.threat.calls==1);++cases;
 }
 {Creature actor;Companion content(&actor);uint32 damage=64000;content.DamageTakenAfterScaling(nullptr,damage,0);assert(!damage&&actor.alive);++cases;}
 {Creature actor;Companion content(&actor);uint32 damage=63999;content.DamageTakenAfterScaling(nullptr,damage,0);assert(damage==63999&&actor.alive);++cases;}
 {Creature actor;Unit attacker;assert(attacker.Deal(&actor,64000)==64000&&!actor.alive&&!attacker.threat.calls);++cases;}
 {Creature actor;Companion content(&actor);actor.primary=&content;Unit attacker;assert(attacker.Deal(&actor,UINT32_MAX)==0&&actor.alive&&attacker.threat.calls==1);++cases;}
 std::cout<<"PASS "<<cases<<" extracted native scaling/target-health/current+suspended-AI cases for both companion bindings; ordinary hits retained, lethal hits/threat guarded, null direct callback safe. Controlled services, not full scenario/client verification\n";
#endif
}
