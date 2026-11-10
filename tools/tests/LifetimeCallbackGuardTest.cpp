// Selected production weak-capture/guard projection,not the encounter body.
// Main Release compiles the unchanged original bodies;source reverse proof and
// PE guard dominance checks are separate evidence.
#include <functional>
#include <vector>
#include <memory>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <new>
#include <string>
using uint8=uint8_t;using uint32=uint32_t;
struct Queue{std::vector<std::pair<uint32,std::function<void()>>> q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,std::move(f));}void Update(uint32 n){std::vector<std::function<void()>> ready;for(auto i=q.begin();i!=q.end();)if(i->first<=n){ready.push_back(std::move(i->second));i=q.erase(i);}else{i->first-=n;++i;}for(auto&f:ready)f();}void Cancel(){q.clear();}};
struct Creature{uint32 calls=0;Queue events,combats;void Touch(){++calls;}void AddDelayedEvent(uint32 n,std::function<void()>f){events.Add(n,std::move(f));}void AddDelayedCombat(uint32 n,std::function<void()>f){combats.Add(n,std::move(f));}};
#include "Selected.inc"
static void Require(bool b,char const*m){if(!b)throw std::runtime_error(m);}
int main(int argc,char**argv){try{Require(argc==2,"callback index");uint32 i=uint32(std::stoul(argv[1]));Require(i<sizeof(combat)/sizeof(*combat),"index range");Creature actor,peer;Creature&owner=other[i]?peer:actor;Queue&q=combat[i]?owner.combats:owner.events;
#ifndef COUNTERFACTUAL
 {auto live=std::make_unique<Frame>(&actor,&peer);live->Queue(i);q.Update(122);Require(!actor.calls,"controlled guard probe clock");q.Update(1);Require(actor.calls==1,"live source AI admits original role");q.Update(60000);Require(actor.calls==1,"once-only admitted callback");}
 actor.calls=0;
#endif
 {auto retired=std::make_unique<Frame>(&actor,&peer);retired->Queue(i);retired.reset();q.Update(123);Require(!actor.calls,"retired source AI cannot enter payload");}
#ifndef COUNTERFACTUAL
 {auto cancelled=std::make_unique<Frame>(&actor,&peer);cancelled->Queue(i);q.Cancel();cancelled.reset();q.Update(60000);Require(!actor.calls,"Unit queue cancellation retained");}
 {alignas(Frame)unsigned char slot[sizeof(Frame)];Frame*old=new(slot)Frame(&actor,&peer);old->Queue(i);old->~Frame();Frame*replacement=new(slot)Frame(&actor,&peer);replacement->Queue(i);q.Update(123);Require(actor.calls==1,"same-address AI replacement cannot revive old weak admission");replacement->~Frame();q.Update(60000);Require(actor.calls==1,"ABA witness settles once");}
#endif
 std::cout<<"PASS selected actor queue weak-admission projection:live,retired,cancelled and same-address replacement;production payload not executed here\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
