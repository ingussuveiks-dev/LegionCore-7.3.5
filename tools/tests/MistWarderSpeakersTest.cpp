// Extracted native Mist Warder TALK/targets/events/LINK/DIE and SmartAI death/hit dispatch.
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
struct Creature:Unit {uint32 entry=102532;bool combat=true,expired=false,world=true,dead=false;float health=100;Unit* victim=nullptr;
 void (*onKill)(Creature*,Creature*)=nullptr;unsigned kills=0;
 void Kill(Creature* target){if(target->dead)return;++kills;target->dead=true;target->health=0;if(onKill)onKill(this,target);}
 bool isDead()const{return dead;}Creature(){creature=true;}uint32 GetEntry()const{return entry;}bool isInCombat()const{return combat;}bool IsQuestObjectExpired()const{return expired;}bool IsInWorld()const{return world;}
 uint32 GetMaxHealth()const{return 100;}float GetHealthPct()const{return health;}Unit* getVictim(){return victim;}
};
uint32 ObjectGuid::GetEntry()const{return unit&&unit->creature?unit->ToCreature()->entry:0;}
Creature* Unit::ToCreature(){return static_cast<Creature*>(this);}Player* Unit::ToPlayer(){return static_cast<Player*>(this);}
bool IsCreature(Unit* u){return u&&u->creature;}bool IsPlayer(Unit* u){return u&&u->player;}
using ObjectList=std::vector<Unit*>;struct SpellInfo{uint32 Id=3355;struct Misc{struct {uint32 SchoolMask=16;} MiscData;}misc;Misc const* GetMisc()const{return &misc;}};struct GameObject{};
struct SmartScriptHolder {
 uint32 entryOrGuid=102532,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,parameter=0,second=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;
  struct {uint32 spell=0,school=0,cooldownMin=0,cooldownMax=0;} spellHit;
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
#include "SpellHitEvent.inc"
 }
}
#include "ActionPrefix.inc"
 calls.push_back({now,e.actionType,e.parameter});
 switch(e.GetActionType()){
#include "TalkAction.inc"
#include "DieAction.inc"
 // Unchanged native services are observed as requests, not granted progression.
 case 11:case 205:case 45:case 42:case 75:case 12:case 22:case 50:case 80:break;
 }
#include "LinkTail.inc"
void SmartScript::Update(uint32 diff){now+=diff;
#include "TextTimer.inc"
static std::vector<SmartScriptHolder> Read(char const* path,uint32 entry){std::ifstream f(path);std::string line;std::vector<SmartScriptHolder> rows;while(std::getline(f,line)){
 std::stringstream ss(line);std::vector<std::string> fields;std::string cell;while(std::getline(ss,cell,'\t'))fields.push_back(cell);assert(fields.size()==31);
 auto n=[&](size_t i){return uint32(std::stoul(fields[i]));};if(n(0)!=entry||n(1)!=0)continue;SmartScriptHolder e;e.entryOrGuid=n(0);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);
 e.event.spellHit={n(9),n(10),n(11),n(12)};e.event.textOver={n(9),n(10)};e.event.dataSet={n(9),n(10),n(11),n(12)};e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};
 e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.talk={n(15),n(16)};e.targetType=n(21);rows.push_back(e);
 }return rows;}
static void Texts(char const* path){std::ifstream f(path);std::string line;while(std::getline(f,line)){std::stringstream ss(line);std::string entry,group;std::getline(ss,entry,'\t');std::getline(ss,group,'\t');chat.groups[uint32(std::stoul(entry))][uint8(std::stoul(group))]=1;}chat.Prepare();}
static SmartScript Script(std::vector<SmartScriptHolder> const& rows,uint32 entry,Unit* victim){return SmartScript(rows,entry,victim);}

