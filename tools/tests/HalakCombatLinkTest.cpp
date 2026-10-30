// Native event admission, TALK, target SELF/VICTIM, TEXT_OVER timer and LINK.
// Chat/range search/cast/criteria/data/death endpoints are controlled.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <map>
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
 bool StartPath(bool run,uint32 entry,bool repeat,Unit* invoker){(void)invoker;assert(run&&!repeat);++attempts;path=entry;if(combat&&*combat)return false;++started;return true;}
 void SetDespawnTime(uint32 value){despawn=value;}void SetData(uint32 field,uint32 value){dataCalls.push_back({field,value});}
};
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct Unit {bool player=false,creature=false;Creature* ToCreature();Player* ToPlayer();GameObject* ToGameObject();ObjectGuid GetGUID(){return {this};}};
struct Player:Unit {Player(){player=true;}};
struct Creature:Unit {uint32 entry=93949;bool combat=false,expired=false,casting=false;float health=100;Unit* victim=nullptr;
 FakeAI ai;ReactStates reaction=9;FakeAI* AI(){return &ai;}void SetReactState(ReactStates value){reaction=value;}
 Creature(){creature=true;ai.combat=&combat;}uint32 GetEntry()const{return entry;}bool isInCombat()const{return combat;}bool HasUnitState(uint32 mask)const{assert(mask==UNIT_STATE_CASTING);return casting;}bool IsQuestObjectExpired()const{return expired;}
 uint32 GetMaxHealth()const{return 100;}float GetHealthPct()const{return health;}Unit* getVictim(){return victim;}
};
Creature* Unit::ToCreature(){return static_cast<Creature*>(this);}Player* Unit::ToPlayer(){return static_cast<Player*>(this);}
bool IsCreature(Unit* u){return u&&u->creature;}bool IsPlayer(Unit* u){return u&&u->player;}
bool IsGameObject(Unit*){return false;}
struct GameObject:Unit {FakeAI ai;FakeAI* AI(){return &ai;}};
GameObject* Unit::ToGameObject(){return static_cast<GameObject*>(this);}
using ObjectList=std::vector<Unit*>;struct SpellInfo{};
struct SmartScriptHolder {
 uint32 entryOrGuid=93949,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,targetEntry=0,parameter=0,second=0,timer=0;bool active=true,runOnce=false,enableTimed=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;
  struct {uint32 textGroupID=0,creatureEntry=0;} textOver;
  struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;} dataSet;
  struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;
 } event;
 struct {struct {uint32 textGroupID=0,duration=0;} talk;
  struct {uint32 field=0,data=0;} setData;
  struct {uint32 spell=0,flags=0;} cast;
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
 std::vector<SmartScriptHolder> mEvents,mTimedActionList;std::vector<Call> calls;Creature actor,minerva;Creature* me=&actor;Player nearby;bool alliesPresent=true;std::map<uint32,Creature> allies;unsigned castRequests=0,castRejected=0;bool castAccepted=true;std::unique_ptr<ObjectList> escortTargets;
 ObjectGuid mLastInvoker,mTextGUID;unsigned errors=0;uint32 now=0,mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false,rangePlayer=true;
 SmartScript(){for(uint32 id:{109357u,109358u,109359u,109360u,109361u,109316u})allies[id].entry=id;}
 Creature* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}
 bool IsSmart()const{return me!=nullptr;}void StoreTargetList(ObjectList* list,uint32 id){assert(id==SMART_ESCORT_TARGETS);escortTargets.reset(list);}
 bool IsInPhase(uint32)const{return true;}void RecalcTimer(SmartScriptHolder&,uint32,uint32);void InitTimer(SmartScriptHolder&);void UpdateTimer(SmartScriptHolder&,uint32 const);void Init(){for(auto& e:mEvents)InitTimer(e);}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){auto l=new ObjectList();switch(e.GetTargetType()){
#include "TargetSelfVictim.inc"
  case 18:if(rangePlayer)l->push_back(&nearby);break;
  case 19:if(alliesPresent&&allies.count(e.targetEntry))l->push_back(&allies[e.targetEntry]);break;
 }return l;}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void Update(uint32 diff);
};
static bool maximum=false;
uint32 urand(uint32 min,uint32 max){assert(min<=max);return maximum?max:min;}
#define CAST_AI(type,ai) (ai)
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_DEBUG(...) ((void)0)
#include "FindLinkedEvent.inc"
#include "InitialTimer.inc"
#include "RecalculateTimer.inc"
#include "UpdateTimer.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LinkEvent.inc"
#include "NoParamEvents.inc"
#include "TextOverEvent.inc"
#include "DataSetEvent.inc"
#include "HealthEvent.inc"
#include "CombatEvent.inc"
 }
}
#include "ActionPrefix.inc"
 calls.push_back({now,e.actionType,e.parameter});
 switch(e.GetActionType()){
#include "TalkAction.inc"
 // Unchanged native services are observed as requests, not granted progression.
#include "SetDataAction.inc"
#include "WaypointAction.inc"
 case 11:{auto targets=GetTargets(e,unit);if(!targets||targets->empty()){delete targets;return;}++castRequests;if(!castAccepted){++castRejected;delete targets;return;}delete targets;break;}
 case 12:case 206:case 205:case 37:case 42:break;
 }
