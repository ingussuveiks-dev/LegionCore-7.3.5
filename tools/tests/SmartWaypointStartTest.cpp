#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <vector>
using uint32=uint32_t;using uint8=uint8_t;
enum {SMART_ACTION_WP_START=53,SMART_ACTION_TALK=1,SMART_EVENT_LINK=61,SMART_EVENT_DATA_SET=38,SMART_EVENT_WAYPOINT_START=39,
    SMART_EVENT_FLAG_NOT_REPEATABLE=1,SMART_ESCORT_TARGETS=0xFFFFFF,SMART_ESCORT_ESCORTING=1};
enum ReactStates {REACT_PASSIVE=0,REACT_DEFENSIVE=1,REACT_AGGRESSIVE=2};
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) assert(false)
#define CAST_AI(type, object) static_cast<type*>(object)
struct Creature;struct SmartScript;
struct ObjectGuid {uint32 value=0;};
struct Unit {ObjectGuid GetGUID(){return {1};}};
struct Position {float x=10,y=20,z=30;};
struct WayPoint {uint32 id=1;float x=179.385f,y=1310.59f,z=-58.9791f;};
using WPPath=std::map<uint32,WayPoint*>;using ObjectList=std::list<Unit*>;
struct MotionMaster {uint32 moves=0,last=0;void MovePoint(uint32 identity,float,float,float){++moves;last=identity;}};
struct SmartScriptHolder {
    uint32 event_id=30,link=31,actionType=53,type=38,source_type=0;int64_t entryOrGuid=100397;bool runOnce=false;
    struct {uint32 event_phase_mask=0,event_flags=1;struct {uint32 id=9,value=9,cooldownMin=0,cooldownMax=0;}dataSet;}event;
    struct {struct {uint32 run=1,pathID=10039703,repeat=0,quest=0,despawnTime=0,reactState=0;}wpStart;}action;
    uint32 GetActionType()const{return actionType;}uint32 GetEventType()const{return type;}
};
struct SmartAI {
    Creature* me;SmartScript* script;WPPath* mWayPoints=nullptr;uint32 mCurrentWPID=0,escort=0,mEscortQuestID=0,despawnTime=0,stops=0;
    WayPoint* mLastWP=nullptr;Position mLastOOCPos;bool mCanRepeatPath=false,running=false;
    SmartScript* GetScript(){return script;}bool HasEscortState(uint32 state){return (escort&state)!=0;}
    void AddEscortState(uint32 state){escort|=state;}void SetRun(bool run){running=run;}void SetDespawnTime(uint32 value){despawnTime=value;}
    void StopPath(){assert(mLastWP);++stops;escort=0;mWayPoints=nullptr;mCurrentWPID=0;mLastWP=nullptr;}
    bool LoadPath(uint32);WayPoint* GetNextWayPoint();
#ifdef WAYPOINT_BASELINE
    void StartPath(bool,uint32,bool,Unit*);
#else
    bool StartPath(bool,uint32,bool,Unit*);
#endif
};
struct Creature:Unit {
    SmartAI* ai=nullptr;bool combat=false,active=false;MotionMaster motion;ReactStates reaction=REACT_AGGRESSIVE;
    bool isInCombat(){return combat;}uint32 GetEntry(){return 100397;}SmartAI* AI(){return ai;}
    Position GetPosition(){return {};}MotionMaster* GetMotionMaster(){return &motion;}
    void SetReactState(ReactStates state){reaction=state;}void setActive(bool value){active=value;}
};
struct WaypointManager {std::map<uint32,WPPath*> paths;WPPath* GetPath(uint32 identity){auto found=paths.find(identity);return found==paths.end()?nullptr:found->second;}} waypointManager;
auto sSmartWaypointMgr=&waypointManager;
struct SmartScript {
    Creature* me;SmartAI ai;ObjectGuid mLastInvoker;uint32 path=0,starts=0,speeches=0,storedTargets=0;bool reenter=false;
    SmartScriptHolder event,linkedEvent;
    explicit SmartScript(Creature* actor):me(actor),ai{actor,this}{me->ai=&ai;linkedEvent.actionType=1;linkedEvent.event_id=31;linkedEvent.link=0;linkedEvent.type=61;}
    bool IsSmart(){return true;}bool IsInPhase(uint32){return false;}uint32 GetPathId(){return path;}void SetPathId(uint32 value){path=value;}
    ObjectList* GetTargets(SmartScriptHolder const&,Unit* invoker){auto* result=new ObjectList;result->push_back(invoker?invoker:me);return result;}
    void StoreTargetList(ObjectList* values,uint32 identity){assert(identity==SMART_ESCORT_TARGETS);++storedTargets;delete values;}
    SmartScriptHolder FindLinkedEvent(uint32 identity){assert(identity==31);return linkedEvent;}
    void RecalcTimer(SmartScriptHolder&,uint32,uint32){}
    void ProcessEventsFor(uint32 type,Unit* unit=nullptr,uint32 point=0,uint32 identity=0){
        if(type==SMART_EVENT_WAYPOINT_START){assert(point==1&&identity==path);++starts;if(reenter)ProcessEvent(event,unit,9,9);}
    }
#define e nativeEvent
    void ProcessEvent(SmartScriptHolder& nativeEvent,Unit* unit=nullptr,uint32 var0=9,uint32 var1=9,bool=false,void const* =nullptr,void* =nullptr){
#include "RepeatGuard.inc"
        switch(nativeEvent.GetEventType()){
#include "DataEvent.inc"
            case SMART_EVENT_LINK:ProcessAction(nativeEvent,unit);break;
        }
    }
    void ProcessAction(SmartScriptHolder& nativeEvent,Unit* unit=nullptr,uint32 var0=0,uint32 var1=0,bool bvar=false,void const* spell=nullptr,void* gob=nullptr){
#include "ActionStart.inc"
        switch(nativeEvent.GetActionType()){
#include "WaypointAction.inc"
            case SMART_ACTION_TALK:++speeches;break;
            default:assert(false);
        }
#include "LinkTail.inc"
#undef e
};
#include "GetNextWayPoint.inc"
#include "LoadPath.inc"
#include "StartPath.inc"
struct Fixture {
    Creature actor;SmartScript script;WayPoint first,second{2};WPPath path;
    Fixture():script(&actor){waypointManager.paths.clear();path[1]=&first;waypointManager.paths[10039703]=&path;}
    void Signal(Unit* invoker=nullptr){script.ProcessEvent(script.event,invoker,9,9);}
};
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);(void)argv;
    for(uint32 failure=0;failure<4;++failure){Fixture test;
        switch(failure){case 0:test.actor.combat=true;break;case 1:waypointManager.paths.clear();break;case 2:test.path.clear();break;case 3:test.path.clear();test.path[2]=&test.second;break;}
        test.Signal();assert(!test.actor.motion.moves&&!test.script.starts);
#ifdef WAYPOINT_BASELINE
        assert(test.script.event.runOnce&&test.script.speeches==1);
        test.actor.combat=false;test.path.clear();test.path[1]=&test.first;waypointManager.paths[10039703]=&test.path;
        test.Signal();assert(!test.actor.motion.moves&&test.script.speeches==1);
#else
        assert(!test.script.event.runOnce&&!test.script.speeches&&!test.script.ai.HasEscortState(SMART_ESCORT_ESCORTING));
        for(uint32 count=0;count<8;++count)test.Signal();assert(!test.actor.motion.moves&&!test.script.event.runOnce&&!test.script.speeches);
        test.actor.combat=false;test.path.clear();test.path[1]=&test.first;waypointManager.paths[10039703]=&test.path;
        test.Signal();assert(test.actor.motion.moves==1&&test.script.starts==1&&test.script.event.runOnce&&test.script.speeches==1);
        for(uint32 count=0;count<8;++count)test.Signal();assert(test.actor.motion.moves==1&&test.script.speeches==1);
#endif
    }
