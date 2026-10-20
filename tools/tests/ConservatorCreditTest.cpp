#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <set>
#include <vector>
using uint32=uint32_t;using CriteriaTypes=uint32;
constexpr uint32 SMART_TARGET_PLAYER_DISTANCE=18,SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA=205;
#define TC_LOG_DEBUG(...) ((void)0)
struct Player;struct Unit;
struct WorldObject{
 float x=0;uint32 phase=1,map=1533;
 virtual ~WorldObject()=default;virtual Player* ToPlayer(){return nullptr;}virtual Unit* ToUnit(){return nullptr;}
 bool IsWithinDist(WorldObject* other,float range,bool)const{return std::abs(x-other->x)<range;}
 bool InSamePhase(WorldObject* other)const{return phase==other->phase;}
};
struct Unit:WorldObject{Unit* ToUnit()override{return this;}bool HasAura(uint32){return false;}};
struct Player:Unit{uint32 count=0;Unit* lastKiller=nullptr;Player* ToPlayer()override{return this;}
 void UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32,uint32,Unit* killer,bool){assert(type==92&&asset==47985);++count;lastKiller=killer;}
};
Player* GetScriptCreditPlayer(WorldObject* object){return object?object->ToPlayer():nullptr;}
using ObjectList=std::list<WorldObject*>;
namespace Trinity{
 class AllWorldObjectsInRange{
 public:AllWorldObjectsInRange(WorldObject const*,float);bool operator()(WorldObject*);
 private:WorldObject const* m_pObject;float m_fRange;
 };
#include "ConservatorRange.inc"
 template<class Check>struct WorldObjectListSearcher{
  WorldObject* source;ObjectList& targets;Check& check;
  WorldObjectListSearcher(WorldObject* obj,ObjectList& list,Check& predicate):source(obj),targets(list),check(predicate){}
 };
 std::vector<WorldObject*> objects;
 template<class Search>void VisitNearbyObject(WorldObject* source,float,Search& search){
  // Grid traversal/map visibility is modeled; the actual range and phase
  // predicate, SmartScript selection and criteria action are extracted.
  for(auto object:objects)if(source->map==object->map&&search.check(object))search.targets.push_back(object);
 }
}
struct SmartScriptHolder{
 uint32 targetType=18;
 struct{struct{uint32 dist=30,spell=0;}playerDistance;}target;
 struct{struct{uint32 type=92,misc1=47985,misc2=0,misc3=0;}achievementCriteria;}action;
};
struct SmartScript{
 WorldObject* base;WorldObject* GetBaseObject(){return base;}
 bool IsPlayer(WorldObject* object){return object&&object->ToPlayer();}
 ObjectList* GetWorldObjectsInDist(float);
 ObjectList* GetTargets(SmartScriptHolder& e,Unit*){auto l=new ObjectList();switch(e.targetType){
#include "ConservatorPlayerTargets.inc"
 }return l;}
 void Credit(SmartScriptHolder& e,Unit* unit){switch(SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA){
#include "ConservatorCriteria.inc"
 }}
};
#include "ConservatorWorldObjects.inc"
int main(int argc,char** argv){
 assert(argc==2);uint32 patchedRange=std::atoi(argv[1]);assert(patchedRange==100);
 for(bool patched:{false,true})for(float distance:{5.f,29.f,40.f,60.f,95.f,110.f})for(uint32 killerKind:{0u,1u,2u}){
  Unit boss,pet;Player owner,nearby,hidden,otherMap,farAway;
  owner.x=distance;nearby.x=5;hidden.x=5;hidden.phase=2;otherMap.x=5;otherMap.map=0;farAway.x=150;
  Trinity::objects={&boss,&pet,&owner,&nearby,&hidden,&otherMap,&farAway};
  SmartScript script{&boss};SmartScriptHolder action;action.target.playerDistance.dist=patched?patchedRange:30;
  Unit* killer=killerKind==0?static_cast<Unit*>(&owner):killerKind==1?&pet:nullptr;
  script.Credit(action,killer);
  bool credited=distance<float(action.target.playerDistance.dist);
  assert(owner.count==uint32(credited)&&nearby.count==1);
  assert(!hidden.count&&!otherMap.count&&!farAway.count);
  if(credited)assert(owner.lastKiller==killer);
  if(!patched&&distance==40.f&&killerKind==1)std::cout<<"BASELINE: pet beside boss, owner 40 yards away -> owner credit 0\n";
 }
 std::cout<<"PASS: production PLAYER_DISTANCE, live object search/range/phase predicate and criteria action; 30-yard loss reproduced, 100-yard repair covers ranged player/pet/null killers and preserves visibility/range exclusions\n";
}
