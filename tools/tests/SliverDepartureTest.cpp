// Native event admission, TALK, target SELF/VICTIM, TEXT_OVER timer and LINK.
// Chat/range search/cast/criteria/data/death endpoints are controlled.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <utility>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;using ReactStates=uint32;
#include "EventConstants.inc"
struct Unit;struct Creature;struct Player;struct GameObject;
struct FakeAI {
 unsigned attempts=0,started=0;uint32 path=0,mEscortQuestID=999,despawn=999;bool* combat=nullptr;
 std::vector<std::pair<uint32,uint32>> dataCalls;
 bool StartPath(bool run,uint32 entry,bool repeat,Unit* invoker){assert(run&&!repeat&&!invoker);++attempts;path=entry;if(combat&&*combat)return false;++started;return true;}
 void SetDespawnTime(uint32 value){despawn=value;}void SetData(uint32 field,uint32 value){dataCalls.push_back({field,value});}
};
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct Unit {bool player=false,creature=false;Creature* ToCreature();Player* ToPlayer();GameObject* ToGameObject();ObjectGuid GetGUID(){return {this};}};
struct Player:Unit {Player(){player=true;}};
struct Creature:Unit {uint32 entry=101539;bool combat=false,expired=false;float health=100;Unit* victim=nullptr;
 FakeAI ai;ReactStates reaction=9;FakeAI* AI(){return &ai;}void SetReactState(ReactStates value){reaction=value;}
 Creature(){creature=true;ai.combat=&combat;}uint32 GetEntry()const{return entry;}bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}
 uint32 GetMaxHealth()const{return 100;}float GetHealthPct()const{return health;}Unit* getVictim(){return victim;}
};
Creature* Unit::ToCreature(){return static_cast<Creature*>(this);}Player* Unit::ToPlayer(){return static_cast<Player*>(this);}
bool IsCreature(Unit* u){return u&&u->creature;}bool IsPlayer(Unit* u){return u&&u->player;}
bool IsGameObject(Unit*){return false;}
struct GameObject:Unit {FakeAI ai;FakeAI* AI(){return &ai;}};
GameObject* Unit::ToGameObject(){return static_cast<GameObject*>(this);}
using ObjectList=std::vector<Unit*>;struct SpellInfo{};
struct SmartScriptHolder {
 uint32 entryOrGuid=101539,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,targetEntry=0,parameter=0,second=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;
  struct {uint32 textGroupID=0,creatureEntry=0;} textOver;
  struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;} dataSet;
  struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;
 } event;
 struct {struct {uint32 textGroupID=0,duration=0;} talk;
  struct {uint32 field=0,data=0;} setData;
  struct {uint32 run=0,pathID=0,repeat=0,quest=0,despawnTime=0,reactState=0;} wpStart;
 } action;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}uint32 GetTargetType()const{return targetType;}
};
struct ConditionSourceInfo {ConditionSourceInfo(Unit*,Unit*){}};using ConditionList=std::vector<int>;
struct Conditions {bool allow=true;bool IsObjectMeetingSmartEventConditions(uint32,uint32,uint32,Unit*,Creature*)const{return allow;}
 ConditionList GetConditionsForSmartEvent(uint32,uint32,uint32)const{return {};}
 bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&)const{return allow;}
} conditions;auto sConditionMgr=&conditions;
struct Speech {uint32 speaker,group;};
struct Chat {std::vector<Speech> requests;void SendChat(Creature* talker,uint8 group,ObjectGuid){requests.push_back({talker->GetEntry(),group});}} chat;auto sCreatureTextMgr=&chat;
struct Call {uint32 time,action,parameter;bool operator==(Call const& b)const{return time==b.time&&action==b.action&&parameter==b.parameter;}};
bool roll_chance_i(uint32){return true;}
struct SmartScript {
 std::vector<SmartScriptHolder> mEvents;std::vector<Call> calls;Creature actor,minerva;Creature* me=&actor;Player nearby;bool minervaPresent=true;std::unique_ptr<ObjectList> escortTargets;
 ObjectGuid mLastInvoker,mTextGUID;unsigned errors=0;uint32 now=0,mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false,rangePlayer=true;
 Creature* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}
 bool IsSmart()const{return me!=nullptr;}void StoreTargetList(ObjectList* list,uint32 id){assert(id==SMART_ESCORT_TARGETS);escortTargets.reset(list);}
 bool IsInPhase(uint32)const{return true;}void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){auto l=new ObjectList();switch(e.GetTargetType()){
#include "TargetSelfVictim.inc"
  case 18:if(rangePlayer)l->push_back(&nearby);break;
  case 19:if(minervaPresent&&e.targetEntry==102561)l->push_back(&minerva);break;
 }return l;}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void Update(uint32 diff);
};
#define CAST_AI(type,ai) (ai)
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_DEBUG(...) ((void)0)
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LinkEvent.inc"
#include "NoParamEvents.inc"
#include "TextOverEvent.inc"
#include "DataSetEvent.inc"
#include "HealthEvent.inc"
 }
}
#include "ActionPrefix.inc"
 calls.push_back({now,e.actionType,e.parameter});
 switch(e.GetActionType()){
#include "TalkAction.inc"
 // Unchanged native services are observed as requests, not granted progression.
#include "SetDataAction.inc"
#include "WaypointAction.inc"
 case 11:case 205:case 37:case 42:break;
 }
