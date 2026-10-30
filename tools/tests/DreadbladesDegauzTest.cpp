#include <limits>
#include <chrono>
#include <algorithm>
#include "OwnerHost.inc"
using uint8=uint8_t;using uint16=uint16_t;using DamageEffectType=uint32;using Milliseconds=std::chrono::milliseconds;
enum{TYPEID_PLAYER=4,CLASS_ROGUE=4,QUEST_STATUS_INCOMPLETE=3,UNIT_FIELD_FLAGS=1,UNIT_FLAG_IMMUNE_TO_PC=2,REACT_PASSIVE=0,UNIT_STATE_CASTING=1};
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
struct ScenarioHost:Scenario {uint32 id=1012,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} liveScenario;
struct ScenarioManager{ScenarioHost* live=&liveScenario;ScenarioHost* GetScenario(uint32 id){assert(id==7);return live;}} manager;
auto sScenarioMgr=&manager;
// Production's variable type is Scenario*, so expose the same accessor surface.
#define Scenario ScenarioHost
struct Generator{uint64 Generate(){return 1;}};
struct ObjectMgr{Generator gen;template<HighGuid>Generator* GetGenerator(){return &gen;}} objects;
auto sObjectMgr=&objects;
struct Conversation{
 inline static bool fail=false;inline static uint32 calls=0;inline static Unit* recipient=nullptr;inline static std::vector<Conversation*> active;
 bool CreateConversation(uint64,uint32 id,Unit* player,void*,Unit& source){assert(id==1121&&player==&source);recipient=player;++calls;if(!fail)active.push_back(this);return !fail;}
 static void Clean(){for(auto p:active)delete p;active.clear();calls=0;recipient=nullptr;fail=false;}
};
struct CreatureAI{
 virtual ~CreatureAI()=default;virtual void Reset(){};virtual void EnterCombat(Unit*){};virtual void MoveInLineOfSight(Unit*){};virtual void UpdateAI(uint32){}
 virtual void DamageTaken(Unit*,uint32&,DamageEffectType){};virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){};
};
struct InstanceScript{void OnCreatureDamageTaken(Creature*,Unit*,uint32&){};};
struct Creature:Unit{
 uint64 health=100000,maximum=100000,targetHealth=1000,targetMaximum=1000;float multiplier=0.01f;bool alive=true;
 uint32 mapId=1545,flags=0,stops=0,despawns=0,removed=0,react=1,moves=0,entry=102185;bool walk=true,failSpawn=false;
 std::vector<uint32> summons;std::vector<Creature*> adds;CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;
 Creature* ToCreature(){return this;}CreatureAI* AI(){return primary;}InstanceScript* GetInstanceScript(){return nullptr;}
 uint64 GetHealth()const{return health;}uint64 GetHealth(Unit*)const{return targetHealth;}
 uint64 GetMaxHealth(Unit*)const{return targetMaximum;}uint64 CountPctFromMaxHealth(uint32 pct,Unit* u)const{return GetMaxHealth(u)*pct/100;}
 bool HealthBelowPct(uint32 pct)const{return health<maximum*pct/100;}
 float GetHealthMultiplierForTarget(Unit*)const{return multiplier;}
 uint32 GetInstanceId()const{return 7;}uint32 GetMapId()const{return mapId;}
 bool IsWithinDistInMap(Unit*,float dist){assert(dist==30);return true;}
 void StopAttack(){++stops;}void SetFlag(uint32 field,uint32 flag){assert(field==1&&flag==2);flags|=flag;}
 void SetReactState(uint32 v){assert(v==REACT_PASSIVE);react=v;}void SetWalk(bool v){walk=v;}
 Creature* GetMotionMaster(){return this;}void MovePoint(uint32 id,float x,float y,float z){assert(!id&&x==-1180.69f&&y==5868.85f&&z==1.83f);++moves;}
 void DespawnOrUnsummon(uint32 delay){assert(delay==10000);++despawns;}void RemoveAura(uint32 id){assert(id==208007);++removed;}
 Creature* SummonCreature(uint32 id,float,float,float,float){assert(id==102212||id==102181||id==102180);summons.push_back(id);return failSpawn?nullptr:this;}
 bool HasUnitState(uint32){return false;}
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Creature* me,uint32 entry,float range){assert(range==100);for(auto add:me->adds)if(add->entry==entry)out.push_back(add);}
uint32 urand(uint32 a,uint32){return a;}
struct ScriptedAI:CreatureAI{
 Creature* me;std::vector<uint32> talks,casts;std::function<void()> callback;
 explicit ScriptedAI(Creature* actor):me(actor){}
 void Talk(uint32 id){talks.push_back(id);if(callback)callback();}void DoCast(uint32 id){casts.push_back(id);if(callback)callback();}
 bool UpdateVictim(){return false;}void DoMeleeAttackIfReady(){}
};
#include "Degauz.inc"
uint32 Unit::Deal(Creature* victim,uint32 damage){
 DamageEffectType damagetype=0;
 if(victim->primary)victim->primary->DamageTaken(this,damage,damagetype);
 if(victim->i_disabledAI&&victim->i_disabledAI!=victim->primary)victim->i_disabledAI->DamageTaken(this,damage,damagetype);
 if(!damage)return 0;uint64 health=victim->GetHealth(this);
#include "ScaledHook.inc"
 if(health<=damage)victim->alive=false;
 else {victim->targetHealth-=damage;victim->health=victim->maximum*victim->targetHealth/victim->targetMaximum;}
 return damage;
}
int main(){
 Map map,other;Player owner,otherPlayer;owner.map=&map;otherPlayer.map=&map;otherPlayer.objectGuid={1,2};
 HashMapHolder<Player>::_objectVector[1]=&owner;HashMapHolder<Player>::_objectVector[2]=&otherPlayer;
 Unit pet;pet.map=&map;pet.owner=owner.objectGuid;Unit guardian=pet;Unit charmed=pet;charmed.charmer=otherPlayer.objectGuid;
 CriteriaTreeEntry credit{46640,29446},next{46656,29453};db2Manager.trees[46639]={&credit};db2Manager.trees[46655]={&next};
 liveScenario.currentTree=46639;assert(liveScenario.CanUpdateCriteria(46640)&&!liveScenario.CanUpdateCriteria(46656));
#ifdef BASELINE
 for(Unit* attacker:{static_cast<Unit*>(&owner),&pet}){
  Creature actor;actor.map=&map;npc_first_helper_degauzAI ai(&actor);actor.primary=&ai;
  assert(attacker->Deal(&actor,10)==1000&&!actor.alive&&ai.healthPct==90&&owner.criteria.empty()&&actor.summons.empty());
 }
 std::cout<<"PASS pinned802295a9 counterfactual: fresh scaled player and pet one-hit kills bypass all Degauz retreat/48449 progression. This is an expected failing baseline, not a repaired encounter.\n";
#else
 uint32 count=0;
 for(Unit* attacker:{static_cast<Unit*>(&owner),&pet,&guardian,&charmed})for(float scale:{0.001f,0.01f,1.0f,100.0f})for(bool suspended:{false,true}){
  Creature actor;actor.map=&map;actor.multiplier=scale;npc_first_helper_degauzAI ai(&actor);CreatureAI charm;
  actor.primary=suspended?&charm:&ai;actor.i_disabledAI=suspended?&ai:nullptr;owner.criteria.clear();otherPlayer.criteria.clear();
  Creature first,second,third;first.entry=102192;second.entry=102227;third.entry=102194;actor.adds={&first,&second,&third};
  uint32 raw=uint32(2000*scale);assert(attacker->Deal(&actor,raw)==0&&actor.alive&&ai.healthPct==0);
  assert((ai.talks==std::vector<uint32>{2,3})&&(ai.casts==std::vector<uint32>{208007}));
  assert(actor.stops==1&&actor.flags==2&&!actor.react&&!actor.walk&&actor.moves==1&&actor.despawns==1&&actor.removed==1);
  assert((actor.summons==std::vector<uint32>{102212,102181,102180})&&ai.events.Empty());
  for(auto add:actor.adds)assert(add->stops==1&&add->flags==2&&!add->react&&!add->walk&&add->moves==1&&add->despawns==1);
  Player* recipient=attacker==&charmed?&otherPlayer:&owner;assert(recipient->criteria==std::vector<uint32>{48449}&&Conversation::recipient==recipient);
  assert(attacker->Deal(&actor,raw)==0&&recipient->criteria.size()==1&&actor.summons.size()==3);
  uint32 late=100;ai.DamageTakenAfterScaling(nullptr,late,0);assert(!late);Conversation::Clean();++count;
 }
 // Exact90/60/30 boundaries remain strict; ordinary hits may cross one or two thresholds.
 Creature ordinary;ordinary.map=&map;ordinary.multiplier=1;npc_first_helper_degauzAI sequence(&ordinary);ordinary.primary=&sequence;owner.criteria.clear();
 assert(owner.Deal(&ordinary,100)==100&&sequence.healthPct==90);
 assert(owner.Deal(&ordinary,1)==1&&sequence.healthPct==60);
 assert(owner.Deal(&ordinary,299)==299&&sequence.healthPct==60);
 assert(owner.Deal(&ordinary,1)==1&&sequence.healthPct==30);
 assert(owner.Deal(&ordinary,299)==299&&sequence.healthPct==30);
 assert(owner.Deal(&ordinary,1)==0&&sequence.healthPct==0&&owner.criteria.size()==1&&ordinary.targetHealth==300);Conversation::Clean();
 // Reentrant callbacks may finish retreat inside an earlier threshold; the outer hit must also be stopped.
 for(int trigger:{0,1}){
  Creature actor;actor.map=&map;npc_first_helper_degauzAI ai(&actor);actor.primary=&ai;owner.criteria.clear();bool once=false;
  ai.callback=[&](){if(!once&&int(ai.casts.size())==trigger){once=true;assert(pet.Deal(&actor,20)==0);}};
  assert(owner.Deal(&actor,8)==0&&once&&actor.alive&&ai.healthPct==0&&owner.criteria.size()==1&&actor.summons.size()==3);Conversation::Clean();
 }
 // Visual failure and missing summoned actors do not repeat native surrender or invent an artifact.
 Creature failed;failed.map=&map;failed.failSpawn=true;npc_first_helper_degauzAI failure(&failed);failed.primary=&failure;owner.criteria.clear();Conversation::fail=true;
 assert(pet.Deal(&failed,20)==0&&owner.criteria.size()==1&&failed.summons.size()==3&&Conversation::active.empty());Conversation::Clean();
 for(int invalid=0;invalid<13;++invalid){
  Creature actor;actor.map=&map;npc_first_helper_degauzAI ai(&actor);owner.criteria.clear();otherPlayer.criteria.clear();Unit unowned;unowned.map=&map;Unit* attacker=&unowned;
  if(invalid==0)attacker=nullptr;else if(invalid>=2)attacker=&owner;
  if(invalid==2)owner.classId=3;if(invalid==3)owner.inWorld=false;if(invalid==4)owner.map=&other;if(invalid==5)owner.phase=false;
  if(invalid==6)owner.status=0;if(invalid==7)owner.status=1;if(invalid==8)actor.mapId=1220;if(invalid==9)manager.live=nullptr;
  if(invalid==10)liveScenario.id=999;if(invalid==11)liveScenario.step=1;
  if(invalid==12){attacker=&pet;owner.preDelete=true;}
  uint32 damage=10000;ai.DamageTakenAfterScaling(attacker,damage,0);assert(damage==10000&&ai.healthPct==90&&owner.criteria.empty()&&otherPlayer.criteria.empty()&&actor.summons.empty());
  owner.classId=4;owner.inWorld=true;owner.map=&map;owner.phase=true;owner.status=3;owner.preDelete=false;manager.live=&liveScenario;liveScenario.id=1012;liveScenario.step=0;
 }
 Creature retry;retry.map=&map;npc_first_helper_degauzAI ai(&retry);retry.primary=&ai;assert(owner.Deal(&retry,2)==200&&ai.healthPct==60);ai.Reset();assert(ai.healthPct==90&&ai.events.Empty());
 ai.EnterCombat(&owner);assert(ai.healthPct==90&&!ai.events.Empty());
 std::cout<<"PASS "<<count<<" scaled player/pet/guardian/charmer and suspended-AI surrender cases; native owner lookup/scenario membership/full AI, strict thresholds, reentry, exactly-once credit/retreat, adds, invalid scope, missing presentation and reset/retry. Actual movement/voice/actor spawn/party/client sequence remain separate.\n";
#endif
 Conversation::Clean();
}
