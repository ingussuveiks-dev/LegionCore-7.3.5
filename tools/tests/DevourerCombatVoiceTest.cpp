// Actual old/current combat callbacks, current helpers and native EventMap.
// Engine scene/actor/cast services are controlled; native payload is separate.
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <tuple>
#include <vector>
using uint64=uint64_t;using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using Milliseconds=std::chrono::milliseconds;
enum{UNIT_STATE_CASTING=1,UNIT_STATE_STUNNED=2};
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
static uint32 now=0,warnings=0,attempts=0,refuse=0;
static std::function<void()> factoryHook;
static std::vector<std::tuple<uint32,uint32,uint32>> voices;
static uint32 const lifetimes[]={7800,9400,8200,9500,6700};
struct Creature{
 bool world=true,deleted=false,alive=true,combat=true,expired=false;uint32 entry=105660,map=1607,state=0;
 std::vector<std::pair<uint32,uint32>> actions;
 bool IsInWorld()const{return world;}bool IsPreDelete()const{return deleted;}bool IsAlive()const{return alive;}bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}
 uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return map;}bool HasUnitState(uint32 mask)const{return state&mask;}
};
using Unit=Creature;
struct ScriptedAI{
 Creature*me;explicit ScriptedAI(Creature*c):me(c){}virtual~ScriptedAI()=default;virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void UpdateAI(uint32){}
 bool UpdateVictim(){return me->combat&&me->alive;}void UpdateEntry(uint32){}void DoCast(uint32 id){me->actions.emplace_back(now,id);}void DoMeleeAttackIfReady(){}
};
enum class HighGuid{Conversation};struct Generator{uint32 Generate(){return 1;}};struct Manager{Generator g;template<HighGuid>Generator*GetGenerator(){return &g;}}manager;static auto sObjectMgr=&manager;
#define TC_LOG_WARN(...) (++warnings)
struct Conversation;static std::vector<Conversation*> owned;
struct Conversation{
 uint32 duration=0;bool CreateConversation(uint32,uint32 id,Unit*source,void*,Unit&anchor){assert(source==&anchor&&id>=1942&&id<=1946);++attempts;if(auto hook=factoryHook)hook();if(id==refuse)return false;duration=lifetimes[id-1942];voices.emplace_back(now,id,duration);owned.push_back(this);return true;}
 uint32 GetDuration()const{return duration;}
};
struct Before:ScriptedAI{using ScriptedAI::ScriptedAI;EventMap events;uint32 convers=1943;
#include "BeforeReset.inc"
#include "BeforeEnter.inc"
#include "BeforeUpdate.inc"
};
struct Current:ScriptedAI{using ScriptedAI::ScriptedAI;EventMap events;uint32 convers=1943;
#include "CombatVoice.inc"
#include "CurrentReset.inc"
#include "CurrentEnter.inc"
#include "CurrentUpdate.inc"
};
static void Clear(){for(auto c:owned)delete c;owned.clear();voices.clear();now=warnings=attempts=refuse=0;factoryHook={};}
static std::vector<uint32> Ids(){std::vector<uint32> out;for(auto [t,id,d]:voices)out.push_back(id);return out;}
static void NoOverlap(){for(size_t i=1;i<voices.size();++i)assert(std::get<0>(voices[i])>=std::get<0>(voices[i-1])+std::get<2>(voices[i-1]));}
template<class AI>static auto Run(uint32 failure,uint32 recovery,uint32 variant){
 Clear();Creature source;AI ai(&source);ai.EnterCombat(&source);refuse=failure;
 for(now=1;now<=200000;++now){if(now==recovery)refuse=0;source.state=variant==1&&now>=13000&&now<17000?UNIT_STATE_CASTING:variant==2&&now>=28000&&now<35000?UNIT_STATE_STUNNED:0;source.combat=!(variant==3&&now>=20000&&now<24000);ai.UpdateAI(1);}
 return source.actions;
}
int main(){uint32 cases=0;
 for(uint32 variant=0;variant<4;++variant){auto before=Run<Before>(0,0,variant);assert(Ids()==std::vector<uint32>({1942,1944,1945,1946}));auto after=Run<Current>(0,0,variant);assert(before==after&&Ids()==std::vector<uint32>({1942,1943,1944,1945,1946})&&!warnings);NoOverlap();++cases;}
 std::cout<<"COUNTERFACTUAL pinned48ec2ea80 skips1943; repaired four-line order leaves exact original206866/224990/209233 cast clocks unchanged in4 normal/cast/stun/victim differential contexts\n";
 for(uint32 id=1943;id<=1946;++id){auto before=Run<Before>(id,70000,0);auto after=Run<Current>(id,70000,0);assert(before==after&&Ids()==std::vector<uint32>({1942,1943,1944,1945,1946})&&warnings==1);NoOverlap();++cases;}
 for(uint32 mode=0;mode<9;++mode){Clear();Creature source;Current ai(&source);ai.combatVoicePending=true;switch(mode){case 0:source.world=false;break;case 1:source.deleted=true;break;case 2:source.alive=false;break;case 3:source.combat=false;break;case 4:source.expired=true;break;case 5:source.entry=105542;break;case 6:source.map=1620;break;case 7:ai.convers=1942;break;case 8:ai.convers=1947;break;}ai.UpdateCombatVoice(1000);assert(!attempts&&voices.empty());++cases;}
 {Clear();Creature source;Current ai(&source);refuse=1943;ai.RequestCombatVoice();assert(attempts==1&&ai.convers==1943&&warnings==1);ai.UpdateCombatVoice(999);assert(attempts==1);refuse=0;now=1000;ai.UpdateCombatVoice(1);assert(attempts==2&&ai.convers==1944&&!ai.combatVoicePending&&warnings==1);++cases;}
 for(bool fail:{false,true}){Clear();Creature source;Current ai(&source);refuse=fail?1943:0;factoryHook=[&]{ai.UpdateCombatVoice(1000);ai.RequestCombatVoice();};ai.RequestCombatVoice();assert(attempts==1&&ai.convers==(fail?1943u:1944u));++cases;}
 for(bool fail:{false,true}){Clear();Creature source;Current ai(&source);refuse=fail?1943:0;factoryHook=[&]{factoryHook={};ai.Reset();};ai.RequestCombatVoice();assert(ai.convers==1943&&!ai.combatVoicePending&&!ai.combatVoiceDelay&&!warnings);ai.RequestCombatVoice();assert(ai.convers==(fail?1943u:1944u));++cases;}
 {Clear();Creature source;Current ai(&source);refuse=1943;ai.RequestCombatVoice();ai.Reset();refuse=0;ai.UpdateCombatVoice(10000);assert(attempts==1&&!ai.combatVoicePending);ai.EnterCombat(&source);now=14000;ai.UpdateAI(14000);ai.UpdateAI(0);assert(Ids()==std::vector<uint32>({1942,1943}));++cases;}
 {Clear();Creature source;Current ai(&source);ai.RequestCombatVoice();auto delay=ai.combatVoiceDelay;ai.RequestCombatVoice();ai.UpdateCombatVoice(delay-1);assert(attempts==1);now=delay;ai.UpdateCombatVoice(1);assert(attempts==2&&Ids()==std::vector<uint32>({1943,1944}));++cases;}
 Clear();std::cout<<"PASS "<<cases<<" ASAN original callbacks/native EventMap differential/whole voice helper contexts; retry/no overlap/no replay, reset generation/reentry, source lifetime/admission, actual native payload and client separately tested\n";
}
