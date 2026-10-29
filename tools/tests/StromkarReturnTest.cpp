#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <cmath>
#include <fstream>
#include <string>
using uint32=uint32_t;using uint8=uint8_t;using int32=int32_t;
enum { CLASS_WARRIOR=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,REACT_PASSIVE=0,
 UNIT_FIELD_NPC_FLAGS=1,UNIT_NPC_FLAG_GOSSIP=1,TEMPSUMMON_TIMED_DESPAWN=3,TEMPSUMMON_MANUAL_DESPAWN=8 };
uint32 warnings=0,foreignResets=0,createdAreas=0;
#define TC_LOG_WARN(...) (++warnings)
#define ASSERT(value) assert(value)
struct ObjectGuid { uint32 id=0;static ObjectGuid const Empty;void Clear(){id=0;}bool operator==(ObjectGuid b)const{return id==b.id;}bool operator!=(ObjectGuid b)const{return id!=b.id;}bool operator<(ObjectGuid b)const{return id<b.id;} };
ObjectGuid const ObjectGuid::Empty{};uint32 nextGuid=1;
struct Position {float x=0,y=0,z=0;};
struct Player;struct Creature;struct AreaTrigger;struct TempSummon;struct CreatureAI;
struct Map;
enum TriggerCastFlags {TRIGGERED_FULL_MASK=1,TRIGGERED_CASTED_BY_AREATRIGGER=2};
enum {EFFECT_0=0,TARGET_UNIT_CASTER=1,AT_ACTION_TYPE_CAST_SPELL=0,AT_ACTION_TYPE_REMOVE_AURA=1,AT_TARGET_FLAG_CAST_AT_SRC=32,AT_TARGET_FLAG_CASTER_IS_TARGET=64,AT_TARGET_FLAG_TARGET_IS_CASTER_2=128};
struct Item{bool IsInWorld(){return true;}};
struct Unit {virtual ~Unit()=default;virtual Creature*ToCreature(){return nullptr;}virtual Player*ToPlayer(){return nullptr;}
 virtual uint32 GetMapId(){return 0;}virtual uint32 GetEntry(){return 0;}virtual bool IsAlive(){return true;}virtual uint32 getClassMask(){return 0;}
 virtual Map*GetMap();bool InInstance(){return true;}Unit*ToUnit(){return this;}bool IsCreature(){return ToCreature()!=nullptr;}bool isPet(){return false;}Unit*GetOwner(){return nullptr;}
 struct Threat{Unit*getHostilTarget(){return nullptr;}}threat;Threat&getThreatManager(){return threat;}uint32 GetTypeId(){return ToPlayer()?4u:ToCreature()?3u:0u;}
 bool gaze=false;uint32 casts=0;void CastSpell(Unit*target,uint32 id,TriggerCastFlags,Item*){assert(target==this&&ToPlayer()&&id==215997);gaze=true;++casts;}
 void CastSpell(float,float,float,uint32,TriggerCastFlags,Item*){assert(false);}void RemoveAura(uint32 id){assert(id==215997);gaze=false;}
};
struct Ref {Player* p;Player*getSource()const{return p;}};
struct InstanceMap {uint32 id=42;uint32 GetInstanceId(){return id;}};
struct Map {InstanceMap instance;InstanceMap*ToInstanceMap(){return &instance;}std::vector<Ref> players;std::map<ObjectGuid,AreaTrigger*>areas;auto const&GetPlayers()const{return players;}AreaTrigger*GetAreaTrigger(ObjectGuid id){auto i=areas.find(id);return i==areas.end()?nullptr:i->second;}}world,otherWorld;
Map*Unit::GetMap(){return &world;}
struct Scenario {uint32 id=1037,step=10;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}}scenarioState;
struct Manager {bool present=true;Scenario*GetScenario(uint32 id){return present&&id==42?&scenarioState:nullptr;}}manager;auto*sScenarioMgr=&manager;
struct Player:Unit {
 Player*ToPlayer()override{return this;}
 uint32 getClassMask()override{return 1u<<(klass-1);}
 bool inWorld=true,contact=true,alive=true,rewarded=false,item=true;uint32 klass=1,map=1539,status=3;int32_t objective=1;Position pos;
 bool IsInWorld(){return inWorld;}bool CanContact(){return contact;}bool IsAlive(){return alive;}uint32 getClass(){return klass;}uint32 GetMapId(){return map;}
 bool GetQuestRewardStatus(uint32 id){assert(id==41105);return rewarded;}uint32 GetQuestStatus(uint32 id){assert(id==41105);return status;}
 int32_t GetQuestObjectiveData(uint32 id,uint32 entry){assert(id==41105&&entry==103114);return objective;}
 bool HasItemCount(uint32 id,uint32 count,bool bank){assert(id==128910&&count==1&&bank);return item;}
 bool InSamePhase(Creature*);bool IsWithinDistInMap(Creature*,float);
};
std::map<ObjectGuid,Creature*>registry;
struct Creature:Unit {
 uint32 entry=104307,map=1539,instance=42;bool alive=true,inWorld=true,phase=true,spawnFail=false;uint32 flags=1,spawns=0,nearCalls=0;Map*worldMap=&world;Position pos;ObjectGuid guid{nextGuid++};std::unique_ptr<CreatureAI> ai;
 Creature*ToCreature()override{return this;}virtual TempSummon*ToTempSummon(){return nullptr;}
 bool IsInWorld(){return inWorld;}bool IsAlive(){return alive;}uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}uint32 GetInstanceId(){return instance;}
 Map*GetMap(){return worldMap;}ObjectGuid GetGUID(){return guid;}bool InSamePhase(Creature*b){return phase&&b->phase;}
 Position GetPosition(){return pos;}void MovePositionToFirstCollision(Position&p,float d,float angle){assert(d==2&&angle==0);++nearCalls;p.x+=2;}
 void SetReactState(uint32 r){assert(r==0);}void RemoveFlag(uint32 f,uint32 v){assert(f==1&&v==1);flags&=~v;}
 Creature*SummonCreature(uint32,Position const&,uint32,uint32);void DespawnOrUnsummon();
};
bool Player::InSamePhase(Creature*c){return c->phase;}
bool Player::IsWithinDistInMap(Creature*c,float d){float dx=pos.x-c->pos.x;return c->GetMapId()==map&&std::fabs(dx)<=d;}
struct TempSummon:Creature {Unit*owner=nullptr;uint32 m_type=8,m_timer=0;TempSummon*ToTempSummon()override{return this;}Unit*GetSummoner(){return owner;}void UnSummon(){DespawnOrUnsummon();}void TickLifetime(uint32 diff){switch(m_type){
#include "SummonLifetime.inc"
default:assert(false);}}};
struct CreatureAI {virtual~CreatureAI()=default;virtual void Reset(){}virtual void UpdateAI(uint32){}};
struct ScriptedAI:CreatureAI {Creature*me;explicit ScriptedAI(Creature*c):me(c){}};
struct SmartAI:ScriptedAI {using ScriptedAI::ScriptedAI;void Reset()override{++foreignResets;}};
struct CreatureScript {explicit CreatureScript(char const*){}virtual~CreatureScript()=default;virtual CreatureAI*GetAI(Creature*)const=0;};
struct ObjectAccessor {static Creature*GetCreature(Creature&,ObjectGuid id){auto i=registry.find(id);return i==registry.end()?nullptr:i->second;}};
enum class HighGuid {AreaTrigger};struct Generator{uint32 Generate(){return nextGuid++;}}generator;
struct ObjectMgr {template<HighGuid>Generator*GetGenerator(){return &generator;}}objectMgr;auto*sObjectMgr=&objectMgr;
using QuestStatus=uint32;
enum {CONDITION_SOURCE_TYPE_AREATRIGGER_ACTION=26,CONDITION_QUEST_OBJECTIVE_DONE=41,CONDITION_OBJECT_ENTRY=31,CONDITION_ITEM=2,CONDITION_CLASS=15,CONDITION_MAPID=22,CONDITION_SCENARION_STEP=44,CONDITION_ALIVE=36,CONDITION_QUESTREWARDED=8,CONDITION_QUESTTAKEN=9,CONDITION_QUEST_COMPLETE=28,QUEST_STATUS_REWARDED=6};
struct ConditionSourceInfo;
struct Condition {uint32 SourceType=26,SourceGroup=11869,SourceEntry=0,ConditionType=41,ConditionTarget=1,ElseGroup=0,ReferenceId=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0;bool NegativeCondition=false;std::string script;bool isLoaded()const{return true;}bool Meets(ConditionSourceInfo&);};
struct ConditionSourceInfo {Unit*mConditionTargets[2];Condition*mLastFailedCondition=nullptr;explicit ConditionSourceInfo(Unit*a=nullptr,Unit*b=nullptr):mConditionTargets{a,b}{}};
struct ConditionScript {explicit ConditionScript(char const*){}virtual~ConditionScript()=default;virtual bool OnConditionCheck(Condition*,ConditionSourceInfo&){return true;}};
bool failArea=false;std::vector<std::unique_ptr<AreaTrigger>>areas;
struct AreaTrigger {
 ObjectGuid guid,casterGuid;Creature*casterUnit=nullptr;Map*map=nullptr;bool inWorld=true,m_withoutCaster=false,_on_remove=false,m_isUpdate=true,_on_despawn=false;Item*m_CastItem=nullptr;int32_t _duration=0;Position pos;
 bool CreateAreaTrigger(uint32 id,uint32 entry,Creature*c,void*,Position const&p,Position const&q,void*,ObjectGuid target,uint32 custom){
  assert(entry==0&&custom==11869&&target==ObjectGuid::Empty&&p.x==q.x);if(failArea)return false;
  guid={id};casterGuid=c->guid;casterUnit=c;map=c->worldMap;pos=p;map->areas[guid]=this;areas.emplace_back(this);++createdAreas;return true;
 }
 void SetDuration(int32_t d){assert(d==600000||d==-1);_duration=d;}int32_t GetDuration(){return _duration;}ObjectGuid GetGUID(){return guid;}ObjectGuid GetCasterGUID(){return casterGuid;}bool IsInWorld(){return inWorld;}
 void Remove(bool=false){inWorld=false;map->areas.erase(guid);}void Despawn(){Remove();}
 void TickLifetime(uint32 p_time){
#include "AreaLifetime.inc"
 }
 Unit*GetCaster(){return casterUnit;}uint32 GetEntry(){return 11869;}bool CheckActionConditions(struct AreaTriggerAction const&,Unit*);void Dispatch(struct AreaTriggerAction const&,Unit*);
 float GetPositionX(){return pos.x;}float GetPositionY(){return pos.y;}float GetPositionZ(){return pos.z;}
};
#include "ReturnController.inc"
std::vector<std::unique_ptr<TempSummon>>summons;
Creature*Creature::SummonCreature(uint32 id,Position const&p,uint32 type,uint32 duration){
 assert(id==93819&&((type==TEMPSUMMON_TIMED_DESPAWN&&duration==600000)||(type==TEMPSUMMON_MANUAL_DESPAWN&&duration==0)));++spawns;if(spawnFail)return nullptr;
 auto s=std::make_unique<TempSummon>();s->m_type=type;s->m_timer=duration;s->entry=id;s->pos=p;s->owner=this;s->worldMap=worldMap;s->map=map;s->phase=phase;s->instance=instance;
 npc_stromkar_valkyr factory;s->ai.reset(factory.GetAI(s.get()));s->ai->Reset();registry[s->guid]=s.get();auto result=s.get();summons.push_back(std::move(s));return result;
}
void Creature::DespawnOrUnsummon(){inWorld=false;for(auto&a:areas)if(a->casterGuid==guid&&a->inWorld)a->Remove();}
void ResetWorld(){summons.clear();areas.clear();registry.clear();world={};otherWorld={};manager={};scenarioState={};failArea=false;warnings=foreignResets=createdAreas=0;}
bool Condition::Meets(ConditionSourceInfo& sourceInfo){Unit*object=sourceInfo.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;
 switch(ConditionType){
#include "Conditions.inc"
 default:assert(false);}
 if(NegativeCondition)condMeets=!condMeets;
 condition_stromkar_return gate;return condMeets&&(script=="condition_stromkar_return"?gate.OnConditionCheck(this,sourceInfo):true);
}
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
#define TC_LOG_DEBUG(...) ((void)0)
struct ConditionMgr{ConditionList rules;ConditionReferenceContainer ConditionReferenceStore;bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;bool IsObjectMeetToConditions(ConditionSourceInfo&s,ConditionList const&c)const{return c.empty()||IsObjectMeetToConditionList(s,c);}ConditionList GetConditionsForAreaTriggerAction(uint32 entry,uint32 id){assert(entry==11869);return id==0?rules:ConditionList{};}}conditionManager;auto*sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
struct AreaTriggerAction{uint32 id=0,actionType=0,targetFlags=8,spellId=215997;};
#include "AreaConditions.inc"
void AreaTrigger::Dispatch(AreaTriggerAction const&data,Unit*unit){if(!unit||!unit->ToPlayer()||!CheckActionConditions(data,unit))return;Unit*caster=GetCaster();
 struct{AreaTriggerAction const*action;}action{&data};bool CallScriptAreaTriggerCast=false;
 struct Target{uint32 GetTarget(){return TARGET_UNIT_CASTER;}};
 struct Effect{Target TargetA;}effect;struct Info{Effect*Effects[1];}info{{&effect}};auto*spellInfo=&info;
 // Native215997 effect0 targets SELF: native area action changes caster to the entering Player.
#include "AreaSelfCaster.inc"
 switch(data.actionType){
#include "AreaCast.inc"
 default:assert(false);}
 (void)CallScriptAreaTriggerCast;
}
int main(int argc,char**argv){
 ResetWorld();Creature ghost;Player player;world.players={{&player}};StromkarReturn::State state;
 StromkarReturn::Update(&ghost,10000,state);assert(!ghost.spawns);
 for(uint32 stage:{0u,7u,8u}){scenarioState.step=stage;StromkarReturn::Signal(&ghost,3,1,state);assert(!state.ready);}
 scenarioState.step=9;StromkarReturn::Signal(&ghost,3,1,state);StromkarReturn::Update(&ghost,10000,state);assert(state.ready&&!ghost.spawns);state={};
 scenarioState.step=10;StromkarReturn::Signal(&ghost,1,1,state);assert(!state.ready);StromkarReturn::Signal(&ghost,3,1,state);
 StromkarReturn::Update(&ghost,1,state);assert(ghost.spawns==1&&summons.size()==1&&!foreignResets&&ghost.nearCalls==1);
 auto*summonedValkyr=summons.back().get();auto*ai=dynamic_cast<StromkarReturn::ValkyrAI*>(summonedValkyr->ai.get());assert(ai&&!summonedValkyr->flags);
 ai->UpdateAI(1);assert(createdAreas==1&&areas.back()->pos.x==2);
#ifdef LIFETIME_BASELINE
 assert(areas.back()->GetDuration()==600000&&summonedValkyr->m_type==TEMPSUMMON_TIMED_DESPAWN);
 summonedValkyr->TickLifetime(600000);assert(!summonedValkyr->inWorld&&!areas.back()->inWorld);
 std::cout<<"EXPECTED COUNTERFACTUAL: actual timed summon branch expires the exit at10min despite an eligible AFK participant. Source retry recovery is a separate path, not certified as absent.\n";
 return 0;
#else
 assert(areas.back()->GetDuration()==-1&&summonedValkyr->m_type==TEMPSUMMON_MANUAL_DESPAWN);
 for(uint32 i=0;i<10;++i){StromkarReturn::Signal(&ghost,3,1,state);StromkarReturn::Update(&ghost,1000,state);ai->UpdateAI(1000);}assert(ghost.spawns==1&&createdAreas==1);
 ai->Reset();assert(!areas.back()->inWorld);ai->UpdateAI(1);assert(createdAreas==2);
 failArea=true;ai->ClearArea();ai->UpdateAI(1000);ai->UpdateAI(1000);assert(warnings==1&&createdAreas==2);failArea=false;ai->UpdateAI(1000);assert(createdAreas==3);
 ghost.inWorld=false;ai->UpdateAI(1);assert(!summonedValkyr->inWorld&&!areas.back()->inWorld);ghost.inWorld=true;
 ghost.spawnFail=true;StromkarReturn::Update(&ghost,1000,state);StromkarReturn::Update(&ghost,1000,state);assert(warnings==2);ghost.spawnFail=false;StromkarReturn::Update(&ghost,1000,state);assert(summons.size()==2);
 condition_stromkar_return conditionScript;Condition condition;ConditionSourceInfo context;
 for(uint32 invalid=0;invalid<10;++invalid){Player p;if(invalid==0)p.inWorld=false;if(invalid==1)p.contact=false;if(invalid==2)p.alive=false;if(invalid==3)p.klass=5;if(invalid==4)p.map=1220;if(invalid==5)p.rewarded=true;if(invalid==6)p.status=0;if(invalid==7)p.objective=0;if(invalid==8)p.objective=-1;if(invalid==9)p.item=false;context.mConditionTargets[1]=&p;assert(!StromkarReturn::Eligible(&p)&&!conditionScript.OnConditionCheck(&condition,context));}
 context.mConditionTargets[1]=nullptr;assert(!conditionScript.OnConditionCheck(&condition,context));condition.SourceGroup=1;assert(conditionScript.OnConditionCheck(&condition,context));
 player.status=QUEST_STATUS_COMPLETE;assert(StromkarReturn::Eligible(&player));player.status=QUEST_STATUS_INCOMPLETE;
 for(uint32 invalid=0;invalid<7;++invalid){ResetWorld();Creature c;Player p;world.players={{&p}};StromkarReturn::State s;s.ready=true;
  if(invalid==0)c.map=1479;if(invalid==1)c.entry=93819;if(invalid==2)c.alive=false;if(invalid==3)c.inWorld=false;if(invalid==4)manager.present=false;if(invalid==5)scenarioState.id=991;if(invalid==6)scenarioState.step=9;
  StromkarReturn::Update(&c,10000,s);assert(!c.spawns);npc_stromkar_valkyr factory;std::unique_ptr<CreatureAI>fallback(factory.GetAI(&c));assert(dynamic_cast<SmartAI*>(fallback.get()));fallback->Reset();assert(foreignResets==1);
 }
 ResetWorld();Creature source;Player p;world.players={{&p}};StromkarReturn::State s;s.ready=true;p.pos.x=61;StromkarReturn::Update(&source,1,s);assert(!source.spawns);p.pos.x=0;source.phase=false;StromkarReturn::Update(&source,1000,s);assert(!source.spawns);source.phase=true;StromkarReturn::Update(&source,1000,s);assert(source.spawns==1);
 auto*owned=summons.back().get();owned->worldMap=&otherWorld;owned->ai->UpdateAI(1);assert(!owned->inWorld);
 {
  ResetWorld();Creature farewell;Player afk,groupMember;world.players={{&afk},{&groupMember}};StromkarReturn::State exit;exit.ready=true;
  StromkarReturn::Update(&farewell,1,exit);auto*helper=summons.back().get();helper->ai->UpdateAI(1);
  // The owned helper remains usable without depending on another source-AI
  // update to replace it after the arbitrary former ten-minute timer.
  for(unsigned i=0;i<1800;++i){helper->TickLifetime(1000);helper->ai->UpdateAI(1000);areas.back()->TickLifetime(1000);}
  assert(helper->inWorld&&areas.back()->inWorld&&farewell.spawns==1&&createdAreas==1);
  afk.item=false;helper->ai->UpdateAI(60000);assert(helper->inWorld); // Other legitimate participant remains.
  groupMember.status=0;helper->ai->UpdateAI(59000);assert(helper->inWorld);
  groupMember.status=QUEST_STATUS_COMPLETE;helper->ai->UpdateAI(1);helper->ai->UpdateAI(60000);assert(helper->inWorld);
  world.players.clear();helper->ai->UpdateAI(59999);assert(helper->inWorld);helper->ai->UpdateAI(1);assert(!helper->inWorld&&!areas.back()->inWorld);
  afk.item=true;world.players={{&afk}};StromkarReturn::Update(&farewell,1000,exit);assert(farewell.spawns==2);helper=summons.back().get();helper->ai->UpdateAI(1);assert(areas.back()->inWorld);
  scenarioState.step=9;helper->ai->UpdateAI(1);assert(!helper->inWorld&&!areas.back()->inWorld);
 }
 if(argc==2){
  std::ifstream input(argv[1]);assert(input);std::vector<Condition>rules;Condition rule;uint32 negative;
  while(input>>rule.ElseGroup>>rule.ConditionType>>rule.ConditionTarget>>rule.ConditionValue1>>rule.ConditionValue2>>rule.ConditionValue3>>negative>>rule.script){rule.NegativeCondition=negative!=0;rules.push_back(rule);}assert(rules.size()==21);
  conditionManager.rules.clear();for(auto&r:rules)conditionManager.rules.push_back(&r);
  ResetWorld();Creature valkyr;valkyr.entry=93819;AreaTrigger trigger;trigger.casterUnit=&valkyr;AreaTriggerAction enter,leave;leave.id=1;leave.actionType=1;
  for(uint32 stage=0;stage<12;++stage)for(uint32 status:{0u,1u,3u,6u}){scenarioState.step=stage;Player candidate;candidate.status=status;trigger.Dispatch(enter,&candidate);assert(candidate.gaze==(stage==10&&(status==1||status==3)));}
  scenarioState.step=10;
  for(uint32 invalid=0;invalid<12;++invalid){Player candidate;if(invalid==0)candidate.klass=5;if(invalid==1)candidate.alive=false;if(invalid==2)candidate.item=false;if(invalid==3)candidate.rewarded=true;if(invalid==4)candidate.objective=0;if(invalid==5)candidate.objective=-1;if(invalid==6)candidate.inWorld=false;if(invalid==7)candidate.contact=false;if(invalid==8)candidate.map=1220;if(invalid==9)valkyr.entry=104307;if(invalid==10)manager.present=false;if(invalid==11)world.instance.id=43;
   trigger.Dispatch(enter,&candidate);assert(!candidate.gaze&&!candidate.casts);candidate.gaze=true;trigger.Dispatch(leave,&candidate);assert(!candidate.gaze);valkyr.entry=93819;manager.present=true;world.instance.id=42;
  }
  Player candidate;trigger.Dispatch(enter,&candidate);assert(candidate.gaze&&candidate.casts==1);trigger.Dispatch(leave,&candidate);assert(!candidate.gaze);
  Unit pet;trigger.Dispatch(enter,&pet);assert(!pet.casts);
  valkyr.map=1220;Player shared;shared.klass=5;assert(trigger.CheckActionConditions(enter,&shared));
  std::cout<<"PASS actual21 SQL conditions/native ElseGroups and AreaTrigger two-target condition caller, native SELF-caster and cast/remove branches: stage/status matrix, class/life/item/reward/real credit/signed-negative/contact/missing instance/pet refusal and unconditional leave cleanup; shared outside-map admission preserved. Spell/aura endpoint controlled, not client button rendering.\n";
 }
 std::cout<<"PASS actual Stromkar return controller/factory: native scoped owner/eligible witness, one Val'kyr/custom11869 retained through30min AFK, native manual/infinite lifetime branches, last-participant grace cleanup/recovery, default SmartAI isolation and retry/reset/source-loss rejection. Map/summon/area/collision services are endpoints; actual client rendering/arrival remain separate.\n";
#endif
}
