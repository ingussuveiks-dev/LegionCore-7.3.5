#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <tuple>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
uint32 errors=0,infos=0,allocated=0,deleted=0,cleaned=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_INFO(...) (++infos)
struct ObjectGuid{uint64 value=0;bool operator==(ObjectGuid o)const{return value==o.value;}bool operator!=(ObjectGuid o)const{return !(*this==o);}bool operator<(ObjectGuid o)const{return value<o.value;}};
struct Position{float x,y,z,o;Position(float a,float b,float c,float d):x(a),y(b),z(c),o(d){}};
namespace G3D{struct Quat{Quat(float a,float b,float c,float d){assert(!a&&!b&&!c&&!d);}};}
constexpr uint32 GO_STATE_READY=1;
struct Creature{
 ObjectGuid guid{2},summoner{1};bool world=true,alive=true,predelete=false,scope=false,temp=true,moving=false,samePhase=true;uint32 entry=104546;float distance=0;
 ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool IsPreDelete(){return predelete;}bool HasQuestObjectScope(){return scope;}uint32 GetEntry(){return entry;}Creature*ToTempSummon(){return temp?this:nullptr;}ObjectGuid GetSummonerGUID(){return summoner;}
 bool InSamePhase(uint32 m){assert(m==3);return samePhase;}bool InSamePhaseId(std::set<uint32>const&p,bool update){assert(!update&&(p.count(6140)||p.count(6068)));return samePhase;}bool isMoving(){return moving;}float GetExactDist(float x,float y,float z){assert(x==4204.35f&&y==7090.24f&&z==268.198f);return distance;}
};
struct WayPoint{float x=4204.35f,y=7090.24f,z=268.198f;};using WPPath=std::map<uint32,WayPoint*>;
struct Waypoints{WPPath path;bool missing=false;WPPath*GetPath(uint32 id){assert(id==104546);return missing?nullptr:&path;}}waypoints;auto*sSmartWaypointMgr=&waypoints;
struct GameObject;
struct ProgressValue{uint32 Counter=0;};struct Achievement{std::map<uint32,ProgressValue>values;auto const*GetCriteriaProgressMap(){return &values;}};
struct Scenario{uint32 id=1065,step=5;Achievement achievements;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}Achievement&GetAchievementMgr(){return achievements;}};
struct Map{
 uint32 creates=0,inserts=0,failCreate=0,failInsert=0,instance=77;Creature*actor=nullptr;std::vector<GameObject*>objects;std::function<void()>onInsert;
 uint32 GetInstanceId(){return instance;}Creature*GetCreature(ObjectGuid g){return actor&&actor->guid==g?actor:nullptr;}bool AddToMap(GameObject*);~Map();
};
struct Scenarios{Scenario*scenario=nullptr;Scenario*GetScenario(uint32 id){assert(id==77);return scenario;}}scenarios;auto*sScenarioMgr=&scenarios;
enum class HighGuid{GameObject};struct Generator{uint64 value=100;uint64 Generate(){return ++value;}};
struct ObjectMgr{Generator generator;template<HighGuid>Generator*GetGenerator(){return &generator;}}objects;auto*sObjectMgr=&objects;
struct GameObject{
 ObjectGuid guid,summoner;Map*map=nullptr;uint32 mask=0;std::set<uint32>phases;bool world=false,byDefault=true;uint32 respawn=99;
 GameObject(){++allocated;}~GameObject(){++deleted;}
 bool Create(uint64 low,uint32 entry,Map*m,uint32 phase,Position p,G3D::Quat,uint32 anim,uint32 state){assert(entry==248553&&anim==100&&state==GO_STATE_READY);assert(p.x==4202.22f&&p.y==7084.17f&&p.z==268.199f&&p.o==2.81046f);map=m;mask=phase;guid={low};++m->creates;if(m->failCreate){--m->failCreate;return false;}return true;}
 void SetPhaseId(std::set<uint32>const&p,bool update){assert(!world&&!update);phases=p;}void SetSummonerGUID(ObjectGuid g){summoner=g;}void SetRespawnTime(uint32 t){respawn=t;}void SetSpawnedByDefault(bool b){byDefault=b;}void CleanupsBeforeDelete(){assert(!world);++cleaned;}ObjectGuid GetGUID(){return guid;}
};
bool Map::AddToMap(GameObject*g){++inserts;assert(g->mask==3&&(g->phases.count(6140)||g->phases.count(6068)));assert(!g->respawn&&!g->byDefault&&g->summoner.value==2);if(failInsert){--failInsert;return false;}g->world=true;objects.push_back(g);if(onInsert)onInsert();return true;}
Map::~Map(){for(auto*g:objects)delete g;}
// The upstream earned death receipt is a controlled boundary here. Its full
// player/pet/guardian/quest/credit/factory sequence has a separate native suite.
namespace BilaalDeath{
 struct Route{uint32 scenario,victory;};
 struct State{Route const*route=nullptr;bool earned=true,done=true;ObjectGuid receiver{2},source{1};uint32 phaseMask=3;std::set<uint32>phases;};
#include "Progress.inc"
#include "Victory.inc"
}
#include "Portal.inc"
struct Test{
 Map map;Creature actor;Scenario scenario;WayPoint endpoint;BilaalDeath::Route route;BilaalDeath::State death;BilaalPortal::State portal;
 explicit Test(bool mage):route{mage?1101u:1065u,mage?47995u:47117u}{scenario.id=route.scenario;scenario.achievements.values[route.victory].Counter=1;death.route=&route;death.phases={mage?6068u:6140u,999};map.actor=&actor;scenarios.scenario=&scenario;waypoints.missing=false;waypoints.path={{1,&endpoint}};}
 void arrive(){portal.Arrived(&map,death,actor.guid);}void ticks(){for(int i=0;i<8;++i)portal.Update(&map,death,500);}void success(){assert(portal.arrived&&portal.done&&!portal.creating&&map.objects.size()==1);auto*g=map.objects.front();assert(g->phases==death.phases&&g->summoner==death.receiver&&g->guid==portal.portal);}
};
#include "BilaalPortalDispatchTest.h"
int main(){
 for(bool mage:{false,true}){
  Dispatch::Check(mage);
  {Test t(mage);t.ticks();assert(!t.map.creates);t.arrive();t.success();for(int i=0;i<50;++i){t.arrive();t.ticks();}assert(t.map.creates==1);}
  for(int denial=0;denial<21;++denial){Test t(mage);if(denial==0)t.death.earned=false;if(denial==1)t.death.done=false;if(denial==2)scenarios.scenario=nullptr;if(denial==3)t.scenario.id=1;if(denial==4)t.scenario.step=4;if(denial==5)t.scenario.achievements.values.clear();if(denial==6)t.actor.guid={3};if(denial==7)t.map.actor=nullptr;if(denial==8)t.actor.world=false;if(denial==9)t.actor.alive=false;if(denial==10)t.actor.predelete=true;if(denial==11)t.actor.entry=104902;if(denial==12)t.actor.scope=true;if(denial==13)t.actor.temp=false;if(denial==14)t.actor.summoner={9};if(denial==15)t.actor.samePhase=false;if(denial==16)waypoints.missing=true;if(denial==17)waypoints.path.clear();if(denial==18)waypoints.path[1]=nullptr;if(denial==19)t.actor.moving=true;if(denial==20)t.actor.distance=1.01f;t.arrive();t.ticks();assert(!t.map.creates&&!t.portal.arrived);}
  {Test t(mage);t.actor.distance=1;t.arrive();t.success();}
  {Test t(mage);waypoints.path[2]=&t.endpoint;t.arrive();assert(!t.portal.arrived);}
  for(bool insertion:{false,true}){Test t(mage);auto before=errors;auto recover=infos;if(insertion)t.map.failInsert=2;else t.map.failCreate=2;t.arrive();assert(!t.portal.done&&t.portal.arrived&&errors==before+1);t.portal.Update(&t.map,t.death,1999);assert(t.map.creates==1);t.portal.Update(&t.map,t.death,1);assert(t.map.creates==2&&!t.portal.done&&errors==before+1);t.map.actor=nullptr;t.ticks();t.success();assert(t.map.creates==3&&infos==recover+1);}
  {Test t(mage);t.map.failCreate=1;t.arrive();t.scenario.step=6;t.ticks();assert(t.portal.done&&t.map.creates==1&&t.map.objects.empty());}
  {Test t(mage);t.map.onInsert=[&]{t.arrive();t.portal.Update(&t.map,t.death,2000);};t.arrive();t.success();assert(t.map.creates==1);}
  {Test t(mage);t.portal.Arrived(nullptr,t.death,t.actor.guid);t.portal.Update(nullptr,t.death,2000);assert(!t.map.creates);}
 }
 assert(allocated==deleted);
 std::cout<<"PASS actual portal receipt/factory both routes,42 denials,real endpoint prerequisite,exactly-once/reentry,insertion/create retry,phase-before-insert,map ownership,emitter removal,reentrant callbacks,cleanup and stage cancellation; upstream earned receipt and engine endpoints controlled\n";
}
