// Actual SmartAI SetData, SmartScript event/condition/LINK/list timers and
// action45/80/28. Map allocation, spell/conversation/criteria/summon sinks are
// controlled; this is not client traversal or a proof of those other systems.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using int64=int64_t;using uint8=uint8_t;
using SpellCastResult=uint32;
constexpr uint32 SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=13,SPELL_FAILED_SPELL_IN_PROGRESS=123;
#include "Constants.inc"
constexpr uint32 UNIT_STATE_CASTING=1,CONDITION_NEAR_CREATURE=29,MAX_CONDITION_TARGETS=3;
struct Unit;using Creature=Unit;using Player=Unit;using WorldObject=Unit;
struct SmartScript;struct SmartScriptHolder;struct Condition;struct GameObject;
struct SmartAI {SmartScript*script=nullptr;SmartScript*GetScript(){return script;}void SetData(uint32,uint32);void SetScript9(SmartScriptHolder&,uint32,Unit*);void SetCombatMove(bool){assert(false);}};
struct Map{}world,otherWorld;
std::vector<Unit*> grid;uint32 clockTime=0;bool failCast=false;
struct Receipt{uint32 time,action,payload;};std::vector<Receipt> receipts;
struct Aura{void ModStackAmount(int32){assert(false);}};
struct Unit {
 bool player=false,alive=true,inWorld=true,expired=false,casting=false;uint32 entry=0,phase=1;float x=0;Map*map=&world;Unit*owner=nullptr;SmartAI ai;uint32 auraRemovals=0;
 Unit*ToUnit(){return this;}Unit*ToPlayer(){return player?this:nullptr;}Unit*ToCreature(){return player?nullptr:this;}
 GameObject*ToGameObject(){return nullptr;}
 bool IsAlive()const{return alive;}bool IsInWorld()const{return inWorld;}uint32 GetEntry()const{return entry;}Unit*GetGUID(){return this;}bool IsQuestObjectExpired(){return expired;}
 Map*GetMap(){return map;}bool isInCombat(){return false;}bool HasUnitState(uint32){return casting;}SmartAI*AI(){return &ai;}
 float GetDistance(Unit const*u)const{return std::fabs(x-u->x);}bool IsWithinDistInMap(Unit const*u,float distance)const{return map==u->map&&phase==u->phase&&inWorld&&u->inWorld&&GetDistance(u)<=distance;}
 Unit*FindNearestCreature(uint32,float,bool);void RemoveAllAuras(){++auraRemovals;}Aura*GetAura(uint32){return nullptr;}void RemoveAurasDueToSpell(uint32){assert(false);}
 bool IsUnit(){return true;}bool HasAura(uint32){return false;}void InterruptNonMeleeSpells(bool){}
 void GetPosition(float&a,float&b,float&c){a=x;b=c=0;}
 SpellCastResult CastSpell(Unit*target,uint32 spell,bool triggered){assert(target==this&&!triggered&&spell==207752);receipts.push_back({clockTime,11,spell});return failCast?SPELL_FAILED_BAD_TARGETS:SPELL_CAST_OK;}
 void CastSpell(float,float,float,uint32,bool){assert(false);}
};
bool IsCreature(Unit*u){return u&&!u->player;}bool IsGameObject(Unit*){return false;}bool IsUnit(Unit*u){return u!=nullptr;}
struct SmartGameObjectAI;struct GameObject{SmartGameObjectAI*AI(){return nullptr;}SpellCastResult CastSpell(Unit*,uint32){return SPELL_FAILED_BAD_TARGETS;}};struct SpellInfo{};
#define TC_LOG_DEBUG(...) ((void)0)
uint32 scriptErrors=0;
#define TC_LOG_ERROR(...) (++scriptErrors)
#define ENSURE_AI(type,ai) (ai)
uint32 urand(uint32 a,uint32 b){assert(a==b);return a;}bool roll_chance_i(uint32){return true;}
namespace Trinity {
#include "NearClass.inc"
#include "NearBody.inc"
}
Unit*Unit::FindNearestCreature(uint32 id,float distance,bool live){Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck check(*this,id,live,distance);Unit*result=nullptr;for(auto u:grid)if(!u->player&&check(u))result=u;return result;}
#include "Closest.inc"
struct ConditionSourceInfo {Unit*mConditionTargets[3];Condition*mLastFailedCondition=nullptr;ConditionSourceInfo(Unit*,Unit*,Unit* =nullptr);};
#include "ConditionSource.inc"
struct Condition {uint32 ConditionType=29,ConditionTarget=0,ConditionValue1=0,ConditionValue2=50,ConditionValue3=0,SourceEntry=106699,SourceType=22,SourceGroup=1,ElseGroup=0,ReferenceId=0;bool NegativeCondition=true;
 bool isLoaded()const{return true;}bool Meets(ConditionSourceInfo&);
};
struct Hooks {bool allow=true;bool OnConditionCheck(Condition*,ConditionSourceInfo&){return allow;}} hooks;auto*sScriptMgr=&hooks;
#include "ConditionPrefix.inc"
 switch(ConditionType){
#include "NearCondition.inc"
 default:assert(false);}
