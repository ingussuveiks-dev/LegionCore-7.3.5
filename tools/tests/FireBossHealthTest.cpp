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

static void Init(SmartScript& s){for(auto& e:s.mEvents)s.InitTimer(e);}
static void TickPhoenix(SmartScript& s,uint32 diff){for(auto& e:s.mEvents)if(e.event_id==15||e.event_id==16)s.UpdateTimer(e,diff);}
static unsigned Phoenix(SmartScript const& s){return unsigned(std::count_if(s.calls.begin(),s.calls.end(),[](Call const& c){return c.action==11&&c.parameter==196327;}));}
static void Thresholds(std::vector<SmartScriptHolder> const& rows,Unit* cause){
 auto s=Script(rows,99615,cause);Init(s);TickPhoenix(s,100);assert(Phoenix(s)==0);
 for(float pct:{100.f,99.f,81.f}){s.actor.health=pct;TickPhoenix(s,100);assert(Phoenix(s)==0);}
 s.actor.health=80;s.actor.casting=true;TickPhoenix(s,100);assert(Phoenix(s)==0);s.actor.casting=false;TickPhoenix(s,2);assert(Phoenix(s)==1);
 for(float pct:{80.f,60.f,21.f}){s.actor.health=pct;TickPhoenix(s,100);assert(Phoenix(s)==1);}
 s.actor.health=20;TickPhoenix(s,100);assert(Phoenix(s)==2);TickPhoenix(s,100);assert(Phoenix(s)==2&&!s.errors);
 auto skipped=Script(rows,99615,cause);Init(skipped);skipped.actor.health=10;TickPhoenix(skipped,100);assert(Phoenix(skipped)==2);TickPhoenix(skipped,100);assert(Phoenix(skipped)==2); // both actual thresholds crossed in one hit
 auto inactive=Script(rows,99615,cause);Init(inactive);inactive.actor.health=10;inactive.actor.combat=false;TickPhoenix(inactive,100);assert(Phoenix(inactive)==0);inactive.actor.combat=true;TickPhoenix(inactive,100);assert(Phoenix(inactive)==2);
 auto retry=Script(rows,99615,cause);Init(retry);retry.actor.health=80;TickPhoenix(retry,100);assert(Phoenix(retry)==1); // actual new instance/reset, not erasing live once flags
}
static void Sequence(std::vector<SmartScriptHolder> const& rows,Unit* cause){
 auto s=Script(rows,99615,cause);s.ProcessEventsFor(SMART_EVENT_AGGRO,cause);assert(s.calls.size()==1&&s.calls.back().parameter==4);s.Update(6000);assert(s.calls.size()==1);s.Update(1);assert(s.calls.size()==2&&s.calls.back().parameter==5);
 s.actor.health=80;s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,cause);assert(Phoenix(s)==1);
 auto start=s.calls.size();s.actor.health=50;s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,cause);
 assert(s.calls.size()==start+5&&s.errors==1);uint32 halfActions[]={75,11,11,1,45},halfParams[]={61204,196262,195739,6,1};for(unsigned i=0;i<5;++i)assert(s.calls[start+i].action==halfActions[i]&&s.calls[start+i].parameter==halfParams[i]);
 s.Update(6999);assert(s.calls.size()==start+5);s.Update(1);assert(s.calls.size()==start+5);s.Update(1);assert(s.calls.size()==start+6&&s.calls.back().action==12&&s.calls.back().parameter==99763);
 start=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DATA_SET,cause,4,4);assert(s.calls.size()==start+3&&s.calls[start].parameter==7&&s.calls[start+1].action==28&&s.calls[start+2].action==45&&s.calls[start+2].parameter==2);
 s.actor.health=20;s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,cause);assert(Phoenix(s)==2);
 start=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DEATH,cause);assert(s.calls.size()==start+5&&s.errors==2);uint32 deathActions[]={1,12,45,50,205},deathParams[]={8,103696,3,247494,92};for(unsigned i=0;i<5;++i)assert(s.calls[start+i].action==deathActions[i]&&s.calls[start+i].parameter==deathParams[i]);
 s.ProcessEventsFor(SMART_EVENT_DEATH,cause);assert(s.calls.size()==start+5&&s.errors==2);
 auto one=Script(rows,99615,cause);one.ProcessEventsFor(SMART_EVENT_DEATH,cause);assert(one.calls.size()==5&&one.errors==1&&Phoenix(one)==0);assert(std::none_of(one.calls.begin(),one.calls.end(),[](Call const& c){return c.action==12&&c.parameter==99763;}));
 // These two original missing links are detected, not repaired or suppressed.
 // One-hit death also does not request the half-health-only return actor.
}
static void Aethas(std::vector<SmartScriptHolder> const& rows){
 Player player;auto s=Script(rows,99763,&player);s.ProcessEventsFor(SMART_EVENT_UPDATE,&player);assert(s.calls.size()==3&&s.calls[0].parameter==12980&&s.calls[1].parameter==196561&&s.calls[2].parameter==196347);
 auto start=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DATA_SET,&player,2,2);assert(s.calls.size()==start+1&&s.calls.back().parameter==196381);
 start=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DATA_SET,&player,3,3);assert(s.calls.size()==start+3&&s.calls[start].action==28&&s.calls[start+1].action==53&&s.calls[start+2].parameter==0);
 start=s.calls.size();s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,&player,2,99763);assert(s.calls.size()==start);s.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,&player,1,99763);assert(s.calls.size()==start+4&&s.calls[start].parameter==1&&s.calls[start+1].parameter==223908&&s.calls[start+2].parameter==99861&&s.calls[start+3].parameter==196520&&!s.errors);
}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);Texts(argv[3]);auto original=Read(argv[1],99615),repaired=Read(argv[2],99615);assert(original.size()==31&&repaired.size()==31);
 Player player;Creature pet,guardian,npc;pet.entry=90001;guardian.entry=90002;npc.entry=90003;
 {auto baseline=Script(original,99615,&player);Init(baseline);TickPhoenix(baseline,100);assert(baseline.actor.health==100&&Phoenix(baseline)==2&&!baseline.errors);} // actual timer0..80/0..20ms baseline
 for(Unit* cause:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)}){Thresholds(repaired,cause);Sequence(repaired,cause);}
 Aethas(Read(argv[2],99763));
 std::puts("PASS actual native timer/event/health/once/casting delay: original two premature196327 casts at100pct vs repaired80/20pct, skipped thresholds, combat/repeat/fresh-instance cases. Full original boss aggro/half-health/7s Aethas/data4/death/loot/205 and Aethas return requests retained for five invoker kinds. Spell/spawn/motion/credit endpoints controlled.");
 std::puts("UNRESOLVED preserved99615 links21->22 and31->32 trigger native missing-link diagnostics in the observed request sequence. One-hit death before half-health does not request99763, so return/re-entry recovery remains OPEN. This is not full Fire/client certification.");
}
