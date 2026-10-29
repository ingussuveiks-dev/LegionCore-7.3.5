#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using uint32=uint32_t;
constexpr uint32 SMART_ACTION_PARAM_COUNT=6,SMART_ACTION_RANDOM_EMOTE=10,SMART_ACTION_RANDOM_PHASE=30;
static unsigned errors,debugs,draws,liveLists;
static uint32 choice,lastMax;
static std::vector<uint32> choices;
static void Require(bool ok){if(!ok)std::abort();}
uint32 urand(uint32 min,uint32 max){Require(min==0&&max<7);lastMax=max;auto result=choices.empty()?choice:choices.at(draws);++draws;Require(result<=max);return result;}
void LogError(char const*,char const*,...){++errors;}
void LogDebug(char const*,char const*,...){++debugs;} // Evaluate real log arguments too.
#define TC_LOG_ERROR(...) LogError(__VA_ARGS__)
#define TC_LOG_DEBUG(...) LogDebug(__VA_ARGS__)
struct Unit;
struct Object {uint32 guid=17;virtual ~Object()=default;virtual Unit* ToUnit(){return nullptr;}uint32 GetGUIDLow()const{return guid;}};
struct Unit:Object {std::vector<uint32> emotes;Unit* ToUnit()override{return this;}void HandleEmoteCommand(uint32 id){emotes.push_back(id);}};
bool IsUnit(Object* object){return object&&object->ToUnit();}
struct ObjectList:std::vector<Object*> {explicit ObjectList(std::vector<Object*> const& values):std::vector<Object*>(values){++liveLists;}~ObjectList(){--liveLists;}};
struct SmartScriptHolder {
 long entryOrGuid=123;uint32 type=10;
 union Action {struct {uint32 emote1,emote2,emote3,emote4,emote5,emote6;} randomEmote;
  struct {uint32 phase1,phase2,phase3,phase4,phase5,phase6;} randomPhase;uint32 raw[6];Action():raw{}{} }action;
 uint32 GetActionType()const{return type;}uint32 GetScriptType()const{return 0;}uint32 GetEventType()const{return 62;}
};
struct SmartScript {
 Object base;Object* baseObject=&base;std::vector<Object*> targetObjects;bool missingTargets=false;uint32 phase=19,phaseCalls=0;
 Object* GetBaseObject(){return baseObject;}ObjectList* GetTargets(SmartScriptHolder const&,Unit*){return missingTargets?nullptr:new ObjectList(targetObjects);}
 void SetPhase(uint32 value){phase=value;++phaseCalls;}
 void ProcessAction(SmartScriptHolder const& e){Unit* unit=nullptr;switch(e.GetActionType()){
#include "RandomEmote.inc"
#include "RandomPhase.inc"
 }}
};
static void Clear(){errors=debugs=draws=0;choice=lastMax=0;choices.clear();Require(!liveLists);}
int main(int argc,char** argv){
 if(argc>1){Clear();SmartScript s;Unit target;s.targetObjects={&target};SmartScriptHolder e;
  e.type=std::string(argv[1])=="emote"?10u:30u;for(unsigned i=0;i<6;++i)e.action.raw[i]=i+1;choice=6;
  s.ProcessAction(e);return 2; // ASan must stop the pinned old temp[6] read.
 }
 unsigned cases=0;
 for(uint32 action:{10u,30u})for(unsigned mask=1;mask<64;++mask){
  std::vector<uint32> expected;SmartScriptHolder e;e.type=action;
  for(unsigned i=0;i<6;++i)if(mask&(1u<<i)){e.action.raw[i]=i+1;expected.push_back(i+1);}
  for(unsigned i=0;i<expected.size();++i){Clear();SmartScript s;Unit first,second;Object nonunit;s.targetObjects={&first,&nonunit,&second};choice=i;
   s.ProcessAction(e);Require(!errors&&lastMax==expected.size()-1&&!liveLists);
   if(action==10){Require(draws==2&&!s.phaseCalls&&first.emotes==std::vector<uint32>{expected[i]}&&second.emotes==first.emotes&&debugs==2);}
   else Require(draws==1&&s.phaseCalls==1&&s.phase==expected[i]&&first.emotes.empty()&&second.emotes.empty()&&debugs==1);
   ++cases;
  }
 }
 for(uint32 action:{10u,30u}){Clear();SmartScript s;Unit target;s.targetObjects={&target};SmartScriptHolder e;e.type=action;s.ProcessAction(e);
  Require(errors==1&&!draws&&!s.phaseCalls&&s.phase==19&&target.emotes.empty()&&!liveLists);
 }
 // Independent per-unit draws and duplicate weighting are unchanged.
 Clear();SmartScript s;Unit first,second;Object nonunit;s.targetObjects={&first,&nonunit,&second};SmartScriptHolder e;e.action.raw[0]=4;e.action.raw[2]=4;e.action.raw[5]=9;choices={0,2};
 s.ProcessAction(e);Require(!errors&&draws==2&&lastMax==2&&first.emotes==std::vector<uint32>{4}&&second.emotes==std::vector<uint32>{9}&&!liveLists);
 Clear();s.targetObjects.clear();s.ProcessAction(e);Require(!draws&&!errors&&!liveLists);
 Clear();s.missingTargets=true;s.ProcessAction(e);Require(!draws&&!errors&&!liveLists);
 Clear();s.missingTargets=false;s.targetObjects={&first};s.baseObject=nullptr;s.ProcessAction(e);Require(draws==1&&!errors&&!liveLists); // Emote needs only an actual Unit target.
 Clear();e.type=30;s.ProcessAction(e);Require(!draws&&!errors&&!s.phaseCalls);
 Clear();s.baseObject=&s.base;e.action.raw[0]=1;e.action.raw[2]=1;e.action.raw[5]=2;for(uint32 i:{0u,1u,2u}){choice=i;s.ProcessAction(e);Require(lastMax==2&&s.phase==(i==2?2u:1u));}
 std::printf("PASS %u exhaustive native emote/phase mask-choice cases, sparse/max/single pools, empty visible rejection/no allocation leak, per-target independent draw, duplicate weighting, missing/nonunit/no-base targets and unchanged phase on refused action. Log arguments evaluated; ASan enabled.\n",cases);
}