#ifndef WAYPOINT_BASELINE
    Unit player,pet,guardian;
    for(Unit* invoker:{static_cast<Unit*>(nullptr),&player,&pet,&guardian}){
        Fixture test;test.script.reenter=true;test.actor.combat=true;test.Signal(invoker);assert(!test.script.event.runOnce&&!test.script.speeches);
        test.actor.combat=false;test.Signal(invoker);assert(test.actor.motion.moves==1&&test.script.speeches==1&&test.script.starts==1);
    }
    {Fixture test;test.script.event.action.wpStart.pathID=0;test.Signal();assert(!test.script.event.runOnce&&!test.actor.motion.moves);
        test.script.ai.mWayPoints=&test.path;test.Signal();assert(test.actor.motion.moves==1&&test.script.event.runOnce);}
    {Fixture test;test.Signal();test.script.event.runOnce=false;test.Signal();assert(test.script.ai.stops==1&&test.actor.motion.moves==2&&test.script.speeches==2);}
    std::ifstream rows(argv[1]);uint32 count=0,nonrepeat=0,linked=0;int64_t entry;uint32 source,id,type,flags,link,run,identity,repeat,quest,despawn,reaction;
    while(rows>>entry>>source>>id>>type>>flags>>link>>run>>identity>>repeat>>quest>>despawn>>reaction){
        Fixture test;test.script.event.event.event_flags=flags;test.script.event.link=link?31:0;
        test.script.event.action.wpStart={run,identity,repeat,quest,despawn,reaction};
        waypointManager.paths[identity]=&test.path;if(!identity)test.script.ai.mWayPoints=&test.path;
        test.actor.combat=true;test.Signal();assert(!test.script.event.runOnce&&!test.script.speeches&&!test.script.ai.mEscortQuestID&&!test.script.ai.despawnTime);
        test.actor.combat=false;test.Signal();assert(test.actor.motion.moves==1&&test.script.starts==1&&test.script.event.runOnce&&test.script.speeches==uint32(link!=0));
        assert(test.script.ai.mEscortQuestID==quest&&test.script.ai.despawnTime==despawn&&test.script.ai.running==(run!=0)&&test.script.ai.mCanRepeatPath==(repeat!=0)&&uint32(test.actor.reaction)==reaction);
        ++count;nonrepeat+=(flags&1)!=0;linked+=link!=0;
    }
    assert(count>1000);std::cout<<"PASS: "<<count<<" live WP_START parameter shapes ("<<nonrepeat<<" nonrepeatable, "<<linked<<" linked) retain successful run/repeat/quest/despawn/react/waypoint/LINK settings under a controlled DATA_SET trigger. Actual per-source dispatch/targets/world traversal are not modeled.\n";
    std::cout<<"PASS: native combat/missing/empty/invalid-first path failure retains retry without premature LINK, escort state or one-shot consumption; recovery, repeated/reentrant signals and path0/replacement exercised.\n";
#else
    std::cout<<"PASS: pinned pre-fix combat/missing/empty/invalid-first path reproduces consumed one-shot, premature LINK and lost recovery.\n";
#endif
}