#include "ConditionTail.inc"
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;using ConditionTypeContainer=std::map<uint32,ConditionList>;using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr {ConditionReferenceContainer ConditionReferenceStore;SmartEventConditionContainer SmartEventConditionStore;
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(ConditionSourceInfo&i,ConditionList const&l)const{return l.empty()||IsObjectMeetToConditionList(i,l);}
 ConditionList GetConditionsForSmartEvent(int64,uint32,uint32);
 bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*)const;
} nativeConditions;auto*sConditionMgr=&nativeConditions;
#include "ConditionGroups.inc"
#include "GetConditions.inc"
#include "EventConditions.inc"
struct SmartScriptHolder {
 int64 entryOrGuid=0;uint32 source_type=0,event_id=0,link=0,actionType=0,targetType=0,timer=0,payload=0;bool active=true,runOnce=false,enableTimed=true;
 struct Event {uint32 type=0,event_phase_mask=0,event_flags=0,event_chance=100;struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;struct{uint32 id=0,value=0,cooldownMin=0,cooldownMax=0;}dataSet;}event;
 struct Action {struct{uint32 field=0,data=0;}setData;struct{uint32 id=0,timerType=0;}timedActionList;struct{uint32 spell=0;int32 stack=0;}removeAura;struct{uint32 flags=0,spell=0;}cast;}action;
 struct{float x=0,y=0,z=0,o=0;}target;
 uint32 GetEventType()const{return event.type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}uint32 GetScriptType()const{return source_type;}
};
using SmartAIEventList=std::vector<SmartScriptHolder>;
struct ScriptMgr {SmartAIEventList rows;SmartAIEventList GetScript(uint32 id,uint32 type){SmartAIEventList out;for(auto const&r:rows)if(r.entryOrGuid==id&&r.source_type==type)out.push_back(r);return out;}}scriptMgr;auto*sSmartScriptMgr=&scriptMgr;
using ObjectList=std::list<Unit*>;
struct SmartScript {
 Unit*me=nullptr;GameObject*go=nullptr;Unit*mLastInvoker=nullptr;SmartAIEventList mEvents,mTimedActionList;bool isProcessingTimedActionList=false;uint32 listStarts=0;
 Unit*GetBaseObject(){return me;}Unit*GetLastInvoker(){return mLastInvoker;}bool IsInPhase(uint32){return true;}bool IsSmart(Unit*u){return u&&u->ai.script;}bool IsSmartGO(GameObject*){return false;}
 ObjectList*GetTargets(SmartScriptHolder const&e,Unit*){auto*l=new ObjectList;if(e.targetType==1)l->push_back(me);else if(e.targetType==19){for(auto u:grid)if(!u->player&&u->alive&&u->entry==e.payload&&me->IsWithinDistInMap(u,50)){l->push_back(u);break;}}else assert(false);if(l->empty()){delete l;return nullptr;}return l;}
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 SmartScriptHolder FindLinkedEvent(uint32);void SetScript9(SmartScriptHolder&,uint32);void InitTimer(SmartScriptHolder&);void RecalcTimer(SmartScriptHolder&,uint32,uint32);void UpdateTimer(SmartScriptHolder&,uint32);
 void Tick(uint32 diff){
#include "TimedTick.inc"
 }
};
// The unused GO alternative is still type checked in the native action80.
struct SmartGameObjectAI {void SetScript9(SmartScriptHolder&,uint32,Unit*){assert(false);}void SetData(uint32,uint32){assert(false);}};
#include "FindLink.inc"
#include "Dispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "UpdateEvents.inc"
#include "DataEvent.inc"
 case SMART_EVENT_DEATH:ProcessAction(e,unit,var0,var1,bvar,spell,gob);break;
 default:assert(false);}
}
#include "ActionPrefix.inc"
 switch(e.GetActionType()){
#include "DataAction.inc"
#include "ListAction.inc"
#include "RemoveAction.inc"
#include "CastAction.inc"
 // Retain actual payload and native timed ordering; endpoint services only.
 case 12:receipts.push_back({clockTime,12,e.payload});break;
 case 41:receipts.push_back({clockTime,41,0});break;
 case 205:case 206:receipts.push_back({clockTime,e.actionType,e.payload});break;
 default:assert(false);}
