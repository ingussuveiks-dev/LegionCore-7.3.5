// Actual SmartScript dispatcher/guards/LINK/CAST/close and personal spell-hit body.
// Cast scheduling and quest storage are controlled endpoints, not a live client/quest.
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;
#include "Constants.inc"
using QuestStatus=uint32;
constexpr uint32 QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_NONE=0,QUEST_STATUS_REWARDED=6;
constexpr uint32 CONDITION_QUESTTAKEN=9,CONDITION_OBJECT_ENTRY=31,CONDITION_ALIVE=36;
struct SmartScriptHolder;struct Unit;
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS};
struct ObjectGuid {Unit* unit=nullptr;};
struct Talk {unsigned closes=0;void SendCloseGossip(){++closes;}};
struct AI {void SetCombatMove(bool){}};
struct Unit {
 bool player=false,pet=false,alive=true,expired=false,combat=false;QuestStatus status=QUEST_STATUS_INCOMPLETE;Unit* owner=nullptr;::AI ai;Talk talk;Talk* PlayerTalkClass=&talk;
 unsigned casts=0,credits=0;uint32 lastSpell=0;bool lastTriggered=false;Unit* lastTarget=nullptr;SpellCastResult castResult=SPELL_CAST_OK;
 Unit* ToUnit(){return this;}Unit* ToPlayer(){return player?this:nullptr;}Unit* ToCreature(){return player?nullptr:this;}Unit* ToGameObject(){return nullptr;}
 bool IsCreature()const{return !player;}bool IsPlayer()const{return player;}bool IsUnit()const{return true;}bool isPet()const{return pet;}Unit* GetOwner(){return owner;}
 struct Threat {Unit* getHostilTarget(){return nullptr;}} threat;Threat& getThreatManager(){return threat;}
 QuestStatus GetQuestStatus(uint32 id)const{return id==41919?status:QUEST_STATUS_NONE;}
 uint32 GetTypeId()const{return player?4:3;}uint32 GetEntry()const{return player?0:105332;}
 bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}bool isInCombat()const{return combat;}
 ObjectGuid GetGUID(){return {this};}::AI* AI(){return &ai;}bool HasAura(uint32)const{return false;}
 void GetPosition(float& x,float& y,float& z)const{x=y=z=0;}float GetPositionZ()const{return 0;}
 void InterruptNonMeleeSpells(bool){}
 SpellCastResult CastSpell(Unit* target,uint32 id,bool triggered=false){++casts;lastSpell=id;lastTarget=target;lastTriggered=triggered;return castResult;}
 SpellCastResult CastSpell(float,float,float,uint32,bool){return castResult;}
 // Award storage is a controlled endpoint; native full quest caps/group rules aren't modeled.
 void KilledMonsterCredit(uint32 entry){if(entry==105332)++credits;}
};
bool IsUnit(Unit* unit){return unit!=nullptr;}bool IsPlayer(Unit* unit){return unit&&unit->player;}
using Player=Unit;using Creature=Unit;using GameObject=Unit;using ObjectList=std::vector<Unit*>;struct SpellInfo{};
struct Rule {uint32 group,kind,target,value,second;};using ConditionList=std::vector<Rule>;
struct ConditionSourceInfo {Unit* targets[2];ConditionSourceInfo(Unit* a,Unit* b):targets{a,b}{}};
bool Evaluate(Rule const& rule,ConditionSourceInfo const& info){
 Unit* object=info.targets[rule.target];if(!object)return false;
 uint32 ConditionValue1=rule.value,ConditionValue2=rule.second;bool condMeets=false;
 switch(rule.kind){
#include "QuestCondition.inc"
#include "PlayerCondition.inc"
#include "AliveCondition.inc"
 }
 return condMeets;
}
struct Conditions {
 ConditionList rules;
 ConditionList GetConditionsForSmartEvent(uint32 entry,uint32 id,uint32 source)const{ConditionList out;if(entry==105332&&source==0)for(auto r:rules)if(r.group==id+1)out.push_back(r);return out;}
 bool IsObjectMeetToConditions(ConditionSourceInfo const& info,ConditionList const& list)const{for(auto r:list)if(!Evaluate(r,info))return false;return true;}
 bool IsObjectMeetingSmartEventConditions(uint32 entry,uint32 id,uint32 source,Unit* unit,Unit* baseObject)const{
#include "SmartTargetBinding.inc"
  return IsObjectMeetToConditions(sourceInfo,GetConditionsForSmartEvent(entry,id,source));
 }
} conditions;Conditions* sConditionMgr=&conditions;
struct SmartScriptHolder {
 uint32 entryOrGuid=0,source_type=0,event_id=0,link=0,type=0,actionType=0,parameter=0,targetType=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;struct {uint32 sender=0,action=0,cooldownMin=0,cooldownMax=0;} gossip;} event;
 struct {struct {uint32 spell=0,flags=0;} cast;} action;
 struct {float x=0,y=0,z=0;} target;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetTargetType()const{return targetType;}uint32 GetScriptType()const{return source_type;}
};
bool roll_chance_i(uint32){return true;}
#define ENSURE_AI(type, value) (value)
#define TC_LOG_ERROR(...) ++errors
#define TC_LOG_DEBUG(...) ((void)0)
struct SmartScript {
 // This older scope has no linked-cast opt-in; unexpected use must fail.
 template<class... Args>void QueueLinkedCastRetry(Args const&...){assert(false);}
 template<class... Args>uint32 BeginLinkedCastAttempt(Args const&...){assert(false);return 0;}
 template<class... Args>bool FinishLinkedCastAttempt(Args const&...){assert(false);return false;}
 void UpdateLinkedCastRetries(uint32){}
 void ResetLinkedCastRetries(){}

