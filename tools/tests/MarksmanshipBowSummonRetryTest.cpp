#define MARKSMANSHIP_INQUISITOR_TEST
#define MARKSMANSHIP_POST_INQUISITOR_TEST
#define MARKSMANSHIP_SEARCH_PATH_TEST
#define MARKSMANSHIP_BOW_REVEAL_TEST
constexpr unsigned CONDITION_ALIVE=36,SMART_ACTION_SET_EVENT_PHASE=22,SMART_EVENT_PHASE_COUNT=12;
constexpr unsigned SMART_ACTION_SUMMON_GO=50,SMART_TARGET_POSITION=8,SMART_TARGET_RANDOM_POSITION=128;
constexpr unsigned SMART_SUMMON_GAMEOBJECT_RETRY_ON_FAILURE=2;
constexpr unsigned SMART_SUMMON_GAMEOBJECT_NO_CREATOR=4;
constexpr unsigned SMART_EVENT_WAYPOINT_PAUSED=55,SMART_EVENT_WAYPOINT_RESUMED=56,SMART_EVENT_WAYPOINT_STOPPED=57,SMART_EVENT_WAYPOINT_ENDED=58;
constexpr double M_PI=3.14159265358979323846;
float rand_norm(){return 0;}
#include "MarksmanshipHeraldChallengeTest.cpp"
#include "FindLink.inc"
#include "PhaseMethods.inc"
bool SmartAI::StartPath(bool,unsigned,bool,Unit*){return false;}
struct ExpectedSummon {uint32 entry=248419,lifetime=3600;float positionX=62.6788f,positionY=1215.55f,positionZ=-38.6585f,orientation=0.705752f;} expected;
GameObject* WorldObject::SummonGameObject(uint32 entry,float positionX,float positionY,float positionZ,float orientation,float,float,float,float,uint32 lifetime,ObjectGuid,void*,bool creator){
    assert(creator);
    assert(entry==expected.entry&&lifetime==expected.lifetime&&std::abs(positionX-expected.positionX)<0.001f&&std::abs(positionY-expected.positionY)<0.001f&&std::abs(positionZ-expected.positionZ)<0.001f&&std::abs(orientation-expected.orientation)<0.001f);
    ++summonAttempts;if(!summonAvailable)return nullptr;
    ++summons;static GameObject object;return &object;
}
struct RetryFixture {
    Creature vereesa;SmartScript script;
    RetryFixture(){scenarioManager={};scenarioManager.scenario.step=6;vereesa.identity=100397;script.me=&vereesa;vereesa.ai.script=&script;script.mEvents=definitions;script.SetPathId(10039704);}
    void Reach(){script.ProcessEventsFor(SMART_EVENT_WAYPOINT_REACHED,nullptr,14);}
    void Tick(){script.UpdateTimer(script.mEvents[2],1001);}
};
unsigned loaderErrors=0;bool validTemplate=true;
#undef TC_LOG_ERROR
#define TC_LOG_ERROR(...) (++loaderErrors)
bool IsGameObjectValid(SmartScriptHolder const&,uint32){return validTemplate;}
bool ValidateSummon(SmartScriptHolder& row){
#define e row
    switch(row.GetActionType()){
#include "ValidateSummon.inc"
        default:assert(false);
    }return true;
#undef e
}
int main(int argc,char** argv){
    _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);
    std::vector<Condition> gates(8);std::ifstream conditions(argv[2]);
    for(auto& gate:gates){int64 actor;uint32 group;assert(conditions>>actor>>group>>gate.ElseGroup>>gate.ConditionType>>gate.ConditionTarget>>gate.ConditionValue1>>gate.ConditionValue2>>gate.ConditionValue3>>gate.NegativeCondition);conditionManager.SmartEventConditionStore[{actor,0}][group].push_back(&gate);}
    std::ifstream events(argv[1]);
    for(uint32 index=0;index<3;++index){SmartScriptHolder row;uint32 first,second,third,fourth,actionFirst,actionSecond,actionThird;
        assert(events>>row.entryOrGuid>>row.event_id>>row.link>>row.type>>row.event.event_phase_mask>>row.event.event_flags>>first>>second>>third>>fourth>>row.actionType>>actionFirst>>actionSecond>>actionThird>>row.targetType>>row.target.x>>row.target.y>>row.target.z>>row.target.o);
        row.event.waypoint={first,second};row.event.minMaxRepeat={first,second,third,fourth};row.action.talk={actionFirst,actionSecond};row.action.setEventPhase={actionFirst};row.action.summonGO={actionFirst,actionSecond,actionThird};definitions.push_back(row);
    }
    assert(definitions[0].event_id==45&&definitions[0].link==50&&definitions[1].event_id==50&&definitions[1].actionType==22&&definitions[1].action.setEventPhase.phase==2);
    assert(definitions[2].event_id==62&&definitions[2].type==60&&definitions[2].event.event_phase_mask==2&&definitions[2].actionType==50&&!definitions[2].link);
    assert(definitions[2].action.summonGO.flags==2&&ValidateSummon(definitions[2]));
    {auto row=definitions[2];row.action.summonGO.flags=6;assert(ValidateSummon(row));row.type=61;assert(!ValidateSummon(row));assert(loaderErrors==1);loaderErrors=0;}
    for(unsigned invalid=0;invalid<5;++invalid){auto row=definitions[2];switch(invalid){case 0:row.targetType=1;break;case 1:row.type=61;break;case 2:row.event.event_phase_mask=0;break;case 3:row.event.event_flags=0;break;case 4:validTemplate=false;break;}assert(!ValidateSummon(row));validTemplate=true;}
    assert(loaderErrors==4);
    {RetryFixture pending;pending.vereesa.summonAvailable=false;pending.Reach();assert(pending.script.mEventPhase==2&&pending.vereesa.speeches==1&&!pending.vereesa.summonAttempts);
        pending.Tick();assert(pending.vereesa.summonAttempts==1&&!pending.vereesa.summons);
#ifdef SUMMON_RETRY_BASELINE
        assert(pending.script.mEvents[2].runOnce);pending.vereesa.summonAvailable=true;for(unsigned repeat=0;repeat<20;++repeat)pending.Tick();assert(!pending.vereesa.summons&&pending.vereesa.speeches==1);
#else
        assert(!pending.script.mEvents[2].runOnce&&pending.script.mEvents[2].timer==1000);pending.script.UpdateTimer(pending.script.mEvents[2],1000);assert(pending.vereesa.summonAttempts==1&&!pending.script.mEvents[2].timer);
        for(unsigned repeat=0;repeat<20;++repeat){pending.Tick();pending.Reach();}assert(!pending.vereesa.summons&&pending.vereesa.speeches==1&&!pending.script.mEvents[2].runOnce);
        scenarioManager.scenario.step=7;pending.vereesa.summonAvailable=true;pending.Tick();assert(pending.vereesa.summons==1&&pending.script.mEvents[2].runOnce&&pending.vereesa.speeches==1&&scenarioManager.scenario.step==7);
        scenarioManager.scenario.step=6;for(unsigned repeat=0;repeat<20;++repeat)pending.Tick();assert(pending.vereesa.summons==1);
#endif
    }