constexpr uint32 SMART_ESCORT_ESCORTING=1,DONE=1,ENCOUNTER_FRAME_DISENGAGE=1,ENCOUNTER_FRAME_INSTANCE_END=2;
struct Instance {void SetBossState(uint32,uint32){}void SaveToDB(){}void SendEncounterUnit(uint32,Creature*){}};
struct SmartAI {SmartScript* script;Creature* me;Instance* instance=nullptr;uint32 _bossId=0;
 SmartScript* GetScript(){return script;}bool HasEscortState(uint32)const{return false;}void EndPath(bool){}
 void JustDied(Unit*);void SpellHit(Unit*,SpellInfo const*);
};
#include "JustDied.inc"
#include "SpellHit.inc"
static SmartAI* activeAI=nullptr;
static void Killed(Creature* killer,Creature* target){assert(activeAI&&target==activeAI->me);activeAI->JustDied(killer);}
static void Check(std::vector<SmartScriptHolder> const& rows,Unit* victim,bool repaired,bool trap){
 chat.requests.clear();auto s=Script(rows,100207,victim);assert(s.me==&s.actor);SmartAI ai{&s,&s.actor};activeAI=&ai;s.actor.onKill=Killed;
 s.ProcessEventsFor(SMART_EVENT_AGGRO,victim);assert(chat.requests.size()==1&&chat.requests[0].speaker==100207&&chat.requests[0].group==0);
 s.actor.health=51;s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);assert(chat.requests.size()==1);s.actor.health=50;s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);
 bool healthOwn=repaired||!IsCreature(victim);assert(chat.requests.size()==(healthOwn?2u:1u));if(healthOwn)assert(chat.requests.back().speaker==100207&&chat.requests.back().group==1);
 auto count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_HEALTH_PCT,victim);assert(s.calls.size()==count);
 auto textCount=chat.requests.size();
 if(trap){SpellInfo other;other.Id=1;ai.SpellHit(victim,&other);assert(!s.actor.kills&&s.calls.size()==count);SpellInfo freeze;ai.SpellHit(victim,&freeze);assert(s.actor.kills==1&&s.actor.dead);}
 else {s.actor.dead=true;s.actor.health=0;ai.JustDied(victim);}
 // Source21 closest-player credit remains independent of who killed/trapped.
 unsigned criteria=0;for(auto const& c:s.calls)if(c.action==205){++criteria;assert(c.parameter==92);}assert(criteria==1&&!s.errors);
 bool deathOwn=repaired||trap||!IsCreature(victim);assert(chat.requests.size()==textCount+unsigned(deathOwn));if(deathOwn)assert(chat.requests.back().speaker==100207&&chat.requests.back().group==2);
 count=s.calls.size();ai.JustDied(victim);SpellInfo freeze;ai.SpellHit(victim,&freeze);assert(s.calls.size()==count+(trap?0u:1u)&&s.actor.kills==unsigned(trap));
 // A late first trap after a modeled ordinary death can request DIE once;
 // its own dead guard and nonrepeatable DEATH still prevent duplicate credit.
 criteria=0;for(auto const& c:s.calls)if(c.action==205)++criteria;assert(criteria==1&&!s.errors);
}
int main(int argc,char** argv){
 if(argc!=4)return 1;Texts(argv[3]);auto original=Read(argv[1],100207),repaired=Read(argv[2],100207);assert(original.size()==7&&repaired.size()==7);
 Player player;Creature pet,guardian,npc;pet.entry=9001;guardian.entry=9002;npc.entry=9003;
 unsigned cases=0;for(Unit* victim:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)})for(bool trap:{false,true}){Check(original,victim,false,trap);Check(repaired,victim,true,trap);cases+=2;}
 // One-hit kills never visit the health threshold. Native DEATH->LINK4 still
 // speaks as100207 and retains one original closest-player criterion request.
 for(Unit* killer:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian)}){chat.requests.clear();auto s=Script(repaired,100207,killer);SmartAI ai{&s,&s.actor};ai.JustDied(killer);assert(s.calls.size()==2&&s.calls[0].action==205&&s.calls[1].action==1&&s.calls[1].parameter==2&&chat.requests.size()==1&&chat.requests[0].speaker==100207&&!s.errors);}
 for(unsigned mode=0;mode<3;++mode){chat.requests.clear();auto s=Script(repaired,100207,&pet);conditions.allow=mode!=0;s.actor.expired=mode==1;if(mode==2)s.me=nullptr;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(chat.requests.empty());if(mode<2)assert(s.calls.empty());}
 conditions.allow=true;
 std::printf("PASS native Mist Warder event/TALK/SELF/VICTIM/INVOKER/CreatureTextMgr and linked real SmartAI SpellHit/JustDied/DIE: %u original/repaired player-pet-guardian-NPC-null ordinary/trap cases,51/50 threshold,one-hit,repeated hits/deaths,invalid/missing actor; original205 request retained once. Kill/credit/range/cast/chat packet endpoints controlled; no full encounter/client certification.\n",cases);
}
