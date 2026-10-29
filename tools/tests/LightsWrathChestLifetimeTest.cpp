#define main PreservedInsertionMain
#include "GameObjectSummonInsertionTest.cpp"
#undef main
#include <cmath>
using GameObjectList=std::list<GameObject*>;
#include "RemoveAllGameObjects.inc"
#include "IsSpawned.inc"
constexpr uint32 SMART_ACTION_SUMMON_GO=50,SMART_TARGET_POSITION=8,SMART_TARGET_RANDOM_POSITION=128;
#include "SummonFlags.inc"
constexpr double M_PI=3.14159265358979323846;
float rand_norm(){return 0.141143f/float(2*M_PI);}
bool IsUnit(WorldObject* object){return object->ToUnit()!=nullptr;}
using ObjectList=std::list<WorldObject*>;
struct SmartScriptHolder{
 bool runOnce=true;
 struct{struct{uint32 entry=248572,despawnTime=0,flags=0;}summonGO;}action;
 struct{float x=1868.87f,y=2198.75f,z=37.3797f,o=0.141143f;struct{float range=0,distance=0,angle=0;}randomPos;}target;
 uint32 targetType=SMART_TARGET_POSITION;uint32 GetTargetType()const{return targetType;}
};
struct SmartScript{
 Unit* me;WorldObject* GetBaseObject(){return me;}ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){return e.targetType==1?new ObjectList{me}:nullptr;}
 void ProcessAction(SmartScriptHolder& e,Unit* unit=nullptr){switch(SMART_ACTION_SUMMON_GO){
#include "SummonGo.inc"
 default:assert(false);
 }}
};
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && std::string(argv[1])=="restoration"));assert(!PreservedInsertionMain(1,argv));
 uint32 expectedEntry=argc==2?248499:248572,expectedLifetime=argc==2?3600:0;
 objectTemplate.entry=expectedEntry;uint32 cases=0;
 for(uint32 target:{1u,8u,128u})for(uint32 flags:{0u,1u,4u,5u})for(bool admitted:{false,true}){
  Reset();Map map;map.admission=admitted;Unit actor;actor.kind=2;actor.map=&map;units[actor.identity.value]=&actor;
  SmartScript script{&actor};SmartScriptHolder e;e.targetType=target;e.action.summonGO.flags=flags;
  e.action.summonGO.entry=expectedEntry;e.action.summonGO.despawnTime=expectedLifetime;
  if(target==1)e.target.x=e.target.y=e.target.z=e.target.o=0;
  script.ProcessAction(e);assert(map.attempts==1&&errors==uint32(!admitted));
  if(admitted){assert(allocations.size()==1);auto go=*allocations.begin();bool creator=(flags&4)==0 && expectedLifetime==0;
   assert(go->world&&go->isSpawned()&&go->lifetime==expectedLifetime&&go->summoner==actor.identity&&go->owner==ObjectGuid{creator?actor.identity.value:0});
   assert(actor.m_gameObj.size()==uint32(creator));actor.RemoveAllGameObjects();actor.RemoveFromWorld();
   assert(go->deleted==creator&&actor.m_gameObj.empty());
   // Lifetime0 is the old source-cleanup counterfactual. A creature's positive
   // native lifetime already selects map ownership even with legacy flag0.
   Dispose(go);
  }else assert(allocations.empty()&&actor.m_gameObj.empty());
  assert(allocations.empty()&&created==destroyed);++cases;
 }
 for(uint32 flags:{2u,6u})for(bool admitted:{false,true}){
  Reset();Map map;map.admission=admitted;Unit actor;actor.kind=2;actor.map=&map;units[actor.identity.value]=&actor;
  SmartScript script{&actor};SmartScriptHolder e;e.action.summonGO.flags=flags;
  e.action.summonGO.entry=expectedEntry;e.action.summonGO.despawnTime=expectedLifetime;script.ProcessAction(e);
  assert(e.runOnce==admitted&&map.attempts==1);if(admitted){auto go=*allocations.begin();assert(go->owner.IsEmpty()==bool(expectedLifetime || (flags&4)));Dispose(go);}assert(allocations.empty());++cases;
 }
 std::cout<<"PASS "<<cases<<" native Smart action50 -> WorldObject summon/owner/map admission -> Unit::RemoveAllGameObjects: lifetime0 creature ownership deletes on source cleanup unless flag4; positive lifetime is already map-owned with legacy flag0. Tested entry "<<expectedEntry<<" lifetime "<<expectedLifetime<<", native summoner/scope, fixed/unit/random targets, flags0/1/4, retry2+4 and false insertion cleanup. Grid geometry/despawn scheduling/rendering remain adapters.\n";
}
