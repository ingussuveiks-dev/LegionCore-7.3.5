// Exact old/current combat tails and current voice helpers. Engine services
// controlled; native Conversation/Map/admission coverage is a separate test.
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <tuple>
#include <vector>
using uint32=uint32_t;
enum {EVENT_1=1,EVENT_2,EVENT_3,UNIT_STATE_CASTING=1,UNIT_STATE_STUNNED=2};
enum class HighGuid{Conversation};
static uint32 now=0,warnings=0;
static bool refuse=false;
static std::function<void()> factoryHook;
static std::vector<std::tuple<uint32,uint32,uint32>> voices;
static uint32 const lifetimes[]={7150,8400,9150,7850,7600,9200};
static uint32 const audio[]={1935,5132,5056,5229,2616,8328};
struct Creature {
 bool world=true,deleted=false,alive=true,combat=true,expired=false,spawn=true;uint32 entry=107831,map=1620,state=0;
 std::vector<std::pair<uint32,uint32>> actions;
 bool IsInWorld()const{return world;}bool IsPreDelete()const{return deleted;}bool IsAlive()const{return alive;}bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}
 uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return map;}bool HasUnitState(uint32 mask)const{return state&mask;}
 float GetPositionX()const{return 0;}float GetPositionY()const{return 0;}float GetPositionZ()const{return 0;}float GetOrientation()const{return 0;}
 Creature* SummonCreature(uint32 id,float,float,float){assert(id==108324);actions.emplace_back(now,id);return spawn?this:nullptr;}
 Creature* GetMotionMaster(){return this;}void MoveJump(float,float,float,float,float,float){}
 void CastSpell(Creature*,uint32 id){assert(id==158342);}
};
static int irand(int,int){return 0;}
struct EventMap {
 uint32 clock=0;std::map<uint32,uint32> due;
 void Reset(){due.clear();clock=0;}void RescheduleEvent(uint32 id,uint32 delay){due[id]=clock+delay;}
 void Update(uint32 diff){clock+=diff;}uint32 ExecuteEvent(){uint32 id=0,when=UINT32_MAX;for(auto value:due)if(value.second<=clock&&value.second<when){id=value.first;when=value.second;}if(id)due.erase(id);return id;}
};
struct SummonList{void DespawnAll(){}};
struct ScriptedAI {
 Creature* me;explicit ScriptedAI(Creature* c):me(c){}virtual void Reset(){}virtual void EnterCombat(Creature*){}virtual void UpdateAI(uint32){}
 bool UpdateVictim(){return me->alive&&me->combat;}void UpdateIntro(uint32){}
 void DoCast(uint32 id){me->actions.emplace_back(now,id);}void DoMeleeAttackIfReady(){}
};
using Unit=Creature;
struct Generator{uint32 Generate(){return 1;}};
struct Manager{template<HighGuid>Generator* GetGenerator(){static Generator g;return &g;}}manager;
static auto sObjectMgr=&manager;
#define TC_LOG_WARN(...) (++warnings)
struct Conversation;
static std::vector<Conversation*> owned;
struct Conversation {
 uint32 duration=0;
 bool CreateConversation(uint32,uint32 id,Creature* source,void*,Creature& anchor){assert(source==&anchor&&id>=2442&&id<2448);if(factoryHook)factoryHook();if(refuse)return false;duration=lifetimes[id-2442];voices.emplace_back(now,id,duration);owned.push_back(this);return true;}
 uint32 GetDuration()const{return duration;}
};
// Model the world's ownership of admitted conversations, retaining all
// successful allocations until the fake map is torn down.
struct Before:ScriptedAI{using ScriptedAI::ScriptedAI;EventMap events;SummonList summons;uint32 convid=0;
#include "BeforeReset.inc"
#include "BeforeEnter.inc"
#include "BeforeUpdate.inc"
};
struct Current:ScriptedAI{using ScriptedAI::ScriptedAI;EventMap events;SummonList summons;uint32 convid=0;
#include "CombatVoice.inc"
#include "CurrentReset.inc"
#include "CurrentEnter.inc"
#include "CurrentUpdate.inc"
};
static void Teardown(){for(auto p:owned)delete p;owned.clear();factoryHook={};refuse=false;voices.clear();warnings=0;now=0;}
int main(){
 // Baseline evidence, not an expected crashing ordinary run: all three
 // remaining lines start on the single six-add wave at 25 seconds.
 {Creature c;Before ai(&c);ai.EnterCombat(nullptr);for(now=1000;now<=25000;now+=1000)ai.UpdateAI(1000);
  uint32 waveLines=0;for(auto row:voices)if(std::get<0>(row)==25000)++waveLines;
  assert(waveLines==3&&ai.convid==2448);std::cout<<"COUNTERFACTUAL three distinct lines in one six-add wave tick\n";}
 Teardown();
 {Creature c;Before ai(&c);refuse=true;ai.EnterCombat(nullptr);for(now=1000;now<=30000;now+=1000)ai.UpdateAI(1000);
  assert(voices.empty()&&ai.convid==2448);std::cout<<"COUNTERFACTUAL refused factories consume all six original combat lines\n";}
 Teardown();uint32 cases=0;
 for(uint32 mode=0;mode<6;++mode){
  Creature source;Current ai(&source);ai.EnterCombat(nullptr);
  if(mode==1)source.spawn=false;
  if(mode==2)refuse=true;
  for(now=1000;now<=120000;now+=1000){
   if(mode==2&&now==20000)refuse=false;
   if(mode==3)source.state=now<30000?UNIT_STATE_CASTING:0;
   if(mode==4)source.state=now<30000?UNIT_STATE_STUNNED:0;
   if(mode==5&&now==30000){ai.Reset();ai.EnterCombat(nullptr);voices.clear();}
   ai.UpdateAI(1000);
  }
  assert(ai.convid==2448&&voices.size()==6);
  for(size_t i=0;i<voices.size();++i){assert(std::get<1>(voices[i])==2442+i);if(i)assert(std::get<0>(voices[i])>=std::get<0>(voices[i-1])+std::get<2>(voices[i-1]));assert(std::get<2>(voices[i])>=audio[i]);}
  auto actions=source.actions;Teardown();
  Creature before;Before old(&before);before.spawn=source.spawn;old.EnterCombat(nullptr);
  for(now=1000;now<=120000;now+=1000){if(mode==3)before.state=now<30000?UNIT_STATE_CASTING:0;if(mode==4)before.state=now<30000?UNIT_STATE_STUNNED:0;if(mode==5&&now==30000){old.Reset();old.EnterCombat(nullptr);}old.UpdateAI(1000);}
  assert(actions==before.actions);Teardown();++cases;
 }
 for(uint32 mode=0;mode<10;++mode){Creature c;Current ai(&c);ai.EnterCombat(nullptr);refuse=true;ai.RequestCombatVoice();assert(ai.convid==2442&&warnings==1);for(int i=0;i<5;++i)ai.UpdateCombatVoice(1000);assert(warnings==1);refuse=false;
  switch(mode){case 0:c.world=false;break;case 1:c.deleted=true;break;case 2:c.alive=false;break;case 3:c.combat=false;break;case 4:c.expired=true;break;case 5:c.entry=1;break;case 6:c.map=1;break;case 7:ai.Reset();break;case 8:factoryHook=[&]{ai.Reset();};break;case 9:factoryHook=[&]{ai.UpdateCombatVoice(1000);};break;}
  ai.UpdateCombatVoice(1000);if(mode<8)assert(voices.empty()&&ai.convid==2442);if(mode==8)assert(ai.convid==2442&&!ai.combatVoicePending);if(mode==9)assert(voices.size()==1&&ai.convid==2443);Teardown();++cases;
 }
 std::cout<<"PASS "<<cases<<" exact combat/helper ASAN contexts: one admitted line, published lifetime, retry/reset/reentrancy/source guards and unchanged spell/add timeline\n";
}
