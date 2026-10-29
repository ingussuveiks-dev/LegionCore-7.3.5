// Extracted native SmartAI DEATH/LINK dispatch, source22 grouping/predicates,
// closest-player/creature predicates, CAST, position summons and UPDATE timers.
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
 bool player=false,alive=true,expired=false,world=true,m_IsInKillingProcess=false,isGo=false;float health=100;uint32 entry=0,phase=1;float x=0,y=0,z=0;Unit* owner=nullptr;Map* map=&origin;FakeAI ai;
 std::vector<uint32> spawnRequests;Unit* SummonCreature(uint32 id,float a,float b,float c,float,TempSummonType type,uint32 duration){assert(type==7&&!duration);spawnRequests.push_back(id);if(failSummon)return nullptr;if(id==97386){static Unit boss;boss.entry=id;boss.x=a;boss.y=b;boss.z=c;boss.map=map;boss.phase=phase;boss.alive=true;arena.push_back(&boss);}return this;}
 void GetPosition(float& a,float& b){a=x;b=y;}float GetPositionZ(){return z;}
 Unit* ToPlayer(){return player?this:nullptr;}Unit* ToUnit(){return this;}Unit* GetCharmerOrOwnerPlayerOrPlayerItself(){return player?this:owner&&owner->player?owner:nullptr;}
 Unit* GetGUID(){return this;}bool IsUnit()const{return true;}bool IsAlive()const{return alive;}bool isDead(bool)const{return !alive;}bool IsQuestObjectExpired()const{return expired;}bool isInCombat()const{return false;}uint32 GetMaxHealth()const{return 100;}float GetHealthPct()const{return health;}bool HasUnitState(uint32)const{return false;}
 bool InInstance()const{return map->instance;}Map* GetMap(){return map;}uint32 GetEntry()const{return entry;}
 bool IsWithinDistInMap(Unit const* u,float range)const{return u&&world&&u->world&&u->map==map&&u->phase==phase&&GetDistance(u)<=range;}
 float GetDistance(Unit const* u)const{return std::sqrt((x-u->x)*(x-u->x)+(y-u->y)*(y-u->y)+(z-u->z)*(z-u->z));}FakeAI* AI(){return &ai;}
 Unit* FindNearestCreature(uint32,float,bool);Unit* SelectNearestPlayer(float)const;
 bool HasAura(uint32)const{return false;}void InterruptNonMeleeSpells(bool){}void GetPosition(float& a,float& b,float& c){a=x;b=y;c=z;}
 SpellCastResult CastSpell(Unit*,uint32 spell,bool){assert(spell==192357);return failCast?SPELL_FAILED_BAD_TARGETS:SPELL_CAST_OK;}
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
struct GameObject:Unit {SpellCastResult CastSpell(Unit*,uint32){return SPELL_FAILED_BAD_TARGETS;}};struct SpellInfo{uint32 Id=192344;struct Misc {struct {uint32 SchoolMask=4;} MiscData;} misc;Misc const* GetMisc()const{return &misc;}};
struct ConditionSourceInfo {Unit* targets[2];ConditionSourceInfo(Unit* a=nullptr,Unit* b=nullptr):targets{a,b}{}};
struct Rule {
 uint32 entry=0,group=0,source=0,ElseGroup=0,ConditionType=0,targetIndex=0,ConditionValue1=0,ConditionValue2=0,ReferenceId=0;bool NegativeCondition=false;
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
 ConditionList GetConditionsForSmartEvent(uint32 entry,uint32 id,uint32 source)const{ConditionList out;for(auto const& r:rules)if(r.source==source&&r.entry==entry&&r.group==id+1)out.push_back(&r);return out;}
 bool IsObjectMeetingSmartEventConditions(uint32 entry,uint32 id,uint32 source,Unit* unit,Unit* baseObject)const{
#include "SmartTargetBinding.inc"
 return IsObjectMeetToConditions(sourceInfo,GetConditionsForSmartEvent(entry,id,source));}
} conditionMgr;auto sConditionMgr=&conditionMgr;
#include "ConditionGroups.inc"
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder {
 uint32 entryOrGuid=0,source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0,parameter=0,second=0,timer=0;bool active=true,runOnce=false,enableTimed=false;
 struct {uint32 event_flags=0,event_phase_mask=0,event_chance=100;struct {uint32 min=0,max=0,repeatMin=0,repeatMax=0;} minMaxRepeat;struct {uint32 spell=0,school=0,cooldownMin=0,cooldownMax=0;} spellHit;} event;
 struct {struct {uint32 creature=0,type=0,duration=0,attackInvoker=0,phaseByTarget=0;} summonCreature;struct {uint32 spell=0,flags=0;} cast;struct {uint32 type=0,misc1=0,misc2=0,misc3=0;} achievementCriteria;} action;
 struct {float x=0,y=0,z=0,o=0;struct {uint32 dist=0;} playerDistance;} target;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}uint32 GetTargetType()const{return targetType;}
};
bool roll_chance_i(uint32){return true;}
struct SmartScript {
 std::vector<SmartScriptHolder> mEvents,mTimedActionList;uint32 mScriptType=0;Unit* me=nullptr;GameObject* go=nullptr;Unit* mLastInvoker=nullptr;unsigned errors=0,retries=0;std::vector<std::pair<uint32,uint32>> data;std::vector<uint32> requests;
 Unit* GetBaseObject(){return me?me:go;}void InstallEvents(){}void OnUpdate(uint32);void UpdateTimer(SmartScriptHolder&,uint32);Unit* GetLastInvoker(){return mLastInvoker;}bool IsInPhase(uint32)const{return true;}void RecalcTimer(SmartScriptHolder&,uint32,uint32);
 ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){auto l=new ObjectList();switch(e.GetTargetType()){
 case 1:if(GetBaseObject())l->push_back(GetBaseObject());break;
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
#include "UpdateEvent.inc"
#include "HealthEvent.inc"
#include "SpellHitEvent.inc"
 default:break;
 }
}
#include "ActionPrefix.inc"
 requests.push_back(e.actionType);
 switch(e.GetActionType()){
#include "CastAction.inc"
#include "CreditAction.inc"
 case 12:{assert(e.targetType==8);
#include "PositionSummon.inc"
 break;}
 case 45:assert(e.parameter==1&&e.second==1);data.push_back({1,1});break; // unchanged receiver endpoint
 case 81:assert(!e.parameter);break; // NPC flags endpoint
 case 50:assert(e.parameter==247612&&e.second==3600);break; // original pillar exit-prop endpoint
 default:assert(false);
 }
