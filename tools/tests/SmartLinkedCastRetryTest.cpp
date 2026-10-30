// Production SmartAI cast/once/LINK dispatch and detached retry queue. World,
// spell completion, criteria storage and summon/door endpoints are controlled.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <set>
#include <tuple>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using int64=int64_t;
#include "RetryConstants.inc"
constexpr uint32 UNIT_STATE_CASTING=1;
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS};
uint32 errors=0,warnings=0;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_WARN(...) (++warnings)
#define ENSURE_AI(type,ai) (ai)
struct ObjectGuid{uint32 value=0;static ObjectGuid const Empty;bool IsEmpty()const{return !value;}bool operator==(ObjectGuid b)const{return value==b.value;}bool operator!=(ObjectGuid b)const{return value!=b.value;}bool operator<(ObjectGuid b)const{return value<b.value;}};ObjectGuid const ObjectGuid::Empty{};
struct SpellInfo{uint32 Id=199728,attributes=0;bool HasAttribute(uint32 mask)const{return (attributes&mask)!=0;}SpellInfo const*GetMisc()const{return this;}struct{uint32 SchoolMask=127;}MiscData;};
struct SpellMgr{std::map<uint32,SpellInfo>rows{{199728,{199728}},{199770,{199770}},{187027,{187027,SPELL_ATTR0_CASTABLE_WHILE_DEAD}}};SpellInfo const*GetSpellInfo(uint32 id){auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;}}spells;auto*sSpellMgr=&spells;
struct Unit;using Creature=Unit;using WorldObject=Unit;
struct SmartScript;
struct CreatureAI{
#include "RetryCorpseDefault.inc"
 virtual ~CreatureAI()=default;
};
struct SmartAI:CreatureAI{SmartScript*script=nullptr;SmartScript*GetScript(){assert(script);return script;}void UpdateCorpseAI(uint32)override;void SetCombatMove(bool){assert(false);}};
struct Unit{uint32 guid=1;bool world=true,alive=true,expired=false,busy=false;SmartAI ai;std::vector<uint32>requests,accepted;std::map<uint32,uint32>reject;std::function<void(uint32)>onCast;
 uint32 unitUpdates=0;std::function<void()>onUpdate;void Update(uint32){++unitUpdates;if(onUpdate)onUpdate();}
 ObjectGuid GetGUID()const{return {guid};}bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}bool isInCombat()const{return false;}bool IsUnit()const{return true;}Unit*ToUnit(){return this;}bool HasAura(uint32)const{return false;}void InterruptNonMeleeSpells(bool){}SmartAI*AI(){return &ai;}bool HasUnitState(uint32)const{return busy;}void GetPosition(float&a,float&b,float&c){a=b=c=0;}
 SpellCastResult CastSpell(Unit*target,uint32 id,bool triggered){assert(target==this&&!triggered);requests.push_back(id);if(!alive&&!spells.GetSpellInfo(id)->HasAttribute(SPELL_ATTR0_CASTABLE_WHILE_DEAD))return SPELL_FAILED_BAD_TARGETS;if(busy)return SPELL_FAILED_SPELL_IN_PROGRESS;if(reject[id]){--reject[id];return SPELL_FAILED_BAD_TARGETS;}accepted.push_back(id);if(onCast)onCast(id);return SPELL_CAST_OK;}
 void CastSpell(float,float,float,uint32,bool){assert(false);}
};
struct GameObject{uint32 guid=3;ObjectGuid GetGUID()const{return {guid};}SpellCastResult CastSpell(Unit*,uint32){assert(false);return SPELL_FAILED_BAD_TARGETS;}};
std::map<uint32,Unit*>units;std::map<uint32,GameObject*>objects;
namespace ObjectAccessor{Unit*GetUnit(Unit&,ObjectGuid id){auto i=units.find(id.value);return i==units.end()||!i->second->world?nullptr:i->second;}GameObject*GetGameObject(Unit&,ObjectGuid id){auto i=objects.find(id.value);return i==objects.end()?nullptr:i->second;}}
struct ConditionSourceInfo{ConditionSourceInfo(Unit*,Unit*){}};using ConditionList=int;
struct Conditions{bool allow=true;int GetConditionsForSmartEvent(int64,uint32,uint32){return 0;}bool IsObjectMeetToConditions(ConditionSourceInfo&,int){return allow;}bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,Unit*){return allow;}}conditions;auto*sConditionMgr=&conditions;
struct SmartScriptHolder{int64 entryOrGuid=101325;uint32 source_type=0,event_id=0,link=0,actionType=0,targetType=1,timer=0,parameter=0;bool active=true,runOnce=false,enableTimed=false;
 struct{uint32 type=0,event_flags=0,event_phase_mask=0,event_chance=100;struct{uint32 min=0,max=0,repeatMin=0,repeatMax=0;}minMaxRepeat;struct{uint32 spell=0,school=0,cooldownMin=0,cooldownMax=0;}spellHit;}event;
 struct{struct{uint32 spell=0,flags=0;}cast;}action;struct{float x=0,y=0,z=0;struct{uint32 dist=0;}playerDistance;}target;
 uint32 GetEventType()const{return event.type;}uint32 GetScriptType()const{return source_type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}
};using SmartAIEventList=std::vector<SmartScriptHolder>;using ObjectList=std::vector<Unit*>;
bool roll_chance_i(uint32){return true;}uint32 urand(uint32 a,uint32 b){assert(a==b);return a;}
struct SmartScript{Unit*me=nullptr;GameObject*go=nullptr;ObjectGuid mLastInvoker;SmartAIEventList mEvents,mTimedActionList;uint32 credit=0,baseRequests=0,lastVar0=0,lastVar1=0,lastSpell=0;bool lastBvar=false;Unit*lastUnit=nullptr;GameObject*lastGob=nullptr;
#include "RetryState.inc"
 Unit*GetBaseObject(){return me;}Unit*GetLastInvoker(){return me?ObjectAccessor::GetUnit(*me,mLastInvoker):nullptr;}bool IsInPhase(uint32){return true;}bool IsUnit(Unit*u){return u!=nullptr;}void SetPhase(int){}void ResetBaseObject(){}
 ObjectList*GetTargets(SmartScriptHolder const&e,Unit*){assert(e.targetType==SMART_TARGET_SELF);return me?new ObjectList{me}:nullptr;}
 SmartScriptHolder FindLinkedEvent(uint32);void InitTimer(SmartScriptHolder&);void RecalcTimer(SmartScriptHolder&,uint32,uint32);void UpdateTimer(SmartScriptHolder&,uint32);void OnReset();void OnCorpseUpdate(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
#include "RetryQueue.inc"
#include "RetryBegin.inc"
#include "RetryFinish.inc"
#include "RetryUpdate.inc"
#include "RetryReset.inc"
#include "RetryCorpseUpdate.inc"
#include "RetryCorpseAI.inc"
constexpr uint32 CORPSE=1,ALIVE=2;
struct CorpseDriver:Unit{uint32 m_deathState=CORPSE;bool IsAIEnabled=true,m_AI_locked=false;CreatureAI*i_AI=nullptr;CreatureAI*AI(){return i_AI;}void Tick(uint32 diff){do{
#include "RetryCorpseDispatch.inc"
 }while(false);}};
#include "RetryOnReset.inc"
#include "RetryFindLink.inc"
#include "RetryDispatch.inc"
#include "RetryEventPrefix.inc"
 switch(e.GetEventType()){
#include "RetryUpdateEvents.inc"
#include "RetrySpellHit.inc"
 default:assert(false);}
}
#include "RetryActionPrefix.inc"
 switch(e.GetActionType()){
#include "RetryCast.inc"
 case 205:assert(e.parameter==48172);++credit;break;
 case 12:assert(e.parameter==101360);++baseRequests;lastVar0=var0;lastVar1=var1;lastBvar=bvar;lastSpell=spell?spell->Id:0;lastUnit=unit;lastGob=gob;break;
 default:assert(false);}
#include "RetryLinkTail.inc"
#include "RetryInitTimer.inc"
#include "RetryRecalcTimer.inc"
bool IsSpellValid(SmartScriptHolder&,uint32 id){return id==199770||id==187027;}
bool Validate(SmartScriptHolder&e){switch(e.GetActionType()){
#include "RetryValidation.inc"
 default:return false;}return true;}
static SmartAIEventList Read(char const*path){std::ifstream f(path);assert(f);SmartAIEventList out;std::string line;while(std::getline(f,line)){std::stringstream s(line);std::vector<uint32>r;uint32 n;while(s>>n)r.push_back(n);assert(r.size()==18);SmartScriptHolder e;e.entryOrGuid=r[0];e.source_type=r[1];e.event_id=r[2];e.link=r[3];e.event.type=r[4];e.event.event_phase_mask=r[5];e.event.event_chance=r[6];e.event.event_flags=r[7];e.event.spellHit={r[8],r[9],r[10],r[11]};e.actionType=r[12];e.parameter=e.actionType==205?r[14]:r[13];e.action.cast={r[13],r[14]};e.targetType=r[15];out.push_back(e);}return out;}
static SmartScript Setup(Unit&stone,SmartAIEventList const&rows){errors=warnings=0;conditions={};units.clear();objects.clear();units[stone.guid]=&stone;SmartScript s;s.me=&stone;s.mEvents=rows;return s;}
static void Hit(SmartScript&s,Unit*owner){s.ProcessEventsFor(SMART_EVENT_SPELLHIT,owner,0,0,false,spells.GetSpellInfo(199728));}
int main(int argc,char**argv){assert(argc==3);auto old=Read(argv[1]),fixed=Read(argv[2]);Unit owner;owner.guid=2;
 {Unit stone;auto s=Setup(stone,old);stone.reject[199770]=1;Hit(s,&owner);for(uint32 i=0;i<20;++i)s.UpdateLinkedCastRetries(500);assert(s.credit==1&&s.baseRequests==0&&stone.requests==std::vector<uint32>{199770}&&s.mLinkedCastRetries.empty());}
 uint32 cases=0;for(uint32 first:{0u,1u,3u})for(uint32 second:{0u,1u,3u}){Unit stone;auto s=Setup(stone,fixed);units[owner.guid]=&owner;stone.reject[199770]=first;stone.reject[187027]=second;Hit(s,&owner);for(uint32 i=0;i<20;++i){Hit(s,&owner);s.UpdateLinkedCastRetries(500);}assert(s.credit==1&&s.baseRequests==1&&errors==1&&!warnings&&s.mLinkedCastRetries.empty());assert((stone.accepted==std::vector<uint32>{199770,187027}));assert(stone.requests.size()==2+first+second);++cases;}
 {Unit stone;auto s=Setup(stone,fixed);stone.busy=true;Hit(s,&owner);s.UpdateLinkedCastRetries(499);assert(stone.requests.size()==1&&s.credit==1&&!s.baseRequests);stone.busy=false;s.UpdateLinkedCastRetries(1);assert(s.baseRequests==1&&s.credit==1&&errors==1&&!warnings);++cases;}
 for(uint32 mode=0;mode<8;++mode){Unit stone;auto s=Setup(stone,fixed);stone.reject[199770]=1;Hit(s,&owner);if(mode==0)stone.alive=false;if(mode==1)stone.world=false;if(mode==2)stone.expired=true;if(mode==3)++stone.guid;if(mode==4)s.me=nullptr;if(mode==5)s.OnReset();if(mode==6)conditions.allow=false;if(mode==7)s.ResetLinkedCastRetries();s.UpdateLinkedCastRetries(500);assert(s.mLinkedCastRetries.empty()&&s.credit==1&&!s.baseRequests&&stone.requests.size()==1&&!errors&&!warnings);++cases;}
 for(bool busy:{false,true}){Unit stone;auto s=Setup(stone,fixed);stone.busy=busy;stone.reject[199770]=100;Hit(s,&owner);for(uint32 i=0;i<25;++i)s.UpdateLinkedCastRetries(500);assert(s.credit==1&&!s.baseRequests&&warnings==1&&!errors&&stone.requests.size()==11&&s.mLinkedCastRetries.empty());++cases;}
 {Unit stone;auto s=Setup(stone,fixed);stone.reject[199770]=1;Hit(s,&owner);s.UpdateLinkedCastRetries(6000);assert(stone.requests.size()==1&&warnings==1&&s.mLinkedCastRetries.empty());++cases;}
 {Unit stone;auto s=Setup(stone,fixed);units[owner.guid]=&owner;stone.reject[199770]=1;Hit(s,&owner);units.erase(owner.guid);s.UpdateLinkedCastRetries(500);assert(s.credit==1&&s.baseRequests==1&&errors==1&&!warnings);++cases;}
 {Unit stone;auto s=Setup(stone,fixed);stone.reject[199770]=1;Hit(s,&owner);stone.onCast=[&](uint32 id){if(id==199770)s.ResetLinkedCastRetries();};s.UpdateLinkedCastRetries(500);assert(s.mLinkedCastRetries.empty()&&s.credit==1&&!s.baseRequests&&s.mLinkedCastAttempts.empty());++cases;}
 // Runic Lightning199823 may really kill the Wardstone. The following native
 //187027 cosmetic explicitly admits a dead caster; retain that allowed retry.
 {Unit stone;auto s=Setup(stone,fixed);stone.ai.script=&s;CorpseDriver corpse;corpse.i_AI=&stone.ai;stone.reject[187027]=1;stone.onCast=[&](uint32 id){if(id==199770)stone.alive=false;};Hit(s,&owner);assert(!stone.alive&&!s.baseRequests&&s.mLinkedCastRetries.size()==1);corpse.IsAIEnabled=false;corpse.Tick(500);assert(!s.baseRequests&&s.mLinkedCastRetries.front().delay==500);corpse.IsAIEnabled=true;corpse.Tick(499);assert(!s.baseRequests&&s.mLinkedCastRetries.front().delay==1);corpse.Tick(1);assert(s.credit==1&&s.baseRequests==1&&errors==1&&!warnings&&s.mLinkedCastRetries.empty());assert((stone.accepted==std::vector<uint32>{199770,187027}));assert(!corpse.m_AI_locked&&corpse.unitUpdates==3);++cases;}
 {Unit stone;auto s=Setup(stone,fixed);stone.ai.script=&s;CorpseDriver corpse;corpse.i_AI=&stone.ai;stone.reject[187027]=1;Hit(s,&owner);corpse.onUpdate=[&]{corpse.m_deathState=ALIVE;};corpse.Tick(500);assert(!s.baseRequests&&s.mLinkedCastRetries.front().delay==500&&!corpse.m_AI_locked);++cases;}
 {CreatureAI ordinary;ordinary.UpdateCorpseAI(500);Unit stone;auto s=Setup(stone,fixed);stone.ai.script=&s;CorpseDriver corpse;corpse.i_AI=&stone.ai;corpse.Tick(500);assert(!s.credit&&!s.baseRequests&&stone.requests.empty());++cases;}
 for(uint32 mode=0;mode<3;++mode){Unit stone;auto s=Setup(stone,fixed);GameObject object;units[owner.guid]=&owner;objects[object.guid]=&object;s.QueueLinkedCastRetry(fixed[1],&owner,7,8,true,spells.GetSpellInfo(199728),&object);if(mode==1)units.erase(owner.guid);if(mode==2)objects.clear();stone.reject[199770]=1;s.UpdateLinkedCastRetries(500);assert(s.mLinkedCastRetries.size()==1);s.UpdateLinkedCastRetries(500);assert(!s.credit&&s.baseRequests==1&&s.lastVar0==7&&s.lastVar1==8&&s.lastBvar&&s.lastSpell==199728&&s.lastUnit==(mode==1?nullptr:&owner)&&s.lastGob==(mode==2?nullptr:&object));++cases;}
 {Unit stone;auto s=Setup(stone,fixed);auto e=fixed[1];e.link=0;for(uint32 i=0;i<16;++i){e.event_id=100+i;s.QueueLinkedCastRetry(e,nullptr,0,0,false,nullptr,nullptr);}assert(s.mLinkedCastRetries.size()==16);e.event_id=116;s.QueueLinkedCastRetry(e,nullptr,0,0,false,nullptr,nullptr);assert(errors==1&&s.mLinkedCastRetries.size()==16);errors=0;bool once=false;stone.onCast=[&](uint32){if(once)return;once=true;for(uint32 i=0;i<16;++i){e.event_id=200+i;s.QueueLinkedCastRetry(e,nullptr,0,0,false,nullptr,nullptr);}stone.reject[199770]=100;};s.UpdateLinkedCastRetries(500);assert(s.mLinkedCastRetries.size()==16&&errors==15&&!warnings);s.ResetLinkedCastRetries();assert(s.mLinkedCastRetries.empty());++cases;}
 {Unit stone;auto s=Setup(stone,fixed);stone.reject[199770]=1;Hit(s,&owner);for(uint32 i=0;i<20;++i)s.QueueLinkedCastRetry(fixed[1],&owner,0,0,false,spells.GetSpellInfo(199728),nullptr);assert(s.mLinkedCastRetries.size()==1);s.UpdateLinkedCastRetries(100);assert(s.mLinkedCastRetries.front().lifetime==5900&&s.mLinkedCastRetries.front().delay==400);s.QueueLinkedCastRetry(fixed[1],&owner,0,0,false,nullptr,nullptr);assert(s.mLinkedCastRetries.front().lifetime==5900);s.UpdateLinkedCastRetries(400);assert(s.credit==1&&s.baseRequests==1&&s.mLinkedCastRetries.empty());++cases;}
 for(uint32 mode=0;mode<10;++mode){auto e=fixed[1];assert(Validate(e));if(mode==0)e.event.type=SMART_EVENT_SPELLHIT;if(mode==1)e.targetType=SMART_TARGET_VICTIM;if(mode==2)e.source_type=SMART_SCRIPT_TYPE_TIMED_ACTIONLIST;if(mode==3)e.actionType=SMART_ACTION_INVOKER_CAST;if(mode==4)e.actionType=SMART_ACTION_CAST_CUSTOM;if(mode==5)e.action.cast.flags|=SMARTCAST_TRIGGERED;if(mode==6)e.action.cast.flags|=SMARTCAST_RETRY_HEALTH_THRESHOLD;if(mode==7)e.action.cast.flags|=SMARTCAST_INVOKER_PLAYER;if(mode==8)e.source_type=SMART_SCRIPT_TYPE_GAMEOBJECT;if(mode==9)e.event.event_flags=0;assert(!Validate(e));++cases;}
 for(bool deferred:{false,true}){Unit stone;auto s=Setup(stone,fixed);if(deferred)stone.reject[199770]=1;stone.onCast=[&](uint32 id){if(id==199770){auto nested=fixed[1];s.ProcessEvent(nested,&owner);}};Hit(s,&owner);if(deferred)s.UpdateLinkedCastRetries(500);assert(s.credit==1&&s.baseRequests==1&&errors==1&&!warnings&&s.mLinkedCastAttempts.empty()&&s.mLinkedCastRetries.empty());assert((stone.accepted==std::vector<uint32>{199770,187027}));auto same=fixed[1];s.ProcessEvent(same,&owner);assert(stone.accepted.size()==2&&s.baseRequests==1);++cases;}
 std::cout<<"PASS "<<cases<<" actual native linked-cast retry/validation cases: pinned no-flag stall, independent first/second rejection, once-only root/tail, busy/owner loss, reset/re-entry/expired/dead/replaced actor, finite10-attempt/6s diagnostics and reentrant reset. Spell effects, world, source22/criteria storage and base allocation remain controlled/separately regressed.\n";
}
