// Full production actor under ASAN. Factory/map/clock services are controlled.
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;using int32=int32_t;
enum{EVENT_1=1,EVENT_2=2,DAY=86400,UNIT_STATE_CASTING=1,UNIT_STATE_STUNNED=2};
struct EventMap{uint64_t now=0;std::map<uint32,uint64_t> due;void Reset(){due.clear();now=0;}void Update(uint32 ms){now+=ms;}void RescheduleEvent(uint32 id,uint32 ms){due[id]=now+ms;}uint32 ExecuteEvent(){for(auto p:due)if(p.second<=now){due.erase(p.first);return p.first;}return 0;}};
struct GameObject{};struct Creature;struct Motion{unsigned moves=0;void MovePoint(uint32,float,float,float){++moves;}};
struct Creature{Motion motion;bool visible=false,visualOK=true,artifactOK=true;unsigned state=0,visualAttempts=0,artifactAttempts=0,visuals=0,artifacts=0,removedAura=0;std::function<void()> visualHook,artifactHook;void SetVisible(bool v){visible=v;}void RemoveAura(uint32 id){assert(id==214940);++removedAura;}Motion* GetMotionMaster(){return &motion;}bool HasUnitState(unsigned flag){return state&flag;}Creature* SummonCreature(uint32 id,float,float,float,float){assert(id==108325);++visualAttempts;if(visualHook)visualHook();if(!visualOK)return nullptr;++visuals;return this;}GameObject* SummonGameObject(uint32 id,float,float,float,float,float,float,float,float,uint32 day){assert(id==251107&&day==DAY);++artifactAttempts;if(artifactHook)artifactHook();if(!artifactOK)return nullptr;++artifacts;static GameObject object;return &object;}};
struct CreatureAI{virtual ~CreatureAI()=default;virtual void Reset(){}virtual void DoAction(int32){}virtual void UpdateAI(uint32){}};
struct ScriptedAI:CreatureAI{Creature* me;explicit ScriptedAI(Creature* c):me(c){}void SetCanSeeEvenInPassiveMode(bool){}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual CreatureAI* GetAI(Creature*)const=0;};
enum class HighGuid{Conversation};struct Generator{uint32 Generate(){return 1;}};struct Mgr{template<HighGuid>Generator* GetGenerator(){static Generator value;return &value;}}manager;auto sObjectMgr=&manager;
static std::map<uint32,unsigned> voices;static std::vector<void*> accepted;static unsigned warnings;
struct Conversation{bool CreateConversation(uint32,uint32 id,Creature*,void*,Creature&){++voices[id];accepted.push_back(this);return true;}};
#define TC_LOG_WARN(...) (++warnings)
#include "GaronaReveal.inc"
int main(){unsigned cases=0;
 for(unsigned missing=0;missing<4;++missing)for(unsigned mode=0;mode<9;++mode){
  voices.clear();warnings=0;Creature actor;actor.visualOK=!(missing&1);actor.artifactOK=!(missing&2);
  npc_garona_halforcen_outro::npc_garona_halforcen_outroAI ai(&actor);ai.Reset();assert(actor.visible);ai.DoAction(1);
  if(mode==1){ai.DoAction(1);ai.DoAction(1);}if(mode==2)ai.Reset();
  assert(voices[2449]==1&&actor.removedAura==1&&actor.motion.moves==1);ai.UpdateAI(22999);assert(voices[2450]==0&&actor.artifactAttempts==0);ai.UpdateAI(1);assert(voices[2450]==1);
  if(mode==3)ai.Reset();if(mode==4)ai.DoAction(1);ai.UpdateAI(10999);assert(!actor.artifactAttempts);
  if(mode==5)actor.state=UNIT_STATE_CASTING;if(mode==6)actor.state=UNIT_STATE_STUNNED;
  if(mode==8)actor.visualHook=actor.artifactHook=[&]{ai.DoAction(1);ai.Reset();ai.UpdateAI(0);};
  ai.UpdateAI(1);if(mode==5||mode==6){assert(!actor.visualAttempts&&!actor.artifactAttempts);actor.state=0;ai.UpdateAI(1);}
  assert(actor.visuals==unsigned(!(missing&1))&&actor.artifacts==unsigned(!(missing&2)));unsigned firstWarnings=warnings;
  bool consumed=mode==7&&!(missing&2);if(consumed)actor.artifacts=0;
  if(missing){ai.Reset();ai.DoAction(1);ai.UpdateAI(1999);assert(actor.visualAttempts==1&&actor.artifactAttempts==1);ai.UpdateAI(1);assert(warnings==firstWarnings);actor.visualOK=actor.artifactOK=true;ai.UpdateAI(2000);}
  assert(actor.visuals==1&&actor.artifacts==unsigned(!consumed));assert(actor.motion.moves==2&&voices[2449]==1&&voices[2450]==1);
  unsigned attempts=actor.visualAttempts+actor.artifactAttempts;actor.artifacts=0;ai.DoAction(1);ai.Reset();for(unsigned i=0;i<60;++i)ai.UpdateAI(1000);
  assert(actor.visualAttempts+actor.artifactAttempts==attempts&&!actor.artifacts&&actor.visuals==1&&actor.motion.moves==2&&voices[2449]==1&&voices[2450]==1);
  for(auto object:accepted)delete static_cast<Conversation*>(object);accepted.clear();++cases;
 }
 std::cout<<"PASS "<<cases<<" whole reveal AI ASAN contexts: native delays modeled, both/each refused factory, bounded retry/warnings, independent accepted/consumed object, duplicate starts, Reset all phases, casting/stun, reentrant factory callback. No native/client claim\n";
}
