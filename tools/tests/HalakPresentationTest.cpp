// Native event admission, TALK, target SELF/VICTIM, TEXT_OVER timer and LINK.
// Chat/range search/cast/criteria/data/death endpoints are controlled.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <list>
#include <memory>
#include <cstdlib>
#include <vector>
#include <set>
using uint64=uint64_t;using uint32=uint32_t;using uint8=uint8_t;
using CriteriaTypes=int;
#include "EventConstants.inc"
struct Unit;struct Creature;struct Player;
struct ObjectGuid {Unit* unit=nullptr;static ObjectGuid const Empty;uint32 GetEntry()const;bool IsPlayer()const;};ObjectGuid const ObjectGuid::Empty{};
struct Map{} firstMap,secondMap;
struct Position {float x=0,y=0,z=0;void Relocate(float a,float b,float c,float){x=a;y=b;z=c;}};
struct Unit {bool player=false,creature=false,world=true,phase=true,available=true,scope=false,scopeAllows=true;float distance=1;Map* map=&firstMap;Unit* owner=nullptr;Unit* charmer=nullptr;
 Creature* ToCreature();Player* ToPlayer();Player const* ToPlayer()const;Unit* ToUnit(){return this;}ObjectGuid GetGUID(){return {this};}
 ObjectGuid GetCharmerOrOwnerGUID()const{return {charmer?charmer:owner};}Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;
 bool IsInWorld()const{return world;}Map* GetMap()const{return map;}bool InSamePhase(Unit*)const{return phase;}bool IsWithinDistInMap(Unit*,float maximum)const{return distance<=maximum;}
 bool HasQuestObjectScope()const{return scope;}bool CanTargetQuestObject(Unit*)const{return scopeAllows;}Position GetPosition()const{return {1,2,3};}
};using WorldObject=Unit;
struct Player:Unit {uint32 criteria=0,lastAsset=0;Unit* criteriaInvoker=nullptr;Player(){player=true;}void UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32,uint32,Unit* invoker,bool){assert(type==92);++criteria;lastAsset=asset;criteriaInvoker=invoker;}};
struct Creature:Unit {uint32 entry=102532;bool combat=true,expired=false;float health=100;Unit* victim=nullptr;
 Creature(){creature=true;}uint32 GetEntry()const{return entry;}bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}bool IsInWorld()const{return world;}
 uint32 GetMaxHealth()const{return 100;}float GetHealthPct()const{return health;}Unit* getVictim(){return victim;}
};
uint32 ObjectGuid::GetEntry()const{return unit&&unit->creature?unit->ToCreature()->entry:0;}
Creature* Unit::ToCreature(){return creature?static_cast<Creature*>(this):nullptr;}Player* Unit::ToPlayer(){return player?static_cast<Player*>(this):nullptr;}Player const* Unit::ToPlayer()const{return player?static_cast<Player const*>(this):nullptr;}
bool ObjectGuid::IsPlayer()const{return unit&&unit->player;}
struct ObjectAccessor{static Player* GetPlayer(Unit const&,ObjectGuid id){return id.unit&&id.unit->available?id.unit->ToPlayer():nullptr;}};
#include "OwnerPlayer.inc"
#include "CreditPlayer.inc"
#include "PlayerResolver.inc"
enum class HighGuid{Conversation};
struct Generator{uint64 Generate(){return 1;}};
struct ObjectMgr{Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}} objectManager;auto sObjectMgr=&objectManager;
struct Story{uint32 id;Player* player;};std::vector<Story> stories;bool failConversation=false;
struct Conversation{bool CreateConversation(uint64,uint32,Unit*,void*,Position const&);};std::vector<std::unique_ptr<Conversation>> liveStories;
bool Conversation::CreateConversation(uint64,uint32 id,Unit* owner,void*,Position const& pos){assert(id&&owner&&owner->ToPlayer()&&pos.x==1);if(failConversation)return false;stories.push_back({id,owner->ToPlayer()});liveStories.emplace_back(this);return true;}

bool IsCreature(Unit* u){return u&&u->creature;}bool IsPlayer(Unit* u){return u&&u->player;}
using ObjectList=std::list<Unit*>;struct SpellInfo{};struct GameObject{};
struct SmartScriptHolder {
 uint32 entryOrGuid=102532,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,parameter=0,second=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;
  struct {uint32 textGroupID=0,creatureEntry=0;} textOver;
  struct {uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;} dataSet;
  struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;
 } event;
 struct {struct {uint32 textGroupID=0,duration=0;} talk;struct{uint32 id=0,targetX=0,targetY=0,targetZ=0;}conversation;struct{uint32 type=0,misc1=0,misc2=0,misc3=0;}achievementCriteria;} action;
 struct{struct{uint32 dist=0;}playerDistance;}target;
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
#include "InvokerTarget.inc"
#include "PlayerTarget.inc"
 }
#include "TargetTail.inc"
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
#include "UpdateEvent.inc"
 }
}
#include "ActionPrefix.inc"
 calls.push_back({now,e.actionType,e.parameter});
 switch(e.GetActionType()){
#include "TalkAction.inc"
#include "CriteriaAction.inc"
#include "ConversationAction.inc"
 // Unchanged native services are observed as requests, not granted progression.
 case 2:case 11:case 45:case 37:case 42:case 75:case 12:case 22:case 50:case 80:break;
 }