#ifndef SUMMON_RETRY_BASELINE
    for(unsigned phase: {0u,1u}){RetryFixture dormant;dormant.script.SetPhase(phase);for(unsigned repeat=0;repeat<10;++repeat)dormant.Tick();assert(!dormant.vereesa.summonAttempts&&!dormant.script.mEvents[2].runOnce);}
    for(unsigned stage=0;stage<12;++stage){if(stage==6||stage==7)continue;RetryFixture blocked;blocked.Reach();scenarioManager.scenario.step=stage;blocked.Tick();assert(!blocked.vereesa.summonAttempts&&!blocked.script.mEvents[2].runOnce);scenarioManager.scenario.step=7;blocked.Tick();assert(blocked.vereesa.summons==1);}
    for(unsigned blocker=0;blocker<6;++blocker){RetryFixture blocked;blocked.Reach();switch(blocker){case 0:blocked.vereesa.alive=false;break;case 1:scenarioManager.present=false;break;case 2:scenarioManager.scenario.id=1010;break;case 3:blocked.vereesa.instanced=false;break;case 4:blocked.vereesa.expired=true;break;case 5:blocked.script.me=nullptr;break;}
        blocked.Tick();assert(!blocked.vereesa.summonAttempts&&!blocked.script.mEvents[2].runOnce);blocked.script.me=&blocked.vereesa;blocked.vereesa.alive=true;blocked.vereesa.instanced=true;blocked.vereesa.expired=false;scenarioManager={};scenarioManager.scenario.step=6;blocked.Tick();assert(blocked.vereesa.summons==1);}
    {RetryFixture happy;happy.Reach();happy.script.UpdateTimer(happy.script.mEvents[2],1);assert(happy.vereesa.summons==1&&happy.vereesa.speeches==1&&happy.script.mTextTimer==5000&&scenarioManager.scenario.step==6);}
    for(unsigned phase: {2u,3u}){RetryFixture cue;cue.Reach();cue.script.mEvents[2].event.event_phase_mask=6;cue.script.SetPhase(phase);scenarioManager.scenario.step=7;cue.Tick();assert(cue.vereesa.summons==1&&cue.script.mEvents[2].runOnce);}
