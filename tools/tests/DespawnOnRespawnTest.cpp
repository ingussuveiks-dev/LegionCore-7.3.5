// Complete production helper; actor endpoints/clock are controlled for ASAN.
#include <functional>
#include <vector>
#include <memory>
#include <cstdint>
#include <iostream>
#include <stdexcept>
using uint32=uint32_t;
struct Creature{
 struct Pending{uint32 delay;std::function<void()> body;};std::vector<Pending> pending;
 uint32 despawns=0,respawn=0;
 void AddDelayedEvent(uint32 ms,std::function<void()> f){pending.push_back({ms,std::move(f)});}
 void DespawnOrUnsummon(){++despawns;}
 void SetRespawnTime(uint32 value){respawn=value;}
 void Advance(uint32 ms){std::vector<std::function<void()>> ready;for(auto i=pending.begin();i!=pending.end();)if(i->delay<=ms){ready.push_back(std::move(i->body));i=pending.erase(i);}else{i->delay-=ms;++i;}for(auto& f:ready)f();}
};
struct CreatureAI{Creature* me;explicit CreatureAI(Creature* c):me(c){}void DespawnOnRespawn(uint32);};
#include "DespawnOnRespawn.inc"
static void Require(bool ok,char const* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(uint32 delay:{0u,3u,5u}){
  Creature actor,other;auto old=std::make_unique<CreatureAI>(&actor);old->DespawnOnRespawn(delay);old.reset();
  actor.Advance(99);Require(!actor.despawns,"100ms deadline changed");actor.Advance(1);Require(actor.despawns==1&&actor.respawn==delay,"Retired AI lost actor request or respawn argument");actor.Advance(60000);Require(actor.despawns==1&&!other.despawns,"Duplicate or wrong actor despawn");
 }
#ifndef COUNTERFACTUAL
 {Creature actor;CreatureAI live(&actor);live.DespawnOnRespawn(5);actor.pending.clear();actor.Advance(1000);Require(!actor.despawns,"Queue cancellation changed");}
 {Creature actor,other;alignas(CreatureAI)unsigned char slot[sizeof(CreatureAI)];auto old=new(slot)CreatureAI(&actor);old->DespawnOnRespawn(3);old->~CreatureAI();auto replacement=new(slot)CreatureAI(&other);actor.Advance(100);Require(actor.despawns==1&&!other.despawns&&actor.respawn==3,"Same-address AI replacement redirected pending actor request");replacement->~CreatureAI();}
#endif
 std::cout<<"PASS complete DespawnOnRespawn helper:100ms/zero-three-five seconds,retired source AI,original actor,once-only,cancellation and same-address replacement;controlled actor endpoints\n";return 0;
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
