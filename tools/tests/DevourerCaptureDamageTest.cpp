#include "OwnerHost.inc"
using DamageEffectType=uint32;
enum{CLASS_ROGUE=4,QUEST_STATUS_INCOMPLETE=3};
struct CreatureAI{virtual ~CreatureAI()=default;virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
#include "NativeDefault.inc"
};
struct InstanceScript{void OnCreatureDamageTaken(Creature*,Unit*,uint32&){};};
struct Manager{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 id){return present&&id==7?&scenario:nullptr;}} manager;
auto sScenarioMgr=&manager;
struct Creature:Unit{
 uint64 health=100000,maximum=100000,targetHealth=1000,targetMaximum=1000;float multiplier=0.01f;bool alive=true;uint32 mapId=1607;
 CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;
 Creature* ToCreature(){return this;}CreatureAI* AI(){return primary;}InstanceScript* GetInstanceScript(){return nullptr;}
 uint64 GetHealth(Unit*)const{return targetHealth;}float GetHealthMultiplierForTarget(Unit*)const{return multiplier;}
 uint32 GetInstanceId()const{return 7;}uint32 GetMapId()const{return mapId;}
};
struct ScriptedAI:CreatureAI{Creature* me;explicit ScriptedAI(Creature* actor):me(actor){}};
struct Guard:ScriptedAI{using ScriptedAI::ScriptedAI;
#include "Guard.inc"
};
uint32 Unit::Deal(Creature* victim,uint32 damage){
 DamageEffectType damagetype=0;uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
 if(health<=damage)victim->alive=false;
 else{victim->targetHealth-=damage;victim->health=victim->maximum*victim->targetHealth/victim->targetMaximum;}return damage;
}
int main(){
 Map map,other;Player owner,friendPlayer;owner.map=&map;friendPlayer.map=&map;friendPlayer.objectGuid={1,2};
 HashMapHolder<Player>::_objectVector[1]=&owner;HashMapHolder<Player>::_objectVector[2]=&friendPlayer;
 Unit pet;pet.map=&map;pet.owner=owner.objectGuid;Unit guardian=pet;Unit charmed=pet;charmed.charmer=friendPlayer.objectGuid;
 CriteriaTreeEntry capture{47353,29781};db2Manager.trees[47352]={&capture};assert(manager.scenario.CanUpdateCriteria(47353));
#ifdef BASELINE
 for(Unit* attacker:{static_cast<Unit*>(&owner),&pet}){Creature actor;actor.map=&map;Guard ai(&actor);actor.primary=&ai;assert(attacker->Deal(&actor,20)==2000&&!actor.alive&&owner.criteria.empty());}
 std::cout<<"PASS pinned a0206501 class has no final-damage policy: native inherited empty hook lets scaled player/pet one-hit kill opening Akaari before original timed capture. Expected failing counterfactual.\n";
#else
 uint32 cases=0;
 for(Unit* attacker:{static_cast<Unit*>(&owner),&pet,&guardian,&charmed})for(float scale:{0.001f,0.01f,1.0f,100.0f})for(bool suspended:{false,true}){
  Creature actor;actor.map=&map;actor.multiplier=scale;Guard ai(&actor);CreatureAI temporary;actor.primary=suspended?&temporary:&ai;actor.i_disabledAI=suspended?&ai:nullptr;
  assert(attacker->Deal(&actor,uint32(2000*scale))==999&&actor.alive&&actor.targetHealth==1);
  assert(attacker->Deal(&actor,uint32(2000*scale))==0&&actor.alive&&actor.targetHealth==1);assert(owner.criteria.empty()&&friendPlayer.criteria.empty());++cases;
 }
 Creature ordinary;ordinary.map=&map;ordinary.multiplier=1;Guard ai(&ordinary);ordinary.primary=&ai;assert(owner.Deal(&ordinary,500)==500&&ordinary.targetHealth==500);assert(owner.Deal(&ordinary,500)==499&&ordinary.targetHealth==1);
 Creature huge;huge.map=&map;huge.targetHealth=uint64(1)<<33;Guard large(&huge);uint32 largest=uint32(-1);large.DamageTakenAfterScaling(&owner,largest,0);assert(largest==uint32(-1));
 Creature tiny;tiny.map=&map;tiny.targetHealth=2;Guard small(&tiny);tiny.primary=&small;assert(pet.Deal(&tiny,2)==1&&tiny.targetHealth==1&&tiny.alive);
 for(int invalid=0;invalid<13;++invalid){
  Creature actor;actor.map=&map;Guard scope(&actor);Unit unowned;unowned.map=&map;Unit* attacker=&unowned;
  if(invalid==0)attacker=nullptr;else if(invalid>=2)attacker=&owner;
  if(invalid==2)owner.classId=3;if(invalid==3)owner.inWorld=false;if(invalid==4)owner.map=&other;if(invalid==5)owner.phase=false;
  if(invalid==6)owner.status=0;if(invalid==7)owner.status=1;if(invalid==8)actor.mapId=1220;if(invalid==9)manager.present=false;
  if(invalid==10)manager.scenario.id=999;if(invalid==11)manager.scenario.step=1;if(invalid==12){attacker=&pet;owner.preDelete=true;}
  uint32 damage=2000;scope.DamageTakenAfterScaling(attacker,damage,0);assert(damage==2000&&owner.criteria.empty()&&friendPlayer.criteria.empty());
  owner.classId=4;owner.inWorld=true;owner.map=&map;owner.phase=true;owner.status=3;owner.preDelete=false;manager={};
 }
 std::cout<<"PASS "<<cases<<" native scaled owner/charmer/guardian and primary/suspended cases: preserve one actual target-relative HP, nonlethal/tiny health/repeated hits and13 invalid contexts, no cast/credit or timer mutation. Damage guard leaves capture timers/progression unchanged; helper visual lifetime is separately regressed,10s transfer/client/retry remain OPEN.\n";
#endif
}
