// Native event admission, TALK, target SELF/VICTIM, TEXT_OVER timer and LINK.
// Chat/range search/cast/criteria/data/death endpoints are controlled.
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;
#include "EventConstants.inc"
struct Unit;struct Creature;struct Player;
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid const Empty;uint32 GetEntry()const;};ObjectGuid const ObjectGuid::Empty{};
struct Unit {bool player=false,creature=false;Creature* ToCreature();Player* ToPlayer();ObjectGuid GetGUID(){return {this};}};
struct Player:Unit {Player(){player=true;}};
struct Creature:Unit {uint32 entry=99615;bool combat=true,expired=false,world=true,casting=false;float health=100;Unit* victim=nullptr;
 Creature(){creature=true;}uint32 GetEntry()const{return entry;}bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}bool IsInWorld()const{return world;}
 bool HasUnitState(uint32)const{return casting;}uint32 GetMaxHealth()const{return 100;}float GetHealthPct()const{return health;}Unit* getVictim(){return victim;}
};
uint32 ObjectGuid::GetEntry()const{return unit&&unit->creature?unit->ToCreature()->entry:0;}
Creature* Unit::ToCreature(){return static_cast<Creature*>(this);}Player* Unit::ToPlayer(){return static_cast<Player*>(this);}
bool IsCreature(Unit* u){return u&&u->creature;}bool IsPlayer(Unit* u){return u&&u->player;}
using ObjectList=std::vector<Unit*>;struct SpellInfo{};struct GameObject{};
struct SmartScriptHolder {
 uint32 entryOrGuid=99615,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,parameter=0,second=0,timer=0;bool active=true,runOnce=false,enableTimed=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;
  struct {uint32 textGroupID=0,creatureEntry=0;} textOver;struct{uint32 pointID=0,pathID=0;}waypoint;
  struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;} dataSet;
  struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;
 } event;
 struct {struct {uint32 textGroupID=0,duration=0;} talk;struct{uint32 spell=0,flags=0;}cast;} action;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}uint32 GetTargetType()const{return targetType;}
};
struct ConditionSourceInfo {ConditionSourceInfo(Unit*,Unit*){}};using ConditionList=std::vector<int>;
struct Conditions {bool allow=true;bool IsObjectMeetingSmartEventConditions(uint32,uint32,uint32,Unit*,Creature*)const{return allow;}
 ConditionList GetConditionsForSmartEvent(uint32,uint32,uint32)const{return {};}
 bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&)const{return allow;}
} conditions;auto sConditionMgr=&conditions;
struct Speech {uint32 speaker,group;};
using CreatureTextHolder=std::map<uint8,uint32>;
#define TC_LOG_DEBUG(...) ((void)0)
struct Chat {std::vector<Speech> requests;std::map<uint32,CreatureTextHolder> groups;std::vector<CreatureTextHolder*> mTextList;
 void Prepare(){mTextList.assign(110557,nullptr);for(auto& item:groups)mTextList[item.first]=&item.second;}
 uint32 SendChat(Creature* source,uint8 textGroup,ObjectGuid){
#include "ChatAdmission.inc"
 requests.push_back({source->GetEntry(),textGroup});return 1;}
} chat;auto sCreatureTextMgr=&chat;
struct Call {uint32 time,action,parameter;bool operator==(Call const& b)const{return time==b.time&&action==b.action&&parameter==b.parameter;}};
bool roll_chance_i(uint32){return true;}
struct SmartScript {
 SmartScript()=default;SmartScript(std::vector<SmartScriptHolder> rows,uint32 entry,Unit* victim):mEvents(std::move(rows)){actor.entry=entry;actor.victim=victim;}
 std::vector<SmartScriptHolder> mEvents,mTimedActionList;std::vector<Call> calls;Creature actor;Creature* me=&actor;Player nearby;
 ObjectGuid mLastInvoker,mTextGUID;unsigned errors=0;uint32 now=0,mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false,rangePlayer=true;
 Creature* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}
 bool IsInPhase(uint32)const{return true;}void InitTimer(SmartScriptHolder&);void RecalcTimer(SmartScriptHolder&,uint32,uint32);void UpdateTimer(SmartScriptHolder&,uint32);uint32 GetPathId()const{return actor.entry;}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit* unit){Unit* trigger=unit?unit:GetLastInvoker();auto l=new ObjectList();switch(e.GetTargetType()){
#include "TargetSelfVictim.inc"
#include "TargetInvoker.inc"
  case 21:case 18:if(rangePlayer)l->push_back(&nearby);break;
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
#include "TimerEvent.inc"
#include "WaypointEvent.inc"
 }
}
#include "ActionPrefix.inc"
 calls.push_back({now,e.actionType,e.parameter});
 switch(e.GetActionType()){
#include "TalkAction.inc"
 // Unchanged native services are observed as requests, not granted progression.
 case 2:case 11:case 205:case 45:case 37:case 42:case 75:case 12:case 22:case 50:case 80:break;
 }
