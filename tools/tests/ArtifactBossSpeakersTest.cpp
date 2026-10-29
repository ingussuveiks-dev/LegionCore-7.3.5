// Native event admission, TALK, target SELF/VICTIM, TEXT_OVER timer and LINK.
// Chat/range search/cast/criteria/data/death endpoints are controlled.
#include <cassert>
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
struct Creature:Unit {uint32 entry=102532;bool combat=true,expired=false,world=true;float health=100;Unit* victim=nullptr;
 Creature(){creature=true;}uint32 GetEntry()const{return entry;}bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}bool IsInWorld()const{return world;}
 uint32 GetMaxHealth()const{return 100;}float GetHealthPct()const{return health;}Unit* getVictim(){return victim;}
};
uint32 ObjectGuid::GetEntry()const{return unit&&unit->creature?unit->ToCreature()->entry:0;}
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
using CreatureTextHolder=std::map<uint8,uint32>;
#define TC_LOG_DEBUG(...) ((void)0)
struct Chat {std::vector<Speech> requests;std::map<uint32,CreatureTextHolder> groups;std::vector<CreatureTextHolder*> mTextList;
 void Prepare(){mTextList.assign(108169,nullptr);for(auto& item:groups)mTextList[item.first]=&item.second;}
 uint32 SendChat(Creature* source,uint8 textGroup,ObjectGuid){
#include "ChatAdmission.inc"
 requests.push_back({source->GetEntry(),textGroup});return 1;}
} chat;auto sCreatureTextMgr=&chat;
struct Call {uint32 time,action,parameter;bool operator==(Call const& b)const{return time==b.time&&action==b.action&&parameter==b.parameter;}};
bool roll_chance_i(uint32){return true;}
struct SmartScript {
 SmartScript()=default;SmartScript(std::vector<SmartScriptHolder> rows,uint32 entry,Unit* victim):mEvents(std::move(rows)){actor.entry=entry;actor.victim=victim;}
 std::vector<SmartScriptHolder> mEvents;std::vector<Call> calls;Creature actor;Creature* me=&actor;Player nearby;
 ObjectGuid mLastInvoker,mTextGUID;unsigned errors=0;uint32 now=0,mTalkerEntry=0,mLastTextID=0,mTextTimer=0;bool mUseTextTimer=false,rangePlayer=true;
 Creature* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}
 bool IsInPhase(uint32)const{return true;}void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit* unit){auto l=new ObjectList();switch(e.GetTargetType()){
#include "TargetSelfVictim.inc"
  case 7:if(unit)l->push_back(unit);break;
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
 case 11:case 205:case 45:case 37:case 42:case 75:case 12:case 22:case 50:case 80:break;
 }
#include "LinkTail.inc"
void SmartScript::Update(uint32 diff){now+=diff;
#include "TextTimer.inc"
static std::vector<SmartScriptHolder> Read(char const* path,uint32 entry){std::ifstream f(path);std::string line;std::vector<SmartScriptHolder> rows;while(std::getline(f,line)){
 std::stringstream ss(line);std::vector<std::string> fields;std::string cell;while(std::getline(ss,cell,'\t'))fields.push_back(cell);assert(fields.size()==31);
 auto n=[&](size_t i){return uint32(std::stoul(fields[i]));};if(n(0)!=entry||n(1)!=0)continue;SmartScriptHolder e;e.entryOrGuid=n(0);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
 e.event.textOver={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};
 e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.targetType=n(21);rows.push_back(e);
 }return rows;}
static void Texts(char const* path){std::ifstream f(path);std::string line;while(std::getline(f,line)){std::stringstream ss(line);std::string entry,group;std::getline(ss,entry,'\t');std::getline(ss,group,'\t');chat.groups[uint32(std::stoul(entry))][uint8(std::stoul(group))]=1;}chat.Prepare();}
static SmartScript Script(std::vector<SmartScriptHolder> const& rows,uint32 entry,Unit* victim){return SmartScript(rows,entry,victim);}
static void Farthing(std::vector<SmartScriptHolder> const& rows,Unit* victim,bool repaired){
 chat.requests.clear();auto s=Script(rows,101148,victim);assert(s.me==&s.actor);s.ProcessEventsFor(SMART_EVENT_AGGRO,victim);
 assert(s.calls.size()==1&&s.calls[0].parameter==1);if(repaired||!IsCreature(victim))assert(chat.requests.size()==1&&chat.requests[0].speaker==101148);
 s.actor.health=70;s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);assert(s.calls.back().action==1&&s.calls.back().parameter==2&&s.mTextTimer==5000);
 // Original chain requests15 mind actors; this test observes, not creates, them.
 unsigned summons=0;for(auto const& c:s.calls)if(c.action==12){assert(c.parameter==102784);++summons;}assert(summons==15);
 auto count=s.calls.size();s.Update(4999);s.Update(1);assert(s.calls.size()==count);s.Update(1);
 if(!repaired&&IsCreature(victim)){assert(s.calls.size()==count&&chat.requests.empty());return;} // native missing-pet text + wrongTEXT_OVER speaker stalls
 assert(s.calls.back().parameter==3&&s.mTextTimer==7000);s.Update(7000);s.Update(1);assert(s.calls.back().parameter==4&&s.mTextTimer==5000);
 assert(chat.requests.size()==4);uint32 expected[]={1,2,3,4};for(size_t i=0;i<4;++i){assert(chat.requests[i].speaker==101148&&chat.requests[i].group==expected[i]);}
 count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_AGGRO,victim);s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);s.ProcessEventsFor(SMART_EVENT_TEXT_OVER,victim,2,101148);s.ProcessEventsFor(SMART_EVENT_TEXT_OVER,victim,3,101148);assert(s.calls.size()==count&&!s.errors);
}
int main(int argc,char** argv){
 if(argc!=4)return 1;Texts(argv[3]);auto farOld=Read(argv[1],101148),farFixed=Read(argv[2],101148),bOld=Read(argv[1],108168),bFixed=Read(argv[2],108168);assert(farOld.size()==34&&farFixed.size()==34&&bOld.size()==7&&bFixed.size()==7);
 Player player;Creature pet,guardian,npc;pet.entry=9001;guardian.entry=9002;npc.entry=9003;
 for(Unit* victim:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)}){
  Farthing(farOld,victim,false);Farthing(farFixed,victim,true);
  // Both Balaadur roots belong to108168 even if its current victim is absent
  // or a pet. Real last killer remains separate from the current victim.
  for(bool repaired:{false,true}){chat.requests.clear();auto s=Script(repaired?bFixed:bOld,108168,victim);s.ProcessEventsFor(SMART_EVENT_AGGRO,victim);s.actor.health=50;s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);assert(s.calls.size()==2);if(repaired||!IsCreature(victim)){assert(chat.requests.size()==2);assert(chat.requests[0].speaker==108168&&chat.requests[0].group==0&&chat.requests[1].speaker==108168&&chat.requests[1].group==4);}else assert(chat.requests.empty());
   auto count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_AGGRO,victim);s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);assert(s.calls.size()==count);
   // Preserve the full original death request sequence and visible unknown
   // LINK8 finding. This speaker repair does not invent or suppress that edge.
   s.ProcessEventsFor(SMART_EVENT_DEATH,victim);assert(s.errors==1&&s.calls.size()==5);assert(s.calls[2].action==50&&s.calls[2].parameter==251049&&s.calls[3].action==12&&s.calls[3].parameter==108202&&s.calls[4].action==205&&s.calls[4].parameter==92);
  }
 }
 for(uint32 entry:{101148u,108168u})for(unsigned mode=0;mode<3;++mode){chat.requests.clear();auto s=Script(entry==101148?farFixed:bFixed,entry,&pet);conditions.allow=mode!=0;s.actor.expired=mode==1;s.actor.health=100;
  if(mode<2){s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(s.calls.empty()&&chat.requests.empty());}else{s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,&pet);assert(s.calls.empty());}
 }conditions.allow=true;
 chat.requests.clear();auto removed=Script(farFixed,101148,&pet);removed.actor.health=70;removed.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,&pet);auto count=removed.calls.size();removed.me=nullptr;removed.Update(5001);assert(removed.calls.size()==count);
 std::puts("PASS native Farthing/Balaadur TALK/SELF/VICTIM/INVOKER/event+LINK/text timer and actual CreatureTextMgr entry/group admission: original pet speaker yields no text and stalls Farthing; repaired own aggro +2/5000ms->3/7000ms->4, Balaadur aggro/50-percent, player/pet/guardian/NPC/no victim, repeats/invalid/removed actor. Original15 shadow requests and Balaadur death chest/NPC/criterion+knownmissingLINK8 retained. Text packet/render/cast/summon/progression endpoints not client traversal proof.");
}
