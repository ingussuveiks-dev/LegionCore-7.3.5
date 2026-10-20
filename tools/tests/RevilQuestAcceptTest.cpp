#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <vector>
using uint32=uint32_t;using ReactStates=uint32;using TempSummonType=uint32;
constexpr uint32 SMART_ACTION_SUMMON_CREATURE=12,SMART_ACTION_WP_START=53,SMART_ACTION_ACTIVATE_GOBJECT=9,SMART_ACTION_SET_NPC_FLAG=81;
constexpr uint32 SMART_EVENT_LINK=61,SMART_EVENT_JUST_SUMMONED=54,SMART_EVENT_ACCEPTED_QUEST=19,SMART_EVENT_REWARD_QUEST=20;
constexpr uint32 SMART_EVENT_WAYPOINT_REACHED=40,SMART_EVENT_WAYPOINT_RESUMED=56,SMART_EVENT_WAYPOINT_PAUSED=55,SMART_EVENT_WAYPOINT_STOPPED=57,SMART_EVENT_WAYPOINT_ENDED=58,SMART_EVENT_WAYPOINT_START=39;
constexpr uint32 SMART_ESCORT_TARGETS=1,SMART_ESCORT_ESCORTING=1,UNIT_FIELD_NPC_FLAGS=0,GO_READY=0;
uint32 errors=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
#define CAST_AI(type,value) (value)
struct SmartScript;struct SmartAI;struct Creature;struct GameObject;struct TempSummon;
struct Position{float x=0,y=0,z=0;};struct WayPoint:Position{uint32 id=0;};
struct Unit {
 virtual ~Unit()=default;virtual bool IsCreature(){return false;}virtual Creature* ToCreature(){return nullptr;}virtual GameObject* ToGameObject(){return nullptr;}
 Unit* ToUnit(){return this;}uint32 GetEntry(){return 100812;}uint32 GetGUIDLow(){return 1;}uint32 GetPhaseMask(){return 1;}uint32 GetPhases(){return 5575;}
 virtual void SetUInt32Value(uint32,uint32){}virtual void GetPosition(float&,float&,float&,float&){}
};
using ObjectList=std::list<Unit*>;
struct SmartScriptHolder {
 uint32 event_id=0,link=0,eventType=0,actionType=0,targetType=1,entryOrGuid=100812;
 struct{struct{uint32 creature=100812,type=8,duration=0,attackInvoker=0,phaseByTarget=0;}summonCreature;
 struct{uint32 run=1,pathID=100812,repeat=0,reactState=0,quest=0,despawnTime=0;}wpStart;
 struct{uint32 flag=0;}unitFlag;}action;
 struct{float x=0,y=0,z=0,o=0;}target;
 struct{struct{uint32 quest=0,cooldownMin=0,cooldownMax=0;}quest;struct{uint32 pointID=0,pathID=0;}waypoint;}event;
 uint32 GetActionType(){return actionType;}uint32 GetEventType(){return eventType;}uint32 GetScriptType(){return 0;}
};
std::vector<SmartScriptHolder> rows;std::vector<WayPoint> points;
struct Motion {uint32 first=0;void MovePoint(uint32 id,float,float,float){first=id;}};
struct Creature:Unit {
 SmartAI* ai=nullptr;bool IsAIEnabled=true,combat=false,failSummon=false;uint32 flags=2,summons=0;Position position;Motion motion;
 std::unique_ptr<TempSummon> clone;
 bool IsCreature()override{return true;}Creature* ToCreature()override{return this;}SmartAI* AI(){return ai;}
 bool isInCombat(){return combat;}Position GetPosition(){return position;}void GetPosition(float&x,float&y,float&z,float&o)override{x=position.x;y=position.y;z=position.z;o=0;}
 Motion* GetMotionMaster(){return &motion;}void SetReactState(ReactStates){}void SetPhaseMask(uint32,bool){}void SetPhaseId(uint32,bool){}
 void SetUInt32Value(uint32,uint32 f)override{flags=f;}Creature* SummonCreature(uint32,float,float,float,float,TempSummonType,uint32);
 ~Creature();
};
struct GameObject:Unit{uint32 uses=0;GameObject* ToGameObject()override{return this;}void SetLootState(uint32){}void UseDoorOrButton(uint32,bool,Unit*){++uses;}}gate;
struct SmartAI {
 Creature* me;SmartScript* script;std::vector<WayPoint>* mWayPoints=nullptr;WayPoint* mLastWP=nullptr;Position mLastOOCPos;
 uint32 mEscortQuestID=0,despawnTime=0,path=0;bool escort=false,mCanRepeatPath=false;
 SmartScript* GetScript(){return script;}void IsSummonedBy(Unit*);void JustSummoned(Creature*){}
 void AttackStart(Unit*){}void StartPath(bool,uint32,bool,Unit*);bool HasEscortState(uint32){return escort;}void AddEscortState(uint32){escort=true;}void StopPath(){escort=false;}
 bool LoadPath(uint32 id){path=id;mWayPoints=&points;return id==100812&&!points.empty();}void SetRun(bool){}WayPoint* GetNextWayPoint(){return mWayPoints&&!mWayPoints->empty()?&mWayPoints->front():nullptr;}
 void SetDespawnTime(uint32 n){despawnTime=n;}
};
struct TempSummon:Creature {Unit* owner=nullptr;Unit* GetSummoner(){return owner;}void InitSummon();};
Creature::~Creature()=default;
struct SmartScript {
 Creature* me;SmartAI ai;std::vector<SmartScriptHolder> mEvents;uint32 lines=0;
 explicit SmartScript(Creature* c):me(c),ai{c,this},mEvents(rows){c->ai=&ai;}
 bool IsSmart(){return true;}bool IsUnit(Unit* u){return u&&!u->ToGameObject();}bool IsGameObject(Unit* u){return u&&u->ToGameObject();}
 Creature* GetBaseObject(){return me;}uint32 GetPathId(){return ai.path;}
 ObjectList* GetTargets(SmartScriptHolder const&e,Unit*){auto p=new ObjectList;if(e.targetType==1)p->push_back(me);if(e.targetType==15)p->push_back(&gate);return p;}
 void StoreTargetList(ObjectList* p,uint32){delete p;}void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
 SmartScriptHolder FindLinkedEvent(uint32);
 void ProcessAction(SmartScriptHolder&e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,void* spell=nullptr,void* gob=nullptr){switch(e.actionType){
 case SMART_ACTION_SUMMON_CREATURE:{
#include "Summon.inc"
 break;}
#include "StartAction.inc"
#include "Gate.inc"
#include "Flags.inc"
 case 1:++lines;assert(e.targetType==1);break; // Speech itself is covered by the dialogue regressions.
 case 41:ai.SetDespawnTime(2001);break; // Native delay+next-tick scheduling modeled here.
 default:assert(false);
 }
#include "Link.inc"
 }
 void ProcessEvent(SmartScriptHolder&e,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,void* spell=nullptr,void* gob=nullptr){
  (void)var1;(void)bvar;(void)spell;(void)gob;
  switch(e.eventType){
#include "Quest.inc"
#include "Waypoint.inc"
   case SMART_EVENT_JUST_SUMMONED:case SMART_EVENT_LINK:ProcessAction(e,unit);break;
  }
 }
 void ProcessEventsFor(uint32 event,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0){for(auto&e:mEvents)if(e.eventType==event)ProcessEvent(e,unit,var0,var1);}
};
#include "FindLink.inc"
#include "StartPath.inc"
#include "Summoned.inc"
#include "InitSummon.inc"
std::vector<std::unique_ptr<SmartScript>> cloneScripts;
Creature* Creature::SummonCreature(uint32 entry,float x,float y,float z,float,TempSummonType type,uint32 duration){
 assert(entry==100812&&type==8&&!duration);if(failSummon)return nullptr;
 ++summons;clone=std::make_unique<TempSummon>();clone->owner=this;clone->position={x,y,z};
 cloneScripts.push_back(std::make_unique<SmartScript>(clone.get()));clone->InitSummon();return clone.get();
}
std::vector<std::string> Split(std::string const&line){std::vector<std::string> f;std::stringstream s(line);std::string v;while(std::getline(s,v,'\t'))f.push_back(v);return f;}
int main(int argc,char**argv){
 assert(argc==3);std::ifstream data(argv[1]);std::string line;
 while(std::getline(data,line)){auto f=Split(line);assert(f.size()>=31);if(f[1]!="0")continue;SmartScriptHolder e;
  e.event_id=std::stoul(f[2]);e.link=std::stoul(f[3]);e.eventType=std::stoul(f[5]);e.actionType=std::stoul(f[14]);e.targetType=std::stoul(f[21]);
  e.event.quest.quest=std::stoul(f[9]);e.event.waypoint.pointID=std::stoul(f[9]);e.event.waypoint.pathID=std::stoul(f[10]);
  e.action.summonCreature.creature=std::stoul(f[15]);e.action.summonCreature.type=std::stoul(f[16]);e.action.summonCreature.duration=std::stoul(f[17]);
  e.action.wpStart.run=std::stoul(f[15]);e.action.wpStart.pathID=std::stoul(f[16]);e.action.wpStart.repeat=std::stoul(f[17]);e.action.wpStart.reactState=std::stoul(f[20]);
  e.action.unitFlag.flag=std::stoul(f[15]);rows.push_back(e);
 }
 std::ifstream path(argv[2]);while(std::getline(path,line)){auto f=Split(line);WayPoint p;p.id=std::stoul(f[0]);p.x=std::stof(f[1]);p.y=std::stof(f[2]);p.z=std::stof(f[3]);points.push_back(p);}
 assert(points.size()==18&&points.front().id==1&&points.back().id==18);
 Unit player;
 for(bool old:{true,false})for(uint32 quest:{40623u,40934u}){
  Creature source;source.position={-11112.2f,-2070.07f,45.484f};SmartScript script(&source);errors=0;gate.uses=0;
  if(old)for(auto&e:script.mEvents)if(e.event_id==4||e.event_id==5)e.link=6;
  script.ProcessEventsFor(SMART_EVENT_ACCEPTED_QUEST,&player,quest);assert(source.summons==1&&source.clone&&source.flags==2);
  auto&child=*cloneScripts.back();assert(child.ai.escort&&child.ai.path==100812&&source.clone->motion.first==1&&child.lines==1&&source.clone->flags==0);
  assert(errors==uint32(old)); // Baseline error happens after the clone has already started.
  child.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,17);assert(!gate.uses&&!child.ai.despawnTime);
  child.ai.path=999;child.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,18);assert(!gate.uses);child.ai.path=100812;
  child.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,18);assert(gate.uses==1&&child.ai.despawnTime==2001);
 }
 Creature source;SmartScript script(&source);errors=0;source.failSummon=true;script.ProcessEventsFor(SMART_EVENT_ACCEPTED_QUEST,&player,40623);assert(!source.summons&&!errors);
 script.ProcessEventsFor(SMART_EVENT_ACCEPTED_QUEST,&player,12345);assert(!source.summons&&!errors);
 std::cout<<"PASS: native quest admission/summon branch/TempSummon callbacks/StartPath/LINK dispatch; original missing6 reproduced after clone startup, both quests retain route/speech/clone flags/endpoint gate and delayed despawn; failed summon/unrelated quest unchanged\n";
}
