// Extracted production callbacks: deterministic admission, cadence and reentrant lifetime checks.
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using uint32 = uint32_t;
using ObjectGuid = uint64_t;
using GuidSet = std::set<ObjectGuid>;
static unsigned checks=0,rolls=0; static uint32 roll=1;
static void Require(bool ok){++checks;if(!ok)throw std::runtime_error("outro assertion "+std::to_string(checks));}
uint32 urand(uint32 a,uint32 b){Require(a==0&&b==1);++rolls;return roll;}
struct Scenario { unsigned id=1078; bool done=false; unsigned GetScenarioId(){return id;} bool IsCompleted(bool bonus){Require(!bonus);return done;} };
struct Manager {Scenario* scenario=nullptr;Scenario* GetScenario(unsigned id){Require(id==42);return scenario;}} manager;
auto sScenarioMgr=&manager;
struct Creature {ObjectGuid guid; unsigned entry;bool world=true,deleted=false;unsigned hits=0;std::function<void()> callback;
 ObjectGuid GetGUID(){return guid;}unsigned GetEntry(){return entry;}bool IsInWorld(){return world;}bool IsPreDelete(){return deleted;}
 void CastSpell(Creature* target,unsigned spell){Require(target==this&&spell==(entry==98308?209814u:209813u));++hits;if(callback)callback();}
};
struct Map {std::map<ObjectGuid,Creature*> actors;unsigned GetInstanceId(){return 42;}Creature* GetCreature(ObjectGuid guid){auto it=actors.find(guid);return it==actors.end()?nullptr:it->second;}};
struct Probe {Map* instance;std::list<Creature*> trashGUIDconteiner;uint32 outrotimer;unsigned cache=6;unsigned getScenarionStep(){return cache;}
#include "DevourerOutro.inc"
};
struct Fixture {Scenario scenario;Map map;Probe probe{&map};std::vector<std::unique_ptr<Creature>> actors;
 Fixture(){manager.scenario=&scenario;probe.Initialize();}
 Creature* Add(unsigned entry){actors.emplace_back(new Creature{actors.size()+1,entry});auto c=actors.back().get();map.actors[c->guid]=c;probe.OnCreatureCreate(c);return c;}
 void Remove(Creature* c){probe.OnCreatureRemove(c);map.actors.erase(c->guid);c->world=false;}
};
int main(){try{
 {Fixture f;auto c=f.Add(98307);f.scenario.done=true;f.probe.Update(1999);
#ifdef DEVOURER_OUTRO_BASELINE
  f.probe.Update(1);Require(c->hits==0&&f.probe.outrotimer==2000&&f.probe.cache==6);
  f.probe.cache=7;f.probe.Update(1999);Require(c->hits==0);f.probe.Update(1);Require(c->hits==1&&f.probe.outrotimer==2000);}
  std::cout<<"COUNTERFACTUAL Devourer outro: natural complete/cache6 blocks reactions; synthetic cache7 admits native two-second spell. checks="<<checks<<"\n";
#else
  Require(c->hits==0&&f.probe.outrotimer==1);f.probe.Update(1);Require(c->hits==1&&f.probe.outrotimer==2000&&f.probe.cache==6);
  f.probe.Update(10000);Require(c->hits==2&&f.probe.outrotimer==2000);}
 for(unsigned mode=0;mode<3;++mode){Fixture f;auto c=f.Add(98307);f.probe.cache=7;if(mode==1){f.scenario.done=true;f.scenario.id=1123;}if(mode==2)manager.scenario=nullptr;auto before=rolls;f.probe.Update(10000);Require(c->hits==0&&f.probe.outrotimer==2000&&rolls==before);}
 {Fixture f;f.scenario.done=true;auto a=f.Add(98308);auto b=f.Add(98307);auto c=f.Add(54638);f.Add(1);f.probe.OnCreatureCreate(a);f.probe.trashGUIDconteiner.push_back(nullptr);f.probe.Update(2000);Require(a->hits==1&&b->hits==1&&c->hits==1&&f.actors.back()->hits==0);roll=0;f.probe.Update(2000);Require(a->hits==1&&b->hits==1&&c->hits==1);roll=1;}
 for(unsigned mode=0;mode<5;++mode){Fixture f;f.scenario.done=true;auto a=f.Add(98308);if(mode==0)a->world=false;if(mode==1)a->deleted=true;if(mode==2)a->entry=1;if(mode==3)f.map.actors.clear();if(mode==4)f.probe.OnCreatureRemove(a);auto before=rolls;f.probe.Update(2000);Require(a->hits==0&&rolls==before);}
 for(unsigned mode=0;mode<7;++mode){Fixture f;f.scenario.done=true;auto a=f.Add(98308);auto b=f.Add(98307);Creature* added=nullptr;
  a->callback=[&]{if(mode==0){f.Remove(b);f.actors[1].reset();}if(mode==1)f.Remove(a);if(mode==2)added=f.Add(54638);if(mode==3)f.probe.Initialize();if(mode==4)manager.scenario=nullptr;if(mode==5)f.scenario.done=false;if(mode==6)f.scenario.id=1123;};
  f.probe.Update(2000);Require(a->hits==1&&f.probe.outrotimer==2000);if(mode!=0)Require(b->hits==((mode==1||mode==2)?1u:0u));if(added)Require(added->hits==0);
 }
 {Fixture f;f.scenario.done=true;auto a=f.Add(98308);auto b=f.Add(54638);a->callback=[]{};f.probe.Update(2000);f.probe.Update(2000);Require(a->hits==2&&b->hits==2);}
 std::cout<<"PASS Devourer outro source ASAN:completion/cache6,foreign/missing/incomplete gate,timing/randomness/three entries/dedup,missing/predelete/untracked actors,self/other removal+free,add/reset/scenario mutation,ordinary spell mapping/retry; checks="<<checks<<"\n";
#endif
 return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<"\n";return 1;}}
