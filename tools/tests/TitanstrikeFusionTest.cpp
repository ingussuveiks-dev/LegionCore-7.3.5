#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;
enum {QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1};
struct ObjectGuid {uint32 value=0;void Clear(){value=0;}bool operator==(ObjectGuid r)const{return value==r.value;}};
struct Creature;struct GameObject;struct TempSummon;struct SmartAI;
struct WorldObject {uint32 entry=0,map=1579;bool world=true,alive=true,visible=true;
 virtual ~WorldObject()=default;virtual GameObject* ToGameObject(){return nullptr;}
 uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return map;}bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}
 bool IsWithinDistInMap(Creature*,float)const;float GetDistance(Creature*)const{return 1;}
 Creature* FindNearestCreature(uint32,float,bool=true);};
struct Unit:WorldObject {virtual Creature* ToCreature(){return nullptr;}};
struct Player:Unit {int status=QUEST_STATUS_INCOMPLETE;int GetQuestStatus(uint32 q){assert(q==42158);return status;}};
struct Creature:Unit {ObjectGuid guid;SmartAI* ai=nullptr;uint32 removed=0;
 Creature* ToCreature()override{return this;}virtual TempSummon* ToTempSummon(){return nullptr;}
 ObjectGuid GetGUID()const{return guid;}SmartAI* AI()const{return ai;}
 void SetVisible(bool v){visible=v;}void DespawnOrUnsummon(){world=false;++removed;}};
struct TempSummon:Creature {Creature* owner=nullptr;TempSummon* ToTempSummon()override{return this;}
 ObjectGuid GetSummonerGUID()const{return owner?owner->guid:ObjectGuid{};}Unit* GetSummoner(){return owner;}};
