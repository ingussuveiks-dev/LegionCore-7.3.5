// Extracted production recovery/derived AI and native Smart event/WP handlers.
// World construction, movement completion, spell/chat and storage are controlled.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using int64=int64_t;using uint64=uint64_t;
#include "NativeEnums.inc"
enum {CONDITION_MAPID=22,CONDITION_SCENARION_STEP=44,CONDITION_SOURCE_TYPE_SMART_EVENT=22};
enum TempSummonType{TEMPSUMMON_MANUAL_DESPAWN=8};using ReactStates=uint32;
unsigned warnings=0,nativeErrors=0;
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(category,format,...) (++nativeErrors,std::printf("UNRESOLVED native " format "\n",__VA_ARGS__))
#define TC_LOG_WARN(category,format,...) (++warnings,std::printf("COUNTERFACTUAL expected diagnostic: " format "\n",__VA_ARGS__))
#define CAST_AI(type,value) dynamic_cast<type*>(value)
struct Unit;using Creature=Unit;using Player=Unit;using WorldObject=Unit;struct CreatureAI;struct SmartAI;
struct ObjectGuid{Unit* unit=nullptr;bool operator==(ObjectGuid const& b)const{return unit==b.unit;}};
namespace std{template<>struct hash<ObjectGuid>{size_t operator()(ObjectGuid const& id)const{return hash<Unit*>{}(id.unit);}};}
struct InstanceMap{uint32 id=42;uint32 GetInstanceId()const{return id;}} instanceMap;
struct Map{bool instanced=true;InstanceMap* ToInstanceMap(){return instanced?&instanceMap:nullptr;}} worldMap,otherMap;
struct Position{float x,y,z,o;};struct WayPoint{uint32 id=1;float x=0,y=0,z=0;} nativePoint;
struct CreatureTemplate{std::string AIName;uint32 ScriptID=0;};
struct Motion{unsigned starts=0;void MovePoint(uint32 id,float x,float y,float z){assert(id==1&&x==nativePoint.x&&y==nativePoint.y&&z==nativePoint.z);++starts;}};
struct Unit{
 uint32 entry=0,map=1480,instance=42,dbGuid=0,phase=1;float x=4635.41f,y=2769.5f,z=361.254f,o=0;
 bool world=true,alive=true,contact=true,teleport=false,expired=false,combat=false,scope=false,scopeAllows=true,IsAIEnabled=true;
 CreatureAI* ai=nullptr;CreatureTemplate tpl;Map* runtime=&worldMap;Motion motion;
 ~Unit();ObjectGuid GetGUID(){return {this};}uint32 GetGUIDLow(){return 1;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return map;}uint32 GetInstanceId()const{return instance;}
 bool IsInWorld()const{return world;}bool CanContact()const{return contact;}bool IsAlive()const{return alive;}bool IsBeingTeleported()const{return teleport;}bool IsQuestObjectExpired()const{return expired;}bool isInCombat()const{return combat;}
 bool InInstance()const{return runtime->instanced;}Map* GetMap(){return runtime;}bool InSamePhase(Unit* b){return phase==b->phase;}void ClearUnitState(uint32){}
 Unit* ToCreature(){return this;}bool HasQuestObjectScope()const{return scope;}bool CanTargetQuestObject(Unit*)const{return scopeAllows;}
 bool IsWithinDist3d(float a,float b,float c,float range){return (x-a)*(x-a)+(y-b)*(y-b)+(z-c)*(z-c)<=range*range;}
 CreatureTemplate const* GetCreatureTemplate(){return &tpl;}uint32 GetScriptId()const{return tpl.ScriptID;}uint32 GetDBTableGUIDLow()const{return dbGuid;}CreatureAI* AI(){return ai;}
 Position GetPosition(){return {x,y,z,o};}void GetPosition(float& a,float& b){a=x;b=y;}float GetPositionZ()const{return z;}Motion* GetMotionMaster(){return &motion;}void SetReactState(ReactStates){}
 void GetCreatureListWithEntryInGrid(std::list<Creature*>&,uint32,float)const;
 Creature* SummonCreature(uint32,float,float,float,float,TempSummonType,uint32);
};
struct CriteriaProgress{uint64 Counter=0;};using CriteriaProgressMap=std::unordered_map<uint32,CriteriaProgress>;
struct AchievementMgr{CriteriaProgressMap progress{{45965,{1}}};CriteriaProgressMap const* GetCriteriaProgressMap(){return &progress;}};
struct Scenario{uint32 id=957,step=4;AchievementMgr achievements;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}AchievementMgr& GetAchievementMgr(){return achievements;}};
struct ScenarioMgr{Scenario scenario;bool present=true;uint32 instance=42;Scenario* GetScenario(uint32 id){return present&&id==instance?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct ConditionSourceInfo{WorldObject* mConditionTargets[2];ConditionSourceInfo(WorldObject* a,WorldObject* b):mConditionTargets{a,b}{}};
struct Condition{
 uint32 SourceType=22,SourceGroup=1,SourceEntry=99763,SourceId=0,ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0,ScriptId=0;bool NegativeCondition=false;
 bool isLoaded()const{return true;}bool Meets(ConditionSourceInfo& info){auto object=info.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;switch(ConditionType){
#include "ActorConditions.inc"
 default:assert(false);}return NegativeCondition?!condMeets:condMeets;}
};
using ConditionList=std::list<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;using ConditionTypeContainer=std::map<uint32,ConditionList>;using SmartEventConditionContainer=std::map<std::pair<int64,uint32>,ConditionTypeContainer>;
struct ConditionMgr{
 SmartEventConditionContainer SmartEventConditionStore;ConditionReferenceContainer ConditionReferenceStore;
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& rows)const{return rows.empty()||IsObjectMeetToConditionList(info,rows);}
 bool IsObjectMeetingSmartEventConditions(int64,uint32,uint32,Unit*,WorldObject*)const;ConditionList GetConditionsForSmartEvent(int64,uint32,uint32);
} conditionManager;auto sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
#include "SmartEventConditions.inc"
#include "SmartEventLookup.inc"
struct SmartScriptHolder{
 int32 entryOrGuid=0;uint32 source_type=0,event_id=0,link=0,type=0,actionType=0,targetType=0;bool active=true,runOnce=false;
 struct Event{uint32 event_flags=0,event_phase_mask=0,event_chance=100;union{struct{uint32 param1,param2,param3,param4,param5;}raw;struct{uint32 min,max,repeatMin,repeatMax;}minMaxRepeat;struct{uint32 id,value,cooldownMin,cooldownMax;}dataSet;struct{uint32 pointID,pathID;}waypoint;};Event():raw{}{} }event;
 union Action{struct{uint32 param1,param2,param3,param4,param5,param6;}raw;struct{uint32 creature,type,duration,storageID,attackInvoker,phaseByTarget;}summonCreature;struct{uint32 run,pathID,repeat,quest,despawnTime,reactState;}wpStart;Action():raw{}{} }action;
 struct{struct{uint32 param1=0,param2=0,param3=0,param4=0;}raw;float x=0,y=0,z=0,o=0;}target;
 uint32 GetEventType()const{return type;}uint32 GetActionType()const{return actionType;}uint32 GetScriptType()const{return source_type;}uint32 GetTargetType()const{return targetType;}
};
struct ScriptMgr{std::map<int32,std::vector<SmartScriptHolder>> rows;std::vector<SmartScriptHolder> GetScript(int32 entry,uint32 type){assert(type==0);return rows[entry];}} scriptManager;auto sSmartScriptMgr=&scriptManager;
struct ObjectMgr{std::map<uint32,CreatureTemplate> templates;uint32 GetScriptId(std::string const& name){assert(name=="npc_fire_artifact_aethas");return 9001;}CreatureTemplate const* GetCreatureTemplate(uint32 entry){return &templates.at(entry);}} objectManager;auto sObjectMgr=&objectManager;
struct SpellInfo{};struct GameObject{};using ObjectList=std::list<WorldObject*>;
struct Call{uint32 entry,action,parameter;};std::vector<Call> calls;
std::function<void(Creature*,SmartScriptHolder const&)> actionObserver;
bool roll_chance_i(uint32){return true;}
struct SmartScript{
 std::vector<SmartScriptHolder> mEvents;Creature* me=nullptr;ObjectGuid mLastInvoker;uint32 path=0;
 Unit* GetBaseObject(){return me;}Unit* GetLastInvoker(){return mLastInvoker.unit;}bool IsInPhase(uint32)const{return true;}bool IsSmart(){return true;}uint32 GetPathId()const{return path;}void SetPathId(uint32 value){path=value;}
 void RecalcTimer(SmartScriptHolder&,uint32,uint32){}ObjectList* GetTargets(SmartScriptHolder const& e,Unit*){return e.GetTargetType()==SMART_TARGET_SELF?new ObjectList{me}:nullptr;}void StoreTargetList(ObjectList* targets,uint32){delete targets;}
 bool HasRunEvent(uint32 id,SMART_EVENT type)const{for(auto const& e:mEvents)if(e.event_id==id&&e.GetEventType()==uint32(type)&&e.runOnce)return true;return false;}
 SmartScriptHolder FindLinkedEvent(uint32);void ProcessEventsFor(SMART_EVENT,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);void ProcessEvent(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);void ProcessAction(SmartScriptHolder&,Unit* =nullptr,uint32=0,uint32=0,bool=false,SpellInfo const* =nullptr,GameObject* =nullptr);
};
struct CreatureAI{virtual ~CreatureAI()=default;virtual void Reset(){}virtual void SetData(uint32,uint32){}virtual void AttackStart(Unit*){assert(false);}};
bool pathAvailable=true,pointAvailable=true;
struct SmartAI:CreatureAI{
 Creature* me;SmartScript script;uint32 escort=0,mEscortQuestID=0,mLastWPIDReached=0;bool mCanRepeatPath=false,mRun=false,mWPReached=false;Position mLastOOCPos{};std::vector<WayPoint> points;std::vector<WayPoint>* mWayPoints=nullptr;uint32 mCurrentWPID=0;
 explicit SmartAI(Creature* actor):me(actor){script.me=actor;script.mEvents=scriptManager.GetScript(actor->entry,0);}SmartScript* GetScript(){return &script;}
 void Reset()override{for(auto& e:script.mEvents)e.runOnce=false;escort=0;script.path=0;mWayPoints=nullptr;mLastWPIDReached=0;mWPReached=false;}
 void SetData(uint32,uint32)override;bool HasEscortState(uint32 mask){return (escort&mask)!=0;}void AddEscortState(uint32 mask){escort|=mask;}void StopPath(){escort=0;script.path=0;}
 bool LoadPath(uint32 path){assert(path==99763);if(!pathAvailable)return false;points={nativePoint};mWayPoints=&points;script.path=path;return true;}
 WayPoint* GetNextWayPoint(){return pointAvailable?&points.front():nullptr;}void SetRun(bool run){mRun=run;}void SetDespawnTime(uint32 value){assert(value==0);}bool StartPath(bool,uint32,bool,Unit*);void MovementInform(uint32,uint32);void MovepointReached(uint32);
};
Unit::~Unit(){delete ai;}
#include "SetData.inc"
#include "StartPath.inc"
#include "MovementInform.inc"
#include "MovepointReached.inc"
#include "FindLinkedEvent.inc"
#include "EventDispatch.inc"
#include "EventPrefix.inc"
 switch(e.GetEventType()){
#include "LinkEvent.inc"
#include "UpdateEvent.inc"
#include "DataSetEvent.inc"
#include "WaypointEvent.inc"
#include "NoParamEvents.inc"
 }
}
#include "ActionPrefix.inc"
 calls.push_back({me->entry,e.actionType,e.action.raw.param1});
 if(actionObserver)actionObserver(me,e);
 switch(e.GetActionType()){
#include "StartPathAction.inc"
 case SMART_ACTION_SUMMON_CREATURE:{
#include "PositionSummon.inc"
 break;}
 // Native speech, clear-aura and cast effects are controlled observations.
 default:break;
 }
