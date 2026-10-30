#include <limits>
#include <chrono>
#include <algorithm>
#include "OwnerHost.inc"
using int32=int32_t;using uint8=uint8_t;using uint16=uint16_t;using DamageEffectType=uint32;using Milliseconds=std::chrono::milliseconds;
enum{TYPEID_PLAYER=4,CLASS_ROGUE=4,QUEST_STATUS_INCOMPLETE=3,UNIT_FIELD_FLAGS=1,UNIT_FLAG_IMMUNE_TO_PC=2,REACT_PASSIVE=0,UNIT_STATE_CASTING=1,UNIT_FIELD_BYTES_1=2,REACT_AGGRESSIVE=1,SPELL_CAST_OK=0};
using SpellCastResult=uint32;inline uint32 warningCount=0;
#define TC_LOG_WARN(...) (++warningCount)
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
struct ScenarioHost:Scenario {uint32 id=1012,step=2;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} liveScenario;
struct ScenarioManager{ScenarioHost* live=&liveScenario;ScenarioHost* GetScenario(uint32 id){assert(id==7);return live;}} manager;
auto sScenarioMgr=&manager;
// Production's variable type is Scenario*, so expose the same accessor surface.
#define Scenario ScenarioHost
struct Generator{uint64 Generate(){return 1;}};
struct ObjectMgr{Generator gen;template<HighGuid>Generator* GetGenerator(){return &gen;}} objects;
auto sObjectMgr=&objects;
struct Conversation{
 inline static bool fail=false;inline static uint32 calls=0;inline static Unit* recipient=nullptr;inline static std::vector<Conversation*> active;
 bool CreateConversation(uint64,uint32 id,Unit* player,void*,Unit& source){assert((id==1145||id==1147)&&player==&source);recipient=player;++calls;if(!fail)active.push_back(this);return !fail;}
 static void Clean(){for(auto p:active)delete p;active.clear();calls=0;recipient=nullptr;fail=false;}
};
struct CreatureAI{
 virtual ~CreatureAI()=default;virtual void Reset(){};virtual void EnterCombat(Unit*){};virtual void MoveInLineOfSight(Unit*){};virtual void UpdateAI(uint32){}
 virtual void JustDied(Unit*){};virtual void DoAction(int32){};virtual void Talk(uint32){};virtual void AttackStart(Unit*){};
 virtual void DamageTaken(Unit*,uint32&,DamageEffectType){};virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){};
};
struct InstanceScript{void OnCreatureDamageTaken(Creature*,Unit*,uint32&){};};
struct Position{float GetPositionX(){return 1;}float GetPositionY(){return 2;}float GetPositionZ(){return 3;}};
struct Creature:Unit{
 uint64 health=100000,maximum=100000,targetHealth=1000,targetMaximum=1000;float multiplier=0.01f;bool alive=true;
 uint32 mapId=1545,flags=0,stops=0,despawns=0,removed=0,react=1,moves=0,entry=102185;bool walk=true,failSpawn=false;
 Creature* trigger=nullptr;Unit* nearest=nullptr;uint32 castResult=0,castCount=0,kills=0;
 std::vector<uint32> summons;std::vector<Creature*> adds;CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;
 Creature* ToCreature(){return this;}CreatureAI* AI(){return primary;}InstanceScript* GetInstanceScript(){return nullptr;}
 uint64 GetHealth()const{return health;}uint64 GetHealth(Unit*)const{return targetHealth;}
 uint64 GetMaxHealth(Unit*)const{return targetMaximum;}uint64 CountPctFromMaxHealth(uint32 pct,Unit* u)const{return GetMaxHealth(u)*pct/100;}
 bool HealthBelowPct(uint32 pct)const{return health<maximum*pct/100;}
 float GetHealthMultiplierForTarget(Unit*)const{return multiplier;}
 uint32 GetInstanceId()const{return 7;}uint32 GetMapId()const{return mapId;}
 bool IsWithinDistInMap(Unit*,float dist){assert(dist==30);return true;}
 void StopAttack(){++stops;}void SetFlag(uint32 field,uint32 flag){assert(field==1&&flag==2);flags|=flag;}
 void SetReactState(uint32 v){react=v;}void SetWalk(bool v){walk=v;}
 Creature* GetMotionMaster(){return this;}void MovePoint(uint32 id,float x,float y,float z){assert(!id&&x==-1180.69f&&y==5868.85f&&z==1.83f);++moves;}
 void DespawnOrUnsummon(uint32 delay){assert(delay==10000);++despawns;}void RemoveAura(uint32 id){assert(id==208007);++removed;}
 Creature* SummonCreature(uint32 id,float,float,float,float){assert(id==102212||id==102181||id==102180);summons.push_back(id);return failSpawn?nullptr:this;}

