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
#include <cstdlib>
using uint32=uint32_t;using uint8=uint8_t;
#include "EventConstants.inc"
struct Unit;struct Creature;struct Player;
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid const Empty;uint32 GetEntry()const;};ObjectGuid const ObjectGuid::Empty{};
struct Position{float x=1,y=2,z=3,o=4;void Relocate(float a,float b,float c,float d){x=a;y=b;z=c;o=d;}};
struct Unit {Unit*ToUnit(){return this;}Position GetPosition(){return {};}bool player=false,creature=false;Creature* ToCreature();Player* ToPlayer();ObjectGuid GetGUID(){return {this};}};
struct Player:Unit {Player(){player=true;}};
struct Creature:Unit {uint32 entry=102532;bool combat=true,expired=false,world=true;float health=100;Unit* victim=nullptr;
 Creature(){creature=true;}uint32 GetEntry()const{return entry;}bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}bool IsInWorld()const{return world;}
 uint32 GetMaxHealth()const{return 100;}float GetHealthPct()const{return health;}Unit* getVictim(){return victim;}
};
uint32 ObjectGuid::GetEntry()const{return unit&&unit->creature?unit->ToCreature()->entry:0;}
Creature* Unit::ToCreature(){return creature?static_cast<Creature*>(this):nullptr;}Player* Unit::ToPlayer(){return player?static_cast<Player*>(this):nullptr;}
bool IsCreature(Unit* u){return u&&u->creature;}bool IsPlayer(Unit* u){return u&&u->player;}
using ObjectList=std::vector<Unit*>;struct SpellInfo{};struct GameObject{};
struct SmartScriptHolder {
 uint32 entryOrGuid=102532,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,parameter=0,second=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;
  struct {uint32 quest=0,cooldownMin=0,cooldownMax=0;}quest;struct {uint32 textGroupID=0,creatureEntry=0;} textOver;
  struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;} dataSet;
  struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;
 } event;
 struct {struct{uint32 id=0,targetX=0,targetY=0,targetZ=0;}conversation;struct {uint32 textGroupID=0,duration=0;} talk;} action;
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
enum class HighGuid{Conversation};struct Generator{uint32 next=0;uint32 Generate(){return ++next;}}generator;
struct ObjectMgr{template<HighGuid>Generator*GetGenerator(){return &generator;}}objectMgr;auto*sObjectMgr=&objectMgr;
uint32 creates=0,destroys=0;bool rejectConversation=false;std::vector<void*>liveConversations;
struct Conversation{~Conversation(){++destroys;}bool CreateConversation(uint32,uint32 id,Unit*actor,void*,Position const&pos){assert(id==4192&&actor&&actor->player&&pos.x==1&&pos.y==2&&pos.z==3);++creates;if(rejectConversation)return false;liveConversations.push_back(this);return true;}};
static void ClearConversations(){for(void*p:liveConversations)delete static_cast<Conversation*>(p);liveConversations.clear();creates=destroys=0;rejectConversation=false;conditions.allow=true;}
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
#include "QuestEvent.inc"
#include "HealthEvent.inc"
 }
}
#include "ActionPrefix.inc"
 calls.push_back({now,e.actionType,e.parameter});
 switch(e.GetActionType()){
#include "TalkAction.inc"
#include "ConversationAction.inc"
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
 e.event.quest={n(9),n(10),n(11)};e.action.conversation={n(15),n(16),n(17),n(18)};e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.targetType=n(21);rows.push_back(e);
 }return rows;}
static SmartScript Script(std::vector<SmartScriptHolder> const& rows,uint32 entry,Unit* victim){return SmartScript(rows,entry,victim);}
int main(int argc,char**argv){_set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);auto old=Read(argv[1],100438),fixed=Read(argv[2],100438);assert(old.size()==10&&fixed.size()==10);uint32 cases=0;
 Player player;Creature pet,guardian,npc;
 for(Unit*invoker:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)})for(bool reject:{false,true})for(bool repaired:{false,true}){
  ClearConversations();rejectConversation=reject;auto s=Script(repaired?fixed:old,100438,invoker);s.ProcessEventsFor(SMART_EVENT_REWARD_QUEST,invoker,45440);assert(s.calls.size()==1&&s.calls[0].action==206&&s.calls[0].parameter==4192&&s.errors==(repaired?0u:1u));assert(creates==unsigned(invoker==&player));assert(destroys==unsigned(invoker==&player&&reject));
  s.ProcessEventsFor(SMART_EVENT_REWARD_QUEST,invoker,45440);assert(s.calls.size()==2&&creates==2*unsigned(invoker==&player)&&s.errors==(repaired?0u:2u));++cases;
 }
 for(bool repaired:{false,true})for(unsigned mode=0;mode<3;++mode){ClearConversations();auto s=Script(repaired?fixed:old,100438,&player);if(mode==0)conditions.allow=false;if(mode==1)s.actor.expired=true;s.ProcessEventsFor(SMART_EVENT_REWARD_QUEST,&player,mode==2?45439:45440);assert(!creates&&!s.errors&&s.calls.empty());++cases;}
 ClearConversations();std::printf("PASS %u actual reward-event/action206/player-target/LINK cases: native4192 request unchanged for real45440 reward, player/pet/guardian/NPC/null, successful/failed creation ownership cleanup, repeated callbacks retain native behavior, wrongquest/conditions/expired rejection; original missing10 counterfactual paired with terminal successor. Conversation world/packets and actual quest reward admission are controlled/separate; no new credit/item/quest grant.\n",cases);}