#include "LinkTail.inc"
uint32 urand(uint32 lo,uint32 hi){assert(lo<=hi);return lo;}
constexpr uint32 UNIT_STATE_CASTING=1;
#include "RecalcTimer.inc"
#include "UpdateTimer.inc"
#include "UpdateLoop.inc"
}
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
static std::vector<SmartScriptHolder> Actions(char const* path){std::vector<SmartScriptHolder> out;for(auto const& f:Read(path)){assert(f.size()==31);auto n=[&](size_t i){return uint32(std::stoul(f[i]));};SmartScriptHolder e;e.entryOrGuid=n(0);e.source_type=n(1);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);e.event.minMaxRepeat={n(9),n(10),n(11),n(12)};e.event.spellHit={n(9),n(10),n(11),n(12)};e.actionType=n(14);e.parameter=n(15);e.second=n(16);e.action.summonCreature={n(15),n(16),n(17),n(18),n(19)};e.target.x=std::stof(f[26]);e.target.y=std::stof(f[27]);e.target.z=std::stof(f[28]);e.target.o=std::stof(f[29]);e.action.cast={n(15),n(16)};e.action.achievementCriteria={n(15),n(16),n(17),n(18)};e.targetType=n(21);e.target.playerDistance={n(22)};out.push_back(e);}return out;}
static std::vector<Rule> Rules(char const* path){std::vector<Rule> out;for(auto const& f:Read(path)){assert(f.size()==14);auto n=[&](size_t i){return uint32(std::stoul(f[i]));};if(n(2)!=97168&&n(2)!=97436&&n(2)!=244431)continue;Rule r;r.entry=n(2);r.group=n(1);r.source=n(3);r.ElseGroup=n(4);r.ConditionType=n(5);r.targetIndex=n(6);r.ConditionValue1=n(7);r.ConditionValue2=n(8);r.NegativeCondition=n(10)!=0;out.push_back(r);}return out;}
static SmartScript Script(std::vector<SmartScriptHolder> const& rows,Unit& actor){SmartScript s;if(actor.isGo){s.me=nullptr;s.go=static_cast<GameObject*>(&actor);s.mScriptType=1;}else s.me=&actor;for(auto const& e:rows)if(e.entryOrGuid==actor.entry&&e.source_type==uint32(actor.isGo))s.mEvents.push_back(e);return s;}
static void Reset(uint32 step){scenario={1038,step};scenarioMgr.available=true;origin.instance=true;origin.id=1;other.id=2;arena.clear();criterionCalls=criterionAsset=0;criterionRecipient=criterionInvoker=nullptr;failCast=false;failSummon=false;advance=true;}
int main(int argc,char** argv){
 if(argc!=4)return 1;auto old=Actions(argv[1]),fixed=Actions(argv[2]);conditionMgr.rules=Rules(argv[3]);assert(conditionMgr.rules.size()==7);uint32 cases=0;
 for(uint32 entry:{97168u,97436u,244431u})for(uint32 invokerType=0;invokerType<4;++invokerType)for(bool reject:{false,true}){
  std::vector<uint32> oldRequests;
  for(bool repaired:{false,true}){
   Reset(1);failCast=reject;GameObject actor;actor.entry=entry;actor.isGo=entry==244431;Unit player;player.player=true;Unit pet;pet.owner=&player;Unit guardian=pet;Unit* invoker=invokerType==0?&player:invokerType==1?&pet:invokerType==2?&guardian:nullptr;arena={&actor,&player};
   auto script=Script(repaired?fixed:old,actor);SpellInfo spell;SMART_EVENT root=entry==97168?SMART_EVENT_HEALTH_PCT:entry==97436?SMART_EVENT_SPELLHIT:SMART_EVENT_GOSSIP_HELLO;
   script.ProcessEventsFor(root,invoker,0,0,false,&spell);
   bool tailReached=entry!=97436||!reject;assert(script.errors==unsigned(!repaired&&tailReached));assert(script.data.size()==(entry==97436?1u:2u));assert(actor.spawnRequests.empty());
   if(!repaired)oldRequests=script.requests;else assert(script.requests==oldRequests);
   auto requests=script.requests.size();script.ProcessEventsFor(root,invoker,0,0,false,&spell);assert(script.requests.size()==requests);
   script.OnUpdate(1);assert(actor.spawnRequests.empty()); // stage1 cannot start wave
   scenario.step=2;script.OnUpdate(1);assert(actor.spawnRequests==std::vector<uint32>({97054,97054,97054,97054,97054,97386,94721}));assert(!criterionCalls);
   auto total=script.requests.size();script.OnUpdate(500);assert(actor.spawnRequests.size()==7&&script.requests.size()==total);++cases;
  }
 }
 // No interaction is required to poll stage2: the original UPDATE producer
 // retries missing conditions. Null invoker uses the base object condition.
 for(uint32 entry:{97168u,97436u,244431u})for(uint32 mode=0;mode<7;++mode){Reset(2);GameObject a;a.entry=entry;a.isGo=entry==244431;Unit boss;boss.entry=97386;arena={&a};auto script=Script(fixed,a);
  switch(mode){case 0:scenario.step=1;break;case 1:scenario.id=1122;break;case 2:origin.instance=false;break;case 3:scenarioMgr.available=false;break;case 4:a.expired=true;break;case 5:arena.push_back(&boss);break;case 6:script.me=nullptr;script.go=nullptr;break;}
  script.OnUpdate(1);assert(a.spawnRequests.empty()&&!script.errors);
  scenario={1038,2};origin.instance=true;scenarioMgr.available=true;a.expired=false;arena={&a};if(mode==6){if(a.isGo)script.go=&a;else script.me=&a;}
  script.OnUpdate(1);assert(a.spawnRequests.size()==7&&!script.errors);++cases;
 }
 // Actual static producer positions from map1572: each is within the
 // existing80-yard search of native summon835.474/1019.93/63.5284. Only the
 // first tick may issue the wave; subsequent producers see that live boss.
 std::vector<uint32> order{0,1,2};do{Reset(2);GameObject actors[3];uint32 ids[]={97168,97436,244431};float positions[3][3]={{793.727f,1054.61f,48.4185f},{865.354f,1071.23f,49.5026f},{824.3f,1094.42f,49.3594f}};std::vector<SmartScript> scripts;
  for(uint32 i=0;i<3;++i){actors[i].entry=ids[i];actors[i].isGo=i==2;actors[i].x=positions[i][0];actors[i].y=positions[i][1];actors[i].z=positions[i][2];arena.push_back(&actors[i]);scripts.push_back(Script(fixed,actors[i]));}
  for(auto index:order)scripts[index].OnUpdate(1);uint32 attempts=0;for(uint32 i=0;i<3;++i){assert(!scripts[i].errors);attempts+=uint32(actors[i].spawnRequests.size());}assert(attempts==7&&actors[order[0]].spawnRequests.size()==7);++cases;
 }while(std::next_permutation(order.begin(),order.end()));
 // Failed position spawns preserve the original seven attempts/LINK tail.
 // This does not claim the unchanged nonrepeatable list retries partial spawns.
 for(uint32 entry:{97168u,97436u,244431u}){Reset(2);failSummon=true;GameObject a;a.entry=entry;a.isGo=entry==244431;arena={&a};auto s=Script(fixed,a);s.OnUpdate(1);assert(a.spawnRequests.size()==7&&!s.errors);s.OnUpdate(1);assert(a.spawnRequests.size()==7);}
 // Converting row3 to LINK61 would consume the guarded wave at the original
 // stage1 interaction and remove its later independent UPDATE retry entirely.
 Reset(1);GameObject a;a.entry=97168;arena={&a};auto naive=Script(old,a);for(auto& e:naive.mEvents)if(e.event_id==3)e.type=SMART_EVENT_LINK;naive.ProcessEventsFor(SMART_EVENT_HEALTH_PCT);assert(a.spawnRequests.empty()&&!naive.errors);scenario.step=2;naive.OnUpdate(1);assert(a.spawnRequests.empty());
 std::printf("PASS native Maelstrom interaction/UPDATE timers/source22 base binding/stage/near-creature/cast/position-summon/LINK: %u old/fixed/boundary cases; independent stage2 seven actor requests retained, null/pet/guardian invokers, failed fire cast/spawns, repeated events and guarded retry; naiveLINK conversion stalls. Receiver dialogue/GO-prop/grid/spawn/scenario endpoints controlled; full client and partial-spawn recovery not certified.\n",cases);
}