#include "LinkTail.inc"
void SmartScript::Update(uint32 diff){now+=diff;
#include "TextTimer.inc"
uint32 urand(uint32 low,uint32 high){assert(low<=high);return high;}
#include "Timers.inc"
static std::vector<SmartScriptHolder> Read(char const* path,uint32 entry){std::ifstream f(path);std::string line;std::vector<SmartScriptHolder> rows;while(std::getline(f,line)){
 std::stringstream ss(line);std::vector<std::string> fields;std::string cell;while(std::getline(ss,cell,'\t'))fields.push_back(cell);assert(fields.size()==31);
 auto n=[&](size_t i){return uint32(std::stoul(fields[i]));};if(n(0)!=entry||n(1)!=0)continue;SmartScriptHolder e;e.entryOrGuid=n(0);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
 e.event.textOver={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};
 e.action.cast={n(15),n(16)};e.event.waypoint={n(9),n(10)};e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.targetType=n(21);rows.push_back(e);
 }return rows;}
static void Texts(char const* path){std::ifstream f(path);std::string line;while(std::getline(f,line)){std::stringstream ss(line);std::string entry,group;std::getline(ss,entry,'\t');std::getline(ss,group,'\t');chat.groups[uint32(std::stoul(entry))][uint8(std::stoul(group))]=1;}chat.Prepare();}
static SmartScript Script(std::vector<SmartScriptHolder> const& rows,uint32 entry,Unit* victim){return SmartScript(rows,entry,victim);}


int main(int argc,char**argv){_set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);Texts(argv[3]);auto old=Read(argv[1],104520),fixed=Read(argv[2],104520);assert(old.size()==10&&fixed.size()==10);Player player;Creature pet,guardian,npc;uint32 cases=0;
 for(Unit*invoker:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)})for(bool repaired:{false,true})for(bool oneHit:{false,true}){
  auto s=Script(repaired?fixed:old,104520,invoker);if(!oneHit){s.ProcessEventsFor(SMART_EVENT_AGGRO,invoker);assert(s.calls.size()==1&&s.calls[0].parameter==209822);s.actor.health=50;s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,invoker);assert(s.calls.size()==3&&s.calls[1].parameter==209745&&s.calls[2].parameter==207074);s.actor.health=25;s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,invoker);assert(s.calls.size()==4&&s.calls[3].parameter==207074);s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,invoker);assert(s.calls.size()==4);}
  auto start=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DEATH,invoker);assert(s.calls.size()==start+5&&s.errors==(repaired?0u:1u));uint32 actions[]={205,45,12,12,12},values[]={92,4,106710,106710,106711};for(unsigned i=0;i<5;++i)assert(s.calls[start+i].action==actions[i]&&s.calls[start+i].parameter==values[i]);s.ProcessEventsFor(SMART_EVENT_DEATH,invoker);assert(s.calls.size()==start+5&&s.errors==(repaired?0u:1u));++cases;
 }
 for(bool repaired:{false,true})for(unsigned mode=0;mode<2;++mode){auto denied=Script(repaired?fixed:old,104520,&player);conditions.allow=mode!=0;denied.actor.expired=mode==1;denied.ProcessEventsFor(SMART_EVENT_DEATH,&player);assert(denied.calls.empty()&&!denied.errors);conditions.allow=true;++cases;}
 for(Unit*invoker:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)}){
  auto channeler=Script(Read(argv[2],106711),106711,invoker);channeler.ProcessEventsFor(SMART_EVENT_DEATH,invoker);assert(channeler.calls.size()==2&&channeler.calls[0].action==45&&channeler.calls[0].parameter==2&&channeler.calls[1].parameter==5&&!channeler.errors);channeler.ProcessEventsFor(SMART_EVENT_DEATH,invoker);assert(channeler.calls.size()==4); // native channeler event flags0; real death admission belongs to engine
  auto receiver=Script(Read(argv[2],104902),104902,invoker);receiver.ProcessEventsFor(SMART_EVENT_DATA_SET,invoker,4,4);assert(receiver.calls.size()==2&&receiver.calls[0].action==206&&receiver.calls[0].parameter==1688&&receiver.calls[1].action==29);receiver.ProcessEventsFor(SMART_EVENT_DATA_SET,invoker,5,5);assert(receiver.calls.size()==3&&receiver.calls.back().action==53&&receiver.calls.back().parameter==1&&!receiver.errors);++cases;
 }
 std::printf("PASS %u native event/health/once/LINK/data cases: unchanged aggro and50/25 casts, five real death requests49808/Azuregos4/two106710/one106711 once for player/pet/guardian/NPC/null and one-hit; no10 diagnostic in successor. Native channeler death2/5 and Azuregos4/conversation/follow then5/route request preserved; conditional/expired rejection. Cast/spawn/target/criterion/movement services controlled; actual group owner and shared portal/native encounter checks separate.\n",cases);}