#include "LinkTail.inc"
struct PlayerScript{std::string name;explicit PlayerScript(char const* value):name(value){}virtual ~PlayerScript()=default;virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}};
#include "fire_return_recovery.h"

std::vector<std::unique_ptr<Creature>> actors;std::map<uint32,unsigned> attempts;bool failAethas=false,failPortal=false,disableNewAI=false;uint32 lastSource=0;
void Unit::GetCreatureListWithEntryInGrid(std::list<Creature*>& result,uint32 wanted,float)const{for(auto& actor:actors)if(actor->entry==wanted)result.push_back(actor.get());}
Creature* Unit::SummonCreature(uint32 wanted,float a,float b,float c,float orientation,TempSummonType type,uint32 duration){assert(type==8&&!duration);++attempts[wanted];lastSource=entry;
 if((wanted==99763&&failAethas)||(wanted==99861&&failPortal))return nullptr;
 auto value=std::make_unique<Creature>();value->entry=wanted;value->x=a;value->y=b;value->z=c;value->o=orientation;value->phase=phase;value->runtime=runtime;value->instance=instance;value->tpl=objectManager.templates.at(wanted);
 if(wanted==99763){assert(a==scriptManager.rows[99615].back().target.x||std::fabs(a-4657.79f)<0.02f);value->ai=new npc_fire_artifact_aethas(value.get());value->IsAIEnabled=!disableNewAI;}
 auto result=value.get();actors.push_back(std::move(value));return result;
}
static Creature* Actor(uint32 entry){for(auto& actor:actors)if(actor->world&&actor->entry==entry)return actor.get();return nullptr;}
static unsigned Count(uint32 action,uint32 value){return unsigned(std::count_if(calls.begin(),calls.end(),[&](Call const& c){return c.entry==99763&&c.action==action&&c.parameter==value;}));}
static void Arrive(Creature* actor=nullptr){if(!actor)actor=Actor(99763);assert(actor);auto ai=static_cast<SmartAI*>(actor->AI());assert(ai->HasEscortState(1));actor->x=nativePoint.x;actor->y=nativePoint.y;actor->z=nativePoint.z;ai->MovementInform(POINT_MOTION_TYPE,1);ai->MovementInform(POINT_MOTION_TYPE,1);ai->escort=0;ai->script.path=0;}
static void ResetWorld(){actionObserver={};actors.clear();attempts.clear();calls.clear();warnings=nativeErrors=0;FireReturnRecovery::states.clear();scenarioManager={};worldMap.instanced=true;instanceMap.id=42;pathAvailable=pointAvailable=true;failAethas=failPortal=disableNewAI=false;objectManager.templates[99615]={"SmartAI",0};objectManager.templates[99763]={"SmartAI",9001};objectManager.templates[99861]={"",0};objectManager.templates[103696]={"",0};}
static void Read(char const* rows,char const* conditions,char const* path){
 std::ifstream input(rows);assert(input);std::string line;while(std::getline(input,line)){std::stringstream ss(line);std::vector<std::string> f;std::string cell;while(std::getline(ss,cell,'\t'))f.push_back(cell);assert(f.size()==31);auto n=[&](unsigned i){return uint32(std::stoul(f[i]));};SmartScriptHolder e;e.entryOrGuid=int32(n(0));e.source_type=n(1);e.event_id=n(2);e.link=n(3);e.type=n(5);e.event.event_phase_mask=n(6);e.event.event_chance=n(7);e.event.event_flags=n(8);e.event.raw={n(9),n(10),n(11),n(12),n(13)};e.actionType=n(14);e.action.raw={n(15),n(16),n(17),n(18),n(19),n(20)};e.targetType=n(21);e.target.raw={n(22),n(23),n(24),n(25)};e.target.x=std::stof(f[26]);e.target.y=std::stof(f[27]);e.target.z=std::stof(f[28]);e.target.o=std::stof(f[29]);scriptManager.rows[e.entryOrGuid].push_back(e);}
 static std::vector<Condition> rules;std::ifstream cond(conditions);assert(cond);while(std::getline(cond,line)){std::stringstream ss(line);std::vector<std::string> f;std::string cell;while(std::getline(ss,cell,'\t'))f.push_back(cell);assert(f.size()==14);auto n=[&](unsigned i){return uint32(std::stoul(f[i]));};Condition c;c.SourceType=n(0);c.SourceGroup=n(1);c.SourceEntry=n(2);c.SourceId=n(3);c.ElseGroup=n(4);c.ConditionType=n(5);c.ConditionTarget=n(6);c.ConditionValue1=n(7);c.ConditionValue2=n(8);c.ConditionValue3=n(9);c.NegativeCondition=n(10)!=0;rules.push_back(c);}assert(rules.size()==2);for(auto& c:rules)conditionManager.SmartEventConditionStore[{99763,0}][1].push_back(&c);
 std::ifstream way(path);assert(way);uint32 entry;assert(way>>entry>>nativePoint.id>>nativePoint.x>>nativePoint.y>>nativePoint.z);assert(entry==99763&&nativePoint.id==1);
}
static Creature* Corpse(){auto actor=std::make_unique<Creature>();actor->entry=99615;actor->alive=false;actor->tpl=objectManager.templates.at(99615);auto result=actor.get();actors.push_back(std::move(actor));return result;}
static void Healthy(player_fire_artifact_return& script,Player& player){script.OnUpdate(&player,500);auto actor=Actor(99763);assert(actor&&attempts[99763]==1&&Count(1,0)==1&&!Actor(99861));static_cast<SmartAI*>(actor->AI())->GetScript()->ProcessEventsFor(SMART_EVENT_UPDATE);assert(!Count(11,196561)&&!Count(11,196347));script.OnUpdate(&player,1000);assert(attempts[99763]==1&&Count(1,0)==1&&!Actor(99861));Arrive();assert(Actor(99861)&&Count(1,1)==1&&Count(11,196520)==1);script.OnUpdate(&player,500);script.OnUpdate(&player,500);assert(attempts[99763]==1&&attempts[99861]==1&&Count(1,1)==1&&!warnings&&!nativeErrors);}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);Read(argv[1],argv[2],argv[3]);player_fire_artifact_return script;assert(script.name=="player_fire_artifact_return");
 // The unchanged death graph requests blade/GO/credit, not the half-health NPC.
 ResetWorld();{scenarioManager.scenario.step=3;scenarioManager.scenario.achievements.progress.clear();Player cause;auto boss=Corpse();SmartScript original;original.me=boss;original.mEvents=scriptManager.rows[99615];original.ProcessEventsFor(SMART_EVENT_DEATH,&cause);assert(!Actor(99763)&&!Actor(99861)&&Actor(103696)&&nativeErrors==1);}
 ResetWorld();{Player player;Corpse();Healthy(script,player);assert(lastSource==99763);} // accepted native death proof; owner credit dispatch is separately regressed
 ResetWorld();{Player player;Healthy(script,player);assert(attempts[99763]==1);} // earned re-entry after corpse disappeared
 ResetWorld();{Player player;unsigned reentered=0;actionObserver=[&](Creature* actor,SmartScriptHolder const& row){if(actor->entry==99763&&row.event_id==4){++reentered;actor->AI()->SetData(3,3);}};Healthy(script,player);assert(reentered==1&&Count(28,0)==1&&Count(1,0)==1);} // actual runOnce is set before a reentrant aura-removal callback
 ResetWorld();{Player player;objectManager.templates[99615].ScriptID=42;script.OnUpdate(&player,1);assert(warnings==1&&!attempts[99763]);objectManager.templates[99615].ScriptID=0;script.OnMapChanged(&player);warnings=0;Healthy(script,player);} // custom boss template cannot borrow native summon data
 for(unsigned mode=0;mode<12;++mode){ResetWorld();Player player;if(mode==0)player.world=false;if(mode==1)player.contact=false;if(mode==2)player.alive=false;if(mode==3)player.teleport=true;if(mode==4)player.map=1489;if(mode==5)player.instance=43;if(mode==6)scenarioManager.present=false;if(mode==7)scenarioManager.scenario.id=901;if(mode==8)scenarioManager.scenario.step=3;if(mode==9)scenarioManager.scenario.step=6;if(mode==10)scenarioManager.scenario.achievements.progress.clear();if(mode==11)scenarioManager.scenario.achievements.progress[45965].Counter=0;script.OnUpdate(&player,1000);assert(!attempts[99763]&&!warnings);}
 ResetWorld();{Player player;player.x=0;script.OnUpdate(&player,1000);assert(!attempts[99763]&&!warnings);player.x=4635.41f;Healthy(script,player);}
 ResetWorld();{Player first,peer;first.phase=peer.phase=7;Corpse();script.OnUpdate(&first,1);script.OnUpdate(&peer,1);assert(attempts[99763]==1&&Count(1,0)==1);Arrive();script.OnUpdate(&first,500);script.OnUpdate(&peer,500);assert(attempts[99861]==1&&Count(1,1)==1);}
 ResetWorld();{Player player;failAethas=true;script.OnUpdate(&player,1);script.OnUpdate(&player,499);assert(attempts[99763]==1);failAethas=false;script.OnUpdate(&player,1);assert(attempts[99763]==2&&Count(1,0)==1);Arrive();script.OnUpdate(&player,500);assert(!warnings);}
 ResetWorld();{Player player;disableNewAI=true;script.OnUpdate(&player,1);script.OnUpdate(&player,500);assert(attempts[99763]==1&&!Count(1,0));Actor(99763)->IsAIEnabled=true;script.OnUpdate(&player,500);assert(Count(1,0)==1);Arrive();script.OnUpdate(&player,500);assert(!warnings);}
 ResetWorld();{Player player;disableNewAI=true;script.OnUpdate(&player,1);auto actor=Actor(99763);delete actor->ai;actor->ai=new SmartAI(actor);actor->IsAIEnabled=true;script.OnUpdate(&player,500);assert(attempts[99763]==1&&!Count(1,0));delete actor->ai;actor->ai=new npc_fire_artifact_aethas(actor);script.OnUpdate(&player,500);assert(Count(1,0)==1);Arrive();script.OnUpdate(&player,500);assert(!warnings);}
 for(bool missingPoint:{false,true}){ResetWorld();Player player;pathAvailable=missingPoint;pointAvailable=!missingPoint;script.OnUpdate(&player,1);assert(Actor(99763)&&!Count(1,0)&&!Actor(99861));pathAvailable=pointAvailable=true;script.OnUpdate(&player,500);assert(Count(1,0)==1);script.OnUpdate(&player,500);assert(Count(1,0)==1);Arrive();script.OnUpdate(&player,500);assert(!warnings);}
 ResetWorld();{Player player;script.OnUpdate(&player,1);failPortal=true;Arrive();assert(!Actor(99861)&&Count(1,1)==1&&Count(11,196520)==1);script.OnUpdate(&player,500);assert(attempts[99861]==2&&!Actor(99861));failPortal=false;script.OnUpdate(&player,500);assert(attempts[99861]==3&&Actor(99861)&&Count(1,1)==1&&Count(11,196520)==1&&!warnings);}
 ResetWorld();{Player player;script.OnUpdate(&player,1);auto actor=Actor(99763);actor->combat=true;static_cast<SmartAI*>(actor->AI())->escort=0;script.OnUpdate(&player,500);actor->combat=false;script.OnUpdate(&player,500);assert(Count(1,0)==1);Arrive();script.OnUpdate(&player,500);assert(Actor(99861));}
 ResetWorld();{Player player;script.OnUpdate(&player,1);auto ai=static_cast<SmartAI*>(Actor(99763)->AI());ai->MovementInform(FOLLOW_MOTION_TYPE,1);script.OnUpdate(&player,500);assert(!Actor(99861)&&!ai->script.HasRunEvent(7,SMART_EVENT_WAYPOINT_REACHED));Arrive();script.OnUpdate(&player,500);assert(attempts[99861]==1&&Count(1,1)==1);}
 ResetWorld();{Player player;script.OnUpdate(&player,1);auto actor=Actor(99763);static_cast<npc_fire_artifact_aethas*>(actor->AI())->Reset();script.OnUpdate(&player,500);assert(attempts[99763]==1&&Count(1,0)==2);static_cast<SmartAI*>(actor->AI())->script.ProcessEventsFor(SMART_EVENT_UPDATE);assert(!Count(11,196561));Arrive();script.OnUpdate(&player,500);assert(attempts[99861]==1&&!warnings);}
 ResetWorld();{Player player;auto other=std::make_unique<Creature>();other->entry=99763;other->tpl=objectManager.templates[99763];other->scope=true;other->scopeAllows=false;auto unrelated=other.get();actors.push_back(std::move(other));script.OnUpdate(&player,1);assert(attempts[99763]==1&&!unrelated->ai);auto own=actors.back().get();assert(own!=unrelated);Arrive(own);script.OnUpdate(&player,500);assert(Actor(99861)&&!warnings);}
 ResetWorld();{Player player;Healthy(script,player);Actor(99763)->world=false;Actor(99861)->world=false;script.OnUpdate(&player,500);assert(attempts[99763]==2&&Count(1,0)==2);Arrive();script.OnUpdate(&player,500);assert(attempts[99861]==2&&!warnings);}
 ResetWorld();{Player player;failAethas=true;script.OnUpdate(&player,1);for(unsigned i=0;i<30;++i)script.OnUpdate(&player,1000);assert(warnings==1&&attempts[99763]==31);script.OnUpdate(&player,10000);assert(warnings==1&&attempts[99763]==31);failAethas=false;script.OnMapChanged(&player);script.OnUpdate(&player,1);assert(Actor(99763));Arrive();script.OnLogout(&player);assert(FireReturnRecovery::states.empty());}
 ResetWorld();{Player player;auto saved=scriptManager.rows[99763][5];scriptManager.rows[99763][5].action.raw.param2=1;script.OnUpdate(&player,1);script.OnUpdate(&player,1000);assert(warnings==1&&!attempts[99763]);scriptManager.rows[99763][5]=saved;script.OnLogout(&player);warnings=0;Healthy(script,player);} // expected mismatch diagnostic was printed before correcting the fixture
 ResetWorld();{Player player;script.OnUpdate(&player,1);auto old=Actor(99763);script.OnMapChanged(&player);player.runtime=&otherMap;player.instance=43;scenarioManager.instance=43;instanceMap.id=43;script.OnUpdate(&player,1);assert(attempts[99763]==2&&actors.back().get()!=old&&actors.back()->runtime==&otherMap);Arrive(actors.back().get());script.OnUpdate(&player,500);assert(attempts[99861]==1&&!warnings);}
 FireReturnRecovery::Update(nullptr,1);
 std::cout<<"PASS extracted recovery/registered wrapper/derived AI with actual Smart conditions/event/LINK/WP_START/StartPath/data3/waypoint handlers: missing original one-hit actor vs native recovery, earned proof guards, source/re-entry/group/phase, spawn/AI/path/portal failures, actual-arrival gate, no duplicate speech/story or portal, bounded timeout and logout/map reset. Death credit storage, world creation/movement/effects remain controlled endpoints; actual owner credit and client traversal are separate.\n";
 std::cout<<"UNRESOLVED original99615 death LINK32 remains observed in its baseline; unknown22/32 were not replaced. Injected timeout/native-data mismatch diagnostics were expected counterfactuals paired with recovery; no production runtime error is hidden.\n";
}