 bool HasUnitState(uint32){return false;}
 Creature* FindNearestCreature(uint32 id,float,bool=true){return id==95464?trigger:nullptr;}
 Unit* FindNearestPlayer(float){return nearest;}
 SpellCastResult CastSpell(Creature*,uint32){++castCount;return castResult;}
 SpellCastResult CastSpell(float,float,float,uint32,bool){++castCount;return castResult;}
 void SetByteValue(uint32,uint32,uint32){}void SetAnimKitId(uint32){}void HandleEmoteCommand(uint32){}void SetFacingToObject(Creature*){}
 void RemoveFlag(uint32,uint32 mask){flags&=~mask;}bool HasAura(uint32){return false;}bool isMoving(){return false;}void RemoveAuraFromStack(uint32){}
 Position GetNearPosition(float,uint32){return {};}
 void Kill(Creature* target){assert(target==this);alive=false;++kills;if(primary)primary->JustDied(this);}

};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Creature* me,uint32 entry,float range){assert(range==100);for(auto add:me->adds)if(add->entry==entry)out.push_back(add);}
uint32 urand(uint32 a,uint32){return a;}
struct ScriptedAI:CreatureAI{
 Creature* me;std::vector<uint32> talks,casts;std::function<void()> callback;
 explicit ScriptedAI(Creature* actor):me(actor){}
 void Talk(uint32 id){talks.push_back(id);if(callback)callback();}void DoCast(uint32 id){casts.push_back(id);if(callback)callback();}
 bool haveVictim=false;bool UpdateVictim(){return haveVictim;}void DoMeleeAttackIfReady(){}
};
#include "Brinebeard.inc"
uint32 Unit::Deal(Creature* victim,uint32 damage){
 DamageEffectType damagetype=0;
 if(victim->primary)victim->primary->DamageTaken(this,damage,damagetype);
 if(victim->i_disabledAI&&victim->i_disabledAI!=victim->primary)victim->i_disabledAI->DamageTaken(this,damage,damagetype);
 if(!damage)return 0;uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
 if(health<=damage){victim->alive=false;if(victim->primary)victim->primary->JustDied(this);}
 else {victim->targetHealth-=damage;victim->health=victim->maximum*victim->targetHealth/victim->targetMaximum;}
 return damage;
}

