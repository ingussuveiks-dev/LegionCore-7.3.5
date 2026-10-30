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
 void Prepare(){mTextList.assign(110557,nullptr);for(auto& item:groups)mTextList[item.first]=&item.second;}
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
static std::vector<SmartScriptHolder> Read(char const* path,uint32 entry){std::ifstream f(path);std::string line;std::vector<SmartScriptHolder> rows;while(std::getline(f,line)){
 std::stringstream ss(line);std::vector<std::string> fields;std::string cell;while(std::getline(ss,cell,'\t'))fields.push_back(cell);assert(fields.size()==31);
 auto n=[&](size_t i){return uint32(std::stoul(fields[i]));};if(n(0)!=entry||n(1)!=0)continue;SmartScriptHolder e;e.entryOrGuid=n(0);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
 e.event.textOver={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};
 e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.targetType=n(21);rows.push_back(e);
 }return rows;}
static void Texts(char const* path){std::ifstream f(path);std::string line;while(std::getline(f,line)){std::stringstream ss(line);std::string entry,group;std::getline(ss,entry,'\t');std::getline(ss,group,'\t');chat.groups[uint32(std::stoul(entry))][uint8(std::stoul(group))]=1;}chat.Prepare();}
static SmartScript Script(std::vector<SmartScriptHolder> const& rows,uint32 entry,Unit* victim){return SmartScript(rows,entry,victim);}

static void Boss(std::vector<SmartScriptHolder> const& rows,Unit* cause,bool repaired){
 chat.requests.clear();auto s=Script(rows,101778,cause);s.ProcessEventsFor(SMART_EVENT_AGGRO,cause);
 bool own=repaired||!IsCreature(cause);assert(s.calls.size()==1&&s.calls[0].action==1&&s.calls[0].parameter==0);
 assert(chat.requests.size()==unsigned(own));if(own)assert(chat.requests[0].speaker==101778&&chat.requests[0].group==0);
 s.ProcessEventsFor(SMART_EVENT_AGGRO,cause);assert(s.calls.size()==1); // original once flag
 for(unsigned health:{80u,50u,25u}){auto count=s.calls.size();s.actor.health=float(health);s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,cause);assert(s.calls.size()==count+6);assert(s.calls[count].action==11&&s.calls[count].parameter==203956);for(size_t i=count+1;i<s.calls.size();++i)assert(s.calls[i].action==12&&s.calls[i].parameter==103466);s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,cause);assert(s.calls.size()==count+6);}
 auto before=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DEATH,cause);assert(!s.errors&&s.calls.size()==before+6);
 unsigned actions[]={1,12,50,205,50,45};unsigned parameters[]={1,103795,247566,92,242469,7};
 for(unsigned i=0;i<6;++i){assert(s.calls[before+i].action==actions[i]&&s.calls[before+i].parameter==parameters[i]);}
 auto criterion=std::find_if(rows.begin(),rows.end(),[](SmartScriptHolder const& e){return e.event_id==12;});assert(criterion!=rows.end()&&criterion->second==48788&&criterion->targetType==7);
 assert(chat.requests.size()==(own?2u:0u));if(own)assert(chat.requests.back().speaker==101778&&chat.requests.back().group==1);
 before=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DEATH,cause);assert(s.calls.size()==before);
 chat.requests.clear();auto one=Script(rows,101778,cause);one.ProcessEventsFor(SMART_EVENT_DEATH,cause);assert(one.calls.size()==6&&!one.errors&&chat.requests.size()==unsigned(own));
}
int main(int argc,char** argv){_set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);Texts(argv[3]);auto original=Read(argv[1],101778),repaired=Read(argv[2],101778);assert(original.size()==15&&repaired.size()==15);Player player;Creature pet,guardian,npc;pet.entry=90001;guardian.entry=90002;npc.entry=90003;
 for(Unit* cause:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)}){Boss(original,cause,false);Boss(repaired,cause,true);}
 for(unsigned mode=0;mode<3;++mode){chat.requests.clear();auto s=Script(repaired,101778,&pet);conditions.allow=mode!=0;s.actor.expired=mode==1;if(mode==2)s.me=nullptr;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(chat.requests.empty());if(mode<2)assert(s.calls.empty());}conditions.allow=true;
 std::puts("PASS native Fleshripper own aggro/death TALK/target/CreatureTextMgr/event/health/LINK requests: old pet/guardian/NPC wrong-speaker counterfactual vs repaired own two groups; player/pet/guardian/NPC/null causes, onehit, repeats, three native health waves, original death actor/loot/205/return/data sequence and invalid actor gates. Cast/spawn/credit endpoints controlled; separate real SmartPetCredit native/PE test verifies owner resolution.");
}
