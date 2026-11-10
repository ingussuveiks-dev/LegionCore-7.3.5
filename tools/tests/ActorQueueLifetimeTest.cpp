#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
struct Queue{std::vector<std::pair<uint32,std::function<void()>>>q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,f);}void Update(uint32 n){std::vector<std::function<void()>>v;for(auto i=q.begin();i!=q.end();)if(i->first<=n){v.push_back(i->second);i=q.erase(i);}else{i->first-=n;++i;}for(auto&f:v)f();}void KillAllFunctions(){q.clear();}};
struct Unit;using Creature=Unit;struct Unit{Queue q;Unit*owner=nullptr;bool alive=true,walk=true;uint32 state=0xffffffff,calls=0,last=0;Unit*target=nullptr;Unit*GetAnyOwner(){return owner;}bool IsAlive(){return alive;}void AddDelayedEvent(uint32 n,std::function<void()>f){q.Add(n,f);}void AddDelayedCombat(uint32 n,std::function<void()>f){q.Add(n,f);}void ClearUnitState(uint32 n){state&=~n;++calls;}void SetWalk(bool b){walk=b;++calls;}void RemoveAura(uint32 n){last=n;++calls;}void CastSpell(Unit*u,uint32 n,bool=false){target=u;last=n;++calls;}void DespawnOrUnsummon(uint32 n=0){last=n;++calls;} };
#include "Selected.inc"
static void Require(bool b,char const*m){if(!b)throw std::runtime_error(m);}
int main(int argc,char**argv){try{Require(argc==2,"callback index required");auto index=uint32(std::stoul(argv[1]));Require(index<sizeof(delays)/sizeof(*delays),"valid callback");Unit actor,owner;actor.owner=&owner;auto frame=std::make_unique<Frame>(&actor);frame->Queue(index);frame.reset();actor.q.Update(delays[index]-1);Require(!actor.calls,"authored delay must not fire early");actor.q.Update(1);Require(actor.calls==1,"one authored actor operation after deleted AI frame");actor.q.Update(60000);Require(actor.calls==1,"callback is one-shot");
#ifndef COUNTERFACTUAL
 actor.calls=0;auto cancelled=std::make_unique<Frame>(&actor);cancelled->Queue(index);cancelled.reset();actor.q.KillAllFunctions();actor.q.Update(delays[index]);Require(!actor.calls,"owning actor cancellation retains cleanup semantics");
#endif
 std::cout<<"PASS selected actor queue after actual AI-frame destruction;original delay,payload,one-shot and cancellation\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