#include "LinkTail.inc"
void SmartScript::Update(uint32 diff){now+=diff;
#include "TextTimer.inc"
static std::vector<SmartScriptHolder> Read(char const* path,uint32 entry){std::ifstream f(path);std::string line;std::vector<SmartScriptHolder> rows;while(std::getline(f,line)){
 std::stringstream ss(line);std::vector<std::string> fields;std::string cell;while(std::getline(ss,cell,'\t'))fields.push_back(cell);assert(fields.size()==31);
 auto n=[&](size_t i){return uint32(std::stoul(fields[i]));};if(n(0)!=entry||n(1)!=0)continue;SmartScriptHolder e;e.entryOrGuid=n(0);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
 e.event.textOver={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};
 e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.targetType=n(21);e.target.playerDistance.dist=n(22);e.action.conversation={n(15),n(16),n(17),n(18)};e.action.achievementCriteria={n(15),n(16),n(17),n(18)};rows.push_back(e);
 }return rows;}
static void Texts(char const* path){std::ifstream f(path);std::string line;while(std::getline(f,line)){std::stringstream ss(line);std::string entry,group;std::getline(ss,entry,'\t');std::getline(ss,group,'\t');chat.groups[uint32(std::stoul(entry))][uint8(std::stoul(group))]=1;}chat.Prepare();}
static SmartScript Script(std::vector<SmartScriptHolder> const& rows,uint32 entry,Unit* victim){return SmartScript(rows,entry,victim);}

static void Clear(){chat.requests.clear();stories.clear();liveStories.clear();failConversation=false;}
static void Encounter(std::vector<SmartScriptHolder> const& rows,Unit* attacker,bool repaired){
 Clear();auto s=Script(rows,93949,attacker);s.ProcessEventsFor(SMART_EVENT_AGGRO,attacker);
 auto recipient=repaired?GetScriptCreditPlayer(attacker):(attacker?attacker->ToPlayer():nullptr);
 assert(stories.size()==unsigned(recipient!=nullptr));if(recipient)assert(stories[0].player==recipient&&stories[0].id==741);
 bool own=repaired||!IsCreature(attacker);assert(chat.requests.size()==unsigned(own));if(own)assert(chat.requests[0].speaker==93949&&chat.requests[0].group==0);
 auto count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_AGGRO,attacker);assert(s.calls.size()==count); // native once flags
 s.ProcessEventsFor(SMART_EVENT_DEATH,attacker);assert(chat.requests.size()==(own?2u:0u));if(own)assert(chat.requests.back().speaker==93949&&chat.requests.back().group==1);
 count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DEATH,attacker);assert(s.calls.size()==count&&!s.errors);
 // All three original combat cast requests remain; invalid5->3 stays visible.
 s.ProcessEventsFor(SMART_EVENT_UPDATE_IC);assert(s.errors==1&&s.calls.size()==count+3);
 assert(s.calls[count].action==11&&s.calls[count].parameter==79961&&s.calls[count+1].parameter==192648&&s.calls[count+2].parameter==195272);
 s.ProcessEventsFor(SMART_EVENT_UPDATE_IC);assert(s.errors==1&&s.calls.size()==count+5);
}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);Texts(argv[3]);auto original=Read(argv[1],93949),fixed=Read(argv[2],93949);assert(original.size()==6&&fixed.size()==6);
 Player player,other;Creature pet,guardian,charmed,npc,npcOwned;pet.entry=9001;guardian.entry=9002;charmed.entry=9003;npc.entry=9004;npcOwned.entry=9005;pet.owner=guardian.owner=&player;charmed.charmer=&player;npcOwned.owner=&npc;
 for(Unit* actor:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&charmed),static_cast<Unit*>(&npc),static_cast<Unit*>(&npcOwned),static_cast<Unit*>(nullptr)}){Encounter(original,actor,false);Encounter(fixed,actor,true);}
 // A single lethal hit still requests own death line without a health phase.
 for(Unit* killer:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)}){Clear();auto s=Script(fixed,93949,killer);s.ProcessEventsFor(SMART_EVENT_DEATH,killer);assert(s.calls.size()==1&&chat.requests.size()==1&&chat.requests[0].speaker==93949&&chat.requests[0].group==1&&stories.empty());}
 for(unsigned mode=0;mode<7;++mode){Clear();Player owner;Creature trigger;trigger.owner=&owner;auto s=Script(fixed,93949,&trigger);if(mode==0)owner.world=false;if(mode==1)owner.phase=false;if(mode==2)owner.map=&secondMap;if(mode==3)owner.distance=71;if(mode==4)owner.available=false;if(mode==5){s.actor.scope=true;s.actor.scopeAllows=false;}if(mode==6)s.actor.world=false;s.ProcessEventsFor(SMART_EVENT_AGGRO,&trigger);assert(stories.empty());}
 Clear();{player.distance=70;auto s=Script(fixed,93949,&pet);s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.size()==1&&stories[0].player==&player);player.distance=1;}
 Clear();{auto s=Script(fixed,93949,&pet);failConversation=true;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.empty()&&chat.requests.size()==1);failConversation=false;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.empty());} // native once-on-attempt retained, not a retry claim
 for(unsigned mode=0;mode<3;++mode){Clear();auto s=Script(fixed,93949,&pet);conditions.allow=mode!=0;s.actor.expired=mode==1;if(mode==2)s.me=nullptr;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.empty()&&chat.requests.empty());}conditions.allow=true;
 // A fresh native reset/encounter can repeat its original presentation once.
 Clear();{auto s=Script(fixed,93949,&guardian);s.ProcessEventsFor(SMART_EVENT_AGGRO,&guardian);assert(stories.size()==1);}
 std::puts("PASS native Halak event/TALK/chat admission, actual player owner helper/130 resolver/target and conversation206: original pet loses741 and own0/1 speech; repaired player/pet/guardian/charm, ownerless/no stranger, world/map/phase/range/scope, duplicate events/one-hit death/missing actor. Native six-row combat graph unchanged, invalid5->3 deliberately observed; cast timing/health/world creation/packet rendering controlled or separate. Failed story creation retains original once-on-attempt semantics; no client/full encounter certification.");
 return 0;
}
