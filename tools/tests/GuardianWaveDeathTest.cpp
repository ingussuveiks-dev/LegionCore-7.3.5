// Extracted native SmartAI DEATH/LINK dispatch, source22 grouping/predicates,
// closest-player/creature predicates, CAST and player-owned criteria dispatch.
// Arena visits, spell results, summon/data requests and scenario advancement
// are controlled endpoints, not a complete game/client traversal.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using uint32=uint32_t;
#include "Constants.inc"
using TempSummonType=uint32;using CriteriaTypes=uint32;using SpellCastResult=uint32;
constexpr uint32 SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_FAILED_SPELL_IN_PROGRESS=2;
constexpr uint32 CONDITION_NEAR_CREATURE=29,CONDITION_SCENARION_STEP=44;
struct Scenario {uint32 id=990,step=2;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}} scenario;
struct Map {bool instance=true;uint32 id=1;Map* ToInstanceMap(){return instance?this:nullptr;}uint32 GetInstanceId()const{return id;}} origin,other;
using InstanceMap=Map;
struct ScenarioMgr {bool available=true;Scenario* GetScenario(uint32 id){return available&&id==1?&scenario:nullptr;}} scenarioMgr;auto sScenarioMgr=&scenarioMgr;
struct Unit;using Creature=Unit;using Player=Unit;using WorldObject=Unit;
std::vector<Unit*> arena;bool failCast=false,advance=true,failSummon=false;uint32 criterionCalls=0,criterionAsset=0;Unit* criterionRecipient=nullptr;Unit* criterionInvoker=nullptr;
struct FakeAI {void SetCombatMove(bool){}void AttackStart(Unit*){assert(false);}};
struct Unit {
 bool player=false,alive=true,expired=false,world=true,m_IsInKillingProcess=false;uint32 entry=0,phase=1;float x=0;Unit* owner=nullptr;Map* map=&origin;FakeAI ai;
 std::vector<uint32> spawnRequests;Unit* SummonCreature(uint32 id,float,float,float,float,TempSummonType type,uint32 duration){assert(type==8&&!duration);spawnRequests.push_back(id);return failSummon?nullptr:this;}
 void GetPosition(float& a,float& b){a=x;b=0;}float GetPositionZ(){return 0;}
 Unit* ToPlayer(){return player?this:nullptr;}Unit* ToUnit(){return this;}Unit* GetCharmerOrOwnerPlayerOrPlayerItself(){return player?this:owner&&owner->player?owner:nullptr;}
 Unit* GetGUID(){return this;}bool IsUnit()const{return true;}bool IsAlive()const{return alive;}bool isDead(bool)const{return !alive;}bool IsQuestObjectExpired()const{return expired;}bool isInCombat()const{return false;}
 bool InInstance()const{return map->instance;}Map* GetMap(){return map;}uint32 GetEntry()const{return entry;}
 bool IsWithinDistInMap(Unit const* u,float range)const{return u&&world&&u->world&&u->map==map&&u->phase==phase&&GetDistance(u)<=range;}
 float GetDistance(Unit const* u)const{return std::fabs(x-u->x);}FakeAI* AI(){return &ai;}
 Unit* FindNearestCreature(uint32,float,bool);Unit* SelectNearestPlayer(float)const;
 bool HasAura(uint32)const{return false;}void InterruptNonMeleeSpells(bool){}void GetPosition(float& a,float& b,float& c){a=x;b=c=0;}
 SpellCastResult CastSpell(Unit*,uint32 spell,bool){assert(spell==208568);return failCast?SPELL_FAILED_BAD_TARGETS:SPELL_CAST_OK;}
 void CastSpell(float,float,float,uint32,bool){assert(false);}
 void UpdateAchievementCriteria(CriteriaTypes type,uint32 asset,uint32 v2,uint32 v3,Unit* invoker,bool forced){assert(type==92&&!v2&&!v3&&forced);++criterionCalls;criterionAsset=asset;criterionRecipient=this;criterionInvoker=invoker;if(advance&&scenario.id==990&&asset==48104+scenario.step)++scenario.step;}
};
namespace Trinity {
#include "NearCreatureClass.inc"
#include "NearPlayerClass.inc"
#include "NearCreature.inc"
#include "NearPlayer.inc"
 template<class Check> struct PlayerLastSearcher {Unit*& result;Check& check;PlayerLastSearcher(Unit const*,Unit*& r,Check& c):result(r),check(c){}void Visit(Unit* u){if(u->player&&check(u))result=u;}};
 template<class Search> void VisitNearbyObject(Unit const*,float,Search& s){for(auto u:arena)s.Visit(u);}
}
Unit* Unit::FindNearestCreature(uint32 id,float range,bool aliveState){Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck check(*this,id,aliveState,range);Unit* result=nullptr;for(auto u:arena)if(!u->player&&check(u))result=u;return result;}
#include "SelectPlayer.inc"
#include "ClosestCreature.inc"
#include "CreditOwner.inc"
bool IsUnit(Unit* u){return u!=nullptr;}
struct GameObject {SpellCastResult CastSpell(Unit*,uint32){return SPELL_FAILED_BAD_TARGETS;}};struct SpellInfo{};
struct ConditionSourceInfo {Unit* targets[2];ConditionSourceInfo(Unit* a=nullptr,Unit* b=nullptr):targets{a,b}{}};
struct Rule {
 uint32 entry=0,group=0,ElseGroup=0,ConditionType=0,targetIndex=0,ConditionValue1=0,ConditionValue2=0,ReferenceId=0;bool NegativeCondition=false;
 bool isLoaded()const{return ConditionType>0;}
 bool Meets(ConditionSourceInfo& info)const{Unit* object=info.targets[targetIndex];if(!object)return false;bool condMeets=false;switch(ConditionType){
#include "NearCondition.inc"
#include "ScenarioCondition.inc"
 default:assert(false);
 }
#include "Negation.inc"
 return condMeets;}
};
using ConditionList=std::vector<Rule const*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
#define TC_LOG_DEBUG(...) ((void)0)
struct ConditionMgr {
 std::vector<Rule> rules;ConditionReferenceContainer ConditionReferenceStore;
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(WorldObject*,ConditionList const&);bool IsObjectMeetToConditions(WorldObject*,WorldObject*,ConditionList const&);
 ConditionList GetConditionsForSmartEvent(uint32 entry,uint32 id,uint32 source)const{ConditionList out;if(!source)for(auto const& r:rules)if(r.entry==entry&&r.group==id+1)out.push_back(&r);return out;}
 bool IsObjectMeetingSmartEventConditions(uint32 entry,uint32 id,uint32 source,Unit* unit,Unit* baseObject)const{
#include "SmartTargetBinding.inc"
 return IsObjectMeetToConditions(sourceInfo,GetConditionsForSmartEvent(entry,id,source));}
} conditionMgr;auto sConditionMgr=&conditionMgr;
#include "ConditionGroups.inc"
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder {
 uint32 entryOrGuid=0,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,parameter=0,second=0;bool active=true,runOnce=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;} event;
 struct {struct {uint32 creature=0,type=0,duration=0,attackInvoker=0,phaseByTarget=0;} summonCreature;struct {uint32 spell=0,flags=0;} cast;struct {uint32 type=0,misc1=0,misc2=0,misc3=0;} achievementCriteria;} action;
 struct {float x=0,y=0,z=0,o=0;struct {uint32 dist=0;} playerDistance;} target;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}uint32 GetTargetType()const{return targetType;}
};
bool roll_chance_i(uint32){return true;}
struct SmartScript {
 std::vector<SmartScriptHolder> mEvents;Unit* me=nullptr;GameObject* go=nullptr;Unit* mLastInvoker=nullptr;unsigned errors=0,retries=0;std::vector<std::pair<uint32,uint32>> data;
 Unit* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker;}bool IsInPhase(uint32)const{return true;}void RecalcTimer(SmartScriptHolder&,uint32 lo,uint32 hi){assert(lo==500&&hi==500);++retries;}
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){auto l=new ObjectList();switch(e.GetTargetType()){
 case 1:if(me)l->push_back(me);break;
#include "PlayerTarget.inc"
 default:assert(false);
 }if(l->empty()){delete l;return nullptr;}return l;}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
 void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