#include "LinkTail.inc"
void SmartScript::Update(uint32 diff){now+=diff;for(auto& e:mEvents)UpdateTimer(e,diff);
#include "TextTimer.inc"
static std::vector<SmartScriptHolder> Read(char const* path,uint32 actorId){std::ifstream f(path);std::string line;std::vector<SmartScriptHolder> rows;while(std::getline(f,line)){
 std::stringstream ss(line);std::vector<std::string> fields;std::string cell;while(std::getline(ss,cell,'\t'))fields.push_back(cell);assert(fields.size()==31);
 auto n=[&](size_t i){return uint32(std::stoul(fields[i]));};if(n(0)!=actorId||n(1)!=0)continue;SmartScriptHolder e;e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
 e.event.textOver={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};
 e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.targetType=n(21);e.targetEntry=n(22);e.action.setData={n(15),n(16)};e.action.cast={n(15),n(16)};e.action.wpStart={n(15),n(16),n(17),n(18),n(19),n(20)};rows.push_back(e);
 }return rows;}
static SmartScriptHolder& Event(SmartScript& s,uint32 id){for(auto& e:s.mEvents)if(e.event_id==id)return e;std::abort();}
static unsigned Spell(SmartScript const& s,uint32 spell){unsigned n=0;for(auto const& c:s.calls)if(c.action==11&&c.parameter==spell)++n;return n;}

static void Load(SmartScript& s,char const* path,Unit* victim){s.mEvents=Read(path,93949);assert(s.mEvents.size()==6);s.mEvents.erase(s.mEvents.begin(),s.mEvents.begin()+3);s.actor.entry=93949;s.actor.victim=victim;s.mLastInvoker={victim};s.Init();}
int main(int argc,char** argv){
 if(argc!=3)return 1;Player player;Creature pet,guardian;
 unsigned cases=0;
 for(bool bound:{false,true})for(Unit* victim:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(nullptr)})for(bool accepted:{false,true}){
  ++cases;maximum=bound;SmartScript before,after;Load(before,argv[1],victim);Load(after,argv[2],victim);before.castAccepted=after.castAccepted=accepted;
  assert(Event(after,3).timer==3000&&Event(after,4).timer==10000&&Event(after,5).timer==20000);
  before.Update(90000);after.Update(90000);assert(before.calls.empty()&&after.calls.empty()&&Event(after,3).timer==3000);
  before.actor.combat=after.actor.combat=true;
  before.Update(3000);after.Update(3000);assert(after.calls.empty());before.Update(1);after.Update(1);assert(Spell(after,79961)==1&&Event(after,3).timer==5000);
  // Actual native UpdateTimer holds a due ordinary cast while casting.
  Event(before,3).timer=Event(after,3).timer=0;before.actor.casting=after.actor.casting=true;
  before.Update(100);after.Update(100);assert(Spell(after,79961)==1&&Event(after,3).timer==1);
  before.actor.casting=after.actor.casting=false;before.Update(2);after.Update(2);assert(Spell(after,79961)==2);
  for(unsigned ms=0;ms<90000;++ms){before.Update(1);after.Update(1);}
  assert(before.calls==after.calls&&Spell(after,79961)>=18&&Spell(after,192648)>=6&&Spell(after,195272)==1);
  assert(before.errors==unsigned(accepted)&&!after.errors);
  auto remaining=Event(after,3).timer;after.actor.combat=false;auto count=after.calls.size();after.Update(90000);assert(Event(after,3).timer==remaining&&after.calls.size()==count);
  // Fresh reset/re-entry uses a new original timer state, no immediate casts.
  SmartScript fresh;Load(fresh,argv[2],victim);assert(!fresh.mEvents[0].runOnce&&Event(fresh,3).timer==3000&&fresh.calls.empty());
 }
 for(unsigned mode=0;mode<3;++mode){++cases;SmartScript s;Load(s,argv[2],&player);s.actor.combat=true;conditions.allow=mode!=0;s.actor.expired=mode==1;if(mode==2)s.me=nullptr;s.Update(20001);assert(s.calls.empty()&&!s.errors);}conditions.allow=true;
 SmartScript naive;Load(naive,argv[1],&player);Event(naive,3).type=SMART_EVENT_LINK;naive.Init();naive.actor.combat=true;naive.Update(3001);assert(!Spell(naive,79961));naive.Update(20001);assert(Spell(naive,79961)==1);naive.Update(60000);assert(Spell(naive,79961)==1); // Converting the timer to LINK removes ordinary repeats.
 std::printf("PASS %u native timer/LINK cases: pinned old invalid-link error paired with identical repaired cast requests,3s/5s Holy Shock,10s/15s Holy Light,20s once Avenging Wrath; exact timer boundary,casting deferral,combat pause,condition/expired/missing actor,reset and player/pet/guardian/null victims. Naive LINK conversion destroys repeats. CAST selection/acceptance endpoints controlled; actual combat/voice/client remain separate.\n",cases+1);
}