#include "LinkTail.inc"
#include "InitTimer.inc"
#include "RecalcTimer.inc"
#include "UpdateTimer.inc"
#include "SetList.inc"
#include "AISetData.inc"
#include "AISetList.inc"
static std::vector<std::vector<uint32>>Read(char const*path){std::ifstream f(path);assert(f);std::vector<std::vector<uint32>>out;std::string line;while(std::getline(f,line)){std::stringstream s(line);std::vector<uint32>r;uint32 n;while(s>>n)r.push_back(n);if(!r.empty())out.push_back(r);}return out;}
static SmartAIEventList Rows(char const*path){SmartAIEventList out;for(auto const&r:Read(path)){assert(r.size()==18);SmartScriptHolder e;e.entryOrGuid=r[0];e.source_type=r[1];e.event_id=r[2];e.link=r[3];e.event.type=r[4];e.event.event_phase_mask=r[5];e.event.event_chance=r[6];e.event.event_flags=r[7];e.event.minMaxRepeat={r[8],r[9],r[10],r[11]};e.event.dataSet={r[8],r[9],r[10],r[11]};e.actionType=r[12];e.action.setData={r[13],r[14]};e.action.timedActionList={r[13],r[14]};e.action.removeAura={r[13],int32(r[14])};e.action.cast={r[14],r[13]};e.targetType=r[15];e.payload=e.actionType==45?r[16]:e.actionType==205?r[14]:r[13];out.push_back(e);}return out;}
struct Run {
 Unit source,mobs[3],player,pet,guardian;SmartScript receiver,senders[3];std::vector<Condition>rules;
 Run(SmartAIEventList const&rows,std::vector<std::vector<uint32>>const&cs){grid.clear();receipts.clear();clockTime=scriptErrors=0;nativeConditions={};hooks={};failCast=false;scriptMgr.rows=rows;source.entry=106699;source.ai.script=&receiver;receiver.me=&source;receiver.mEvents=scriptMgr.GetScript(106699,0);for(uint32 i=0;i<3;++i){mobs[i].entry=106700+i;mobs[i].ai.script=&senders[i];senders[i].me=&mobs[i];senders[i].mEvents=scriptMgr.GetScript(mobs[i].entry,0);grid.push_back(&mobs[i]);}grid.push_back(&source);player.player=true;pet.owner=guardian.owner=&player;
  for(auto const&r:cs){assert(r.size()==8);Condition c;c.SourceGroup=r[0];c.ElseGroup=r[1];c.ConditionType=r[2];c.ConditionTarget=r[3];c.ConditionValue1=r[4];c.ConditionValue2=r[5];c.ConditionValue3=r[6];c.NegativeCondition=r[7]!=0;rules.push_back(c);}assert(rules.size()==3);for(auto&c:rules)nativeConditions.SmartEventConditionStore[{106699,0}][c.SourceGroup].push_back(&c);
 }
 void Die(uint32 i,Unit*killer){if(!mobs[i].alive)return;mobs[i].alive=false;senders[i].ProcessEventsFor(SMART_EVENT_DEATH,killer);}
 void Finish(){for(uint32 i=0;i<600;++i){clockTime+=100;receiver.Tick(100);}}
};
int main(int argc,char**argv){
 assert(argc==4);auto rows=Rows(argv[1]);auto old=Read(argv[2]),fixed=Read(argv[3]);assert(old.size()==3&&fixed.size()==3);for(uint32 i=0;i<3;++i){assert(old[i][3]==0&&fixed[i][3]==1);auto copy=fixed[i];copy[3]=0;assert(copy==old[i]);}
 {Run r(rows,old);for(uint32 i=0;i<3;++i)r.Die(i,&r.player);assert(r.receiver.mTimedActionList.empty()&&r.source.auraRemovals==0);}
 uint32 runs=0;std::vector<uint32>order{0,1,2};do{for(uint32 kind=0;kind<4;++kind){Run r(rows,fixed);Unit*k=kind==0?&r.player:kind==1?&r.pet:kind==2?&r.guardian:nullptr;for(uint32 i=0;i<3;++i){r.Die(order[i],k);assert(r.source.auraRemovals==(i==2?1u:0u));if(i<2)assert(r.receiver.mTimedActionList.empty());}assert(r.receiver.mTimedActionList.size()==9);r.source.ai.SetData(1,1);assert(r.source.auraRemovals==1);r.Finish();std::vector<std::pair<uint32,uint32>>actual;for(auto const&x:receipts)actual.push_back({x.action,x.payload});assert((actual==std::vector<std::pair<uint32,uint32>>{{206,2140},{206,1682},{206,1683},{206,1684},{11,207752},{12,106747},{205,49864},{205,50369},{41,0}}));assert(receipts[1].time-receipts[0].time>=12000&&receipts.back().time>=47000&&!scriptErrors);++runs;}}while(std::next_permutation(order.begin(),order.end()));
 for(uint32 mode=0;mode<8;++mode){Run r(rows,fixed);for(auto&m:r.mobs)m.alive=false;if(mode==0)r.mobs[0].alive=true;if(mode==1)r.mobs[1].alive=true;if(mode==2)r.mobs[2].alive=true;if(mode==3)r.source.expired=true;if(mode==4)hooks.allow=false;if(mode==5)r.rules[0].ConditionTarget=3;r.source.ai.SetData(mode==6?2:1,mode==7?2:1);assert(r.source.auraRemovals==0&&r.receiver.mTimedActionList.empty());}
 for(uint32 mode=0;mode<4;++mode){Run r(rows,fixed);for(auto&m:r.mobs)m.alive=false;r.mobs[0].alive=true;if(mode==0)r.mobs[0].x=50.01f;if(mode==1)r.mobs[0].map=&otherWorld;if(mode==2)r.mobs[0].phase=2;if(mode==3)r.mobs[0].entry=105799;r.source.ai.SetData(1,1);assert(r.source.auraRemovals==1);}
 {Run r(rows,fixed);for(auto&m:r.mobs)m.alive=false;r.mobs[0].alive=true;r.mobs[0].x=50;r.source.ai.SetData(1,1);assert(!r.source.auraRemovals);r.mobs[0].alive=false;r.source.ai.SetData(1,1);assert(r.source.auraRemovals==1);}
 // Native cast failure cannot turn an independently timed list into a LINK
 // chain. Native casting-in-progress deferral must hold the same list entry.
 {Run r(rows,fixed);for(uint32 i=0;i<3;++i)r.Die(i,&r.pet);failCast=true;r.Finish();assert(receipts.size()==9&&receipts[4].action==11&&receipts[5].action==12&&receipts[8].action==41&&!scriptErrors);}
 {Run r(rows,fixed);for(uint32 i=0;i<3;++i)r.Die(i,&r.guardian);r.source.casting=true;r.Finish();assert(receipts.size()==4);r.source.casting=false;r.Finish();assert(receipts.size()==9&&!scriptErrors);}
 // Missing receiver or an expired encounter never consumes the death into
 // synthesized progress. Re-entry recovery itself is a separate native path.
 for(uint32 mode=0;mode<3;++mode){Run r(rows,fixed);if(mode==0)r.source.alive=false;if(mode==1)r.source.map=&otherWorld;if(mode==2)r.source.phase=2;for(uint32 i=0;i<3;++i)r.Die(i,&r.player);assert(r.source.auraRemovals==0&&receipts.empty());}
 std::cout<<"PASS native null-invoker baseline blocks receiver; repaired3 source guards: "<<runs<<" full6-order x player/pet/guardian/environment runs, real DATA/LINK/list timers/aura removal, native rejected-cast and busy-cast deferral, repeated/negative/boundary/map/phase/missing-receiver guards and all9 preserved timed payloads. Allocation, combat, criteria/chat and client endpoints remain controlled.\n";
}