int main(){
 Map map,other;Player owner,friendPlayer;owner.map=&map;friendPlayer.map=&map;friendPlayer.objectGuid={1,2};
 HashMapHolder<Player>::_objectVector[1]=&owner;HashMapHolder<Player>::_objectVector[2]=&friendPlayer;
 Unit pet;pet.map=&map;pet.owner=owner.objectGuid;Unit guardian=pet;Unit charmed=pet;charmed.charmer=friendPlayer.objectGuid;
 CriteriaTreeEntry credit{46671,29460},otherStage{46673,29461};db2Manager.trees[46670]={&credit};db2Manager.trees[46672]={&otherStage};
 liveScenario.currentTree=46670;assert(liveScenario.CanUpdateCriteria(46671)&&!liveScenario.CanUpdateCriteria(46673));
#ifdef BASELINE
 for(Unit* attacker:{static_cast<Unit*>(&owner),&pet}){
  Creature actor;actor.map=&map;npc_lord_brinebeardAI ai(&actor);actor.primary=&ai;
  assert(attacker->Deal(&actor,10)==1000&&!actor.alive&&!ai.outro&&owner.criteria.empty());Conversation::Clean();
 }
 Creature missing;missing.map=&map;missing.health=20000;missing.targetHealth=200;npc_lord_brinebeardAI stuck(&missing);missing.primary=&stuck;
 assert(owner.Deal(&missing,1)==100&&stuck.outro&&owner.criteria.size()==1&&missing.alive&&stuck.events.Empty());
 stuck.UpdateAI(4000);assert(missing.alive&&!missing.kills);
 std::cout<<"PASS pinned403d9516 counterfactual: fresh scaled player/pet kills lose48531; missing emitter leaves credited Brinebeard without the only death timer. Expected failing baseline, not ordinary success.\n";
#else
 uint32 cases=0;
 for(Unit* attacker:{static_cast<Unit*>(&owner),&pet,&guardian,&charmed})for(float scale:{0.001f,0.01f,1.0f,100.0f})for(int visual=0;visual<3;++visual){
  Creature actor,emitter;actor.map=&map;actor.multiplier=scale;actor.trigger=visual?&emitter:nullptr;emitter.castResult=visual==2?7:0;
  npc_lord_brinebeardAI ai(&actor);actor.primary=&ai;owner.criteria.clear();friendPlayer.criteria.clear();warningCount=0;
  ai.EnterCombat(attacker);uint32 raw=uint32(2000*scale);bool reentered=false;
  Player* recipient=attacker==&charmed?&friendPlayer:&owner;
  recipient->onCredit=[&](){if(!reentered){reentered=true;assert(attacker->Deal(&actor,raw)==0);}};
  assert(attacker->Deal(&actor,raw)==0&&actor.alive&&ai.outro&&!ai.intro&&!actor.react&&actor.stops==1);
  assert(reentered);recipient->onCredit={};assert(recipient->criteria==std::vector<uint32>{48531});
  assert(warningCount==uint32(visual!=1)&&emitter.castCount==uint32(visual!=0));
  assert(attacker->Deal(&actor,raw)==0&&recipient->criteria.size()==1&&emitter.castCount==uint32(visual!=0));
  auto talks=ai.talks.size();ai.EnterCombat(attacker);ai.DoAction(1);assert(ai.talks.size()==talks&&!ai.intro);
  ai.Reset(); // A combat reset must retain this native finale.
  ai.UpdateAI(2999);assert(actor.alive&&!actor.kills);ai.UpdateAI(1);assert(!actor.alive&&actor.kills==1&&Conversation::calls==2);
  ai.UpdateAI(10000);assert(actor.kills==1&&recipient->criteria.size()==1);Conversation::Clean();++cases;
 }
 // Exact30 remains non-terminal, crossing below30 with the next hit claims once.
 Creature actor;actor.map=&map;actor.multiplier=1;npc_lord_brinebeardAI ai(&actor);actor.primary=&ai;owner.criteria.clear();
 assert(owner.Deal(&actor,700)==700&&!ai.outro);assert(owner.Deal(&actor,1)==0&&ai.outro&&actor.alive&&owner.criteria.size()==1);
 // Lost victim, absent emitter, casting state and failed conversation cannot consume the pending death.
 Conversation::fail=true;ai.haveVictim=false;ai.UpdateAI(3000);assert(!actor.alive&&actor.kills==1&&Conversation::calls==2);Conversation::Clean();
 // JustDied tolerates a null killer and resolves an owned-pet presentation recipient.
 npc_lord_brinebeardAI death(&actor);death.JustDied(nullptr);assert(Conversation::recipient==&actor);Conversation::Clean();
 death.JustDied(&pet);assert(Conversation::recipient==&owner);Conversation::Clean();
 for(int invalid=0;invalid<13;++invalid){
  Creature wrong;wrong.map=&map;npc_lord_brinebeardAI scope(&wrong);owner.criteria.clear();Unit unowned;unowned.map=&map;Unit* attacker=&unowned;
  if(invalid==0)attacker=nullptr;else if(invalid>=2)attacker=&owner;
  if(invalid==2)owner.classId=3;if(invalid==3)owner.inWorld=false;if(invalid==4)owner.map=&other;if(invalid==5)owner.phase=false;
  if(invalid==6)owner.status=0;if(invalid==7)owner.status=1;if(invalid==8)wrong.mapId=1220;if(invalid==9)manager.live=nullptr;
  if(invalid==10)liveScenario.id=999;if(invalid==11)liveScenario.step=3;if(invalid==12){attacker=&pet;owner.preDelete=true;}
  uint32 damage=2000;scope.DamageTakenAfterScaling(attacker,damage,0);assert(damage==2000&&!scope.outro&&scope.events.Empty()&&owner.criteria.empty());
  owner.classId=4;owner.inWorld=true;owner.map=&map;owner.phase=true;owner.status=3;owner.preDelete=false;manager.live=&liveScenario;liveScenario.id=1012;liveScenario.step=2;
 }
 Creature fresh;fresh.map=&map;npc_lord_brinebeardAI intro(&fresh);fresh.primary=&intro;
 intro.DoAction(1);intro.UpdateAI(5000);intro.UpdateAI(5000);intro.UpdateAI(5000);assert(!intro.intro&&fresh.react==REACT_AGGRESSIVE&&!(fresh.flags&UNIT_FLAG_IMMUNE_TO_PC));
 intro.EnterCombat(&owner);assert(!intro.events.Empty());intro.Reset();assert(intro.events.Empty()&&!intro.outro);
 std::cout<<"PASS "<<cases<<" scaled player/pet/guardian/charmer x visual success/rejection/missing-emitter cases; native owner/scenario/death bridge, exactly-once48531, strict30, no-victim three-second timer, reset/repeated update, null death, failed conversations, invalid contexts and intro/combat reset. No client visual/party proof.\n";
#endif
 Conversation::Clean();
}
