// Native event admission, TALK, target SELF/VICTIM, TEXT_OVER timer and LINK.
// Chat/range search/cast/criteria/data/death endpoints are controlled.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;
#include "EventConstants.inc"
struct Unit;struct Creature;struct Player;
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct Unit {bool player=false,creature=false;Creature* ToCreature();Player* ToPlayer();ObjectGuid GetGUID(){return {this};}};
struct Player:Unit {Player(){player=true;}};
struct Creature:Unit {uint32 entry=102532;bool combat=true,expired=false;float health=100;Unit* victim=nullptr;
 Creature(){creature=true;}uint32 GetEntry()const{return entry;}bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}
 uint32 GetMaxHealth()const{return 100;}float GetHealthPct()const{return health;}Unit* getVictim(){return victim;}
};
Creature* Unit::ToCreature(){return static_cast<Creature*>(this);}Player* Unit::ToPlayer(){return static_cast<Player*>(this);}
bool IsCreature(Unit* u){return u&&u->creature;}bool IsPlayer(Unit* u){return u&&u->player;}
using ObjectList=std::vector<Unit*>;struct SpellInfo{};struct GameObject{};
struct SmartScriptHolder {
 uint32 entryOrGuid=102532,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,parameter=0,second=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;
  struct {uint32 textGroupID=0,creatureEntry=0;} textOver;
  struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;} dataSet;
  struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;
 } event;
 struct {struct {uint32 textGroupID=0,duration=0;} talk;} action;
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
 std::vector<SmartScriptHolder> mEvents;std::vector<Call> calls;Creature actor;Creature* me=&actor;Player nearby;
 ObjectGuid mLastInvoker,mTextGUID;unsigned errors=0;uint32 now=0,mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false,rangePlayer=true;
 Creature* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}
 bool IsInPhase(uint32)const{return true;}void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){auto l=new ObjectList();switch(e.GetTargetType()){
#include "TargetSelfVictim.inc"
  case 18:if(rangePlayer)l->push_back(&nearby);break;
 }return l;}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void Update(uint32 diff);
};
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
 case 11:case 205:case 45:case 37:case 42:break;
 }
#include "LinkTail.inc"
void SmartScript::Update(uint32 diff){now+=diff;
#include "TextTimer.inc"
static std::vector<SmartScriptHolder> Read(char const* path){std::ifstream f(path);std::string line;std::vector<SmartScriptHolder> rows;while(std::getline(f,line)){
 std::stringstream ss(line);std::vector<std::string> fields;std::string cell;while(std::getline(ss,cell,'\t'))fields.push_back(cell);assert(fields.size()==31);
 auto n=[&](size_t i){return uint32(std::stoul(fields[i]));};SmartScriptHolder e;e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
 e.event.textOver={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};
 e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.targetType=n(21);rows.push_back(e);
 }return rows;}
static unsigned BadSpeakers(){unsigned n=0;for(auto const& s:chat.requests)if(s.speaker!=102532)++n;return n;}
static void Intro(SmartScript& s,Unit* invoker){
 s.ProcessEventsFor(SMART_EVENT_AGGRO,invoker);assert(s.calls==std::vector<Call>({{0,1,0},{0,205,92}}));
 assert(chat.requests.size()==1&&chat.requests[0].speaker==102532&&s.mTextTimer==5000);
 s.Update(4999);s.Update(1);assert(s.calls.size()==2);s.Update(1);
 assert(s.calls==std::vector<Call>({{0,1,0},{0,205,92},{5001,1,1}})&&s.mTextTimer==7000);
 s.ProcessEventsFor(SMART_EVENT_AGGRO,invoker);assert(s.calls.size()==3); // Native once admission.
}
int main(int argc,char** argv){
 if(argc!=3)return 1;auto old=Read(argv[1]),fixed=Read(argv[2]);assert(old.size()==18&&fixed.size()==18);
 Player player;Creature pet,guardian,other;pet.entry=9001;guardian.entry=9002;other.entry=9003;
 for(Unit* victim:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&other),static_cast<Unit*>(nullptr)}){
  chat.requests.clear();SmartScript before;before.mEvents=old;before.actor.victim=victim;Intro(before,victim);
  assert(before.errors==1);if(IsCreature(victim))assert(BadSpeakers()==1);else assert(!BadSpeakers());
  chat.requests.clear();SmartScript after;after.mEvents=fixed;after.actor.victim=victim;Intro(after,victim);assert(!after.errors&&!BadSpeakers()&&before.calls==after.calls);
  if(IsCreature(victim)){
   before.actor.health=90;before.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);before.Update(5001);
   assert(before.calls.back().parameter==2); // Pet talker entry prevents native TEXT_OVER2 -> line3.
   chat.requests.resize(2); // Keep only the repaired Intro's two observed chat requests.
  }
  after.actor.health=90;after.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);assert(after.calls.back().parameter==2&&after.mTextTimer==5000);
  after.Update(5001);assert(after.calls.back().action==1&&after.calls.back().parameter==3&&after.mTextTimer==7000);
  after.actor.health=40;after.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);assert(after.calls.back().parameter==4);
  after.actor.health=1;after.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);assert(after.calls.back().parameter==5);
  after.ProcessEventsFor(SMART_EVENT_DATA_SET,victim,1,1);assert(after.calls.back().parameter==6);
  after.Update(5001);assert(after.calls.back().action==37&&!after.errors&&!BadSpeakers());
  assert(chat.requests.size()==7);for(size_t i=0;i<7;++i)assert(chat.requests[i].group==i);
  auto count=after.calls.size();after.ProcessEventsFor(SMART_EVENT_TEXT_OVER,victim,6,102532);assert(after.calls.size()==count);
 }
 // The common naive conversion erases the original first speech's pause.
 chat.requests.clear();SmartScript naive;naive.mEvents=old;naive.actor.victim=&player;naive.mEvents[1].type=SMART_EVENT_LINK;
 naive.ProcessEventsFor(SMART_EVENT_AGGRO,&player);assert(naive.calls.size()==3&&naive.calls.back().time==0&&naive.mTextTimer==7000);
 for(unsigned mode=0;mode<5;++mode){chat.requests.clear();SmartScript s;s.mEvents=fixed;conditions.allow=mode!=0;s.actor.expired=mode==1;
  if(mode<2)s.ProcessEventsFor(SMART_EVENT_AGGRO,&player);
  else s.ProcessEventsFor(SMART_EVENT_TEXT_OVER,&player,mode==2?99:0,mode==3?9001:0);
  assert(s.calls.empty()&&!s.errors);
 }conditions.allow=true;
 chat.requests.clear();SmartScript removed;removed.mEvents=fixed;removed.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);removed.me=nullptr;removed.Update(60000);assert(removed.calls.size()==2);
 chat.requests.clear();SmartScript oneshot;oneshot.mEvents=fixed;oneshot.actor.victim=&pet;oneshot.actor.health=1;oneshot.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,&pet);assert(!oneshot.errors&&!BadSpeakers());
 std::puts("PASS native Ariden TALK/SELF/VICTIM/event guards/LINK/TEXT_OVER timer: old invalid immediate call and pet-as-speaker counterfactual, repaired real5000ms first pause/seven boss text groups, all five changed speeches for player/pet/guardian/creature/no victim; repeats/health1/null/foreign/condition/expiry/actor removal and retained finale request order. Chat/cast/progression endpoints controlled; no full fight/client certification.");
}