 std::vector<SmartScriptHolder> mEvents;Unit actor;Unit* me=&actor;Unit* go=nullptr;ObjectGuid mLastInvoker;
 unsigned errors=0,creditCalls=0;
 Unit* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}bool IsInPhase(uint32)const{return true;}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){Unit* target=e.targetType==7?GetLastInvoker():nullptr;return target?new ObjectList{target}:nullptr;}
 void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LinkEvent.inc"
#include "SelectEvent.inc"
#include "NoParamEvents.inc"
 default:break;
 }
}
#include "ActionPrefix.inc"
 switch(e.GetActionType()){
#include "CastAction.inc"
#include "CloseAction.inc"
 case 33:++creditCalls;break;
 default:break;
 }
#include "LinkTail.inc"
static std::vector<std::vector<std::string>> Read(char const* path){
 std::ifstream file(path);std::string line;std::vector<std::vector<std::string>> out;
 while(std::getline(file,line)){std::stringstream stream(line);std::string field;std::vector<std::string> row;while(std::getline(stream,field,'\t'))row.push_back(field);if(!line.empty()&&line.back()=='\t')row.push_back("");out.push_back(row);}return out;
}
static std::vector<SmartScriptHolder> Actions(char const* path){
 std::vector<SmartScriptHolder> out;for(auto const& f:Read(path)){if(f.size()!=31)return {};auto n=[&](size_t i){return uint32(std::stoul(f[i]));};if(n(0)!=105332)continue;
 SmartScriptHolder e;e.entryOrGuid=n(0);e.source_type=n(1);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_flags=n(8);e.event.gossip={n(9),n(10),n(11),n(12)};e.actionType=n(14);e.parameter=n(15);e.action.cast={n(15),n(16)};e.targetType=n(21);out.push_back(e);}return out;
}
using SpellEffIndex=uint32;constexpr uint32 SPELL_EFFECT_HANDLE_HIT_TARGET=1;
struct SpellEffectInfo {int MiscValue=105332;};struct CreditInfo {SpellEffectInfo effect;SpellEffectInfo const* GetEffect(uint32,uint32)const{return &effect;}} creditInfo;
struct Spell {uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;Unit* unitTarget=nullptr;CreditInfo* m_spellInfo=&creditInfo;void EffectKillCreditPersonal(SpellEffIndex);};
#include "PersonalCredit.inc"
int main(int argc,char** argv){
 if(argc!=4)return 1;auto original=Actions(argv[1]),fixed=Actions(argv[2]);if(original.size()!=1||fixed.size()!=2)return 2;
 ConditionList rules;for(auto const& f:Read(argv[3])){if(f.size()!=14)return 3;rules.push_back({uint32(std::stoul(f[1])),uint32(std::stoul(f[5])),uint32(std::stoul(f[6])),uint32(std::stoul(f[7])),uint32(std::stoul(f[8]))});}if(rules.size()!=6)return 4;
 Unit first;first.player=true;Unit second;second.player=true;
 conditions.rules.clear();SmartScript old;old.mEvents=original;old.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&first,19470,0);if(old.creditCalls!=1||old.errors!=1||first.casts)return 5;
 conditions.rules=rules;
 SmartScript s;s.mEvents=fixed;
 s.ProcessEventsFor(SMART_EVENT_GOSSIP_HELLO,&first);if(first.talk.closes||first.casts||s.creditCalls)return 6;
 s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&first,19470,0);
 if(s.errors||s.creditCalls||first.talk.closes!=1||first.casts!=1||first.lastSpell!=208755||first.lastTarget!=&first||first.lastTriggered||first.credits)return 7;
 s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&second,19470,0);if(second.casts!=1||first.casts!=1||second.lastTarget!=&second)return 8;
 s.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&first,19470,0);if(first.casts!=2||first.talk.closes!=2||first.credits)return 9;
 for(unsigned mode=0;mode<9;++mode){SmartScript denied;denied.mEvents=fixed;Unit p;p.player=true;p.status=mode==0?QUEST_STATUS_NONE:mode==1?QUEST_STATUS_COMPLETE:mode==2?QUEST_STATUS_REWARDED:QUEST_STATUS_INCOMPLETE;if(mode==3)p.alive=false;if(mode==4){p.player=false;p.pet=true;p.owner=&first;}if(mode==5)p.player=false;if(mode==6)denied.actor.expired=true;Unit* invoker=mode==7?nullptr:&p;
  denied.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,invoker,mode==8?19471:19470,0);if(denied.creditCalls||p.casts||p.talk.closes||denied.errors)return 10;
 }
 for(SpellCastResult result:{SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_SPELL_IN_PROGRESS}){SmartScript failed;failed.mEvents=fixed;Unit p;p.player=true;p.castResult=result;failed.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&p,19470,0);if(p.casts!=1||p.talk.closes!=1||p.credits||failed.creditCalls||failed.errors)return 11;p.castResult=SPELL_CAST_OK;failed.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&p,19470,0);if(p.casts!=2||p.lastTriggered||p.credits)return 12;}
 // No cast hit => no credit. Only the actual personal spell HIT_TARGET effect invokes credit.
 Spell spell;spell.unitTarget=&first;spell.effectHandleMode=0;spell.EffectKillCreditPersonal(0);if(first.credits)return 13;
 spell.effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET;spell.EffectKillCreditPersonal(0);if(first.credits!=1||second.credits)return 14;
 Unit pet;pet.pet=true;pet.owner=&first;for(Unit* target:{&pet,&s.actor,static_cast<Unit*>(nullptr)}){spell.unitTarget=target;spell.EffectKillCreditPersonal(0);}if(first.credits!=1||second.credits||pet.credits)return 15;
 SmartScript unavailable;unavailable.mEvents=fixed;unavailable.me=nullptr;Unit p;p.player=true;unavailable.ProcessEventsFor(SMART_EVENT_GOSSIP_SELECT,&p,19470,0);if(p.casts||unavailable.errors)return 16;
 std::puts("PASS native search dispatch/LINK/CAST force-self and close-gossip, no immediate/group action33, normal non-triggered cast, own second-player/retry, failed-cast/no-hit/no-credit, native personal effect HIT_TARGET/player-only guards and refused inactive/dead/pet/guardian/null/wrong-menu/expired invokers. Spell engine timing/interruption and quest award storage are controlled, client pending.");
}
