#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
struct ObjectGuid {uint64 counter=0;uint32 entry=0;uint32 GetEntry()const{return entry;}uint64 GetCounter()const{return counter;}bool operator==(ObjectGuid b)const{return counter==b.counter&&entry==b.entry;}bool operator!=(ObjectGuid b)const{return !(*this==b);}};
struct Creature;struct TempSummon;
struct Unit {virtual~Unit()=default;virtual Creature*ToCreature(){return nullptr;}bool IsCreature(){return ToCreature()!=nullptr;}};
struct Map {uint32 instance=42;};
struct CreatureAI {virtual~CreatureAI()=default;virtual void JustSummoned(Creature*){}virtual void IsSummonedBy(Unit*){}};
struct Creature:Unit {
 uint32 entry=104276,mapId=1539,phase=1;ObjectGuid guid;Map*map=nullptr;bool world=true,alive=true,preDelete=false,IsAIEnabled=true;float x=0;CreatureAI*brain=nullptr;
 Creature*ToCreature()override{return this;}virtual TempSummon*ToTempSummon(){return nullptr;}CreatureAI*AI(){return brain;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return mapId;}Map*GetMap()const{return map;}uint32 GetInstanceId()const{return map->instance;}ObjectGuid GetGUID()const{return guid;}uint64 GetGUIDLow()const{return guid.counter;}
 bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool IsPreDelete()const{return preDelete;}bool InSamePhase(Creature const*other)const{return phase==other->phase;}void DespawnOrUnsummon(){preDelete=true;}
};
struct TempSummon:Creature {
 Unit*unitSummoner=nullptr;ObjectGuid ownerGuid;
 TempSummon*ToTempSummon()override{return this;}Unit*GetSummoner(){return unitSummoner;}ObjectGuid GetSummonerGUID(){return ownerGuid;}void InitSummon();
};
std::vector<Creature*> actors;
struct ObjectAccessor {static Creature*GetCreature(Creature&context,ObjectGuid guid){for(auto*a:actors)if(a->guid==guid&&a->map==context.map&&!a->preDelete)return a;return nullptr;}};
uint32 searches=0,logs=0;
void GetCreatureListWithEntryInGrid(std::list<Creature*>&result,Creature*source,uint32 entry,float range){assert(entry==104307&&range==100);++searches;for(auto*a:actors)if(a->entry==entry&&a->map==source->map&&std::abs(a->x-source->x)<=range)result.push_back(a);}
struct Scenario{uint32 id=1037,step=9;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}}scenarioState;
struct ScenarioMgr {bool present=true;Scenario*GetScenario(uint32 instance){return present&&instance==42?&scenarioState:nullptr;}}scenarioMgr;auto*sScenarioMgr=&scenarioMgr;
#define TC_LOG_INFO(...) (++logs)
struct SmartAI:CreatureAI {
 Creature*me;uint32 summonEvents=0,introEvents=0;std::function<void()>onSummon;
 explicit SmartAI(Creature*c):me(c){}
 void JustSummoned(Creature*)override{++summonEvents;if(onSummon){auto f=std::move(onSummon);onSummon={};f();}}
 void IsSummonedBy(Unit*)override{++introEvents;}
};
struct npc_stromkar_zakajz:SmartAI {using SmartAI::SmartAI;ObjectGuid thoradin;
#include "ClaimThoradin.inc"
#include "IsFinalThoradin.inc"
#include "JustSummoned.inc"
#include "HasThoradin.inc"
};
struct BaselineBoss:SmartAI {using SmartAI::SmartAI;ObjectGuid thoradin;
#include "BaselineJustSummoned.inc"
#include "BaselineHasThoradin.inc"
};
struct npc_stromkar_thoradin:SmartAI {using SmartAI::SmartAI;bool initialized=false,preserveIntro=false;
#include "ActorSummoned.inc"
};
struct BaselineThoradin:SmartAI {using SmartAI::SmartAI;bool initialized=false,preserveIntro=false;
#include "BaselineActorSummoned.inc"
};
#include "NativeInitSummon.inc"
struct Fixture {
 Map map,otherMap{43};Creature boss;std::unique_ptr<SmartAI> ownerAI;std::vector<std::unique_ptr<TempSummon>> summons;std::vector<std::unique_ptr<SmartAI>> brains;uint64 next=100;bool baseline;
 explicit Fixture(bool old=false):baseline(old){actors.clear();searches=logs=0;scenarioState={};scenarioMgr={};boss.map=&map;boss.guid={1,104276};actors.push_back(&boss);RenewOwner();}
 void RenewOwner(){if(baseline)ownerAI=std::make_unique<BaselineBoss>(&boss);else ownerAI=std::make_unique<npc_stromkar_zakajz>(&boss);boss.brain=ownerAI.get();}
 TempSummon*Spawn(){auto s=std::make_unique<TempSummon>();s->entry=104307;s->guid={next++,104307};s->map=boss.map;s->unitSummoner=&boss;s->ownerGuid=boss.guid;auto*actor=s.get();if(baseline)brains.push_back(std::make_unique<BaselineThoradin>(actor));else brains.push_back(std::make_unique<npc_stromkar_thoradin>(actor));actor->brain=brains.back().get();actors.push_back(actor);summons.push_back(std::move(s));actor->InitSummon();return actor;}
 uint32 Live()const{return uint32(std::count_if(summons.begin(),summons.end(),[](auto const&s){return s->world&&s->alive&&!s->preDelete;}));}
 uint32 Intros()const{uint32 n=0;for(auto const&b:brains)n+=b->introEvents;return n;}
 npc_stromkar_zakajz*Content(){return dynamic_cast<npc_stromkar_zakajz*>(ownerAI.get());}
};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);uint32 cases=0;
 // Counterfactuals use the pinned production methods and the full native
 // TempSummon notification order, not a second copy of the proposed repair.
 for(bool old:{true,false})for(bool ownerEnabled:{false,true}){
  Fixture f(old);f.boss.IsAIEnabled=ownerEnabled;f.Spawn();f.Spawn();assert(f.Live()==(old?2u:1u));assert(f.Intros()==(old?2u:1u));if(!old)assert(f.Content()->HasThoradin());++cases;
 }
 for(bool old:{true,false}){Fixture f(old);f.ownerAI->onSummon=[&]{f.Spawn();};f.Spawn();assert(f.Live()==(old?2u:1u)&&f.Intros()==(old?2u:1u));assert(f.ownerAI->summonEvents==(old?2u:1u));++cases;}
 for(bool old:{true,false}){Fixture f(old);auto*a=f.Spawn();a->InitSummon();assert(f.Intros()==(old?2u:1u));++cases;}
 for(uint32 stage:{9u,10u})for(bool enabled:{false,true}){Fixture f;scenarioState.step=stage;auto*first=f.Spawn();f.RenewOwner();f.boss.IsAIEnabled=enabled;auto*second=f.Spawn();assert(!first->preDelete&&second->preDelete&&f.Live()==1&&f.Intros()==1&&f.Content()->thoradin==first->guid);++cases;}
 {Fixture f;auto*first=f.Spawn();first->x=1000;auto before=searches;auto*second=f.Spawn();assert(!first->preDelete&&second->preDelete&&searches==before&&f.Intros()==1);++cases;}
 {Fixture f;f.next=0xffffffffULL;auto*first=f.Spawn();f.RenewOwner();auto*second=f.Spawn();assert(!first->preDelete&&second->preDelete&&f.Content()->thoradin==first->guid);++cases;}
 // A previous owned final actor remains the finale even after its boss has
 // disappeared. A replacement owner must not replay the native conversation.
 {Fixture f;auto*first=f.Spawn();first->unitSummoner=nullptr;f.boss.guid={2,104276};f.RenewOwner();auto*second=f.Spawn();assert(!first->preDelete&&second->preDelete&&f.Content()->HasThoradin()&&f.Intros()==1);++cases;}
 for(uint32 invalid=0;invalid<8;++invalid){Fixture f;auto*first=f.Spawn();f.RenewOwner();switch(invalid){case 0:first->world=false;break;case 1:first->alive=false;break;case 2:first->preDelete=true;break;case 3:first->phase=2;break;case 4:first->map=&f.otherMap;break;case 5:first->entry=1;break;case 6:first->ownerGuid.entry=123;break;case 7:first->x=101;break;}auto*second=f.Spawn();assert(!second->preDelete&&f.Content()->thoradin==second->guid&&f.Intros()==2);++cases;}
 {Fixture f;Creature unrelated;unrelated.map=&f.map;unrelated.entry=123;unrelated.guid={4,123};f.ownerAI->JustSummoned(&unrelated);assert(!unrelated.preDelete&&f.ownerAI->summonEvents==1);++cases;}
 for(uint32 invalid=0;invalid<4;++invalid){Fixture f;switch(invalid){case 0:scenarioMgr.present=false;break;case 1:scenarioState.id=991;break;case 2:scenarioState.step=8;break;case 3:f.boss.mapId=1;break;}f.Spawn();f.Spawn();assert(f.Live()==2&&f.Intros()==2&&logs==0);++cases;}
 std::cout<<"PASS "<<cases<<" native InitSummon/owner-before-SmartAI/actor-claim/single-intro, recreated-owner, moved/orphan/foreign/context cases. Grid allocation/removal and real client duplication remain separate checks.\n";
}