#include "LinkTail.inc"
void SmartScript::Update(uint32 diff){now+=diff;
#include "TextTimer.inc"
static std::vector<SmartScriptHolder> Read(char const* path){std::ifstream f(path);std::string line;std::vector<SmartScriptHolder> rows;while(std::getline(f,line)){
 std::stringstream ss(line);std::vector<std::string> fields;std::string cell;while(std::getline(ss,cell,'\t'))fields.push_back(cell);assert(fields.size()==31);
 auto n=[&](size_t i){return uint32(std::stoul(fields[i]));};if(n(0)!=101539||n(1)!=0)continue;SmartScriptHolder e;e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
 e.event.textOver={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};
 e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.targetType=n(21);e.targetEntry=n(22);e.action.setData={n(15),n(16)};e.action.wpStart={n(15),n(16),n(17),n(18),n(19),n(20)};rows.push_back(e);
 }return rows;}
static void Departure(SmartScript& s,Unit* invoker,bool failFirst){
 s.ProcessEventsFor(SMART_EVENT_DATA_SET,invoker,9,9);assert(s.calls==std::vector<Call>({{0,1,12},{0,205,92}})&&s.mTextTimer==7000);
 s.Update(6999);s.Update(1);assert(s.calls.size()==2&&!s.actor.ai.attempts);
 s.actor.combat=failFirst;s.Update(1);
 assert(s.actor.ai.attempts==1&&s.actor.ai.path==10153907&&s.actor.reaction==0);
 assert(s.minerva.ai.dataCalls.size()==unsigned(s.minervaPresent));
 if(s.minervaPresent)assert(s.minerva.ai.dataCalls[0]==std::make_pair(3u,3u));
 if(failFirst){assert(!s.actor.ai.started&&!s.mEvents[34].runOnce);s.actor.combat=false;s.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,12,101539);assert(s.actor.ai.attempts==2);}
 assert(s.actor.ai.started==1&&s.actor.ai.mEscortQuestID==0&&s.actor.ai.despawn==0&&s.escortTargets&&s.escortTargets->size()==1);
 auto attempts=s.actor.ai.attempts;auto calls=s.calls.size();s.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,12,101539);
 assert(s.actor.ai.attempts==attempts&&s.calls.size()==calls&&s.minerva.ai.dataCalls.size()==unsigned(s.minervaPresent));
}
int main(int argc,char** argv){
 if(argc!=3)return 1;auto old=Read(argv[1]),fixed=Read(argv[2]);assert(old.size()==42&&fixed.size()==42&&fixed[34].event_id==35);
 Player player;Creature pet,guardian;
 for(Unit* invoker:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(nullptr)})for(bool failure:{false,true}){
  SmartScript before,after;before.mEvents=old;after.mEvents=fixed;Departure(before,invoker,failure);Departure(after,invoker,failure);
  assert(before.errors==1&&!after.errors&&before.calls==after.calls);
 }
 SmartScript naive;naive.mEvents=old;naive.mEvents[34].type=SMART_EVENT_LINK;naive.ProcessEventsFor(SMART_EVENT_DATA_SET,&player,9,9);naive.actor.combat=true;naive.Update(7001);
 assert(naive.actor.ai.attempts==1&&!naive.actor.ai.started&&!naive.errors);naive.actor.combat=false;naive.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,12,101539);
 assert(naive.actor.ai.attempts==1&&!naive.actor.ai.started); // Parent34 consumed once; linked copy35 cannot retry.
 SmartScript absent;absent.mEvents=fixed;absent.minervaPresent=false;Departure(absent,&player,false);assert(!absent.errors);
 for(unsigned mode=0;mode<4;++mode){SmartScript s;s.mEvents=fixed;conditions.allow=mode!=0;s.actor.expired=mode==1;
  if(mode<2)s.ProcessEventsFor(SMART_EVENT_DATA_SET,&player,9,9);else s.ProcessEventsFor(SMART_EVENT_TEXT_OVER,nullptr,mode==2?99:12,mode==3?999:101539);
  assert(s.calls.empty()&&!s.errors&&!s.actor.ai.attempts);
 }conditions.allow=true;
 SmartScript removed;removed.mEvents=fixed;removed.ProcessEventsFor(SMART_EVENT_DATA_SET,&player,9,9);removed.me=nullptr;removed.Update(8000);assert(!removed.actor.ai.attempts&&removed.calls.size()==2);
 std::puts("PASS native Sliver event guards/TALK7000ms/TEXT_OVER/data3+3/WP_START/LINK: old malformed-call diagnostic counterfactual, repaired identical actual departure requests and independent failed-path retry; player/pet/guardian/null, repeats/missing Minerva/foreign/condition/expiry/removed actor; naiveLINK conversion loses retry. Data/path/search endpoints controlled; client route/Minerva traversal separate.");
}
