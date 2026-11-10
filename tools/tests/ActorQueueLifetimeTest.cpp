#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <cmath>
#include <iostream>
#include <memory>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
struct Queue{std::vector<std::pair<uint32,std::function<void()>>>q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,f);}void Update(uint32 n){std::vector<std::function<void()>>v;for(auto i=q.begin();i!=q.end();)if(i->first<=n){v.push_back(i->second);i=q.erase(i);}else{i->first-=n;++i;}for(auto&f:v)f();}void KillAllFunctions(){q.clear();}};
struct Position{float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}};struct ObjectGuid{uint64 lo=0,hi=0;};
struct Unit;static Unit*queueSpawned=nullptr;static bool queueChildAiPresent=true;struct GameObject{Unit*source;void SetByteValue(uint32,uint32,uint32);};using Creature=Unit;struct Motion{Unit*owner;void MoveRandom(float);template<class...T>void MovePoint(T...);};struct Unit{Motion motion{this};GameObject go{this};Queue q;Unit*owner=nullptr;bool alive=true,walk=true;uint32 mapToken=1;float x=0,y=0,z=0;uint32 faction=0,team=0,entry=542003,summonedEntry=0;uint64 maxHealth=123,health=123;uint32 state=0xffffffff,calls=0,last=0;Unit*target=nullptr;Unit*FindNearestCreature(uint32,float,bool=true){return this;}GameObject*FindNearestGameObject(uint32,float){return &go;}void CreateConversation(uint32 n){last=n;++calls;}void SetPower(uint32,uint32 n){last=n;++calls;}void PlayOneShotAnimKit(uint32 n){last=n;++calls;}void SendPlaySpellVisualKit(uint32 n,uint32){last=n;++calls;}void NearTeleportTo(float,float,float,float){++calls;}Motion*GetMotionMaster(){return &motion;}void RemoveFlag(uint32,uint32){++calls;}void SetVisible(bool){++calls;}void SetSpeed(uint32,float){++calls;}void Kill(Unit*){++calls;}template<class...T>Unit*SummonCreature(uint32 n,T...){summonedEntry=n;++calls;return queueSpawned;}Unit*AI(){return this==queueSpawned&&!queueChildAiPresent?nullptr:this;}void DoZoneInCombat(){++calls;}Unit*ToCreature(){return this;}Unit*ToPlayer(){return this;}uint32 GetTeam(){return team;}uint32 GetEntry(){return entry;}float GetOrientation(){return 0;}void AttackStart(Unit*u){target=u;++calls;}void SetUInt32Value(uint32,uint32){++calls;}void RemoveAurasDueToSpell(uint32){++calls;}uint64 GetMaxHealth(){return maxHealth;}void SetMaxHealth(uint64 n){maxHealth=n;++calls;}void SetHealth(uint64 n){health=n;++calls;}void Talk(uint32 n){last=n;++calls;}void DoCast(uint32 n){last=n;++calls;}void DoCast(Unit*,uint32 n,bool=false){last=n;++calls;}void SetReactState(uint32){++calls;}void HandleEmoteCommand(uint32){++calls;}void setFaction(uint32 n){faction=n;++calls;}float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}Position GetPosition(){return{x,y,z,0};}ObjectGuid GetGUID(){return{};}bool IsInWorld(){return true;}void*GetMap(){return reinterpret_cast<void*>(uintptr_t(mapToken));}void CastSpell(Position const&,uint32 n,bool=false){last=n;++calls;}Unit*GetAnyOwner(){return owner;}bool IsAlive(){return alive;}void AddDelayedEvent(uint32 n,std::function<void()>f){q.Add(n,f);}void AddDelayedCombat(uint32 n,std::function<void()>f){q.Add(n,f);}void ClearUnitState(uint32 n){state&=~n;++calls;}void SetWalk(bool b){walk=b;++calls;}void RemoveAura(uint32 n){last=n;++calls;}void CastSpell(Unit*u,uint32 n,bool=false){target=u;last=n;++calls;}void DespawnOrUnsummon(uint32 n=0){last=n;++calls;} };
void GameObject::SetByteValue(uint32,uint32,uint32){++source->calls;}
void Motion::MoveRandom(float){++owner->calls;}template<class...T>void Motion::MovePoint(T...){++owner->calls;}
static Unit*queueTarget=nullptr;static ObjectGuid QueueTargetGuid(){return{};}static Unit*QueueTarget(Creature*c){return queueTarget?queueTarget:c;}namespace ObjectAccessor{static Unit*GetUnit(Unit&,ObjectGuid){return queueTarget;}static Creature*GetCreature(Unit&,ObjectGuid){return queueTarget;}}struct TextMgr{void SendChat(Creature*c,uint8 n,ObjectGuid){c->Talk(n);}};static TextMgr textMgr;static TextMgr*sCreatureTextMgr=&textMgr;static void GetCreatureListWithEntryInGrid(std::list<Creature*>&v,Creature*c,uint32,float){v.push_back(c);}static uint32 urand(uint32 a,uint32){return a;}static float frand(float,float){return 0;}static int irand(int a,int){return a;}
#include "Selected.inc"
static void Require(bool b,char const*m){if(!b)throw std::runtime_error(m);}
int main(int argc,char**argv){try{Require(argc==2,"callback index required");auto index=uint32(std::stoul(argv[1]));Require(index<sizeof(delays)/sizeof(*delays),"valid callback");Unit actor,owner;actor.owner=&owner;auto capturedTarget=std::make_unique<Unit>();
#ifdef CASE_EVALAVISIBILITY
 if(index==2)actor.owner=capturedTarget.get();
#endif
 auto frame=std::make_unique<Frame>(&actor);frame->Queue(index);frame.reset();capturedTarget.reset();
#ifdef CASE_EVALAVISIBILITY
 if(index==2)actor.owner=nullptr;
#endif
 actor.q.Update(delays[index]-1);Require(!actor.calls,"authored delay must not fire early");actor.q.Update(1);Require(actor.calls==effects[index],"one authored actor operation after deleted AI frame");actor.q.Update(60000);Require(actor.calls==settled[index],"all authored nested operations settle once");actor.q.Update(60000);Require(actor.calls==settled[index],"settled queue is one-shot");
#ifndef COUNTERFACTUAL
 actor.calls=0;actor.owner=&owner;auto cancelled=std::make_unique<Frame>(&actor);cancelled->Queue(index);cancelled.reset();actor.q.KillAllFunctions();actor.q.Update(delays[index]);Require(!actor.calls,"owning actor cancellation retains cleanup semantics");
#endif

#ifndef COUNTERFACTUAL
#ifdef CASE_EVALAVISIBILITY
 if(index==2){actor.owner=&owner;queueTarget=&owner;owner.calls=0;auto present=std::make_unique<Frame>(&actor);present->Queue(index);present.reset();actor.q.Update(delays[index]);Require(owner.calls==1,"living same-map target receives one summon request");for(uint32 mode=0;mode<2;++mode){owner.calls=0;owner.alive=mode!=0;owner.mapToken=mode==1?2:1;auto invalid=std::make_unique<Frame>(&actor);invalid->Queue(index);invalid.reset();actor.q.Update(delays[index]);Require(!owner.calls,"dead or transferred target rejected");}owner.alive=true;owner.mapToken=1;queueTarget=nullptr;}
#endif
#ifdef CASE_LURAPORTAL
 if(index==0){Unit spawned;queueSpawned=&spawned;for(bool hasAi:{true,false}){queueChildAiPresent=hasAi;actor.calls=0;spawned.calls=0;auto positive=std::make_unique<Frame>(&actor);positive->Queue(index);positive.reset();actor.q.Update(delays[index]);Require(actor.calls==1&&spawned.calls==(hasAi?1u:0u),"successful portal spawn delegates combat only to an available child AI");}queueSpawned=nullptr;queueChildAiPresent=true;}
#endif
#ifdef CASE_BANKERRIFT
 {Unit spawned;queueSpawned=&spawned;for(uint32 entry:{542003u,542006u})for(uint32 team:{469u,67u}){actor.entry=entry;actor.owner=&owner;owner.team=team;actor.calls=0;spawned.calls=0;auto positive=std::make_unique<Frame>(&actor);positive->Queue(index);positive.reset();actor.q.Update(delays[index]);Require(actor.calls==1&&actor.summonedEntry==(entry==542003?542004u:542005u)&&spawned.calls==1&&spawned.faction==(team==469?12u:29u),"both rifts preserve exact summon entry and current owner faction after AI destruction");}actor.owner=nullptr;actor.calls=0;spawned.calls=0;auto missing=std::make_unique<Frame>(&actor);missing->Queue(index);missing.reset();actor.q.Update(delays[index]);Require(actor.calls==1&&!spawned.calls,"missing current owner skips only faction assignment");queueSpawned=nullptr;actor.owner=&owner;}
#endif
#ifdef CASE_SAPRISHCOMPANIONS
 if(index<2){actor.calls=0;owner.maxHealth=5000000123ULL;queueTarget=&owner;auto positive=std::make_unique<Frame>(&actor);positive->Queue(index);positive.reset();actor.q.Update(delays[index]);Require(actor.calls==2&&actor.maxHealth==owner.maxHealth&&actor.health==owner.maxHealth,"full native health-width copy after AI replacement");for(uint32 mode=0;mode<2;++mode){actor.calls=0;owner.alive=mode!=0;owner.mapToken=mode==1?2:1;auto invalid=std::make_unique<Frame>(&actor);invalid->Queue(index);invalid.reset();actor.q.Update(delays[index]);Require(!actor.calls,"dead or transferred Saprish rejected");}owner.alive=true;owner.mapToken=1;queueTarget=nullptr;}
#endif
#ifdef CASE_INQUISITIONDIALOGUE
 if(index==1){actor.calls=0;owner.x=10;queueTarget=&owner;auto positive=std::make_unique<Frame>(&actor);positive->Queue(index);positive.reset();actor.q.Update(delays[index]);Require(!actor.calls,"outer callback schedules authored missile burst");actor.q.Update(2000);Require(actor.calls>0,"nested missiles survive retired AI");auto n=actor.calls;actor.q.Update(60000);Require(actor.calls==n,"nested burst ends once");queueTarget=nullptr;}
#endif
#endif
 std::cout<<"PASS selected actor queue after actual AI-frame destruction;original delay,payload,one-shot and cancellation\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