struct GameObject:WorldObject {GameObject* ToGameObject()override{return this;}};
std::map<uint32,Creature*> actors;
bool WorldObject::IsWithinDistInMap(Creature* c,float)const{return c&&c->world&&c->map==map;}
struct NearestCreatureEntryWithLiveStateInObjectRangeCheck {
 WorldObject const& i_obj;uint32 i_entry;bool i_alive;float i_range;
 bool operator()(Creature* u);
};
#include "Nearest.inc"
Creature* WorldObject::FindNearestCreature(uint32 e,float r,bool alive) {
 NearestCreatureEntryWithLiveStateInObjectRangeCheck check{*this,e,alive,r};Creature* result=nullptr;
 for(auto& p:actors)if(check(p.second))result=p.second;return result;
}
namespace ObjectAccessor {Creature* GetCreature(Creature& source,ObjectGuid g){auto p=actors.find(g.value);return p!=actors.end()&&p->second->map==source.map&&p->second->world?p->second:nullptr;}}
struct SmartAI {Creature* me;uint32 mDespawnState=0,mDespawnTime=0,data=0,gossip=0,summons=0,reset=0,guidCalls=0;
 explicit SmartAI(Creature* c):me(c){c->ai=this;}virtual ~SmartAI()=default;
 virtual void Reset(){++reset;}virtual void sGossipSelect(Player*,uint32,uint32){++gossip;}
 virtual void JustSummoned(Creature*){++summons;}virtual void SetGUID(ObjectGuid const&,int32){++guidCalls;}
 virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32 id,uint32 v){
  ++data;if((me->entry==106613||me->entry==106925)&&id==1&&v==1){SetDespawnTime(1);StartDespawn();}}
 void SetDespawnTime(uint32 t);void StartDespawn();void UpdateDespawn(uint32 diff);
};
#include "Despawn.inc"
struct Condition{};struct ConditionSourceInfo{WorldObject* mConditionTargets[2];};
struct ConditionScript {explicit ConditionScript(char const*){}virtual bool OnConditionCheck(Condition*,ConditionSourceInfo&)=0;};
#include "Fusion.inc"
#include "Ready.inc"
void advance(SmartAI& ai,uint32 milliseconds){for(uint32 tick=0;tick<milliseconds;tick+=100)if(ai.me->world)ai.UpdateDespawn(100);}
void baseline(uint32 gap) {
 Creature owner;owner.entry=106558;owner.guid={1};SmartAI mim(&owner);
 TempSummon a,b;a.entry=106613;a.guid={2};a.owner=&owner;b.entry=106925;b.guid={3};b.owner=&owner;SmartAI aa(&a),ba(&b);actors={{1,&owner},{2,&a},{3,&b}};
 auto panel=[&](SmartAI& core,uint32 other){core.SetData(1,1);if(!owner.FindNearestCreature(other,50))mim.SetData(1,1);};
 panel(aa,106925);advance(aa,gap);panel(ba,106613);assert(mim.data==(gap>=6000?1u:0u));
 advance(aa,6000);advance(ba,6000);assert(!a.world&&!b.world&&a.removed==1&&b.removed==1);
 // Both source70 panel events are consumed. Removing the cores later emits
 // no new DATA1/1 to the original Mimiron receiver.
 assert(mim.data==(gap>=6000?1u:0u));
}
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);
 for(uint32 gap:{0u,100u,1000u,5000u,6000u})baseline(gap);
 uint32 cases=0;
 for(bool reverse:{false,true})for(uint32 gap:{0u,100u,1000u,6000u}) {
  Creature owner;owner.entry=106558;owner.guid={1};TitanstrikeFusion::MimironAI mim(&owner);Player p;
  TempSummon a,b;a.entry=106613;a.guid={2};a.owner=&owner;b.entry=106925;b.guid={3};b.owner=&owner;
  TitanstrikeFusion::CoreAI aa(&a),ba(&b);actors={{1,&owner},{2,&a},{3,&b}};
  GameObject panel;panel.entry=249765;ConditionSourceInfo info{{&p,&panel}};condition_titanstrike_fusion_ready ready;
  auto check=[&]{return ready.OnConditionCheck(nullptr,info);};
  mim.SetData(1,1);assert(mim.data==0&&!check());mim.sGossipSelect(&p,19589,0);
  mim.JustSummoned(&a);mim.JustSummoned(&b);assert(mim.summons==2&&!check());
  SmartAI& first=reverse?static_cast<SmartAI&>(ba):static_cast<SmartAI&>(aa);
  SmartAI& second=reverse?static_cast<SmartAI&>(aa):static_cast<SmartAI&>(ba);
  first.SetData(1,1);assert(!check());advance(first,gap);second.SetData(1,1);assert(check());
  mim.SetData(1,1);assert(mim.data==1&&!check());mim.SetData(1,1);assert(mim.data==1);
  advance(aa,6000);advance(ba,6000);assert(!a.world&&!b.world&&a.removed==1&&b.removed==1);
  mim.sGossipSelect(&p,19589,0);assert(!check());mim.SetGUID(a.guid,TitanstrikeFusion::Activated);assert(!check());
  // One missing/failed summon and old GUIDs must never become two receipts.
  a.world=true;aa.activated=false;mim.JustSummoned(&a);aa.SetData(1,1);assert(!check());
  b.world=true;ba.activated=false;Creature foreign;foreign.guid={9};foreign.entry=106558;b.owner=&foreign;
  mim.JustSummoned(&b);ba.SetData(1,1);assert(!check());b.owner=&owner;
  // Complete only with a newly bound native child for this experiment.
  mim.JustSummoned(&b);ba.SetData(1,1);assert(check());
  panel.entry=1;assert(!check());panel.entry=249773;assert(check());panel.map=1;assert(!check());panel.map=1579;
  owner.alive=false;assert(!check());owner.alive=true;mim.Reset();assert(!check()&&mim.reset==1);
  p.status=QUEST_STATUS_COMPLETE;mim.sGossipSelect(&p,19589,0);mim.JustSummoned(&a);mim.JustSummoned(&b);
  aa.SetData(1,1);ba.SetData(1,1);assert(!check());++cases;
 }
 Creature unrelated;unrelated.entry=1;TitanstrikeFusion::MimironAI unrelatedAI(&unrelated);
 unrelatedAI.SetData(1,1);assert(unrelatedAI.data==1); // shared script fallback
 std::cout<<"PASS 4 expected-broken rapid-panel cases, 1 slow baseline control and "<<cases<<" repaired actual receiver/core/condition sequences; both orders, native removal timing, duplicate/missing/foreign/stale children, reset/retry, shared-script fallback and admission. Cast/route/world services controlled; no whole client claim\n";
}