#define TC_LOG_ERROR(...) ++errors
#define ENSURE_AI(type,ai) (ai)
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LinkEvent.inc"
#include "NoParamEvents.inc"
 default:break;
 }
}
#include "ActionPrefix.inc"
 switch(e.GetActionType()){
#include "CastAction.inc"
#include "CreditAction.inc"
 case 12:{assert(e.targetType==8);
#include "PositionSummon.inc"
 break;}
 case 45:assert(e.parameter==2&&e.second==2);data.push_back({2,2});break; // controlled Spirit endpoint
 default:assert(false);
 }
#include "LinkTail.inc"
constexpr uint32 SMART_ESCORT_ESCORTING=1,DONE=1,ENCOUNTER_FRAME_DISENGAGE=1,ENCOUNTER_FRAME_INSTANCE_END=2;
struct Instance {void SetBossState(uint32,uint32){}void SaveToDB(){}void SendEncounterUnit(uint32,Unit*){}};
struct SmartAI {SmartScript* script;Unit* me;Instance* instance=nullptr;uint32 _bossId=0;SmartScript* GetScript(){return script;}bool HasEscortState(uint32){return false;}void EndPath(bool){}void JustDied(Unit*);};
#include "JustDied.inc"
void Kill(Unit* victim,Unit* killer,SmartScript& script){
#include "DeathGuard.inc"
 // Controlled death endpoint follows the checked native ordering: victim dead
 // before SmartAI::JustDied; no duplicate/reentrant kill admission.
 victim->m_IsInKillingProcess=true;victim->alive=false;SmartAI ai{&script,victim};ai.JustDied(killer);victim->m_IsInKillingProcess=false;
}
static std::vector<std::vector<std::string>> Read(char const* path){std::ifstream file(path);assert(file);std::string line;std::vector<std::vector<std::string>> out;while(std::getline(file,line)){std::stringstream stream(line);std::string cell;std::vector<std::string> row;while(std::getline(stream,cell,'\t'))row.push_back(cell);if(!line.empty()&&line.back()=='\t')row.push_back("");out.push_back(row);}return out;}
static std::vector<SmartScriptHolder> Actions(char const* path){std::vector<SmartScriptHolder> out;for(auto const& f:Read(path)){assert(f.size()==31);auto n=[&](size_t i){return uint32(std::stoul(f[i]));};SmartScriptHolder e;e.entryOrGuid=n(0);e.source_type=n(1);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.summonCreature={n(15),n(16),n(17),n(18),n(19)};e.target.x=std::stof(f[26]);e.target.y=std::stof(f[27]);e.target.z=std::stof(f[28]);e.target.o=std::stof(f[29]);e.action.cast={n(15),n(16)};e.action.achievementCriteria={n(15),n(16),n(17),n(18)};e.targetType=n(21);e.target.playerDistance={n(22)};out.push_back(e);}return out;}
static std::vector<Rule> Rules(char const* path){std::vector<Rule> out;for(auto const& f:Read(path)){assert(f.size()==14);auto n=[&](size_t i){return uint32(std::stoul(f[i]));};if(n(2)!=101384&&n(2)!=101388&&n(2)!=101389)continue;Rule r;r.entry=n(2);r.group=n(1);r.ElseGroup=n(4);r.ConditionType=n(5);r.targetIndex=n(6);r.ConditionValue1=n(7);r.ConditionValue2=n(8);r.NegativeCondition=n(10)!=0;out.push_back(r);}return out;}
static SmartScript Script(std::vector<SmartScriptHolder> const& rows,Unit& actor){SmartScript s;s.me=&actor;for(auto const& e:rows)if(e.entryOrGuid==actor.entry&&!e.source_type)s.mEvents.push_back(e);return s;}
static void Reset(uint32 step){scenario={990,step};scenarioMgr.available=true;origin.instance=true;origin.id=1;other.id=2;arena.clear();criterionCalls=criterionAsset=0;criterionRecipient=criterionInvoker=nullptr;failCast=false;failSummon=false;advance=true;}
int main(int argc,char** argv){
 if(argc!=5)return 1;auto old=Actions(argv[1]),fixed=Actions(argv[2]);auto oldRules=Rules(argv[3]),fixedRules=Rules(argv[4]);assert(fixedRules.size()==oldRules.size()+4);
 uint32 runs=0;
 for(uint32 step=2;step<=5;++step){uint32 list=step==2?101742:10174198+step;std::vector<uint32> wave;for(auto const& e:fixed)if(e.source_type==9&&e.entryOrGuid==list&&e.actionType==12)wave.push_back(e.parameter);assert(!wave.empty());std::sort(wave.begin(),wave.end());
  do{for(uint32 killerType=0;killerType<3;++killerType)for(bool rejectCast:{false,true})for(bool synchronous:{false,true}){
   Reset(step);conditionMgr.rules=fixedRules;failCast=rejectCast;advance=synchronous;
   Unit player;player.player=true;Unit pet;pet.owner=&player;Unit guardian=pet;Unit* killer=killerType==0?&player:killerType==1?&pet:&guardian;
   std::vector<Unit> mobs(wave.size());arena.push_back(&player);for(size_t i=0;i<wave.size();++i){mobs[i].entry=wave[i];arena.push_back(&mobs[i]);}
   for(size_t i=0;i<mobs.size();++i){auto script=Script(fixed,mobs[i]);Kill(&mobs[i],killer,script);assert(!script.errors);assert(criterionCalls==(i+1==mobs.size()?1u:0u));if(!mobs[i].alive)Kill(&mobs[i],killer,script);assert(criterionCalls==(i+1==mobs.size()?1u:0u));}
   assert(criterionAsset==48104+step&&criterionRecipient==&player&&criterionInvoker==killer&&scenario.step==step+uint32(synchronous));++runs;
  }}while(std::next_permutation(wave.begin(),wave.end()));
 }
 // Original counterfactual: another living Shadowstalker is omitted from
 // every one of its four credit groups, despite actual multi-actor waves.
 for(uint32 step=2;step<=5;++step){Reset(step);Unit player;player.player=true;Unit dying,remaining;dying.entry=remaining.entry=101388;arena={&player,&dying,&remaining};conditionMgr.rules=oldRules;auto before=Script(old,dying);Kill(&dying,&player,before);assert(criterionCalls==1&&scenario.step==step+1);assert(before.errors==unsigned(step>2));
  Reset(step);dying.alive=true;arena={&player,&dying,&remaining};conditionMgr.rules=fixedRules;auto after=Script(fixed,dying);Kill(&dying,&player,after);assert(!criterionCalls&&!after.errors&&scenario.step==step);
 }
 // All original gates remain: null invoker, no live nearby player, wrong
 // scenario/stage/map, expired actor and still-living enemy block progress.
 for(uint32 mode=0;mode<10;++mode){Reset(3);conditionMgr.rules=fixedRules;Unit player;player.player=true;Unit actor;actor.entry=101388;Unit enemy;enemy.entry=101384;arena={&player,&actor};Unit* killer=&player;
  switch(mode){case 0:killer=nullptr;break;case 1:player.alive=false;break;case 2:player.x=51;break;case 3:scenario.id=1037;break;case 4:scenario.step=6;break;case 5:origin.instance=false;break;case 6:actor.map=&other;break;case 7:actor.expired=true;break;case 8:arena.push_back(&enemy);break;case 9:scenarioMgr.available=false;break;}
  auto s=Script(fixed,actor);Kill(&actor,killer,s);assert(!criterionCalls&&!s.errors);
 }
 // Existing radius/map/phase boundaries remain local; do not turn this SQL
 // into a global wave counter or silently expand its original scope.
 for(uint32 mode=0;mode<3;++mode){Reset(3);conditionMgr.rules=fixedRules;Unit p;p.player=true;Unit a;a.entry=101388;Unit b;b.entry=101388;if(mode==0)b.x=101;if(mode==1)b.map=&other;if(mode==2)b.phase=2;arena={&p,&a,&b};auto s=Script(fixed,a);Kill(&a,&p,s);assert(criterionCalls==1&&!s.errors);}
 // Native positive helper LINKs survive: both summon attempts (even failed
 // endpoints) and Spirit DATA2/2 occur; unrelated death-cast failure cannot
 // block an independent criterion. A naive LINK61 conversion loses it.
 for(bool failure:{false,true}){Reset(2);conditionMgr.rules=fixedRules;Unit p;p.player=true;Unit a;a.entry=101388;arena={&p,&a};auto s=Script(fixed,a);failSummon=failure;Kill(&a,&p,s);assert((a.spawnRequests==std::vector<uint32>({101393,101754})&&s.data==std::vector<std::pair<uint32,uint32>>({{2,2}})&&criterionCalls==1&&!s.errors));}
 Reset(4);conditionMgr.rules=fixedRules;Unit p;p.player=true;Unit a;a.entry=101389;arena={&p,&a};failCast=true;auto independent=Script(fixed,a);Kill(&a,&p,independent);assert(criterionCalls==1&&independent.retries==1&&!independent.errors);
 Reset(4);conditionMgr.rules=fixedRules;failCast=true;a.alive=true;arena={&p,&a};auto naive=Script(old,a);for(auto& e:naive.mEvents)if(e.event_id==1||e.event_id==2)e.type=SMART_EVENT_LINK;Kill(&a,&p,naive);assert(!criterionCalls&&naive.retries==1);
 Reset(3);conditionMgr.rules=fixedRules;a.entry=101388;a.alive=true;a.m_IsInKillingProcess=true;arena={&p,&a};auto reentrant=Script(fixed,a);Kill(&a,&p,reentrant);assert(a.alive&&!criterionCalls);Kill(nullptr,&p,reentrant);assert(!criterionCalls);
 std::printf("PASS native Guardian DEATH/LINK/condition29+44/nearest predicates/CAST/criteria owner dispatch: %u complete distinct-species wave kill orders x player/pet/guardian/cast-result/stage timing; original premature-credit counterfactual; exactly once, invalid actors/recipients/range/map/phase, helper links and failed cast independence. Arena/summon/data/scenario endpoints controlled; actual client traversal remains separate.\n",runs);
}