#endif
    conditionManager.SmartEventConditionStore.clear();
    std::ifstream shapes(argv[3]);unsigned rows=0;int64 actor;uint32 source,id,flags,link,entry,lifetime,originalSummonFlags;float positionX,positionY,positionZ,orientation;
    while(shapes>>actor>>source>>id>>flags>>link>>entry>>lifetime>>originalSummonFlags>>positionX>>positionY>>positionZ>>orientation){
        expected={entry,lifetime,positionX,positionY,positionZ,orientation};RetryFixture shape;SmartScriptHolder action;action.entryOrGuid=actor;action.source_type=source;action.event_id=73;action.link=74;action.type=60;action.event.event_flags=1;action.event.event_phase_mask=2;action.actionType=50;action.action.summonGO={entry,lifetime,2};action.targetType=8;action.target.x=positionX;action.target.y=positionY;action.target.z=positionZ;action.target.o=orientation;shape.script.SetPhase(2);assert(ValidateSummon(action));
        SmartScriptHolder after;after.entryOrGuid=actor;after.source_type=source;after.event_id=74;after.type=61;after.actionType=1;after.action.talk={99,0};after.targetType=1;shape.script.mEvents={action,after};shape.vereesa.summonAvailable=false;
        shape.script.ProcessEvent(shape.script.mEvents[0]);assert(shape.vereesa.summonAttempts==1&&!shape.vereesa.summons);
#ifdef SUMMON_RETRY_BASELINE
        assert(shape.script.mEvents[0].runOnce&&shape.vereesa.speeches==1);shape.vereesa.summonAvailable=true;shape.script.ProcessEvent(shape.script.mEvents[0]);assert(!shape.vereesa.summons);
#else
        assert(!shape.script.mEvents[0].runOnce&&!shape.vereesa.speeches);shape.vereesa.summonAvailable=true;shape.script.ProcessEvent(shape.script.mEvents[0]);assert(shape.vereesa.summons==1&&shape.vereesa.speeches==1&&shape.script.mEvents[0].runOnce);
        for(unsigned repeat=0;repeat<10;++repeat)shape.script.ProcessEvent(shape.script.mEvents[0]);assert(shape.vereesa.summons==1&&shape.vereesa.speeches==1);
        shape.script.me=nullptr;action.runOnce=false;shape.script.ProcessAction(action);assert(!action.runOnce&&shape.vereesa.summons==1);
#endif
        ++rows;
        if(!(originalSummonFlags&2)){RetryFixture legacy;action.action.summonGO.flags=originalSummonFlags;action.event.event_phase_mask=0;action.runOnce=false;assert(ValidateSummon(action));legacy.script.mEvents={action,after};legacy.vereesa.summonAvailable=false;legacy.script.ProcessEvent(legacy.script.mEvents[0]);assert(legacy.script.mEvents[0].runOnce&&legacy.vereesa.speeches==1&&!legacy.vereesa.summons);}
    }
    assert(rows>=192);
    expected={};
    for(unsigned target: {1u,128u}){RetryFixture retained;SmartScriptHolder action;action.type=61;action.actionType=50;action.action.summonGO={248419,3600};action.targetType=target;action.target.x=expected.positionX;action.target.y=expected.positionY;action.target.z=expected.positionZ;action.target.o=expected.orientation;
        if(target==128){expected.positionX=expected.positionY=expected.positionZ=expected.orientation=0;}retained.script.ProcessAction(action);assert(retained.vereesa.summons==1&&action.runOnce);retained.vereesa.summonAvailable=false;retained.script.ProcessAction(action);assert(action.runOnce&&retained.vereesa.summons==1);expected={};}
#ifdef SUMMON_RETRY_BASELINE
    std::cout<<"PASS baseline: native3e67822d SUMMON_GO consumes failed fixed-position one-shot, dispatches dependent LINK, and loses a later successful source signal; armed bow poll alone cannot recover.\n";
#else
    std::cout<<"PASS: opt-in fixed-position summon failure retains one-shot and skips LINK; later success consumes once. All "<<rows<<" live parameter shapes tested in controlled phased one-shot host; unflagged/legacy1/list/random semantics retained. Native loader rejects unphased/repeating/non-UPDATE/non-position retries. Armed phase2 retries at972/6-7 without repeated discovery speech, early phase/late stage creation or progress grants. Null creation is controlled, not actual map/rendering/loot traversal or persisted reset recovery.\n";
#endif
}
